#include "WPGraphics/WPClawHammerPCH.hpp"
#include "MaterialShaderEditorDX11.hpp"
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Memory/ObjectObserver.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <workphone_graphics_renderer.h>
#include <mutex>
#include <unordered_map>

namespace workphone::render
{
    namespace
    {
        struct Entry
        {
            ObjectObserver<ClawMaterial> material;
            SourceBudgetString vertex,pixel,error;
            u64 version=0,processed=0,id=0;
            bool watching=true;
            MaterialProgramReloadDX11::Ptr candidate;
        };
        std::recursive_mutex mutex;
        using Pair=std::pair<const ClawMaterial *const,std::shared_ptr<Entry>>;
        std::unordered_map<const ClawMaterial *,std::shared_ptr<Entry>,std::hash<const ClawMaterial *>,
            std::equal_to<const ClawMaterial *>,SourceBudgetAllocator<Pair>> entries;
        std::shared_ptr<Entry> entry(ClawMaterial *material)
        {
            auto found=entries.find(material);
            if(found!=entries.end() && !found->second->material.expired()) return found->second;
            auto value=std::allocate_shared<Entry>(SourceBudgetAllocator<Entry>{});value->material=material;
            entries[material]=value;return value;
        }
    }
    namespace detail
    {
        void shaderReloadAttached(ClawMaterial *material,u64 id)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            if(!id && entries.find(material)==entries.end()) return;
            auto value=entry(material);value->id=id;
        }
        void forgetShaderReload(ClawMaterial *material) noexcept
        { try { std::lock_guard<std::recursive_mutex> lock(mutex);entries.erase(material); } catch(...) {} }
        void appendShaderFiles(const ClawMaterial *material,const SmartPtr<Properties> &properties)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            auto found=entries.find(material);if(found==entries.end()) return;
            properties->setProperty("runtimeShaderVertexFile",String(found->second->vertex.c_str()));
            properties->setProperty("runtimeShaderPixelFile",String(found->second->pixel.c_str()));
            properties->setProperty("runtimeShaderWatching",found->second->watching);
        }
    }
    bool ClawMaterial::requestRuntimeShaderFiles(const String &vertex,const String &pixel)
    {
        if(pixel.empty()) return false;
        SourceBudgetString v=vertex.c_str(),p=pixel.c_str();
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto value=entry(this);value->vertex=std::move(v);value->pixel=std::move(p);++value->version;value->error.clear();return true;
    }
    bool ClawMaterial::requestRuntimeShaderReload()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto found=entries.find(this);if(found==entries.end()) return false;
        auto &value=*found->second;
        if(value.id && MaterialProgramReloadDX11::QueueReload(value.id)) return true;
        if(value.pixel.empty()) return false;
        ++value.version;value.error.clear();return true;
    }
    bool ClawMaterial::setRuntimeShaderWatching(bool enabled)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto found=entries.find(this);if(found==entries.end()) return false;
        auto &value=*found->second;
        if(value.pixel.empty() && !value.id) return false;
        value.watching=enabled;
        if(value.id) MaterialProgramReloadDX11::QueueWatching(value.id,enabled);
        return true;
    }
    SmartPtr<Properties> ClawMaterial::getRuntimeShaderReloadStatus() const
    {
        u64 id=0;bool pending=false,configured=false,watching=false;String vertex,pixel,error;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            auto found=entries.find(this);
            if(found!=entries.end())
            {
                const auto &value=*found->second;id=value.id;pending=value.version!=value.processed;
                vertex=value.vertex.c_str();pixel=value.pixel.c_str();error=value.error.c_str();watching=value.watching;configured=!value.pixel.empty();
            }
        }
        auto status=MaterialProgramReloadDX11::EditorStatus(id);
        bool available=false;status->getPropertyValue("available",available);
        if(configured || pending)
        {
            status->setProperty("available",true);status->setProperty("vertexFile",vertex);status->setProperty("pixelFile",pixel);
            status->setProperty("watching",watching);
        }
        if(pending) { status->setProperty("pending",true);status->setProperty("status",String("queued")); }
        if(!error.empty()) { status->setProperty("diagnostic",error);status->setProperty("success",false);status->setProperty("status",String("failed")); }
        if(!available && !configured) status->setProperty("diagnostic",String("No reloadable shader is configured for this material."));
        return status;
    }
    void ClawMaterial::processRuntimeShaderReloads(ClawRendererDX11 &renderer)
    {
        auto native=renderer.getNativeRenderer();auto dx=native?wp_renderer_get_dx11(native):nullptr;
        if(!dx) return;
        for(;;)
        {
            std::shared_ptr<Entry> work;u64 version=0;String vertex,pixel;bool build=false;
            {
                std::lock_guard<std::recursive_mutex> lock(mutex);
                for(auto it=entries.begin();it!=entries.end();)
                {
                    if(it->second->material.expired()) { it=entries.erase(it);continue; }
                    auto &value=*it->second;
                    if(value.version!=value.processed || (value.candidate && value.candidate->HasProgram()))
                    {
                        work=it->second;version=value.version;build=value.version!=value.processed;
                        vertex=value.vertex.c_str();pixel=value.pixel.c_str();break;
                    }
                    ++it;
                }
            }
            if(!work) break;
            auto material=work->material.lock();if(!material) continue;
            try
            {
                String error;
                auto candidate=build?MaterialProgramReloadDX11::CreateFromFiles(wp_renderer_dx11_get_device(dx),
                    material->getShaderParameters().GetLayout(),vertex,pixel,1,error):work->candidate;
                std::lock_guard<std::recursive_mutex> lock(mutex);
                if(work->version!=version) continue;
                work->processed=version;
                if(!candidate) { work->error=error.c_str();work->candidate.reset();continue; }
                work->id=candidate->GetId();work->candidate=candidate;
                MaterialProgramReloadDX11::QueueWatching(work->id,work->watching);
                if(candidate->HasProgram())
                {
                    // Candidate preparation precedes this owner-thread installation.
                    material->setRuntimeReload(candidate,material->runtimeShaderResources());
                    work->candidate.reset();work->error.clear();
                }
            }
            catch(const std::exception &error)
            {
                std::lock_guard<std::recursive_mutex> lock(mutex);
                if(work->version==version) {work->processed=version;work->error=error.what();work->candidate.reset();}
            }
        }
    }
}
