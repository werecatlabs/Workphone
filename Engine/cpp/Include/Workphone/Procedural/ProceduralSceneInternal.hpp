#pragma once
// Private declarations shared by the procedural scene generation translation units
// (ProceduralScene*.cpp, ProceduralCity*.cpp, ProceduralCircuitScene.cpp, ...).
// The public entry points are declared in Workphone/Procedural/ProceduralSceneGeneration.hpp.
#include <Workphone/Procedural/ProceduralSceneGeneration.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Procedural/CityBuildingStyles.hpp>
#include <Workphone/Procedural/CitySurfaceLibrary.hpp>
#include "Workphone/Procedural/CitySurfaceScalar.hpp"
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Mesh/Geometry.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Scene/Components/MeshImposter.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <Workphone/Interface/Procedural/OpenCityStreetDetails.hpp>
#include <Workphone/Interface/Procedural/OpenCityDebugBlocks.hpp>
#include <Workphone/Procedural/CityRoadBlockFaces.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/Interface/Procedural/OpenCityLegacyRoute.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Procedural/CityRoadParcelSupport.hpp>
#include <Workphone/Procedural/CityRoadFootways.hpp>
#include <Workphone/Scene/Components/CollisionMesh.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/CollisionTerrain.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Graphics/ProceduralSurfaceSourceExport.hpp>
#include <Workphone/Graphics/GeneratedTextureSource.hpp>
#include <Workphone/Graphics/FoliageImpostorBaker.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include "Workphone/Procedural/CityBuildingKit.hpp"
#include "Workphone/Procedural/CityTrafficAppearance.hpp"
#include "Workphone/Procedural/CityTrafficVehicle.hpp"
#include <Workphone/Scene/Components/WheelController.hpp>
#include "Workphone/Procedural/CityRoadCrossings.hpp"
#include "Workphone/Procedural/CityStreetFixture.hpp"
#include "Workphone/Scene/RuntimeSceneMembership.hpp"
#include <algorithm>
#include <optional>
#include <cmath>
#include <random>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <map>
#include <tuple>
#include <cstdio>

// Builders and helpers shared between the scene generation units. The city component
// headers (Workphone/Scene/Components/City*.hpp) are code fragments that are included
// into this namespace by the unit that owns them.
namespace workphone::scene::procedural_scene::scene_build
{
	using namespace procedural;
	constexpr float pi = 3.14159265358979323846f;

	template <class C>
	void reserveOwnerSlot( C &owners )
	{
		if( owners.size() != owners.capacity() )
		{
			return;
		}
		if( owners.size() == owners.max_size() )
		{
			throw std::length_error( "Scene inventory capacity overflow" );
		}
		const auto next =
			owners.capacity() > owners.max_size() / 2
				? owners.max_size()
				: std::min( owners.max_size(), std::max<size_t>( 8, owners.capacity() * 2 ) );
		owners.reserve( next );
	}

#include "Workphone/Procedural/CityHillsideSpatial.hpp"
#include "Workphone/Procedural/CityParkLayout.hpp"
#include "Workphone/Procedural/CityReserveLayout.hpp"

	// ProceduralSceneMaterials.cpp
	SmartPtr<render::IMaterial> material( SceneAssets &assets, ColourF colour, float roughness,
										  float metalness = 0 );
	SmartPtr<render::ITexture> uploadPixels(
		SceneAssets &assets, const u8 *pixels, size_t bytes, u32 width, u32 height,
		render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1,
		const char *citySurface = nullptr, bool sourceIsBgra = false );
	SmartPtr<render::ITexture> upload(
		SceneAssets &assets, const TextureBuffer &buffer,
		render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1,
		const char *citySurface = nullptr );
	SmartPtr<render::ITexture> uploadLinearRadiance( SceneAssets &assets, const Array<f32> &rgba,
													 Vector2I size );
	void maps( SceneAssets &assets, SmartPtr<render::IMaterial> m, const TextureBuffer &albedo,
			   const TextureBuffer &normal, const TextureBuffer &orm,
			   const char *citySurface = nullptr );
	bool applyCitySurface( SceneAssets &assets, SmartPtr<render::IMaterial> material,
						   CitySurface family, u32 size, float normalStrength, u32 projection,
						   u32 seed = 0 );
	SmartPtr<render::IMaterial> raceAsphalt( SceneAssets &assets, SmartPtr<ITextureForge> forge,
											 const SurfaceBakeParams &params, u32 seed );

