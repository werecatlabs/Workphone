#pragma once
#include "ProceduralHelperBodyBudgetContracts.hpp"
#include <WPProcedural/CSkyAtmosphere.hpp>
#include <Workphone/Scene/Components/ProceduralSky.hpp>
namespace sky_body_budget_contracts
{
    using namespace procedural_helper_body_budget_contracts;
    inline void run()
    {
        Budget budget;
        const auto baseline = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof(WPSkyAtmosphere), alignof(std::max_align_t) );
        budget.set(baseline + bytes - 1);
        denied([]{ auto sky = std::make_unique<WPSkyAtmosphere>(); });
        budget.set(baseline + bytes);
        auto sky = std::make_unique<WPSkyAtmosphere>();
        SkyState state; state.timeOfDay = 7;
        sky->setState(state); sky->update();
        WPSkyAtmosphere reference; reference.setState(state); reference.update();
        check(budget.used() == baseline + bytes && sky->getResults().sunDir == reference.getResults().sunDir,
              "Sky helper admits one exact body; embedded state and sampling add no lease");
        sky.reset(); check(budget.used() == baseline, "Sky helper retires at the cap");
        budget.set(budget.original);
        {
            FactoryTemplate<CSkyAtmosphere> factory;
            const auto before = budget.used();
            const auto body = sourceBudgetStorageBytes(sizeof(CSkyAtmosphere), alignof(CSkyAtmosphere));
            budget.set(before + body - 1); denied([&]{ factory.allocateMemory(); });
            budget.set(before + body); auto memory = factory.allocateMemory();
            check(budget.used() == before + body, "Sky adapter factory admits its body");
            factory.freeMemory(memory); check(budget.used() == before, "Sky factory retires at cap");
            budget.set(budget.original);
        }
        {
            SurfaceRuntime runtime;
            auto producer = make_ptr<FactoryTemplate<CSkyAtmosphere>>();
            producer->setObjectTypeName("ISkyAtmosphere");
            producer->setObjectTypeHash(StringUtil::getHash64("ISkyAtmosphere"));
            runtime.factories->addFactory(producer); producer->setPool(nullptr);
            auto component = make_ptr<scene::ProceduralSky>();
            check(component->regenerate(), "Sky component uses real factory");
            const auto old = component->getResults(); component->setAtmosphere(nullptr);
            budget.set(budget.used());
            check(!component->regenerate() && component->getResults().sunDir == old.sunDir,
                  "Denied cold sky body returns false and preserves prior results");
            budget.set(budget.original);
            check(component->regenerate() && component->getResults().sunDir == old.sunDir,
                  "Sky factory admission retries with deterministic output");
        }
    }
}
