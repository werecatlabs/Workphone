#pragma once
#include <Workphone/System/StateContext.hpp>
#include <Workphone/State/States/State.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/State/States/MaterialStateData.hpp>
#include <functional>
#include <stdexcept>

namespace material_state_lookup_contracts
{
    using namespace workphone;
    inline void require(bool value,const char *message)
    { if(!value) throw std::runtime_error(message); }
    class Context final : public StateContext
    {
    public:
        mutable u32 snapshots = 0;
        Array<SmartPtr<IState>> getStates() const override
        { ++snapshots; return StateContext::getStates(); }
    };
    class Record final : public State
    {
    public:
        u32 dirties = 0;
        std::function<void()> onDirty;
        void setDirty(bool dirty) override
        { ++dirties; if(onDirty) onDirty(); State::setDirty(dirty); }
    };
    template<class Fixture> void run(Fixture &)
    {
        auto pass=make_ptr<render::ClawMaterialPass>();
        auto context=make_ptr<Context>(); context->load(nullptr);
        pass->setStateContext(context);
        auto record=make_ptr<Record>();record->setId(pass->getId());
        auto data=make_ptr<MaterialPassStateData>();record->setData(data);
        context->addState(record);
        const auto old=data->getRenderState();context->snapshots=0;
        {
            MaterialPublicationScope update;
            pass->setDiffuse(ColourF(.2f,.4f,.6f,1));pass->setRoughness(.3f);
            require(data->getRenderState()==old,"ID lookup preserves deferred immutable publication");
        }
        require(!context->snapshots && record->dirties==2 &&
            data->getRenderState()->revision==old->revision+1 && data->getRenderState()->roughness==.3f,
            "single-state pass writes avoid registration snapshots and retain dirty/publication semantics");
        {
            const auto scheduler=WorldStreamingScheduler::shared();
            const auto stats=scheduler->stats();const auto source=size_t(StreamMemoryClass::SourceCpu);
            struct Restore
            {
                std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;
                ~Restore(){String error;scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error);}
            } restore{scheduler,stats.limits[source]};
            String error;
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,stats.used[source],error),"fix lookup quota at current residency");
            require(context->getStateById(pass->getId())==record &&
                scheduler->stats().used[source]==stats.used[source],
                "pinned single-state lookup succeeds without admission headroom");
        }
        context->removeState(record);
        auto wrong=make_ptr<Record>();wrong->setId(pass->getId());wrong->setData(make_ptr<MaterialStateData>());
        context->addState(wrong);context->addState(record);context->snapshots=0;
        pass->setRoughness(.7f);
        require(context->snapshots==1 && wrong->dirties==0 && data->getRenderState()->roughness==.7f,
            "a shared context skips a wrong-type duplicate ID through the original derived search");
        context->removeState(record);const auto retained=data->getRenderState();
        pass->setRoughness(.8f);
        require(data->getRenderState()==retained,"removed state data is never edited through a stale cached lookup");
        context->removeState(wrong);context->addState(record);
        bool removeOnce=true;
        record->onDirty=[&] { if(removeOnce){removeOnce=false;context->removeState(record);} };
        pass->setRoughness(.9f);
        require(!context->getStateById(pass->getId()) && data->getRenderState()->roughness==.9f,
            "dirty callbacks run outside the container lock while the selected state/data stay pinned");
        record->onDirty={};pass->setStateContext(nullptr);context->unload(nullptr);
    }
}
