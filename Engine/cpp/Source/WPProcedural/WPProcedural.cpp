#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPProcedural.hpp"
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CRoadSection.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CProceduralManager.hpp>
#include <Workphone/Workphone.hpp>
#include <WPProcedural/ProceduralServices.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CRoadConnectionData.hpp>
#include <WPProcedural/CRoadMeshElement.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/CProceduralTexture.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CRoadGenerator.hpp>
#include <WPProcedural/CRoadGeneratorCity.hpp>
#include <WPProcedural/CRoadGeneratorGrid.hpp>
#include <WPProcedural/CRoadHitPoint.hpp>
#include <WPProcedural/CTerrainGenerator.hpp>
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/CWorldGenerator.hpp>
#include <WPProcedural/MeshFace.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>

namespace workphone
{
    namespace procedural
    {
        SmartPtr<WPProcedural> WPProcedural::m_sPlugin;

        WPProcedural::WPProcedural()
        {
        }

        WPProcedural::~WPProcedural()
        {
        }

        void WPProcedural::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() ) return;
            CSkyAtmosphere::setupTypeInfo();
            CTextureForge::setupTypeInfo();
            CVehicleAppearance::setupTypeInfo();
            CVehicleDamage::setupTypeInfo();
            CVehicleDynamics::setupTypeInfo();
            CVehicleEffects::setupTypeInfo();
            CVehicleGenerator::setupTypeInfo();
            CVehicleGeometry::setupTypeInfo();
            CVehiclePresentation::setupTypeInfo();
            CVehiclePhysics::setupTypeInfo();
            CRoadSystem::setupTypeInfo();

            FactoryUtil::addFactoryByName<CRoadSystem>( "IRoadSystem" );
            FactoryUtil::addFactoryByName<CSkyAtmosphere>( "ISkyAtmosphere" );
            FactoryUtil::addFactoryByName<CTextureForge>( "ITextureForge" );
            FactoryUtil::addFactoryByName<CVehicleAppearance>( "IVehicleAppearance" );
            FactoryUtil::addFactoryByName<CVehicleDamage>( "IVehicleDamage" );
            FactoryUtil::addFactoryByName<CVehicleDynamics>( "IVehicleDynamics" );
            FactoryUtil::addFactoryByName<CVehicleEffects>( "IVehicleEffects" );
            FactoryUtil::addFactoryByName<CVehicleGenerator>( "IVehicleGenerator" );
            FactoryUtil::addFactoryByName<CVehicleGeometry>( "IVehicleGeometry" );
            FactoryUtil::addFactoryByName<CVehiclePhysics>( "IVehiclePhysics" );
            FactoryUtil::addFactoryByName<CVehiclePresentation>( "IVehiclePresentation" );
            FactoryUtil::addFactoryByName<CityGeneratorDefault>( "CityGeneratorDefault" );
            FactoryUtil::addFactoryByName<CRoad>( "CRoad" );
            FactoryUtil::addFactoryByName<CRoadElement>( "CRoadElement" );
            FactoryUtil::addFactoryByName<CRoadSection>( "CRoadSection" );
            FactoryUtil::addFactoryByName<CRoadNode>( "CRoadNode" );
            FactoryUtil::addFactoryByName<CRoadNetwork>( "CRoadNetwork" );
            FactoryUtil::addFactoryByName<CProceduralCity>( "CProceduralCity" );
            FactoryUtil::addFactoryByName<CProceduralManager>( "CProceduralManager" );
            FactoryUtil::addFactoryByName<CCityBlock>( "CCityBlock" );
            FactoryUtil::addFactoryByName<CRoadConnection>( "CRoadConnection" );
            FactoryUtil::addFactoryByName<CRoadConnectionData>( "CRoadConnectionData" );
            FactoryUtil::addFactoryByName<CRoadMeshElement>( "CRoadMeshElement" );

