#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/RuntimeSceneLeafFactory.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <set>

namespace workphone::resource
{
    namespace
    {
        class MeshMetadataLeaf final : public ResourceLeafProvider
        {
            struct Owned {StreamBudget admission;RuntimeResource value;};
            std::shared_ptr<const RuntimeResource> m_value;
            u64 m_bytes=0;
        public:
            MeshMetadataLeaf(const RuntimeResourceRangeInfo &info,const std::shared_ptr<ResourcePayloadMount> &mount,String &error)
            {
                m_bytes=8192;
                for(const auto &id:info.header.installDependencies)m_bytes+=256+4*id.str().size();
                for(const auto &pin:info.header.installArtifacts)m_bytes+=256+4*pin.path.size();
                auto lease=mount->reserveOwner(StreamMemoryClass::SourceCpu,m_bytes,error);
                if(!lease)throw std::runtime_error(error.c_str());
                auto owner=std::make_shared<Owned>();owner->admission=std::move(lease);owner->value.header=info.header;
                owner->value.streamData=std::make_shared<RuntimeResourceStreamData>();
                m_value=std::shared_ptr<const RuntimeResource>(owner,&owner->value);
            }
            ResourceLeafStatus poll(std::shared_ptr<const RuntimeResource> &value,String &error) override
            {error.clear();if(!m_value){error="LOD metadata request was cancelled";return ResourceLeafStatus::Error;}value=m_value;return ResourceLeafStatus::Ready;}
            u64 maximumResidentBytes() const override{return m_bytes;}
            void cancel() override{m_value.reset();}
            void drain() override{cancel();}
        };
        class SceneLeaves final : public ResourceLeafProviderFactory
        {
            StreamBudget m_factoryBudget;
            std::shared_ptr<ResourceLeafProviderFactory> m_graphics,m_audio;
            StreamBudget m_recipeBudget;
            std::set<ResourceID> m_meshes;
            std::shared_ptr<ResourcePayloadMount> m_mount;
        public:
            SceneLeaves(std::shared_ptr<ResourceLeafProviderFactory> graphics,std::shared_ptr<ResourceLeafProviderFactory> audio,std::shared_ptr<ResourcePayloadMount> mount):m_graphics(std::move(graphics)),m_audio(std::move(audio)),m_mount(std::move(mount))
            {String error;m_factoryBudget=m_mount->reserveOwner(StreamMemoryClass::SourceCpu,4096,error);if(!m_factoryBudget)throw std::runtime_error(error.c_str());}
            std::shared_ptr<ResourceLeafProviderFactory> cloneForRequest(const ResourceID &,String &error) override
            {error.clear();return std::make_shared<SceneLeaves>(m_graphics,m_audio,m_mount);}
            bool handles(const ResourceID &id) const override
            {return id.type()==ResourceTypeID("folpage")||m_meshes.count(id)||(m_graphics&&m_graphics->handles(id))||(m_audio&&m_audio->handles(id));}
            bool observeResourceReady(const std::shared_ptr<const RuntimeResource> &value,String &error) override
            {
                if(!value){error="Missing runtime resource";return false;}
                if(value->header.resourceType==ResourceTypeID("fbscene")||value->header.resourceType==ResourceTypeID("prefab")) {
                    Array<ResourceID> meshes;if(!inspectActorGraphStreamingMeshes(*value,meshes,error))return false;
                    u64 bytes=4096;for(const auto &id:meshes)bytes+=256+4*id.str().size();
                    auto admission=m_mount->reserveOwner(StreamMemoryClass::SourceCpu,bytes,error);if(!admission)return false;
                    std::set<ResourceID> staged(meshes.begin(),meshes.end());m_meshes.swap(staged);m_recipeBudget=std::move(admission);
                }
                if(value->header.resourceType==ResourceTypeID("skinres"))for(const auto &dependency:value->header.installDependencies)
                    if(m_meshes.count(dependency)){error="A streamed LOD mesh cannot be a retained skin dependency";return false;}
                return (!m_graphics||m_graphics->observeResourceReady(value,error))&&(!m_audio||m_audio->observeResourceReady(value,error));
            }
            std::shared_ptr<ResourceLeafProvider> create(std::shared_ptr<ResourcePayloadMount> mount,const ResourceID &id,
                const RuntimeResourceRangeInfo &info,s32 priority,String &error) override
            {
                try {
                    if(id.type()==ResourceTypeID("folpage")||m_meshes.count(id))return std::make_shared<MeshMetadataLeaf>(info,mount,error);
                    if(m_graphics&&m_graphics->handles(id))return m_graphics->create(std::move(mount),id,info,priority,error);
                    if(m_audio&&m_audio->handles(id))return m_audio->create(std::move(mount),id,info,priority,error);
                    error="No native sparse provider for resource";return {};
                }catch(const std::exception &failure){error=failure.what();return {};}
            }
        };
    }
    std::shared_ptr<ResourceLeafProviderFactory> createRuntimeSceneLeafFactory(std::shared_ptr<ResourcePayloadMount> mount,String &error)
    {
        error.clear();try {
            if(!mount){error="Runtime leaf loaders require a captured mount";return {};}
            auto app=core::IApplicationManager::instancePtr();
            auto graphics=app?app->getGraphicsSystem():nullptr;auto audio=app?app->getSoundManager():nullptr;
            auto graphicsFactory=graphics?graphics->createRuntimeLeafProviderFactory(mount,error):nullptr;if(!error.empty())return {};
            auto audioFactory=audio?audio->createRuntimeLeafProviderFactory(mount,error):nullptr;if(!error.empty())return {};
            return std::make_shared<SceneLeaves>(std::move(graphicsFactory),std::move(audioFactory),std::move(mount));
        }catch(const std::exception &failure){error=failure.what();return {};}
    }
}
