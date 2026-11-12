#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLandscapeOcean.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/ProceduralOcean.hpp>
#include <Workphone/Workphone.hpp>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    bool configureLandscapeOcean( TerrainSystem *terrain, bool enabled, f32 localSeaLevel, f32 margin,
                                  String &error )
    {
        SmartPtr<IGameActor> created;
        SmartPtr<IGameManager> manager;
        try
        {
            auto parent = terrain ? terrain->getActor() : nullptr;
            if( !parent )
            {
                throw std::runtime_error( "Ocean setup requires a terrain actor." );
            }
            SmartPtr<IGameActor> oceanActor;
            SmartPtr<ProceduralOcean> ocean;
            for( auto child : parent->getChildren() )
            {
                if( auto candidate = child->getComponent<ProceduralOcean>();
                    candidate && candidate->isLandscapeOwned() )
                {
                    oceanActor = child;
                    ocean = candidate;
                    break;
                }
            }
            if( !enabled )
            {
                if( oceanActor )
                {
                    oceanActor->setEnabled( false, false );
                }
                error.clear();
                return true;
            }

            const auto source = terrain->getTerrainSnapshot();
            if( !source )
            {
                throw std::runtime_error( "Ocean setup requires terrain height data." );
            }
            if( source->dimensions.x < 2 || source->dimensions.y < 2 ||
                !std::isfinite( source->origin.x ) || !std::isfinite( source->origin.y ) ||
                !std::isfinite( source->spacing.x ) || !std::isfinite( source->spacing.y ) ||
                source->spacing.x <= 0 || source->spacing.y <= 0 )
            {
                throw std::runtime_error( "Ocean setup requires valid terrain dimensions and spacing." );
            }
            if( !std::isfinite( localSeaLevel ) || !std::isfinite( margin ) || margin < 0 )
            {
                throw std::runtime_error( "Ocean sea level and nonnegative margin must be finite." );
            }
            const auto width = f64( source->dimensions.x - 1 ) * source->spacing.x;
            const auto depth = f64( source->dimensions.y - 1 ) * source->spacing.y;
            const auto oceanWidth = width + 2 * f64( margin );
            const auto oceanDepth = depth + 2 * f64( margin );
            if( !std::isfinite( oceanWidth ) || !std::isfinite( oceanDepth ) || oceanWidth < .01 ||
                oceanDepth < .01 || oceanWidth > 1000000 || oceanDepth > 1000000 )
            {
                throw std::runtime_error( "Ocean dimensions must be between 0.01 and 1000000 metres." );
            }

            if( !oceanActor )
            {
                auto app = core::IApplicationManager::instancePtr();
                manager = app ? app->getGameManager() : nullptr;
                if( !manager )
                {
                    throw std::runtime_error( "Ocean setup requires a scene manager." );
                }
                created = manager->createActor();
                if( !created )
                {
                    throw std::runtime_error( "Cannot create the landscape ocean actor." );
                }
                created->setName( "Landscape Ocean" );
                created->setEnabled( false, false );
                ocean = created->addComponent<ProceduralOcean>();
                if( !ocean )
                {
                    throw std::runtime_error( "Cannot create the landscape ocean component." );
                }
                ocean->setLandscapeOwned( true );
                parent->addChild( created );
                if( created->getParent() != parent )
                {
                    throw std::runtime_error( "Cannot attach the landscape ocean to its terrain." );
                }
                oceanActor = created;
            }
            ocean->setSize( f32( oceanWidth ), f32( oceanDepth ) );
            oceanActor->setLocalOrientation( Quaternion<real_Num>::identity() );
            oceanActor->setLocalScale( Vector3<real_Num>::unit() );
            oceanActor->setLocalPosition( { real_Num( source->origin.x + width * .5 ),
                                            real_Num( localSeaLevel ),
                                            real_Num( source->origin.y + depth * .5 ) } );
            oceanActor->updateTransform();
            if( auto scene = parent->getScene() )
            {
                scene->registerAllUpdates( oceanActor );
            }
            oceanActor->setEnabled( true, false );
            error.clear();
            return true;
        }
        catch( const std::exception &failure )
        {
            if( created )
            {
                try
                {
                    if( auto scene = created->getScene() )
                    {
                        scene->unregisterAll( created );
                    }
                    if( auto parent = created->getParent() )
                    {
                        parent->removeChild( created );
                    }
                    if( manager )
                    {
                        manager->destroyActor( created );
                    }
                }
                catch( ... )
                {
                }
            }
            error = failure.what();
            return false;
        }
    }
}  // namespace workphone::scene
