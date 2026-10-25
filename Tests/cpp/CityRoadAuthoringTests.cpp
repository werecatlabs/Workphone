#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <iostream>
#include <stdexcept>

using namespace workphone;
using namespace workphone::procedural;
using namespace workphone::scene;
namespace
{
    void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error( error.c_str() ); }
    CityRoadSpline road()
    {
        CityRoadSpline r; r.id = "30000000-0000-4000-8000-000000000001";
        r.heightMode = CityRoadHeightMode::Fixed; r.width = 6;
        r.knots.push_back( { "40000000-0000-4000-8000-000000000001", { 0, 0, 20 }, {}, {} } );
        r.knots.push_back( { "40000000-0000-4000-8000-000000000002", { 0, 0, -20 }, {}, {} } );
        return r;
    }
    String encode( const CityRoadSplineSource &source )
    {
        SourceBudgetText json; String error;
        check( serializeCityRoadSplineSource( source, json, error ), error );
        return String( json.data(), json.size() );
    }
    void revisionAndPublication()
    {
        auto document = make_ptr<CityRoadAuthoring>(); String error;
        const auto initial = document->getSourceJSON(); const auto initialRevision = document->getSourceRevision();
        CityRoadSplineSource empty;
        check( deserializeCityRoadSplineSource( std::string_view( initial.data(), initial.size() ), empty, error ), error );
        require( empty.version == 2 && empty.roads.empty() && empty.roundabouts.empty(),
                 "New component has no valid canonical empty document for first-edit Undo" );
        auto source = empty; source.roads.push_back( road() );
        check( document->setSourceJSON( encode( source ), initialRevision, error ), error );
        const auto first = document->getSourceJSON(); const auto firstRevision = document->getSourceRevision();
        require( firstRevision > initialRevision && document->getBake().roads.size() == 1 &&
                 document->getBake().sourceRevision == firstRevision && document->getActiveRevision() == 0,
                 "Headless commit did not prepare a bake or incorrectly claimed native publication" );
        require( !document->setSourceJSON( initial, initialRevision, error ) && document->getSourceJSON() == first,
                 "A stale external edit overwrote the source" );
        require( !document->setSourceJSON( "", firstRevision, error ) && document->getSourceJSON() == first,
                 "Malformed empty JSON erased the source" );

        document->setPublishCallback( []( const auto &, const auto &, auto, const auto &, String &error ) {
            error = "Injected native publication denial"; return false;
        } );
        source.roads[0].width = 8;
        require( !document->setSourceJSON( encode( source ), firstRevision, error ) &&
                 document->getSourceJSON() == first && document->getSourceRevision() == firstRevision,
                 "Rejected publication changed canonical source/revision" );
        document->setPublishCallback( []( const auto &, const auto &, auto, const auto &, String &error ) {
            error.clear(); return true;
        } );
        check( document->setSourceJSON( encode( source ), firstRevision, error ), error );
        require( document->getActiveRevision() == document->getSourceRevision(),
                 "Accepted publisher did not advance the active revision" );
        const auto afterRevision = document->getSourceRevision();
        check( document->setSourceJSON( initial, afterRevision, error ), error );
        require( document->getSource().roads.empty() && document->getBake().roads.empty() &&
                 document->getSourceRevision() > afterRevision,
                 "First-edit Undo did not clear through canonical source or rewound the revision token" );
        check( document->setSourceJSON( first, document->getSourceRevision(), error ), error );
        require( document->getSource().roads.size() == 1 && document->getSourceRevision() > afterRevision + 1,
                 "Redo did not restore the first road with a new revision" );
    }
    void draftsAndPersistence()
    {
        auto document = make_ptr<CityRoadAuthoring>(); String error;
        const auto before = document->getSourceJSON();
        check( document->beginDraft( error, document->getSourceRevision() ), error );
        auto draftRoad = road(); auto firstKnot = draftRoad.knots[0], secondKnot = draftRoad.knots[1];
        draftRoad.knots.clear();
        check( document->addDraftRoad( draftRoad, error ), error );
        const String roadId( draftRoad.id.c_str() );
        check( document->addDraftKnot( roadId, firstKnot, error ), error );
        require( !document->commitDraft( error ) && document->hasDraft() && document->getSourceJSON() == before,
                 "Incomplete draft changed source intent or lost its editable controls" );
        check( document->addDraftKnot( roadId, secondKnot, error ), error );
        check( document->moveDraftKnot( roadId, String( secondKnot.id.c_str() ), { 0, 0, -24 }, {}, {}, error ), error );
        check( document->commitDraft( error ), error );
        require( !document->hasDraft() && document->getSource().roads[0].knots[1].position.z == -24,
                 "Draft commit lost handle/control edits" );

        auto reopened = make_ptr<CityRoadAuthoring>();
        reopened->setProperties( document->getProperties() );
        require( reopened->getSourceJSON() == document->getSourceJSON(),
                 "Component scene Properties did not preserve canonical source and IDs" );
        const auto saved = document->getSourceJSON();
        check( document->beginDraft( error ), error );
        check( document->moveDraftKnot( roadId, String( secondKnot.id.c_str() ), { 0, 0, -30 }, {}, {}, error ), error );
        document->cancelDraft();
        require( document->getSourceJSON() == saved && !document->hasDraft(), "Cancel published draft geometry/source" );
        check( document->beginDraft( error ), error );
        auto external = document->getSource(); external.roads[0].width = 7;
        check( document->setSourceJSON( encode( external ), document->getSourceRevision(), error ), error );
        const auto externalJSON = document->getSourceJSON();
        require( !document->commitDraft( error ) && document->getSourceJSON() == externalJSON,
                 "Stale draft overwrote an accepted external edit" );
        document->cancelDraft();
    }
    void pendingSceneRestore()
    {
        auto saved = make_ptr<CityRoadAuthoring>(); String error;
        auto source = saved->getSource(); source.roads.push_back( road() );
        check( saved->setSourceJSON( encode( source ), saved->getSourceRevision(), error ), error );
        auto restored = make_ptr<CityRoadAuthoring>();
        auto actor = make_ptr<GameActor>(); restored->setActor( actor );
        unsigned attempts = 0;
        restored->setPublishCallback( [&]( const auto &, const auto &, auto, const auto &, String &error ) {
            ++attempts; error = "Scene actor is not attached yet"; return false;
        } );
        restored->setProperties( saved->getProperties() );
        require( restored->getSourceJSON() == saved->getSourceJSON() && restored->getActiveRevision() == 0 &&
                 restored->getBake().sourceRevision == 0, "Pre-scene native rejection discarded serialized road intent" );
        const auto previousAttempts = attempts; restored->update();
        require( attempts == previousAttempts, "Pending source tried publishing before its owner joined a scene" );
        restored->setPublishCallback( []( const auto &, const auto &, auto, const auto &, String &error ) {
            error.clear(); return true;
        } );
        restored->setActor( nullptr );
        check( restored->rebuild( error ), error );
        require( restored->getActiveRevision() == restored->getSourceRevision() &&
                 restored->getSourceJSON() == saved->getSourceJSON(), "Pending source could not publish without losing saved identity" );
    }
    void failedTerrainRefresh()
    {
        auto terrain = make_ptr<TerrainSystem>(); render::TerrainData data; String error;
        data.dimensions = { 65, 65 }; data.origin = { -32, -32 }; data.heights.assign( 65 * 65, 0 );
        check( terrain->applyTerrainData( data, error ), error );
        require( terrain->getHandle() && terrain->getHandle()->getUUID().is_nil(),
                 "Fresh headless terrain fixture should exercise binding a nil persistent identity" );
        auto document = make_ptr<CityRoadAuthoring>();
        document->setPublishCallback( []( const auto &, const auto &, auto, const auto &, String &error ) {
            error.clear(); return true;
        } );
        check( document->bindTerrain( terrain, error ), error );
        require( !terrain->getHandle()->getUUID().is_nil() &&
                 document->getTerrainBindingUUID() == terrain->getHandle()->getUUIDAsString(),
                 "Binding did not assign/preserve the terrain component's stable nonzero identity" );
        auto source = document->getSource(); auto draped = road();
        draped.heightMode = CityRoadHeightMode::Drape; draped.heightOffset = .025f;
        source.roads.push_back( std::move( draped ) );
        check( document->setSourceJSON( encode( source ), document->getSourceRevision(), error ), error );
        const auto saved = document->getSourceJSON(); const auto revision = document->getSourceRevision();
        require( document->getActiveRevision() == revision, "Terrain refresh fixture did not start published" );
        unsigned attempts = 0;
        document->setPublishCallback( [&]( const auto &, const auto &, auto, const auto &, String &error ) {
            ++attempts; error = "Injected terrain recook denial"; return false;
        } );
        data.heights[0] = .1f; check( terrain->applyTerrainData( data, error, terrain->getTerrainRevision() ), error );
        document->update();
        require( attempts == 1 && document->getSourceJSON() == saved && document->getSourceRevision() == revision &&
                 document->getActiveRevision() == 0 && document->getBake().sourceRevision == 0 &&
                 !document->getError().empty(), "Failed terrain recook retained stale active road contacts or erased source intent" );
        document->update();
        require( attempts == 1, "Failed terrain recook retried an expensive unchanged tuple every frame" );
        document->setPublishCallback( []( const auto &, const auto &, auto, const auto &, String &error ) {
            error.clear(); return true;
        } );
        check( document->rebuild( error ), error );
        require( document->getActiveRevision() == revision && document->getBake().sourceRevision == revision,
                 "Explicit retry could not restore the retained source after terrain recook denial" );
    }
}
int main()
{
    TypeManager types; types.load(); TypeManager::setInstance( &types );
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( application );
    application->setFactoryManager( make_ptr<FactoryManager>() );
    application->setGameManager( make_ptr<GameManager>() );
    int result = 0;
    try { revisionAndPublication(); draftsAndPersistence(); pendingSceneRestore(); failedTerrainRefresh(); std::cout << "City road authoring: canonical source, revisions, publication failures, drafts and scene persistence PASS\n"; }
    catch( const std::exception &e ) { std::cerr << "City road authoring: " << e.what() << '\n'; result = 1; }
    application->setGameManager( nullptr ); application->setFactoryManager( nullptr );
    core::IApplicationManager::setInstance( nullptr ); application = nullptr;
    TypeManager::setInstance( nullptr ); types.unload(); WorldStreamingScheduler::shutdownShared(); return result;
}
