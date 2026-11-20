#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Procedural/SceneAssets.hpp>

namespace workphone::scene::procedural_scene
{

    SceneAssets::SceneAssets( const SceneAssets &other ) :
        openCity( other.openCity ),
        useFoliageWorld( other.useFoliageWorld ),
        cityNaturalRoads( other.cityNaturalRoads ),
        cityRoadShowcase( other.cityRoadShowcase ),
        cityTerrainRoads( other.cityTerrainRoads ),
        cityStreamingEnabled( other.cityStreamingEnabled ),
        cityStreamLoadRadius( other.cityStreamLoadRadius ),
        cityStreamRetainRadius( other.cityStreamRetainRadius ),
        citySourceMemoryMiB( other.citySourceMemoryMiB ),
        cityStreaming( other.cityStreaming ),
        roadAuthoring( other.roadAuthoring ),
        roadTerrain( other.roadTerrain ),
        terrainGameplaySource( other.terrainGameplaySource ),
        cityLayout( other.cityLayout ),
        cityBuildingSupportChecks( other.cityBuildingSupportChecks ),
        cityCulledParcels( other.cityCulledParcels ),
        cityBuildingSupportValid( other.cityBuildingSupportValid ),
        citySourceReused( other.citySourceReused ),
        cityTerrainRelief( other.cityTerrainRelief ),
        cityTerrainFitted( other.cityTerrainFitted ),
        cityTerrainFitReport( other.cityTerrainFitReport ),
        cityRaceRouteAvailable( other.cityRaceRouteAvailable ),
        cityRaceRouteError( other.cityRaceRouteError ),
        cityAppliedSourceRevision( other.cityAppliedSourceRevision ),
        cityStreetDetail( other.cityStreetDetail ),
        cityStreetPropDensity( other.cityStreetPropDensity ),
        cityBuildingDensity( other.cityBuildingDensity ),
        cityTrafficDensity( other.cityTrafficDensity ),
        cityTrafficDriveOnLeft( other.cityTrafficDriveOnLeft ),
        cityTraffic( other.cityTraffic ),
        cityTrafficActors( other.cityTrafficActors ),
        cityTrafficAccumulator( other.cityTrafficAccumulator ),
        cityTrafficLastTick( other.cityTrafficLastTick ),
        cityDebugBlocks( other.cityDebugBlocks ),
        cityDebugLots( other.cityDebugLots ),
        cityDebugBlockCount( other.cityDebugBlockCount ),
        cityDebugLotCount( other.cityDebugLotCount ),
        cityFoliageLODDistances( other.cityFoliageLODDistances ),
        cityFoliageImpostorResolution( other.cityFoliageImpostorResolution ),
        cityBuildingImpostorResolution( other.cityBuildingImpostorResolution ),
        cityStreetDetailCount( other.cityStreetDetailCount ),
        cityBinPosition( other.cityBinPosition ),
        cityBinRoadward( other.cityBinRoadward ),
        cityBuildingStyle( other.cityBuildingStyle ),
        cityBuildingMeshes( other.cityBuildingMeshes ),
        cityBuildingModuleCount( other.cityBuildingModuleCount ),
        cityBuildingModulePlacements( other.cityBuildingModulePlacements ),
        cityBuildingKitDiagnostic( other.cityBuildingKitDiagnostic ),
        foliageTerrain( other.foliageTerrain ),
        cityBlocks( other.cityBlocks ),
        cityRoute( other.cityRoute ),
        vehicle( other.vehicle ),
        circuit( other.circuit ),
        wheels( other.wheels ),
        body( other.body ),
        shadow( other.shadow ),
        reflectionActor( other.reflectionActor ),
        ground( other.ground ),
        sceneryRoot( other.sceneryRoot ),
        reflectionTexture( other.reflectionTexture ),
        vehicleMaterials( other.vehicleMaterials ),
        materials( other.materials ),
        textures( other.textures ),
        meshes( other.meshes ),
        actors( other.actors ),
        treeLODs( makeSourceBudgetSnapshot( other.treeLODs ) ),
        vehicleLOD( other.vehicleLOD ),
        sky( other.sky ),
        sun( other.sun ),
        resourcePrefix( other.resourcePrefix ),
        exportSourceDirectory( other.exportSourceDirectory ),
        triangles( other.triangles ),
        textureBytes( other.textureBytes )
    {
    }

    SceneAssets::SceneAssets() = default;

    SceneAssets::SceneAssets( SceneAssets && ) noexcept = default;

    SceneAssets &SceneAssets::operator=( const SceneAssets &other )
    {
        if( this != &other )
        {
            SceneAssets candidate( other );
            swap( candidate );
        }
        return *this;
    }

