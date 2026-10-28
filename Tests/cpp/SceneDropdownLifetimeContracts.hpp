#pragma once

#include <Workphone/Scene/Components/UI/Dropdown.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/ObjectObserver.hpp>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace scene_dropdown_lifetime_contracts
{
    using namespace workphone;

    class Actor final : public scene::GameActor
    {
    public:
        State getState() const override { return State::Create; }
        void setState( State, bool = true ) override {}
        bool isEnabledInScene() const override { return enabled; }
        bool isVisible() const override { return true; }
        bool enabled = true;
    };

    inline void click( SmartPtr<scene::Button> button )
    {
        button->handleEvent( EventType::UI, IEvent::CLICK_HASH, {}, nullptr, nullptr, nullptr );
    }

    inline unsigned run()
    {
        unsigned assertions = 0;
        const auto require = [&]( bool condition, const char *message ) {
            ++assertions;
            if( !condition ) throw std::runtime_error( message );
        };
        {
            auto actor = make_ptr<Actor>();
            auto dropdown = make_ptr<scene::Dropdown>();
            auto first = make_ptr<scene::Button>();
            auto replacement = make_ptr<scene::Button>();
            // Empty real actors let setOpen run its actual visibility path
            // without generating an unrelated composite UI hierarchy.
            dropdown->setActor( actor );
            dropdown->setContent( make_ptr<Actor>() );
            dropdown->setPanel( make_ptr<Actor>() );
            dropdown->setUseNativeUI( false );
            dropdown->setButton( first );
            dropdown->load( nullptr );
            dropdown->setEnabled( true );
            require( dropdown->isLoaded() && dropdown->getButton() == first,
                     "real dropdown load must retain its trigger binding" );
            click( first );
            require( dropdown->isOpen(), "a live trigger must open its dropdown" );
            click( first );
            require( !dropdown->isOpen(), "the same trigger must close its dropdown" );
            actor->enabled = false;
            click( first );
            require( !dropdown->isOpen(), "a disabled scene actor must suppress its dropdown trigger" );
            actor->enabled = true;
            dropdown->setButton( replacement );
            click( first );
            require( !dropdown->isOpen(), "replacing a trigger must detach the previous button" );
            click( replacement );
            require( dropdown->isOpen(), "a replacement trigger must receive the active binding" );
            dropdown->setButton( nullptr );
            click( replacement );
            require( dropdown->isOpen() && !dropdown->getButton(),
                     "clearing a binding must make a retained old trigger inert" );
            dropdown->setButton( replacement );
            click( replacement );
            require( !dropdown->isOpen(), "rebinding a detached trigger must restore one callback" );
            dropdown->unload( nullptr );
            dropdown->unload( nullptr );
            require( dropdown->getLoadingState() == LoadingState::Unloaded && !dropdown->getButton(),
                     "repeated dropdown unload must detach its trigger and release the binding" );
            click( replacement );
            require( !dropdown->isOpen(), "a retained trigger must remain inert after dropdown unload" );
            ObjectObserver<scene::Dropdown> retiredDropdown( dropdown );
            ObjectObserver<scene::Button> retiredFirst( first ), retiredReplacement( replacement );
            dropdown = nullptr;
            first = nullptr;
            replacement = nullptr;
            require( retiredDropdown.expired() && retiredFirst.expired() && retiredReplacement.expired(),
                     "normal unload must release dropdown and both triggers without factory-forced deletion" );
        }
        {
            auto dropdown = make_ptr<scene::Dropdown>();
            auto trigger = make_ptr<scene::Button>();
            ObjectObserver<scene::Button> retiredTrigger( trigger );
            dropdown->setButton( trigger );
            trigger = nullptr;
            require( retiredTrigger.expired() && !dropdown->getButton(),
                     "a trigger callback must not keep its own button alive" );
        }
        {
            auto dropdown = make_ptr<scene::Dropdown>();
            auto trigger = make_ptr<scene::Button>();
            ObjectObserver<scene::Dropdown> retiredDropdown( dropdown );
            ObjectObserver<scene::Button> retiredTrigger( trigger );
            dropdown->setButton( trigger );
            dropdown = nullptr;
            require( retiredDropdown.expired(),
                     "a retained trigger must not own its dropdown through its callback" );
            click( trigger );
            trigger = nullptr;
            require( retiredTrigger.expired(),
                     "late callbacks and subsequent trigger destruction must be safe after owner retirement" );
        }
        {
            auto button = make_ptr<scene::Button>();
            button->setUseNativeUI( false );
            button->load( nullptr );
            auto state = std::make_shared<int>( 40 );
            std::weak_ptr<int> retiredState = state;
            bool retainedDuringCallback = false;
            unsigned calls = 0;
            int continuation = 0;
            button->setCallbackFunction( [owner = button.get(), state, &retiredState,
                                         &retainedDuringCallback, &calls, &continuation]( hash_type ) {
                ++calls;
                owner->unload( nullptr );
                retainedDuringCallback = !retiredState.expired();
                if( retainedDuringCallback ) continuation = *state + 2;
            } );
            state.reset();
            click( button );
            require( retainedDuringCallback && continuation == 42 && calls == 1,
                     "self-unload must retain the executing callback and its captures until it returns" );
            require( retiredState.expired() && button->getLoadingState() == LoadingState::Unloaded,
                     "self-unload must release callback captures immediately after invocation completes" );
            click( button );
            require( calls == 1, "a button that unloaded itself must not invoke its retired callback again" );
        }
        std::printf( "Scene dropdown lifetime contracts: %u assertions, trigger replacement, unload and either retirement order passed.\n",
                     assertions );
        return assertions;
    }
}
