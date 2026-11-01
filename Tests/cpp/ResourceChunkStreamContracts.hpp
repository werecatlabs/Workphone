#pragma once
#include "ResourceRuntimeContracts.hpp"
#include <Workphone/System/ResourceChunkStream.hpp>
#include <Workphone/System/ResourceClosureStream.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <chrono>
#include <fstream>
#include <thread>

namespace resource_chunk_stream_contracts
{
    using namespace workphone;using namespace workphone::resource;namespace fs=std::filesystem;
    inline void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
    inline bool matches(const Array<u8> &bytes,const std::string &expected,size_t offset)
    {return offset<=expected.size()&&bytes.size()<=expected.size()-offset&&std::memcmp(bytes.data(),expected.data()+offset,bytes.size())==0;}
    inline ResourceChunkResult wait(ResourceChunkStream &stream)
    {
        ResourceChunkResult out;const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!stream.take(out)&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(out.generation!=0,"Verified range completes before deadline");return out;
    }
    inline void run(const fs::path &root)
    {
        class Compiler final : public IResourceCompiler
        {
        public:
            String name()const override{return "ChunkPattern";}
            Array<CompilerOutput> outputs()const override{return {{ResourceTypeID("rtres"),1}};}
            CompilationStatus compile(const CompileContext &context,std::ostream &out,Array<String>&)const override
            {std::ifstream in(fs::u8path(context.sourcePath.c_str()),std::ios::binary);out<<in.rdbuf();return out?CompilationStatus::Success:CompilationStatus::Failure;}
        };
        fs::create_directories(root/"source");std::string expected(3*65536+133,'\0');
        for(size_t i=0;i<expected.size();++i)expected[i]=char((i*17+13)%251);
        {std::ofstream out(root/"source/root.rtres",std::ios::binary);out.write(expected.data(),expected.size());require(bool(out),"Chunk source fixture");}
        String error;auto registry=std::make_shared<ResourceCompilerRegistry>();require(registry->registerCompiler(std::make_shared<Compiler>(),&error),error.c_str());
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;
        config.sourceRoot=(root/"source").u8string().c_str();config.compiledRoot=(root/"cooked").u8string().c_str();config.target="chunks";
        require(author.initialize(config,error),error.c_str());const ResourceID id("data://root.rtres");CompilationOptions options;options.packagedBuild=true;
        require(author.compile(id,options).succeeded(),"Compile chunk fixture");
        const auto package=root/"package";require(author.writeRuntimePackage(package.u8string().c_str(),{id},{},error),error.c_str());author.shutdown();
        fs::rename(root/"source",root/"source-unavailable");fs::rename(root/"cooked",root/"cooked-unavailable");
        RuntimeResourceConfig runtime;runtime.compiledRoot=package.u8string().c_str();runtime.manifestPath=(package/"runtime.wprm").u8string().c_str();runtime.target="chunks";
        ResourceSystem reader(nullptr,nullptr);require(reader.initializeRuntime(runtime,error),error.c_str());RuntimeResourceRangeInfo info;
        require(reader.resourceInfo(id,info,error)&&info.manifestVersion==4&&info.independentlyVerified&&info.chunkCount==4&&info.chunkBytes==65536,
            "Default exports publish a bounded manifest-v4 chunk directory");
        fs::path artifact;CompiledResourceHeader actual;
        for(const auto &entry:fs::recursive_directory_iterator(package))if(entry.is_regular_file()&&entry.path().filename()!="runtime.wprm") {
            CompiledResourceHeader header;if(CompiledResourceIO::readHeader(entry.path().u8string().c_str(),header,error)&&header.resourceId==id){artifact=entry.path();actual=header;break;}
        }
        require(!artifact.empty(),"Locate pinned chunk artifact");const auto payloadOffset=fs::file_size(artifact)-actual.payloadSize;
        auto damage=[&](u64 offset,bool broken){std::fstream file(artifact,std::ios::binary|std::ios::in|std::ios::out);file.seekp(payloadOffset+offset);
            const char value=broken?char(static_cast<unsigned char>(expected[size_t(offset)])^1):expected[size_t(offset)];file.write(&value,1);require(bool(file),"Mutate chunk fixture");};
        damage(3*65536+5,true);auto untouched=reader.loadChunk(id,1,error);
        require(untouched&&untouched->bytes.size()==65536&&untouched->payloadBytesRead==65536&&
            matches(untouched->bytes,expected,65536),"A verified chunk reads independently of corrupted unrequested chunks");
        require(!reader.loadChunk(id,3,error)&&!error.empty(),"A requested corrupted chunk rejects against the sealed directory");
        auto scheduler=std::make_shared<WorldStreamingScheduler>();auto mount=ResourcePayloadMount::create(runtime,scheduler,{},error);require(mount!=nullptr,error.c_str());
        {
            const auto category=size_t(StreamMemoryClass::SourceCpu);
            const auto baseline=scheduler->stats().used[category],limit=scheduler->stats().limits[category];
            require(info.metadataBytes&&scheduler->setBudget(StreamMemoryClass::SourceCpu,baseline+info.metadataBytes-1,error),error.c_str());
            RuntimeResourceRangeInfo deniedInfo;
            require(!mount->resourceInfo(id,deniedInfo,error)&&!error.empty()&&!deniedInfo.admission,
                "Captured SourceCpu pressure rejects range metadata before copying its header");
            require(scheduler->stats().used[category]==baseline&&scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error),
                "Rejected range metadata releases its captured admission");
            RuntimeResourceRangeInfo capturedInfo;require(mount->resourceInfo(id,capturedInfo,error),error.c_str());
            require(scheduler->ownsBudget(capturedInfo.admission)&&scheduler->stats().used[category]>=baseline+info.metadataBytes,
                "Range metadata retains the captured scheduler admission alongside physical accounting");capturedInfo={};
            require(scheduler->stats().used[category]==baseline,"Released range metadata retires its captured SourceCpu lease");
            auto sourceBounded=ResourceChunkStream::create(mount,error);require(sourceBounded!=nullptr,error.c_str());
            auto working=scheduler->reserve(StreamMemoryClass::WorkingCpu,scheduler->stats().limits[size_t(StreamMemoryClass::WorkingCpu)],error);require(working!=nullptr,error.c_str());
            require(sourceBounded->request(600,id,0,17,0,0,error),error.c_str());const auto pinned=scheduler->stats().used[category];
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,pinned+info.metadataBytes+65536-1,error),error.c_str());
            const auto rejections=scheduler->stats().rejectedReservations[category];working.reset();auto denied=wait(*sourceBounded);
            require(!denied.resource&&!denied.error.empty()&&scheduler->stats().rejectedReservations[category]>rejections,
                "Captured SourceCpu pressure rejects raw chunk storage before artifact reading");sourceBounded->drain();
            require(scheduler->stats().used[category]==baseline&&scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error),
                "Failed range work and scoped drain release captured metadata and raw storage");
            sourceBounded->setResidencyLimits(1,65536);
            require(sourceBounded->request(601,id,0,17,0,0,error),error.c_str());auto retained=wait(*sourceBounded);sourceBounded->drain();
            require(retained.resource&&scheduler->ownsBudget(retained.resource->admission)&&scheduler->stats().used[category]>baseline,
                "Successful assembled range storage remains charged to its captured scheduler");retained={};
            require(scheduler->stats().used[category]>=baseline+info.metadataBytes+65536,
                "Resident raw chunks retain captured SourceCpu admission after assembled consumers retire");sourceBounded->setResidencyLimits(0,0);
            require(scheduler->stats().used[category]==baseline,"Range eviction retires its final captured SourceCpu storage");
        }
        struct Marker final : RuntimeResourceStreamData {std::shared_ptr<const RuntimeResourceChunk> bootstrap;};
        class Provider final : public ResourceLeafProvider
        {
        public:
            Provider(std::shared_ptr<ResourcePayloadMount> mount,const RuntimeResourceRangeInfo &info,bool grow,std::shared_ptr<std::atomic<u32>> polls,
                std::shared_ptr<std::atomic<s32>> priority,String &error):m_info(info),m_grow(grow),m_polls(std::move(polls)),m_priority(std::move(priority))
            {m_stream=ResourceChunkStream::create(std::move(mount),error);require(m_stream!=nullptr,error.c_str());}
            ResourceLeafStatus poll(std::shared_ptr<const RuntimeResource> &out,String &error) override
            {
                ++*m_polls;if(m_grow){m_estimate=1u<<20;return ResourceLeafStatus::Pending;}
                if(!m_requested){if(!m_stream->request(1,m_info.header.resourceId,0,17,0,0,error))return ResourceLeafStatus::Error;m_requested=true;}
                ResourceChunkResult bytes;if(!m_stream->take(bytes))return ResourceLeafStatus::Pending;
                if(!bytes.resource){error=bytes.error;return ResourceLeafStatus::Error;}
                auto marker=std::make_shared<Marker>();marker->bootstrap=bytes.resource;auto resource=std::make_shared<RuntimeResource>();
                resource->header=m_info.header;resource->streamData=marker;out=resource;return ResourceLeafStatus::Ready;
            }
            u64 maximumResidentBytes()const override{return m_estimate;}
            void setPriority(s32 priority)override{m_priority->store(priority);}
            void cancel()override{m_stream->cancel();}
            void drain()override{m_stream->drain();}
        private:
            RuntimeResourceRangeInfo m_info;bool m_grow=false,m_requested=false;u64 m_estimate=16384;
            std::shared_ptr<std::atomic<u32>> m_polls;std::unique_ptr<ResourceChunkStream> m_stream;
            std::shared_ptr<std::atomic<s32>> m_priority;
        };
        class Factory final : public ResourceLeafProviderFactory
        {
        public:
            bool grow=false;std::shared_ptr<std::atomic<u32>> polls=std::make_shared<std::atomic<u32>>(0);
            std::shared_ptr<std::atomic<s32>> priority=std::make_shared<std::atomic<s32>>(0);
            bool handles(const ResourceID &value)const override{return value==ResourceID("data://root.rtres");}
            std::shared_ptr<ResourceLeafProvider> create(std::shared_ptr<ResourcePayloadMount> view,const ResourceID &,const RuntimeResourceRangeInfo &pin,s32 requestPriority,String &error)override
            {priority->store(requestPriority);return std::make_shared<Provider>(std::move(view),pin,grow,polls,priority,error);}
        };
        auto factory=std::make_shared<Factory>();auto cold=ResourceClosureStream::create(mount,error,factory);require(cold!=nullptr,error.c_str());
        ResourceClosureLimits coldLimits;coldLimits.maximumResourcePayloadBytes=32768;coldLimits.maximumClosurePayloadBytes=32768;coldLimits.maximumResources=1;
        require(cold->request(700,id,coldLimits,0,error)&&cold->request(700,id,coldLimits,73,error),error.c_str());
        ResourcePayloadResult sparse;const auto coldDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!cold->take(sparse)&&std::chrono::steady_clock::now()<coldDeadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(sparse.resource&&sparse.resource->payload.empty()&&sparse.resource->header.payloadSize>coldLimits.maximumResourcePayloadBytes,
            "Cold sparse leaf admission uses bounded resident metadata/bootstrap instead of full logical payload extent");
        require(factory->priority->load()==73,"Reused sparse providers inherit accepted replacement priority before polling");
        auto descriptor=std::dynamic_pointer_cast<const Marker>(sparse.resource->streamData);
        require(descriptor&&descriptor->bootstrap&&descriptor->bootstrap->bytes.size()==17&&descriptor->bootstrap->payloadBytesRead==65536,
            "Worker graph preserves independently leased sparse stream metadata without reading the corrupted distant chunk");cold->drain();
        auto growing=std::make_shared<Factory>();growing->grow=true;auto bounded=ResourceClosureStream::create(mount,error,growing);require(bounded!=nullptr,error.c_str());
        require(bounded->request(701,id,coldLimits,0,error),error.c_str());ResourcePayloadResult rejected;
        require(bounded->take(rejected)&&!rejected.resource&&!rejected.error.empty()&&growing->polls->load()==1,
            "A raised provider estimate is rejected before any later whole-body polling/read");bounded->drain();
        cold.reset();bounded.reset();sparse={};descriptor.reset();
        const auto relative=fs::relative(artifact,package).generic_u8string();
        resource_runtime_contracts::writeManifestFixture(package/"runtime.wprm",runtime.target,{actual},3,{{id.str(),relative.c_str()}});
        ResourceSystem legacy(nullptr,nullptr);require(legacy.initializeRuntime(runtime,error),error.c_str());RuntimeResourceRangeInfo legacyInfo;
        require(legacy.resourceInfo(id,legacyInfo,error)&&!legacyInfo.independentlyVerified&&legacyInfo.manifestVersion==3,"Legacy manifests remain readable without invented chunk pins");
        require(!legacy.loadChunk(id,1,error),"Legacy range fallback checks the entire payload and notices corruption outside the requested chunk");
        bool preserved=false;std::thread owner([&]{String issue;auto view=mount->createReadOnlyOwnerView(issue);RuntimeResourceRangeInfo captured;
            preserved=view&&view->resourceInfo(id,captured,issue)&&captured.manifestVersion==4&&captured.independentlyVerified;
            auto same=mount->createReadOnlyOwnerView(issue);preserved=preserved&&same==view;});owner.join();
        require(preserved,"Cross-owner views preserve the exact captured generation without reparsing a replaced manifest");
        damage(3*65536+5,false);auto checkedLegacy=legacy.loadChunk(id,1,error);
        require(checkedLegacy&&checkedLegacy->legacyWholePayloadFallback&&checkedLegacy->payloadBytesRead==expected.size(),"Legacy fallback returns a checked slice and reports full-payload I/O");
        auto first=ResourceChunkStream::create(mount,error),second=ResourceChunkStream::create(mount,error);require(first&&second,"Shared verified range streams initialize");
        auto pressure=scheduler->reserve(StreamMemoryClass::WorkingCpu,scheduler->stats().limits[size_t(StreamMemoryClass::WorkingCpu)],error);require(pressure!=nullptr,error.c_str());
        require(first->request(1,id,10,40,1,1,error)&&second->request(2,id,100,60,2,2,error),"Overlapping requests admit one shared chunk");
        require(scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]==65536,"Shared pending ranges reserve one physical chunk upfront");
        auto progress=second->progress(2);require(progress.requestedBytes==60&&progress.readyBytes==0&&progress.reservedBytes==65536&&progress.pendingChunks==1,"Progress distinguishes requested, reserved and ready bytes");
        require(first->cancel(1)&&second->setPriority(2,100),"One range cancellation preserves another interest and priority");pressure.reset();auto shared=wait(*second);
        require(shared.resource&&shared.key==2&&matches(shared.resource->bytes,expected,100),"Verified range assembly extracts exactly the requested bytes");
        first->drain();second->drain();require(!scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)],"Consumed uncached chunks retire ResultCpu while assembled bytes retain SourceCpu");
        first->setResidencyLimits(2,2*65536);
        require(first->request(3,id,65530,65550,0,10,error),error.c_str());auto crossing=wait(*first);require(crossing.resource&&crossing.resource->bytes.size()==65550&&
            matches(crossing.resource->bytes,expected,65530),"Unaligned multi-chunk assembly has exact contiguous coverage");
        first->drain();require(first->residentBytes()==2*65536,"Raw residency respects its configured entry/byte quota");
        damage(65536+5,true);require(first->request(4,id,65536+4,8,0,10,error),error.c_str());auto cached=wait(*first);
        require(cached.resource&&cached.resource->payloadBytesRead==0,"Resident chunks preserve verified bytes and perform no new artifact I/O");first->drain();
        require(first->setResidencyPriority(id,65536,65536,-10),"Already resident chunks accept a lower residency priority");
        require(first->request(5,id,3*65536,133,0,5,error),error.c_str());auto tail=wait(*first);require(tail.resource&&tail.resource->bytes.size()==133,"Tail chunk bounds use pinned payload size");first->drain();
        require(first->request(6,id,65536,16,0,0,error),error.c_str());auto evicted=wait(*first);
        require(!evicted.resource&&!evicted.error.empty()&&crossing.resource,"Lower-priority chunk eviction causes checked reread failure while caller fallback remains valid");
        damage(65536+5,false);first->drain();second->drain();first->setResidencyLimits(0,0);
        auto ownerBudget=first->reserveOwner(StreamMemoryClass::Gpu,4096,error);require(ownerBudget&&scheduler->stats().used[size_t(StreamMemoryClass::Gpu)]==4096,
            "Native owner participation uses the same captured scheduler ledger");ownerBudget.reset();
        first.reset();second.reset();mount.reset();require(!scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)],"Range cache and retired work release all raw chunk reservations");
    }
}