            // CCollisionManager and CProceduralCityCenter are abstract and cannot have factories.
            // These implementations inherit type information; register each by its own name.
            FactoryUtil::addFactoryByName<CBlockGenerator>( "CBlockGenerator" );
            FactoryUtil::addFactoryByName<CProceduralScene>( "CProceduralScene" );
            FactoryUtil::addFactoryByName<CProceduralTerrain>( "CProceduralTerrain" );
            FactoryUtil::addFactoryByName<CProceduralTexture>( "CProceduralTexture" );
            FactoryUtil::addFactoryByName<CProceduralWorld>( "CProceduralWorld" );
            FactoryUtil::addFactoryByName<CRoadGenerator>( "CRoadGenerator" );
            FactoryUtil::addFactoryByName<CRoadGeneratorCity>( "CRoadGeneratorCity" );
            FactoryUtil::addFactoryByName<CRoadGeneratorGrid>( "CRoadGeneratorGrid" );
            FactoryUtil::addFactoryByName<CRoadHitPoint>( "CRoadHitPoint" );
            FactoryUtil::addFactoryByName<CTerrainGenerator>( "CTerrainGenerator" );
            FactoryUtil::addFactoryByName<CTerrainGeneratorDefault>( "CTerrainGeneratorDefault" );
            FactoryUtil::addFactoryByName<CWorldGenerator>( "CWorldGenerator" );
            FactoryUtil::addFactoryByName<MeshFace>( "MeshFace" );
            FactoryUtil::addFactoryByName<MeshGeneratorDefault>( "MeshGeneratorDefault" );
            setLoadingState( LoadingState::Loaded );
        }

        void WPProcedural::unload( SmartPtr<ISharedObject> data )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( !app || !app->getFactoryManager() ) return;
            auto factories = app->getFactoryManager();
            for( const auto *name :
                 { "IRoadSystem", "ISkyAtmosphere", "ITextureForge", "IVehicleAppearance", "IVehicleDamage", "IVehicleDynamics", "IVehicleEffects", "IVehicleGenerator", "IVehicleGeometry", "IVehiclePhysics", "IVehiclePresentation", "CityGeneratorDefault", "CRoad", "CRoadElement", "CRoadSection", "CRoadNode", "CRoadNetwork", "CProceduralCity", "CProceduralManager", "CCityBlock", "CRoadConnection", "CRoadConnectionData", "CRoadMeshElement", "CBlockGenerator", "CProceduralScene", "CProceduralTerrain", "CProceduralTexture", "CProceduralWorld", "CRoadGenerator", "CRoadGeneratorCity", "CRoadGeneratorGrid", "CRoadHitPoint", "CTerrainGenerator", "CTerrainGeneratorDefault", "CWorldGenerator", "MeshFace", "MeshGeneratorDefault" } )
            {
                if( auto factory = factories->getFactoryByName( name ) )
                    factories->removeFactory( factory );
            }
            setLoadingState( LoadingState::Unloaded );
        }

        SmartPtr<WPProcedural> WPProcedural::instance()
        {
            return m_sPlugin;
        }

        void WPProcedural::setInstance( SmartPtr<WPProcedural> plugin )
        {
            m_sPlugin = plugin;
        }
    }  // end namespace procedural
}  // namespace workphone

#ifndef _WP_STATIC_LIB_
extern "C"
{
    WP_INTERFACE_EXPORT void WP_INTERFACE_API workphone_get_version( int *major, int *minor, int *patch )
    {
        *major = WP_VERSION_MAJOR;
        *minor = WP_VERSION_MINOR;
        *patch = WP_VERSION_PATCH;
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    loadPlugin( workphone::core::IApplicationManager *applicationManager )
    {
        using namespace workphone;
        core::IApplicationManager::setInstance( applicationManager );
        auto plugin = workphone::make_ptr<procedural::WPProcedural>();
        plugin->load( nullptr );
        procedural::WPProcedural::setInstance( plugin );
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    unloadPlugin( workphone::core::IApplicationManager *applicationManager )
    {
        using namespace workphone;
        if( auto plugin = procedural::WPProcedural::instance() )
        {
            plugin->unload( nullptr );
            procedural::WPProcedural::setInstance( nullptr );
        }
    }
}
#endif