	// ProceduralSceneActors.cpp
	SmartPtr<scene::IGameActor> publishMesh( SceneAssets &assets, const String &name,
											 SmartPtr<IMesh> generated, size_t triangles,
											 SmartPtr<render::IMaterial> m,
											 SmartPtr<scene::IGameActor> parent = nullptr,
											 bool deferNative = false );
	SmartPtr<scene::IGameActor> mesh( SceneAssets &assets, const String &name, const Geometry &g,
									  SmartPtr<render::IMaterial> m,
									  SmartPtr<scene::IGameActor> parent = nullptr,
									  bool deferNative = false );
	Vector3F vector( const VehiclePhysicsVector3 &p );
	void attachWorldActor( SmartPtr<IGameScene> scene, SmartPtr<IGameActor> owner,
						   SmartPtr<IGameActor> actor );
	void staticBox( SceneAssets &assets, const String &name, Vector3F centre, Vector3F size,
					QuaternionF orientation = QuaternionF::identity() );
	void railCollision( SceneAssets &assets, Vector3F start, Vector3F end );
	float cityValueNoise( u32 seed, float x, float z );

	// ProceduralVehicleScene.cpp
	void buildCar( SceneAssets &assets, SmartPtr<scene::IGameActor> actor, u32 seed,
				   VehicleAppearanceQuality quality );

	// ProceduralCircuitScene.cpp
	void buildTrack( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality );
	void buildTerrainGameplayScene( SceneAssets &assets );

	// ProceduralFoliageWorld.cpp
	void buildFoliageWorld(
		SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality,
		const OpenCityLayout *city = nullptr, const CityRoadTerrainMesh *cityFootways = nullptr,
		const SourceBudgetVector<cityParkLandscape::Layout> *parkLayouts = nullptr,
		const SourceBudgetVector<cityReserveLandscape::Site> *reserveSites = nullptr );

	// ProceduralCityStreets.cpp
	SmartPtr<render::IMaterial> streetLampLensMaterial( SceneAssets &assets );
	void buildCityStreetDressing( SceneAssets &assets, const OpenCityLayout &city,
								  VehicleAppearanceQuality quality );
	void measureStreetFixtureSupport( CityStreetFixture &site, const Vector3F &a, const Vector3F &b,
									  const Vector3F &c );
	void buildTerrainCityStreetDressing( SceneAssets &assets, const CityRoadTerrainMesh &walks,
										 const SourceBudgetVector<Vector3F> &existingLamps,
										 VehicleAppearanceQuality quality,
										 SourceBudgetVector<CityStreetFixture> &installed );
	void buildNaturalCityStreets( SceneAssets &assets, const OpenCityLayout &city, Geometry &asphalt,
								  Geometry &pavement, Geometry &markings, Geometry &streetMetal,
								  Geometry &lampSteel, Geometry &lampGlass, VehicleAppearanceQuality quality );

	// ProceduralCityVerges.cpp
	void buildTerrainCityVerges( SceneAssets &assets, const CityRoadTerrainMesh &walks, u32 seed,
								 VehicleAppearanceQuality quality );

	// ProceduralCityTerrain.cpp
	SourceBudgetVector<Vector3F> prepareTerrainCity( SceneAssets &assets, OpenCityLayout &city,
													 SmartPtr<IGameActor> sourceOwner,
													 SmartPtr<IGameActor> vehicle, u32 seed,
													 CityRoadBlockFaces &blockFaces );

	// ProceduralCityDebug.cpp
	void buildCityLayoutDebug( SceneAssets &assets, const OpenCityLayout &city,
							   const CityRoadBlockFaces *terrainBlocks = nullptr );

	// ProceduralCityTraffic.cpp
	void buildCityTraffic( SceneAssets &assets, const OpenCityLayout &city,
						   SmartPtr<IGameActor> player, u32 seed );

	// ProceduralCityBuilder.cpp
	void buildCity( SceneAssets &assets, SmartPtr<IGameActor> sourceOwner,
					SmartPtr<IGameActor> vehicle, u32 seed, VehicleAppearanceQuality quality );

	// ProceduralRoadShowcase.cpp
	void buildRoadAuthoringShowcase( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32,
									 VehicleAppearanceQuality );

	// ProceduralSky.cpp
	void buildSky( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32 seed );
}  // namespace workphone::scene::race::sceneBuild
