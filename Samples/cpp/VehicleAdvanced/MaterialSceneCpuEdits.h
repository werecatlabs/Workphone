#pragma once
#if defined(_WIN32) && WP_GRAPHICS_SYSTEM_CLAW
#include "MaterialSceneCpuProfile.h"
#include <WPGraphics/ClawMaterial.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Graphics/MaterialRenderState.hpp>
#include <set>
#include <sstream>
#include <vector>

namespace workphone::advanced
{
    class MaterialSceneCpuEdits
    {
        struct Entry
        {
            SmartPtr<render::ClawMaterial> material;
            ColourF original,expected,oldColour;
            f32 oldRoughness=0,roughness=0;
            render::MaterialShaderParameterHandle tint;
            MaterialRenderState::Ptr before;
            u64 revision=0;
            bool custom=false;
        };
    public:
        explicit MaterialSceneCpuEdits(const SceneAssets &assets)
        {
            const auto value=std::getenv("WP_MATERIAL_SCENE_EDITS");
            mode=value?value:"none";
            if(mode!="none" && mode!="single" && mode!="all") throw std::runtime_error("Invalid material edit workload");
            std::set<render::ClawMaterial *> unique;
            for(const auto &owner:assets.materials)
            {
                auto material=dynamic_pointer_cast<render::ClawMaterial>(owner);
                if(!material || !unique.insert(material.get()).second) continue;
                Entry entry;entry.material=material;entry.original=material->getDiffuse();entry.custom=material->hasRuntimeProgram();
                if(entry.custom)
                {
                    entry.tint=material->getShaderParameters().FindParameter("Tint");
                    if(!entry.tint.IsValid()) throw std::runtime_error("Edited custom scene requires a typed Tint handle");
                    ++customMaterials;
                }
                entries.push_back(std::move(entry));
            }
            if(entries.empty()) throw std::runtime_error("Material edits require a real material population");
        }

        void apply(MaterialSceneCpuProfile &cpu)
        {
            using namespace material_scene_cpu;
            ++frames;
            if(mode=="none") return;
            const size_t count=mode=="single"?1:entries.size();
            const size_t first=mode=="single"?(frames-1)%entries.size():0;
            cpu.beginStage(EditVerification);
            for(size_t i=0;i<count;++i)
            {
                auto &e=entries[(first+i)%entries.size()];e.before=e.material->getRenderState();
                if(!e.before) throw std::runtime_error("Edited material has no immutable publication");
                e.revision=e.before->revision;e.oldColour=e.before->diffuseColour;e.oldRoughness=e.before->roughness;
                // Alternate on each visit, including the rotating single case.
                const bool alternate=(mode=="single"?(frames-1)/entries.size():frames-1)%2!=0;
                const f32 scale=alternate?.85f:1.f;
                e.expected={e.original.r*scale,e.original.g*scale,e.original.b*scale,e.original.a};
                e.roughness=e.oldRoughness==.35f?.65f:.35f;
            }
            cpu.endStage(EditVerification);
            MaterialPublicationScope publication;
            cpu.beginStage(Authoring);
            for(size_t i=0;i<count;++i)
            {
                auto &e=entries[(first+i)%entries.size()];
                e.material->setDiffuse(e.expected);e.material->setRoughness(e.roughness);
                if(e.custom)
                {
                    if(!e.material->getShaderParameters().SetColour(e.tint,e.expected))
                        throw std::runtime_error("Cached custom Tint edit failed");
                    ++customTintEdits;
                }
            }
            cpu.endStage(Authoring);
            cpu.beginStage(EditPublication);publication.commit();cpu.endStage(EditPublication);
            cpu.beginStage(EditVerification);
            for(size_t i=0;i<count;++i)
            {
                auto &e=entries[(first+i)%entries.size()];auto current=e.material->getRenderState();
                if(!current || current==e.before || current->revision<=e.revision ||
                   current->diffuseColour!=e.expected || current->roughness!=e.roughness ||
                   e.before->revision!=e.revision || e.before->diffuseColour!=e.oldColour || e.before->roughness!=e.oldRoughness ||
                   (e.custom && e.material->getShaderParameters().GetColour(e.tint)!=e.expected))
                    throw std::runtime_error("Material edit publication/value/retained-snapshot verification failed");
                ++verifiedRevisions;
            }
            cpu.endStage(EditVerification);
            // Retained verification pins can otherwise defer retirement outside
            // the measured publication lifecycle. Count their release explicitly.
            cpu.beginStage(EditPublication);
            for(size_t i=0;i<count;++i) entries[(first+i)%entries.size()].before.reset();
            cpu.endStage(EditPublication);
            materialsEdited+=count;++changedFrames;
        }

        std::string json() const
        {
            std::ostringstream out;
            out<<"{\"mode\":\""<<mode<<"\",\"eligibleMaterials\":"<<entries.size()
               <<",\"customMaterials\":"<<customMaterials<<",\"frames\":"<<frames
               <<",\"changedFrames\":"<<changedFrames<<",\"materialsEdited\":"<<materialsEdited
               <<",\"customTintEdits\":"<<customTintEdits<<",\"verifiedRevisions\":"<<verifiedRevisions<<"}";
            return out.str();
        }
    private:
        std::string mode;
        std::vector<Entry> entries;
        u64 frames=0,changedFrames=0,materialsEdited=0,customTintEdits=0,verifiedRevisions=0,customMaterials=0;
    };
}
#endif
