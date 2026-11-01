#pragma once
#include "ResourceRuntimeContracts.hpp"
#include <Workphone/System/ResourceChunkStream.hpp>
#include <Workphone/System/ResourceClosureStream.hpp>
#include <thread>

namespace resource_mount_composition_contracts
{
    using namespace workphone;using namespace workphone::resource;namespace fs=std::filesystem;
    inline void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
    inline bool bytesEqual(const Array<u8> &actual,const std::string &expected,size_t offset=0)
    {return offset<=expected.size()&&actual.size()<=expected.size()-offset&&std::memcmp(actual.data(),expected.data()+offset,actual.size())==0;}
    inline void run(const fs::path &root)
    {
        fs::create_directories(root/"source");String error;
        const ResourceID a("data://a.rtres"),b("data://b.rtres"),shared("data://shared.rtres");
        const String uuidA="10101010-1010-4010-8010-101010101010",uuidB="20202020-2020-4020-8020-202020202020",uuidShared="30303030-3030-4030-8030-303030303030";
        const std::string original(2*65536+31,'r'),updated(2*65536+31,'s');
        resource_runtime_contracts::write(root/"source/a.rtres","install=data://shared.rtres\nA");
        resource_runtime_contracts::write(root/"source/b.rtres","install=data://shared.rtres\nB");
        resource_runtime_contracts::write(root/"source/shared.rtres",original);
        auto registry=std::make_shared<ResourceCompilerRegistry>();require(registry->registerCompiler(std::make_shared<resource_runtime_contracts::Compiler>(),&error),error.c_str());
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;
        config.sourceRoot=(root/"source").u8string().c_str();config.compiledRoot=(root/"cooked").u8string().c_str();config.target="composition";
        require(author.initialize(config,error),error.c_str());CompilationOptions options;options.packagedBuild=true;
        require(author.compile(shared,options).succeeded()&&author.compile(a,options).succeeded()&&author.compile(b,options).succeeded(),"Compile composed package fixtures");
        auto exportPackage=[&](const char *name,const ResourceID &id,const String &uuid){Array<RuntimeResourceBinding> bindings={{uuid,id}};
            if(id!=shared)bindings.push_back({uuidShared,shared});require(author.writeRuntimePackage((root/name).u8string().c_str(),{id},bindings,error),error.c_str());};
        exportPackage("A",a,uuidA);exportPackage("B",b,uuidB);exportPackage("B-uuid-collision",b,uuidA);exportPackage("shared-old",shared,uuidShared);
        resource_runtime_contracts::write(root/"source/shared.rtres",updated);options.force=true;require(author.compile(shared,options).succeeded(),"Compile changed shadow generation");
        exportPackage("shared-new",shared,uuidShared);author.shutdown();
        fs::rename(root/"source",root/"source-unavailable");fs::rename(root/"cooked",root/"cooked-unavailable");
        auto layer=[&](const char *name,const char *directory){RuntimeResourceMountConfig value;value.name=name;value.config.target=config.target;
            value.config.compiledRoot=(root/directory).u8string().c_str();value.config.manifestPath=(root/directory/"runtime.wprm").u8string().c_str();return value;};
        const auto packageA=layer("base-A","A"),packageB=layer("base-B","B"),old=layer("old","shared-old"),patch=layer("patch","shared-new");
        auto scheduler=std::make_shared<WorldStreamingScheduler>();
        require(!ResourcePayloadMount::createComposition({packageA,packageB},RuntimeMountCollisionPolicy::Reject,scheduler,{},error)&&!error.empty(),"Reject policy forbids even identical ResourceID collisions");
        require(!scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)],"Rejected composition releases all captured manifest admissions");
        auto mount=ResourcePayloadMount::createComposition({packageA,packageB},RuntimeMountCollisionPolicy::IdenticalOnly,scheduler,{},error);require(mount!=nullptr,error.c_str());
        RuntimeResourceProvenance provenance;require(mount->provenance(shared,provenance,error)&&scheduler->ownsBudget(provenance.admission)&&provenance.mountName=="base-A"&&provenance.precedence==0&&provenance.shadowedMounts==1&&provenance.manifestVersion==4&&provenance.manifestPayloadHash,
            "Provenance identifies deterministic earlier-package selection and sealed manifest fingerprint");
        auto route=ResourcePayloadStream::create(mount,error);require(route!=nullptr,error.c_str());ResourceID resolved;
        require(route->resolveUUID(uuidA,resolved,error)&&resolved==a&&route->resolveUUID(uuidB,resolved,error)&&resolved==b&&route->resolveUUID(uuidShared,resolved,error)&&resolved==shared,"UUID and ResourceID routing share the composed index");
        fs::path lowerShared;for(const auto &entry:fs::recursive_directory_iterator(root/"B"))if(entry.is_regular_file()){
            CompiledResourceHeader header;if(CompiledResourceIO::readHeader(entry.path().u8string().c_str(),header,error)&&header.resourceId==shared){lowerShared=entry.path();break;}}
        require(!lowerShared.empty(),"Find lower package's duplicate dependency");
        {std::fstream file(lowerShared,std::ios::binary|std::ios::in|std::ios::out);file.seekp(fs::file_size(lowerShared)-original.size()+10);file.put('X');require(bool(file),"Corrupt only lower-priority dependency bytes");}
        ResourceSystem synchronous(nullptr,nullptr);require(synchronous.initializeRuntimeComposition({packageA,packageB},RuntimeMountCollisionPolicy::IdenticalOnly,error),error.c_str());
        auto graph=synchronous.load(b,error);require(graph&&graph->dependencies.size()==1&&bytesEqual(graph->dependencies[0]->payload,original),"Synchronous cross-mount dependency reads selected package instead of parent's corrupt duplicate");
        auto closure=ResourceClosureStream::create(mount,error);require(closure&&closure->requestUUID(10,uuidB,{},5,error),error.c_str());
        ResourcePayloadResult closureResult;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!closure->take(closureResult)&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(closureResult.resource&&closureResult.resource->dependencies.size()==1&&bytesEqual(closureResult.resource->dependencies[0]->payload,original),"Asynchronous compatible-set closure follows exact composed dependency routing");closure->drain();closure.reset();closureResult={};
        auto first=ResourcePayloadStream::create(mount,error),second=ResourcePayloadStream::create(mount,error);require(first&&second,"Composed payload requesters initialize");
        auto pressure=scheduler->reserve(StreamMemoryClass::WorkingCpu,scheduler->stats().limits[size_t(StreamMemoryClass::WorkingCpu)],error);require(pressure!=nullptr,error.c_str());
        const auto pending=scheduler->stats().pending;
        require(first->replace({{11,shared,1ull<<20,1}},error)&&second->replace({{12,shared,1ull<<20,2}},error),error.c_str());
        require(scheduler->stats().pending==pending+1,"Two composed requesters coalesce one selected shared-dependency read");first->cancel();pressure.reset();
        ResourcePayloadResult survivor;const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!second->take(survivor)&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(survivor.resource&&survivor.key==12&&bytesEqual(survivor.resource->payload,original),"Canceling one requester preserves the other composed dependency interest");first->drain();second->drain();survivor={};
        auto chunks=ResourceChunkStream::create(mount,error);require(chunks&&chunks->request(13,shared,65530,32,0,0,error),error.c_str());
        ResourceChunkResult range;const auto rangeUntil=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!chunks->take(range)&&std::chrono::steady_clock::now()<rangeUntil)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(range.resource&&range.resource->bytes.size()==32&&bytesEqual(range.resource->bytes,original,65530),"Verified ranges use selected package chunk pins and exact unaligned offsets");chunks->drain();range={};chunks.reset();
        require(!ResourcePayloadMount::createComposition({patch,old},RuntimeMountCollisionPolicy::IdenticalOnly,scheduler,{},error),"Identical policy rejects changed payload generations");
        auto shadow=ResourcePayloadMount::createComposition({patch,old},RuntimeMountCollisionPolicy::PreferEarlier,scheduler,{},error);require(shadow!=nullptr,error.c_str());
        auto selected=shadow->loadPinnedPayload(shared,error,updated.size());require(selected&&bytesEqual(selected->payload,updated),"Explicit shadow policy selects the earlier changed leaf generation");
        require(!shadow->loadPinnedPayload(shared,error,updated.size()-1),"Worker-safe pinned read enforces its explicit byte cap");
        require(!ResourcePayloadMount::createComposition({patch,packageA},RuntimeMountCollisionPolicy::PreferEarlier,scheduler,{},error)&&error.find("dependency generation")!=String::npos,
            "Shadowing rejects an old selected parent whose dependency fingerprint becomes incompatible");
        {
            fs::create_directories(root/"incompatible");RuntimeResource manifest;
            require(CompiledResourceIO::read((root/"shared-new/runtime.wprm").u8string().c_str(),manifest,error),error.c_str());
            const auto versionOffset=8+(8+config.target.size())+8+(8+shared.str().size())+8+(8+shared.str().size());
            require(versionOffset+8<=manifest.payload.size(),"Locate compiler variant in bounded one-resource manifest");
            for(size_t byte=0;byte<8;++byte)manifest.payload[versionOffset+byte]=byte?0:8;
            manifest.header.payloadHash=hashBytes(manifest.payload.data(),manifest.payload.size());
            resource_runtime_contracts::write(root/"incompatible-payload",std::string(manifest.payload.begin(),manifest.payload.end()));
            require(CompiledResourceIO::writeAtomic((root/"incompatible/runtime.wprm").u8string().c_str(),manifest.header,(root/"incompatible-payload").u8string().c_str(),error),error.c_str());
            require(!ResourcePayloadMount::createComposition({layer("incompatible","incompatible"),patch},RuntimeMountCollisionPolicy::PreferEarlier,scheduler,{},error)&&error.find("variant")!=String::npos,
                "Even explicit shadows fail closed across incompatible compiler variants before artifact I/O");
        }
        const auto collision=layer("uuid-collision","B-uuid-collision");
        require(!ResourcePayloadMount::createComposition({packageA,collision},RuntimeMountCollisionPolicy::IdenticalOnly,scheduler,{},error),"Identical policy rejects UUID collisions routing different ResourceIDs");
        auto uuidShadow=ResourcePayloadMount::createComposition({packageA,collision},RuntimeMountCollisionPolicy::PreferEarlier,scheduler,{},error);require(uuidShadow!=nullptr,error.c_str());
        auto uuidStream=ResourcePayloadStream::create(uuidShadow,error);require(uuidStream&&uuidStream->resolveUUID(uuidA,resolved,error)&&resolved==a,"Explicit UUID precedence resolves the earlier ResourceID without path guessing");
        fs::copy_file(root/"shared-new/runtime.wprm",root/"A/runtime.wprm",fs::copy_options::overwrite_existing);
        bool preserved=false;std::thread owner([&]{String issue;auto view=mount->createReadOnlyOwnerView(issue);RuntimeResourceProvenance captured;
            auto payload=view?view->loadPinnedPayload(shared,issue,original.size()):nullptr;preserved=view&&view->provenance(shared,captured,issue)&&captured.manifestPayloadHash==provenance.manifestPayloadHash&&payload&&bytesEqual(payload->payload,original);});owner.join();
        require(preserved,"Owner views retain the exact old composite generation after mutable manifest replacement");
        require(!ResourcePayloadMount::createComposition({packageA,packageB},RuntimeMountCollisionPolicy::PreferEarlier,scheduler,{},error),"A fresh composition refuses the now-incompatible selected generation");
        selected.reset();shadow.reset();uuidStream.reset();uuidShadow.reset();first.reset();second.reset();route.reset();mount.reset();provenance={};
        require(!scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]&&!scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)],"Composition cancellation, owner views and escaped payload release retire captured leases");
    }
}
