#pragma once
#include "ClawFoliageSceneContracts.hpp"
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Graphics/GraphicsLight.hpp>

namespace claw_primary_light_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    struct GenericLight : GraphicsLight
    {
        SmartPtr<IGraphicsObject> clone( const String & ) const override { return nullptr; }
    };
    inline bool run()
    {
        try
        {
            claw_foliage_scene_contracts::Fixture fixture;
            struct FactoryScope
            {
                SmartPtr<FactoryManager> factory;
                SmartPtr<IGraphicsSystem> graphics;
                FactoryScope( SmartPtr<FactoryManager> value, SmartPtr<IGraphicsSystem> system ) :
                    factory( value ), graphics( system )
                { factory->load( nullptr ); }
                ~FactoryScope()
                {
                    // This CPU selection fixture registers objects without a render task;
                    // cancel their queued GPU requests before releasing fixture services.
                    graphics->clearObjectQueues();
                    factory->unload( nullptr );
                }
            } factoryScope( fixture.services.factory, fixture.graphics );
            const auto require = claw_foliage_contracts::require;
            FactoryUtil::addFactory<ClawLight>( fixture.services.factory );
            auto scene = make_ptr<ClawScene>();
            scene->setFactoryManager( fixture.services.factory );
            require( !scene->getPrimaryDirectionalLight(), "Empty scene selected a directional light" );
            auto first = dynamic_pointer_cast<IGraphicsLight>(
                scene->addGraphicsObjectByTypeId( IGraphicsLight::typeInfo() ) );
            auto second = dynamic_pointer_cast<IGraphicsLight>(
                scene->addGraphicsObjectByTypeId( IGraphicsLight::typeInfo() ) );
            require( first && second, "Primary sun fixture registration failed" );
            require( first->isLoaded() && second->isLoaded(), "Scene light must be configurable before the first render tick" );
            first->setType( LightTypes::LT_DIRECTIONAL ); first->setVisible( true );
            second->setType( LightTypes::LT_DIRECTIONAL ); second->setVisible( true );
            const auto authoredDirection = Vector3F( .3f, -.8f, .4f ).normaliseCopy();
            second->setDirection( authoredDirection ); second->setPowerScale( 1.8f );
            second->setDiffuseColour( { .8f, .7f, .6f, 1 } );
            require( ( second->getDirection() - authoredDirection ).lengthSquared() < 1e-6f &&
                     std::abs( second->getPowerScale() - 1.8f ) < 1e-6f &&
                     std::abs( second->getDiffuseColour().r - .8f ) < 1e-6f,
                     "Pre-frame sun configuration was lost" );
            auto concrete = dynamic_pointer_cast<ClawLight>( second );
            concrete->setPosition( { 3, 4, 5 } );
            auto clone = dynamic_pointer_cast<ClawLight>( concrete->clone() );
            require( clone && clone->isLoaded() && clone->getType() == second->getType() &&
                     clone->isVisible() == second->isVisible() &&
                     ( clone->getDirection() - authoredDirection ).lengthSquared() < 1e-6f &&
                     std::abs( clone->getPowerScale() - 1.8f ) < 1e-6f &&
                     std::abs( clone->getDiffuseColour().r - .8f ) < 1e-6f &&
                     clone->getPosition() == Vector3F( 3, 4, 5 ), "Light clone lost native configuration" );
            auto typed = dynamic_pointer_cast<IGraphicsLight>( scene->addGraphicsObjectByTypeId( ClawLight::typeInfo() ) );
            require( typed && typed->isLoaded(), "Class-ID light creation lost immediate configuration support" );
            typed->setVisible( false );
            require( scene->getPrimaryDirectionalLight() == first, "Default primary sun order changed" );
            require( scene->setPrimaryDirectionalLight( second ) && scene->getPrimaryDirectionalLight() == second,
                     "Registered sun preference was not applied" );
            auto otherScene = make_ptr<ClawScene>();
            otherScene->setFactoryManager( fixture.services.factory );
            auto foreign = dynamic_pointer_cast<IGraphicsLight>(
                otherScene->addGraphicsObjectByTypeId( IGraphicsLight::typeInfo() ) );
            require( bool( foreign ), "Foreign scene sun registration failed" );
            foreign->setType( LightTypes::LT_DIRECTIONAL );
            auto generic = make_ptr<GenericLight>(); generic->setType( LightTypes::LT_DIRECTIONAL );
            require( !scene->setPrimaryDirectionalLight( foreign ) &&
                     !scene->setPrimaryDirectionalLight( generic ) && scene->getPrimaryDirectionalLight() == second,
                     "Invalid sun preference replaced current selection" );
            first->setType( LightTypes::LT_POINT );
            require( !scene->setPrimaryDirectionalLight( first ) && scene->getPrimaryDirectionalLight() == second,
                     "Point light replaced primary sun" );
            first->setType( LightTypes::LT_DIRECTIONAL );
            second->setVisible( false );
            require( scene->getPrimaryDirectionalLight() == first, "Hidden preferred sun prevented fallback" );
            second->setVisible( true );
            require( scene->getPrimaryDirectionalLight() == second, "Visible preferred sun was forgotten" );
            require( scene->setPrimaryDirectionalLight( nullptr ) && scene->getPrimaryDirectionalLight() == first &&
                     scene->setPrimaryDirectionalLight( second ), "Null preference did not restore first-light fallback" );
            require( scene->removeGraphicsObject( second ) && scene->getPrimaryDirectionalLight() == first &&
                     !scene->setPrimaryDirectionalLight( second ),
                     "Removed externally retained sun remained selectable" );
            require( scene->setPrimaryDirectionalLight( first ) && scene->setPrimaryDirectionalLight( nullptr ) &&
                     scene->getPrimaryDirectionalLight() == first, "Clearing preference lost default fallback" );
            first->setVisible( false );
            require( !scene->getPrimaryDirectionalLight(), "No visible sun must yield no selection" );
            first->setVisible( true );
            require( scene->setPrimaryDirectionalLight( first ), "Primary sun reset failed" );
            scene->clear();
            require( !scene->getPrimaryDirectionalLight() && !scene->setPrimaryDirectionalLight( first ),
                     "Scene clear retained primary sun membership" );
            scene = nullptr;
            otherScene->clear();
            otherScene = nullptr;
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "FAIL: primary light contracts: %s\n", error.what() );
            return false;
        }
    }
}
