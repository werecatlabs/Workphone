#pragma once
#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <cstdio>
extern "C" {
#include <lua.h>
}

inline void runLuaRuntimeMemoryContracts()
{
    using namespace workphone;using namespace workphone::resource;
    auto check=[](bool value,const char *message){if(!value)throw std::runtime_error(message);};
    auto scheduler=WorldStreamingScheduler::shared();const auto sourceClass=size_t(StreamMemoryClass::SourceCpu);
    auto used=[&]{return scheduler->stats().used[sourceClass];};
    String error;const auto originalLimit=scheduler->stats().limits[sourceClass];
    struct Restore {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~Restore(){String ignored;scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,ignored);}} restore{scheduler,originalLimit};
    auto limit=[&](u64 bytes){const bool accepted=scheduler->setBudget(StreamMemoryClass::SourceCpu,bytes,error);check(accepted,error.c_str());};
    const auto beforeDenied=used();
    {
        LuaManager denied;
        const bool configured=denied.configureRuntimeResourceResolver([](const String &,String &)->std::shared_ptr<const RuntimeResource>{return {};},error);
        check(configured,error.c_str());
        // Less than the admitted bootstrap envelope: fail before invoking any
        // binding constructor, rather than jumping through its native locals.
        limit(used()+(1ull<<20));denied.load(nullptr);limit(originalLimit);
        check(!denied.isLoaded()&&!denied.getLuaState()&&denied.getError(),"Runtime Lua bootstrap budget denial must leave an unloaded VM");
        denied.unload(nullptr);
    }
    check(used()==beforeDenied,"Denied runtime Lua initialization must release every domain admission");

    auto resource=[](const char *id,const std::string &source){
        auto result=std::make_shared<RuntimeResource>();result->header.resourceId=ResourceID(id);result->header.resourceType=ResourceTypeID("lua");
        result->header.compilerVersion=LuaScriptCompiler().versionFor(result->header.resourceType);result->payload.resize(source.size());
        if(!source.empty())std::memcpy(result->payload.data(),source.data(),source.size());
        result->header.payloadSize=result->payload.size();result->header.payloadHash=hashBytes(result->payload.data(),result->payload.size());
        result->header.sourceHash=result->header.payloadHash;return result;
    };
    const auto helperText=std::string("--")+std::string(128*1024,' ')+"\nreturn { value=42 }";
    auto helper=resource("data://budget/Helper.lua",helperText);
    auto actor=resource("data://budget/Actor.lua","budgetValue=require('budget.Helper').value");
    actor->dependencies.push_back(helper);actor->header.installDependencies.push_back(helper->header.resourceId);
    const String uuid="a1c22764-89ba-4bb2-9326-f6f15815ebca";
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
    // Preload uses an engine String for this source. The shared character pool
    // correctly keeps its returned <=256KiB slab physically SourceCpu-charged.
    // Warm that exact capacity before the baseline so cache growth cannot be
    // mistaken for an escaped Lua VM/domain. Pool capacity/live counts and the
    // complete Source ledger must both return to this same baseline below.
    {String warmed(helperText.data(),helperText.size());warmed.release();}
    const auto poolBeforeVm=String::character_pool_stats();
