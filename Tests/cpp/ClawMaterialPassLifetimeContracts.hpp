#pragma once

#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <cstdio>
#include <stdexcept>

namespace claw_material_pass_lifetime_contracts
{
    using namespace workphone;
    using namespace workphone::render;

    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    class TrackedTexture : public ClawTexture
    {
    public:
        explicit TrackedTexture( bool *destroyed ) : m_destroyed( destroyed )
        {
        }
        ~TrackedTexture() override
        {
            *m_destroyed = true;
        }

    private:
        bool *m_destroyed;
    };

    struct Fixture
    {
        SmartPtr<core::IApplicationManager> application = core::IApplicationManager::instance();
        SmartPtr<IFactoryManager> previousFactory = application->getFactoryManager();
        SmartPtr<IStateManager> previousStates = application->getStateManager();
        SmartPtr<ITimer> previousTimer = application->getTimer();
        SmartPtr<FactoryManager> factory = make_ptr<FactoryManager>();
        SmartPtr<StateManager> states = make_ptr<StateManager>();
        SmartPtr<TimerMT> timer = make_ptr<TimerMT>();

        Fixture()
        {
            application->setFactoryManager( factory );
            application->setTimer( timer );
            timer->load( nullptr );
            application->setStateManager( states );
            states->load( nullptr );
        }
        ~Fixture()
        {
            states->unload( nullptr );
            application->setStateManager( previousStates );
            timer->unload( nullptr );
            application->setTimer( previousTimer );
            application->setFactoryManager( previousFactory );
        }
    };

    inline bool run()
    {
        try
        {
            Fixture fixture;
            require( fixture.states->isLoaded() && fixture.timer->isLoaded(),
                     "pass lifetime fixture must initialize state and timer services" );
            const auto baseline = fixture.states->getStateContexts().size();
            bool ownedTextureDestroyed = false;
            {
                auto pass = make_ptr<ClawMaterialPass>();
                auto context = pass->getStateContext();
                require( context && fixture.states->getStateContexts().size() == baseline + 1,
                         "a standalone pass must own one managed context" );
                {
                    auto data =
                        context->invalidateStateDataById<MaterialPassStateData>( pass->getId(), false );
                    require( data != nullptr, "a standalone pass must create its state record" );
                    data->textures[0] = make_ptr<TrackedTexture>( &ownedTextureDestroyed );
                }
                pass = nullptr;
                require( ownedTextureDestroyed && !context->isLoaded() &&
                             fixture.states->getStateContexts().size() == baseline,
                         "retiring a pass must remove its managed context and release its texture" );
            }

            auto shared = fixture.states->addStateContext();
            auto sentinel = make_ptr<State>();
            sentinel->setId( 0x7fed1234u );
            sentinel->setData( make_ptr<MaterialPassStateData>() );
            shared->addState( sentinel );
            bool sharedTextureDestroyed = false;
            {
                auto pass = make_ptr<ClawMaterialPass>();
                pass->setStateContext( shared );
                require(
                    fixture.states->getStateContexts().size() == baseline + 1 && shared->isLoaded(),
                    "rebinding must retire the original context while preserving the supplied one" );
                pass->load( nullptr );
                {
                    auto data =
                        shared->invalidateStateDataById<MaterialPassStateData>( pass->getId(), false );
                    require( data && shared->getStates().size() == 2,
                             "a pass can create its own record within a supplied context" );
                    data->textures[0] = make_ptr<TrackedTexture>( &sharedTextureDestroyed );
                }
                pass->unload( nullptr );
                require( sharedTextureDestroyed,
                         "unload must release the pass-created record's texture" );
                require( shared->isLoaded(), "unload must leave a supplied context loaded" );
                require( shared->getStates().size() == 1 &&
                             shared->getStates().front() == sentinel,
                         "unload must preserve only the unrelated record in the shared context" );
                require( fixture.states->getStateContexts().size() == baseline + 1,
                         "unload must preserve the supplied context's manager registration" );

                auto external = make_ptr<State>();
                external->setId( pass->getId() );
                external->setData( make_ptr<MaterialPassStateData>() );
                shared->addState( external );
                pass->setStateContext( shared );
                pass->load( nullptr );
                pass = nullptr;
                require( shared->isLoaded() && shared->getStates().size() == 2 &&
                             shared->getStateById( external->getId() ) == external,
                         "pre-existing supplied state records must survive pass destruction" );
            }
            fixture.states->removeStateContext( shared );
            require( fixture.states->getStateContexts().size() == baseline,
                     "shared context owner must retain control over final removal" );
            std::puts(
                "Claw material pass owned/shared context and texture retirement contracts passed." );
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "Material pass lifetime contract failed: %s\n", error.what() );
            return false;
        }
    }
}  // namespace claw_material_pass_lifetime_contracts
