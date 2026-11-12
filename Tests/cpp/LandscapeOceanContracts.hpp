#pragma once
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/ProceduralOcean.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLandscapeOcean.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace landscape_ocean_contracts
{
    using namespace workphone;

    inline void require( bool condition, const char *message )
    {
        if( !condition )
        {
            throw std::runtime_error( message );
        }
    }

    template <class T>
    inline T propertyValue( SmartPtr<Properties> properties, const String &name )
    {
        T result{};
        require( properties->getPropertyValue( name, result ), "Missing ocean property" );
        return result;
    }

    class OceanProbe : public scene::ProceduralOcean
    {
    public:
        u32 builds = 0;

        SmartPtr<IMesh> buildMesh() override
        {
            ++builds;
            return ProceduralOcean::buildMesh();
        }
    };

    inline void requireSurface( OceanProbe &ocean, f32 width, f32 depth )
    {
        auto mesh = ocean.buildMesh();
        require( mesh && mesh->getSubMeshes().size() == 1, "Ocean must generate one material surface" );
        render::FoliageSpecies imported;
        Array<render::FoliageMaterial> materials( 1 );
        String error;
        // The shared importer reads the actual vertex declarations and buffers.
        require( scene::importFoliageMesh( mesh, "test/ocean", materials, imported, error ),
                 error.c_str() );
        const auto &geometry = imported.lods[0];
        require( geometry.positions.size() == 4 && geometry.indices.size() == 6,
                 "A static ocean surface must be a quad" );
        f32 minimumX = std::numeric_limits<f32>::max(), minimumZ = minimumX;
        f32 maximumX = -minimumX, maximumZ = -minimumX;
        for( size_t i = 0; i < geometry.positions.size(); ++i )
        {
            const auto &position = geometry.positions[i];
            const auto &normal = geometry.normals[i];
            const auto &uv = geometry.uvs[i];
            require( std::isfinite( position.x ) && std::isfinite( position.z ) &&
                         std::abs( position.y ) < .00001f && std::abs( normal.x ) < .00001f &&
                         std::abs( normal.y - 1.f ) < .00001f && std::abs( normal.z ) < .00001f &&
                         std::isfinite( uv.x ) && std::isfinite( uv.y ),
                     "Ocean surface must be flat with finite UVs and upward unit normals" );
            minimumX = std::min( minimumX, position.x );
            maximumX = std::max( maximumX, position.x );
            minimumZ = std::min( minimumZ, position.z );
            maximumZ = std::max( maximumZ, position.z );
        }
        require( std::abs( minimumX + width * .5f ) < .00001f &&
                     std::abs( maximumX - width * .5f ) < .00001f &&
                     std::abs( minimumZ + depth * .5f ) < .00001f &&
                     std::abs( maximumZ - depth * .5f ) < .00001f,
                 "Ocean extents must match the configured width and depth around its origin" );
        for( size_t i = 0; i < geometry.indices.size(); i += 3 )
        {
            const auto &a = geometry.positions[geometry.indices[i]];
            const auto &b = geometry.positions[geometry.indices[i + 1]];
            const auto &c = geometry.positions[geometry.indices[i + 2]];
            require( ( b - a ).crossProduct( c - a ).y > 0,
                     "Ocean triangles must face upward without degeneracies" );
        }
    }

    inline void componentContracts()
    {
        OceanProbe source;
        source.setSize( 36, 19 );
        source.setLandscapeOwned( true );
        source.setEnabled( false );
        auto properties = source.getProperties();
        properties->setProperty( "Colour", ColourF( .03f, .18f, .27f, 1 ) );
        properties->setProperty( "Roughness", .17f );
        source.setProperties( properties );
        requireSurface( source, 36, 19 );

        auto parsed = make_ptr<Properties>();
        DataUtil::parse( DataUtil::toString( source.toData().get(), true ), parsed.get() );
        OceanProbe restored;
        restored.fromData( parsed );
        properties = restored.getProperties();
        require( restored.isLandscapeOwned() && !restored.isEnabled() &&
                     propertyValue<f32>( properties, "Width" ) == 36 &&
                     propertyValue<f32>( properties, "Depth" ) == 19 &&
                     std::abs( propertyValue<f32>( properties, "Roughness" ) - .17f ) < .00001f,
                 "Ocean JSON roundtrip must retain ownership, dimensions, roughness and enabled state" );
        const auto colour = propertyValue<ColourF>( properties, "Colour" );
        require( std::abs( colour.r - .03f ) < .00001f && std::abs( colour.g - .18f ) < .00001f &&
                     std::abs( colour.b - .27f ) < .00001f,
                 "Ocean JSON roundtrip must retain its authored colour" );
        requireSurface( restored, 36, 19 );
    }

    inline void helperContracts()
    {
        const auto previousTask = Thread::getCurrentTask();
        const auto previousFlags = Thread::getTaskFlags( TaskId::Application );

        struct RestoreTask
        {
            TaskId task;
            u32 flags;

            ~RestoreTask()
            {
                Thread::setTaskFlags( TaskId::Application, flags );
                Thread::setCurrentTask( task );
            }
        } restoreTask{ previousTask, previousFlags };

        Thread::setCurrentTask( TaskId::Application );
        Thread::setTaskFlag( Thread::Application_Flag, true );
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        require( manager && manager->isLoaded(), "Ocean helper fixture needs a loaded GameManager" );
        auto owner = manager->createActor();
        require( owner != nullptr, "Ocean helper fixture must create a terrain actor" );
        owner->setState( scene::IGameActor::State::Edit );
        require( owner->getState() == scene::IGameActor::State::Edit,
                 "Ocean helper fixture must enter Edit state to evaluate transforms" );

        struct Cleanup
        {
            SmartPtr<scene::IGameManager> manager;
            SmartPtr<scene::IGameActor> owner;

            ~Cleanup()
            {
                manager->destroyActor( owner );
            }
        } cleanup{ manager, owner };

        auto terrain = owner->addComponent<scene::TerrainSystem>();
        require( terrain != nullptr, "Ocean helper fixture must attach terrain" );
        render::TerrainData data;
        data.dimensions = { 5, 7 };
        data.origin = { -20, 30 };
        data.spacing = { 3, 2 };
        data.heights.assign( 35, 0 );
        String error;
        require( terrain->applyTerrainData( data, error ), error.c_str() );
        require( scene::configureLandscapeOcean( terrain.get(), false, 0, 5, error ) &&
                     owner->getChildren().empty(),
                 "Disabled ocean option must not create scene children" );

        auto authored = manager->createActor();
        require( authored != nullptr, "Ocean helper fixture must create authored ocean" );
        owner->addChild( authored );
        auto authoredOcean = authored->addComponent<scene::ProceduralOcean>();
        require( authoredOcean != nullptr, "Ocean helper fixture must attach authored ocean" );
        authoredOcean->setSize( 7, 9 );
        authored->setLocalPosition( { 200, 3, 5 } );
        authored->setEnabled( true, false );
        const auto authoredBefore = DataUtil::toString( authoredOcean->toData().get(), true );

        const auto rotation =
            Quaternion<real_Num>( real_Num( std::sqrt( .5 ) ), 0, real_Num( std::sqrt( .5 ) ), 0 );
        owner->setLocalPosition( { 11, 12, 13 } );
        owner->setLocalOrientation( rotation );
        owner->setLocalScale( { 2, 3, 4 } );
        owner->updateTransform();
        require( scene::configureLandscapeOcean( terrain.get(), true, 4, 5, error ), error.c_str() );
        require( owner->getChildren().size() == 2,
                 "Ocean setup must create one owned child alongside an authored ocean" );
        SmartPtr<scene::IGameActor> oceanActor;
        for( auto child : owner->getChildren() )
        {
            if( auto ocean = child->getComponent<scene::ProceduralOcean>();
                ocean && ocean->isLandscapeOwned() )
            {
                require( !oceanActor, "Ocean setup created multiple owned children" );
                oceanActor = child;
            }
        }
        require( oceanActor && oceanActor->isEnabled(), "Ocean setup must enable its owned child" );
        auto ocean = oceanActor->getComponent<scene::ProceduralOcean>();
        const auto oceanSubscriptionCount = [&] {
            const auto components =
                manager->getRegisteredComponents( Thread::UpdateState::Update, TaskId::Application )
                    .snapshot();
            return std::count( components.begin(), components.end(),
                               SmartPtr<scene::IComponent>( ocean ) );
        };
        require( oceanSubscriptionCount() == 1,
                 "New ocean child must register exactly one Application update subscription" );
        auto properties = ocean->getProperties();
        require( propertyValue<f32>( properties, "Width" ) == 22 &&
                     propertyValue<f32>( properties, "Depth" ) == 22,
                 "Ocean margin must extend both terrain edges" );
        const auto centre = Vector3<real_Num>( -14, 4, 36 );
        const auto world =
            Vector3<real_Num>( 11, 12, 13 ) + rotation * ( centre * Vector3<real_Num>( 2, 3, 4 ) );
        require( ( oceanActor->getLocalPosition() - centre ).length() < .0001 &&
                     ( oceanActor->getPosition() - world ).length() < .001,
                 "Ocean must centre on terrain-local data and inherit its parent transform" );
        require( !oceanActor->getComponent<scene::Collision>() &&
                     !oceanActor->getComponent<scene::Rigidbody>(),
                 "Ocean surface setup must not create physics obstacles" );

        require( scene::configureLandscapeOcean( terrain.get(), true, -2, 10, error ), error.c_str() );
        properties = ocean->getProperties();
        require( owner->getChildren().size() == 2 && oceanActor->getParent() == owner &&
                     propertyValue<f32>( properties, "Width" ) == 32 &&
                     propertyValue<f32>( properties, "Depth" ) == 32 &&
                     std::abs( oceanActor->getLocalPosition().y + 2 ) < .0001,
                 "Repeated ocean setup must update the same child without duplicating it" );
        require( !scene::configureLandscapeOcean( terrain.get(), true, 9, -1, error ) &&
                     !error.empty() && owner->getChildren().size() == 2 &&
                     std::abs( oceanActor->getLocalPosition().y + 2 ) < .0001,
                 "Invalid ocean setup must leave the existing surface unchanged" );
        require( scene::configureLandscapeOcean( terrain.get(), false, 0, 0, error ) &&
                     !oceanActor->isEnabled() && owner->getChildren().size() == 2,
                 "Disabling ocean must hide and retain the owned child" );
        require( authored->isEnabled() &&
                     authored->getLocalPosition() == Vector3<real_Num>( 200, 3, 5 ) &&
                     DataUtil::toString( authoredOcean->toData().get(), true ) == authoredBefore,
                 "Ocean setup must preserve an unmarked user-authored ocean" );
        require( scene::configureLandscapeOcean( terrain.get(), true, 1, 0, error ) &&
                     oceanActor->isEnabled() && owner->getChildren().size() == 2,
                 "Re-enabling ocean must reuse the disabled owned child" );
        require( scene::configureLandscapeOcean( terrain.get(), true, 1, 0, error ) &&
                     oceanSubscriptionCount() == 1,
                 "Unchanged ocean size must retain exactly one Application update subscription" );

        // Restore a serialized ocean onto a child with no scene actor update slot.
        // Its CPU build must still run from the component's Application subscription.
        auto restoredActor = manager->createActor();
        require( restoredActor != nullptr, "Ocean reload fixture must create a child actor" );
        owner->addChild( restoredActor );
        auto restoredOcean = make_ptr<OceanProbe>();
        restoredActor->addComponentInstance( restoredOcean );
        restoredOcean->load( nullptr );
        auto parsed = make_ptr<Properties>();
        DataUtil::parse( DataUtil::toString( ocean->toData().get(), true ), parsed.get() );
        restoredOcean->fromData( parsed );
        restoredOcean->fromData( parsed );
        const auto subscriptionCount = [&] {
            const auto components =
                manager->getRegisteredComponents( Thread::UpdateState::Update, TaskId::Application )
                    .snapshot();
            return std::count( components.begin(), components.end(),
                               SmartPtr<scene::IComponent>( restoredOcean ) );
        };
        require( subscriptionCount() == 1,
                 "Ocean restoration must register exactly one Application update subscription" );
        manager->update();
        require( restoredOcean->builds == 1,
                 "Restored ocean child must reach deferred CPU generation without actor registration" );
        manager->update();
        restoredActor->update();
        require( restoredOcean->builds == 1,
                 "Component and actor update paths must consume one ocean regeneration request" );
        restoredOcean->unload( nullptr );
        require( subscriptionCount() == 0,
                 "Ocean unload must release its Application update subscription" );
        manager->update();
        require( restoredOcean->builds == 1, "Unloaded ocean must not receive deferred generation" );
    }
}  // namespace landscape_ocean_contracts

// Requires the caller's normal TypeManager/ApplicationManager, a loaded
// GameManager with a current scene, StateManager, TimerMT, and a Transform interface factory.
inline void runLandscapeOceanContracts()
{
    landscape_ocean_contracts::componentContracts();
    landscape_ocean_contracts::helperContracts();
}