#endif
    const auto beforeVm=used();u64 loadedBytes=0,unloadedBytes=0;
    {
        LuaManager vm;
        const bool configured=vm.configureRuntimeResourceResolver([actor,uuid](const String &id,String &)->std::shared_ptr<const RuntimeResource>{return id==uuid?actor:nullptr;},error);
        check(configured,error.c_str());vm.load(nullptr);
        check(vm.isLoaded()&&vm.getLuaState(),"Admitted runtime Lua VM must load");
        loadedBytes=used();
        check(used()>=beforeVm+(32ull<<20),"Runtime Lua bootstrap ownership must participate in SourceCpu");
        auto state=vm.getLuaState();void *domain=nullptr;const auto allocate=lua_getallocf(state,&domain);
        check(allocate&&domain,"Runtime Lua must install an owned allocator domain");
        const auto beforeBlock=used();
        struct Block {lua_Alloc allocate;void *domain;void *pointer=nullptr;size_t bytes=0;~Block(){if(pointer)allocate(domain,pointer,bytes,0);}} block{allocate,domain};
        block.pointer=allocate(domain,nullptr,0,65536);block.bytes=65536;check(block.pointer,"Runtime Lua allocation must be admitted");
        std::memset(block.pointer,0x6d,32768);const auto withBlock=used();check(withBlock>=beforeBlock+65536,"Runtime Lua block storage must be charged before allocation");
        limit(withBlock);check(allocate(domain,block.pointer,block.bytes,32768)==block.pointer&&used()==withBlock,"Lua shrink must succeed without releasing still-retained physical capacity");block.bytes=32768;
        limit(withBlock+80000);check(!allocate(domain,block.pointer,block.bytes,131072)&&used()==withBlock,"Lua growth must admit the complete replacement while the old block remains alive");
        for(size_t i=0;i<32768;++i)check(static_cast<unsigned char*>(block.pointer)[i]==0x6d,"Rejected Lua growth damaged the old allocation");
        limit(originalLimit);auto grown=allocate(domain,block.pointer,block.bytes,131072);check(grown,"Runtime Lua growth must recover after admission pressure");
        block.pointer=grown;block.bytes=131072;for(size_t i=0;i<32768;++i)check(static_cast<unsigned char*>(block.pointer)[i]==0x6d,"Admitted Lua growth lost existing bytes");
        allocate(domain,block.pointer,block.bytes,0);block.pointer=nullptr;check(used()==beforeBlock,"Freeing a runtime Lua block must release its allocation-local admission");

        auto source=[&](const char *text,const char *name){const bool result=vm.executeSource(text,name);check(result,vm.getLastDiagnostic().c_str());};
        source("function budgetAllocate() budgetHeld=string.rep('x',4*1024*1024) end; collectgarbage('collect')","=budget-growth");
        limit(used()+65536);vm.callFunction("budgetAllocate");limit(originalLimit);
        check(vm.getError()&&vm.isLoaded()&&vm.getLuaState()==state,"Runtime Lua allocation denial must be a protected script error preserving the VM");
        source("assert(budgetHeld==nil); budgetAllocate(); assert(#budgetHeld==4*1024*1024)","=budget-held");
        const auto held=used();source("budgetHeld=nil; collectgarbage('collect')","=budget-collected");
        check(held>=used()+(3ull<<20),"Lua GC must release admitted post-bootstrap script storage");

        // Refuse native source/preload/debugger copies before changing package
        // preload state; the same captured graph must succeed after pressure.
        limit(used()+65536);const bool denied=vm.loadScriptAsset(uuid);limit(originalLimit);
        check(!denied&&vm.getError()&&vm.getLuaState()==state,"Runtime Lua source copies must participate in admission");
        source("assert(budgetValue==nil)","=budget-preload-denied");
        const auto beforePreload=used();const bool loaded=vm.loadScriptAsset(uuid);check(loaded,vm.getLastDiagnostic().c_str());
        check(used()>=beforePreload+helperText.size()*3,"Runtime preload/debugger source ownership must retain a conservative SourceCpu allowance");
        source("assert(budgetValue==42 and require('budget.Helper').value==42)","=budget-require");
        vm.garbageCollect();vm.unload(nullptr);unloadedBytes=used();check(!vm.getLuaState(),"Runtime Lua teardown must close the managed VM");
    }
    if(used()!=beforeVm)std::fprintf(stderr,"Runtime Lua SourceCpu ledger: before=%llu loaded=%llu unloaded=%llu destroyed=%llu\n",
        static_cast<unsigned long long>(beforeVm),static_cast<unsigned long long>(loadedBytes),static_cast<unsigned long long>(unloadedBytes),static_cast<unsigned long long>(used()));
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
    const auto poolAfterVm=String::character_pool_stats();
    check(poolAfterVm.committedBytes==poolBeforeVm.committedBytes&&poolAfterVm.liveBlocks==poolBeforeVm.liveBlocks&&
        poolAfterVm.slabCount==poolBeforeVm.slabCount&&poolAfterVm.freeBlocks==poolBeforeVm.freeBlocks&&poolAfterVm.bucketCount==poolBeforeVm.bucketCount,
        "Runtime Lua teardown must return shared String pool storage and live blocks to the warmed baseline");
#endif
    check(used()==beforeVm,"Runtime Lua teardown must release VM, source-copy and debugger domain admissions");
}
