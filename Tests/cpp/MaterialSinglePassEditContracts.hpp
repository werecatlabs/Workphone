#pragma once

#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <functional>
#include <stdexcept>

namespace material_single_pass_edits
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require(bool value, const char *message)
    { if(!value) throw std::runtime_error(message); }

    class Technique final : public ClawMaterialTechnique
    {
    public:
        mutable u32 snapshots = 0;
        Array<SmartPtr<IMaterialPass>> getPasses() const override
        { ++snapshots; return ClawMaterialTechnique::getPasses(); }
    };
    class Pass final : public ClawMaterialPass
    {
    public:
        u32 diffuseCalls = 0, roughnessCalls = 0;
        std::function<void()> onRoughness;
        void setDiffuse(const ColourF &value) override
        { ++diffuseCalls; ClawMaterialPass::setDiffuse(value); }
        void setRoughness(f32 value) override
        {
            ++roughnessCalls;
            if(onRoughness) onRoughness();
            ClawMaterialPass::setRoughness(value);
        }
    };

    template<class Fixture> void run(Fixture &)
    {
        auto material = make_ptr<ClawMaterial>();
        auto technique = make_ptr<Technique>(); technique->setMaterial(material.get());
        auto first = make_ptr<Pass>(); technique->setPasses({first});
        material->setTechniques({technique});
        auto state = dynamic_pointer_cast<MaterialPassStateData>(
            first->getStateContext()->getStateById(first->getId())->getData());
        const auto original = state->getRenderState();
        technique->snapshots = 0;
        {
            MaterialPublicationScope update;
            material->setDiffuse(ColourF(.2f,.4f,.6f,1));
            material->setRoughness(.25f);
            require(state->getRenderState() == original, "single-pass edits retain the old publication until commit");
        }
        const auto current = state->getRenderState();
        require(technique->snapshots == 0 && first->diffuseCalls == 1 && first->roughnessCalls == 1 &&
            current->revision == original->revision + 1 && current->roughness == .25f &&
            current->diffuseColour == ColourF(.2f,.4f,.6f,1),
            "single-pass writes avoid graph snapshots, preserve virtual setters and coalesce publication");

        auto second = make_ptr<Pass>(); technique->setPasses({first,second});
        technique->snapshots = 0;
        {
            MaterialPublicationScope update;
            material->setDiffuse(ColourF(.6f,.4f,.2f,1));
            material->setRoughness(.75f);
        }
        require(technique->snapshots == 2 && first->roughnessCalls == 2 && second->roughnessCalls == 1 &&
            first->getRoughness() == .75f && second->getRoughness() == .75f,
            "multi-pass fallback retains snapshot traversal and broadcasts both virtual edits");

        technique->setPasses({first});
        first->onRoughness = [&] { material->setTechniques({}); };
        material->setRoughness(.125f);
        require(!material->getRenderState() && state->getRenderState()->roughness == .125f,
            "a virtual edit may detach the graph without a container lock or lost selected-pass write");
        first->onRoughness = {};
    }
}