    workphone::scene::procedural_scene::SceneAssets &SceneAssets::operator=( SceneAssets && ) noexcept = default;

    void SceneAssets::swap( SceneAssets &other ) noexcept
    {
        using std::swap;
        swap( openCity, other.openCity );
        swap( useFoliageWorld, other.useFoliageWorld );
        swap( cityNaturalRoads, other.cityNaturalRoads );
        swap( foliageTerrain, other.foliageTerrain );
        swap( cityRoadShowcase, other.cityRoadShowcase );
        swap( roadAuthoring, other.roadAuthoring );
        swap( roadTerrain, other.roadTerrain );
        swap( terrainGameplaySource, other.terrainGameplaySource );
        swap( cityTerrainRoads, other.cityTerrainRoads );
        swap( cityStreamingEnabled, other.cityStreamingEnabled );
        swap( cityStreamLoadRadius, other.cityStreamLoadRadius );
        swap( cityStreamRetainRadius, other.cityStreamRetainRadius );
        swap( citySourceMemoryMiB, other.citySourceMemoryMiB );
        cityStreaming.swap( other.cityStreaming );
        cityLayout.swap( other.cityLayout );
        swap( cityBuildingSupportChecks, other.cityBuildingSupportChecks );
        swap( cityCulledParcels, other.cityCulledParcels );
        swap( cityBuildingSupportValid, other.cityBuildingSupportValid );
        swap( citySourceReused, other.citySourceReused );
        swap( cityTerrainRelief, other.cityTerrainRelief );
        swap( cityTerrainFitted, other.cityTerrainFitted );
        swap( cityTerrainFitReport, other.cityTerrainFitReport );
        swap( cityRaceRouteAvailable, other.cityRaceRouteAvailable );
        swap( cityRaceRouteError, other.cityRaceRouteError );
        swap( cityAppliedSourceRevision, other.cityAppliedSourceRevision );
        swap( cityStreetDetail, other.cityStreetDetail );
        swap( cityStreetPropDensity, other.cityStreetPropDensity );
        swap( cityBuildingDensity, other.cityBuildingDensity );
        swap( cityTrafficDensity, other.cityTrafficDensity );
        swap( cityTrafficDriveOnLeft, other.cityTrafficDriveOnLeft );
        cityTraffic.swap( other.cityTraffic );
        cityTrafficActors.swap( other.cityTrafficActors );
        swap( cityTrafficAccumulator, other.cityTrafficAccumulator );
        swap( cityTrafficLastTick, other.cityTrafficLastTick );
        swap( cityDebugBlocks, other.cityDebugBlocks );
        swap( cityDebugLots, other.cityDebugLots );
        swap( cityDebugBlockCount, other.cityDebugBlockCount );
        swap( cityDebugLotCount, other.cityDebugLotCount );
        swap( cityFoliageLODDistances, other.cityFoliageLODDistances );
        swap( cityFoliageImpostorResolution, other.cityFoliageImpostorResolution );
        swap( cityBuildingImpostorResolution, other.cityBuildingImpostorResolution );
        swap( cityStreetDetailCount, other.cityStreetDetailCount );
        swap( cityBinPosition, other.cityBinPosition );
        swap( cityBinRoadward, other.cityBinRoadward );
        swap( cityBuildingStyle, other.cityBuildingStyle );
        swap( cityBuildingMeshes, other.cityBuildingMeshes );
        swap( cityBuildingModuleCount, other.cityBuildingModuleCount );
        swap( cityBuildingModulePlacements, other.cityBuildingModulePlacements );
        swap( cityBuildingKitDiagnostic, other.cityBuildingKitDiagnostic );
        swap( cityBlocks, other.cityBlocks );
        swap( cityRoute, other.cityRoute );
        swap( vehicle, other.vehicle );
        circuit.swap( other.circuit );
        wheels.swap( other.wheels );
        swap( body, other.body );
        swap( shadow, other.shadow );
        swap( reflectionActor, other.reflectionActor );
        swap( ground, other.ground );
        swap( sceneryRoot, other.sceneryRoot );
        swap( reflectionTexture, other.reflectionTexture );
        vehicleMaterials.swap( other.vehicleMaterials );
        materials.swap( other.materials );
        textures.swap( other.textures );
        meshes.swap( other.meshes );
        actors.swap( other.actors );
        treeLODs.swap( other.treeLODs );
        swap( vehicleLOD, other.vehicleLOD );
        swap( sky, other.sky );
        swap( sun, other.sun );
        swap( resourcePrefix, other.resourcePrefix );
        swap( exportSourceDirectory, other.exportSourceDirectory );
        swap( triangles, other.triangles );
        swap( textureBytes, other.textureBytes );
    }

}  // namespace workphone::scene
