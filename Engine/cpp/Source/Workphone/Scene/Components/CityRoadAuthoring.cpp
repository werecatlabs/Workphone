#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Core/Handle.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        bool finite( Vector3F p )
        {
            return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ) &&
                   std::abs( p.x ) <= 1000000 && std::abs( p.y ) <= 1000000 &&
                   std::abs( p.z ) <= 1000000;
        }
        bool matches( const SourceBudgetText &id, const String &text )
        {
            return std::string_view( id.data(), id.size() ) ==
                   std::string_view( text.data(), text.size() );
        }
        String text( const SourceBudgetText &value )
        {
            return String( value.data(), value.size() );
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone::scene, CityRoadAuthoring, Component );
    CityRoadAuthoring::CityRoadAuthoring()
    {
        m_source.version = 2;
        m_source.id = StringUtil::getUUID().c_str();
        String error;
        if( !procedural::serializeCityRoadSplineSource( m_source, m_sourceJSON, error ) )
            throw std::runtime_error( error.c_str() );
        m_bake.sourceRevision = m_source.revision;
    }
    CityRoadAuthoring::~CityRoadAuthoring()
    {
        try { clearRuntime(); } catch( ... ) { }
    }

    void CityRoadAuthoring::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() ) return;
        Component::load( data );
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = app->getGameManager() )
                manager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::Update, this );
        setLoadingState( LoadingState::Loaded );
        // The constructor's empty document is not restored authored intent.
        // Scene admission loads a component before applying its saved properties;
        // accepting that default here would disable pending-source restoration.
        if( !m_sourceJSON.empty() && (m_hasAcceptedSource || !m_terrainBindingUUID.empty() ||
            !m_source.roads.empty() || !m_source.roundabouts.empty()) )
        {
            String error;
            rebuild( error );
        }
    }

    void CityRoadAuthoring::unload( SmartPtr<ISharedObject> data )
    {
        cancelDraft();
        clearRuntime();
        m_activeRevision = 0;
        m_publishCallback = {};
        m_terrain = nullptr;
        m_terrainSnapshot.reset();
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = app->getGameManager() ) manager->unregisterAllComponent( this );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    String CityRoadAuthoring::getSourceJSON() const { return text( m_sourceJSON ); }
    const procedural::CityRoadSplineSource &CityRoadAuthoring::getSource() const { return m_source; }
    const procedural::CityRoadSplineBake &CityRoadAuthoring::getBake() const { return m_bake; }
    u64 CityRoadAuthoring::getSourceRevision() const { return m_source.revision; }
    u64 CityRoadAuthoring::getActiveRevision() const { return m_activeRevision; }
    const String &CityRoadAuthoring::getError() const { return m_error; }
    SmartPtr<TerrainSystem> CityRoadAuthoring::getTerrain() const { return m_terrain.load().lock(); }
    String CityRoadAuthoring::getTerrainBindingUUID() const { return m_terrainBindingUUID; }
    render::TerrainSnapshot CityRoadAuthoring::getTerrainSnapshot() const { return m_terrainSnapshot; }
    const Transform3<real_Num> &CityRoadAuthoring::getTerrainPlacement() const { return m_terrainPlacement; }
    Vector3F CityRoadAuthoring::localToWorld( Vector3F sourcePoint ) const { return sourcePoint; }

    bool CityRoadAuthoring::captureTerrain( render::TerrainSnapshot &snapshot,
                                           Transform3<real_Num> &placement, u64 &serial,
                                           String &error ) const
    {
        auto terrain = getTerrain();
        if( !terrain ) { error = "Bind an available terrain component before fitting roads."; return false; }
        if(auto actor=terrain->getActor())
            if(actor->getScene() && !actor->getFlag(IGameActor::ActorFlagInScene))
            {error="The bound terrain actor has left its scene.";return false;}
        bool detached = false;
        terrain->getTerrainIntegrationState( snapshot, placement, serial, detached );
        if( detached ) { error = "The bound terrain has been detached or unloaded."; return false; }
        if( !snapshot )
        {
            // Headless/initial authoring can precede renderer creation. Capture the
            // authoritative snapshot and the owning terrain actor's validated pose.
            snapshot = terrain->getTerrainSnapshot();
            if( auto actor = terrain->getActor() )
                if( auto transform = actor->getTransform() ) placement = transform->getWorldTransform();
        }
        if( !snapshot ) { error = "The bound terrain has no sample snapshot."; return false; }
        if( snapshot == m_terrainSnapshot && placement == m_terrainPlacement && serial == m_terrainSerial )
            return true;
        return render::validateTerrainPlacement( *snapshot, placement, error );
    }

    bool CityRoadAuthoring::intersectTerrain( const Ray3F &ray, Vector3F &sourceHit,
                                             String &error ) const
    {
        render::TerrainSnapshot snapshot;
        Transform3<real_Num> placement;
        u64 serial = 0;
        if( !captureTerrain( snapshot, placement, serial, error ) ) return false;
        Vector3F hit;
        if( !render::intersectTerrain( *snapshot, placement, ray, hit ) )
        { error = "The pointer does not hit supported terrain; holes and outside areas cannot place a road."; return false; }
        sourceHit = hit;
        error.clear();
        return true;
    }

    bool CityRoadAuthoring::bindTerrain( SmartPtr<TerrainSystem> terrain, String &error )
    {
        if( !terrain ) { error = m_error = "Select a terrain component to bind."; return false; }
        const auto previous = getTerrain();
        const auto previousUuid = m_terrainBindingUUID;
        try
        {
            auto handle = terrain->getHandle();
            if( !handle ) throw std::runtime_error( "The terrain component has no persistent handle." );
            String uuid = handle->getUUIDAsString();
            if( uuid.empty() || handle->getUUID().is_nil() )
            { uuid = StringUtil::getUUID(); handle->setUUID( uuid ); }
            m_terrain = terrain;
            m_terrainBindingUUID = uuid;
            render::TerrainSnapshot snapshot;
            Transform3<real_Num> placement;
            u64 serial = 0;
            if( !captureTerrain( snapshot, placement, serial, error ) ) throw std::runtime_error( error.c_str() );
            if( !m_sourceJSON.empty() )
            {
                auto candidate = m_source;
                candidate.terrainResourceUuid = uuid.c_str();
                if( !applySource( std::move( candidate ), getSourceRevision(), false, error ) )
                    throw std::runtime_error( error.c_str() );
            }
            else
            {
                m_terrainSnapshot = std::move( snapshot );
                m_terrainPlacement = placement;
                m_terrainSerial = serial;
            }
            cancelDraft();
            error.clear(); m_error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            m_terrain = previous;
            m_terrainBindingUUID = previousUuid;
            error = m_error = e.what();
            return false;
        }
    }

    bool CityRoadAuthoring::resolveTerrainBinding()
    {
        if( auto terrain = getTerrain() ) return true;
        if( m_terrainBindingUUID.empty() ) return false;
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = app->getGameManager() )
                for( auto actor : manager->getActors() )
                    if( actor && actor->getFlag(IGameActor::ActorFlagInScene) )
                        if( auto terrain = actor->getComponent<TerrainSystem>() )
                            if( auto handle = terrain->getHandle() )
                                if( handle->getUUIDAsString() == m_terrainBindingUUID )
                                { m_terrain = terrain; return true; }
        return false;
    }

    bool CityRoadAuthoring::setSourceJSON( const String &json, String &error )
    { return setSourceJSON( json, 0, error ); }

    bool CityRoadAuthoring::setSourceJSON( const String &json, u64 expectedRevision, String &error )
    {
        if( expectedRevision && expectedRevision != getSourceRevision() )
        { error = m_error = "The road source changed since this edit began."; return false; }
        procedural::CityRoadSplineSource candidate;
        if( !procedural::deserializeCityRoadSplineSource(
                std::string_view( json.data(), json.size() ), candidate, error ) )
        { m_error = error; return false; }
        return applySource( std::move( candidate ), expectedRevision, false, error );
    }

    bool CityRoadAuthoring::applySource( procedural::CityRoadSplineSource candidate,
                                        u64 expectedRevision, bool preserveRevision, String &error )
    {
        try
        {
            if( expectedRevision && expectedRevision != getSourceRevision() )
                throw std::runtime_error( "The road source changed since this edit began." );
            const u64 oldRevision = getSourceRevision();
            if( !preserveRevision )
            {
                if( oldRevision == std::numeric_limits<u64>::max() )
                    throw std::runtime_error( "Road source revision limit reached." );
                candidate.revision = oldRevision ? oldRevision + 1 : std::max<u64>( 1, candidate.revision );
            }
            if( !procedural::validateCityRoadSplineSource( candidate, error ) )
                throw std::runtime_error( error.c_str() );

            const bool needsTerrain = !candidate.roundabouts.empty() || std::any_of( candidate.roads.begin(), candidate.roads.end(),
                []( const auto &road ) { return road.heightMode == procedural::CityRoadHeightMode::Drape; } );
            render::TerrainSnapshot snapshot;
            Transform3<real_Num> placement;
            u64 serial = 0;
            resolveTerrainBinding();
            if( getTerrain() )
            {
                if( !captureTerrain( snapshot, placement, serial, error ) )
                    throw std::runtime_error( error.c_str() );
                candidate.terrainRevision = snapshot->revision;
            }
            else if( needsTerrain ) throw std::runtime_error( "The source's terrain binding is unavailable." );

            procedural::CityRoadSplineBake bake;
            const procedural::CityRoadHeightSampler sampler = snapshot ?
                procedural::CityRoadHeightSampler( [snapshot, placement]( const Vector3F &point, f32 &height ) {
                    return render::sampleTerrainHeight( *snapshot, placement, point, height );
                } ) : procedural::CityRoadHeightSampler{};
            if( !procedural::sampleCityRoadSplines( candidate, {}, sampler, bake, error ) )
                throw std::runtime_error( error.c_str() );
            SourceBudgetText canonical;
            if( !procedural::serializeCityRoadSplineSource( candidate, canonical, error ) )
                throw std::runtime_error( error.c_str() );

            // A terrain/source change during preparation must be rejected before
            // runtime publication. Every allocation is complete before that call.
            if( expectedRevision && expectedRevision != getSourceRevision() )
                throw std::runtime_error( "The road source changed during preparation." );
            if( snapshot )
            {
                render::TerrainSnapshot current;
                Transform3<real_Num> currentPlacement;
                u64 currentSerial = 0;
                if( !captureTerrain( current, currentPlacement, currentSerial, error ) ||
                    current != snapshot || currentSerial != serial )
                    throw std::runtime_error( "The terrain changed during road preparation." );
            }
            const bool runtime = bool( m_publishCallback ) || bool( getActor() );
            if( m_publishCallback )
            {
                if( !m_publishCallback( candidate, bake, snapshot, placement, error ) )
                    throw std::runtime_error( error.empty() ? "Road runtime publication was rejected." : error.c_str() );
            }
            else if( getActor() && !publishRuntime( candidate, bake, snapshot, placement, error ) )
                throw std::runtime_error( error.empty() ? "Road runtime publication failed." : error.c_str() );

            m_source.swap( candidate );
            m_bake.roads.swap( bake.roads );
            std::swap( m_bake.sourceRevision, bake.sourceRevision );
            m_sourceJSON.swap( canonical );
            m_terrainSnapshot = std::move( snapshot );
            m_terrainPlacement = placement;
            m_terrainSerial = serial;
            m_hasAcceptedSource = true;
            m_pendingPublication = false;
            if( runtime ) m_activeRevision = m_source.revision;
            error.clear(); m_error.clear();
            return true;
        }
        catch( const std::exception &e ) { error = m_error = e.what(); return false; }
    }

    bool CityRoadAuthoring::rebuild( String &error )
    {
        if( m_sourceJSON.empty() ) { error = m_error = "Draw or import a road before building."; return false; }
        try { return applySource( m_source, getSourceRevision(), true, error ); }
        catch( const std::exception &e ) { error = m_error = e.what(); return false; }
    }

    void CityRoadAuthoring::update()
    {
        // Scene loading restores component properties before its owner is added
        // to the scene. Do not turn that temporary state into a failed generation
        // tuple that would suppress the first real publication after attachment.
        if( auto actor = getActor() )
        {
            if(!actor->getScene())return;
            if(!actor->getFlag(IGameActor::ActorFlagInScene))
            {
                if(m_activeRevision)
                {
                    clearRuntime();m_activeRevision=0;m_terrainSnapshot.reset();
                    m_bake.roads.releaseStorage();m_bake.sourceRevision=0;
                    m_pendingPublication=true;m_lastTerrainAttempt.reset();
                    m_error="The road source actor has left its scene.";
                }
                return;
            }
        }
        if( m_sourceJSON.empty() || !isEnabled() ) return;
        if( m_pendingPublication && m_source.roads.empty() && m_source.roundabouts.empty() )
        { String error; rebuild( error ); return; }
        if( m_source.roads.empty() && m_source.roundabouts.empty() ) return;
        resolveTerrainBinding();
        render::TerrainSnapshot snapshot;
        Transform3<real_Num> placement;
        u64 serial = 0;
        String error;
        if( !captureTerrain( snapshot, placement, serial, error ) )
        {
            m_error = error;
            if( m_activeRevision )
            {
                try { clearRuntime(); m_activeRevision = 0; m_terrainSnapshot.reset(); }
                catch( const std::exception &e ) { m_error = e.what(); }
            }
            return;
        }
        // A consumer may acknowledge the same immutable terrain/pose with a
        // newer integration serial. It changes no road geometry or authored token.
        if(!m_pendingPublication && m_activeRevision && snapshot==m_terrainSnapshot && placement==m_terrainPlacement)
        {m_terrainSerial=serial;return;}
        if( m_pendingPublication || snapshot != m_terrainSnapshot || serial != m_terrainSerial || placement != m_terrainPlacement )
        {
            if( m_lastTerrainAttempt.lock() == snapshot && m_lastAttemptSerial == serial &&
                m_lastAttemptSourceRevision == getSourceRevision() && m_lastAttemptPlacement == placement ) return;
            m_lastTerrainAttempt = snapshot; m_lastAttemptSerial = serial;
            m_lastAttemptSourceRevision = getSourceRevision(); m_lastAttemptPlacement = placement;
            bool published = false;
            try { published = applySource( m_source, getSourceRevision(), true, error ); }
            catch( const std::exception &e ) { m_error = e.what(); }
            if( !published )
            {
                // Terrain has already changed. Old road contacts can no longer
                // be treated as last-good support over the newly published holes
                // or elevations. Retain editable source intent, remove stale solids.
                const String refreshError = m_error.empty() ? error : m_error;
                try
                {
                    clearRuntime(); m_activeRevision = 0;
                    m_bake.roads.releaseStorage(); m_bake.sourceRevision = 0;
                    m_terrainSnapshot = snapshot; m_terrainPlacement = placement; m_terrainSerial = serial;
                    m_error = refreshError;
                }
                catch( const std::exception &e ) { m_error = e.what(); }
            }
        }
    }

    SmartPtr<Properties> CityRoadAuthoring::getProperties() const
    {
        auto properties = Component::getProperties();
        properties->setProperty( "Source JSON", getSourceJSON() );
        properties->setProperty( "Terrain Component UUID", m_terrainBindingUUID );
        return properties;
    }

    void CityRoadAuthoring::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties ) return;
        Component::setProperties( properties );
        String json = getSourceJSON(), binding = m_terrainBindingUUID;
        properties->getPropertyValue( "Source JSON", json );
        properties->getPropertyValue( "Terrain Component UUID", binding );
        procedural::CityRoadSplineSource candidate;
        String error;
        if( !procedural::deserializeCityRoadSplineSource(
                std::string_view( json.data(), json.size() ), candidate, error ) )
        { m_error = error; return; }
        const auto oldTerrain = getTerrain();
        const auto oldBinding = m_terrainBindingUUID;
        if( binding != m_terrainBindingUUID ) { m_terrain = nullptr; m_terrainBindingUUID = binding; }
        resolveTerrainBinding();
        try
        {
            if( applySource( candidate, 0, !m_hasAcceptedSource, error ) ) { cancelDraft(); return; }
        }
        catch( const std::exception &e ) { error = e.what(); }
        // First restoration owns valid serialized intent even when the owner and
        // terrain exist but their GPU/physics generation is not active yet. Only
        // later edits are transactional against a previously accepted document.
        if( !m_hasAcceptedSource )
        {
            const auto publicationError=error;
            SourceBudgetText canonical;
            if( procedural::serializeCityRoadSplineSource( candidate, canonical, error ) )
            {
                m_source.swap( candidate ); m_sourceJSON.swap( canonical );
                m_hasAcceptedSource = true; m_pendingPublication = true;
                m_bake.roads.clear(); m_bake.sourceRevision = 0;
                m_error = "Road source restored; waiting for publication: "+publicationError;
                return;
            }
        }
        m_terrain = oldTerrain;
        m_terrainBindingUUID = oldBinding;
        m_error = error;
    }

    bool CityRoadAuthoring::beginDraft( String &error, u64 expectedRevision )
    {
        if( expectedRevision && expectedRevision != getSourceRevision() )
        { error = "The road source changed before the draft began."; return false; }
        try
        {
            procedural::CityRoadSplineSource draft = m_source;
            if( m_sourceJSON.empty() )
            {
                draft.id = StringUtil::getUUID().c_str();
                draft.terrainResourceUuid = m_terrainBindingUUID.c_str();
            }
            m_draft.swap( draft ); m_draftRevision = getSourceRevision(); m_hasDraft = true;
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
    bool CityRoadAuthoring::hasDraft() const { return m_hasDraft; }
    const procedural::CityRoadSplineSource &CityRoadAuthoring::getDraftSource() const { return m_draft; }

    bool CityRoadAuthoring::addDraftRoad( const procedural::CityRoadSpline &road, String &error )
    {
        if( !m_hasDraft ) { error = "Begin a road draft first."; return false; }
        if( m_draft.roads.size() >= 128 ) { error = "The draft road limit is 128."; return false; }
        try { m_draft.roads.push_back( road ); error.clear(); return true; }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    bool CityRoadAuthoring::addDraftKnot( const String &roadId,
                                         const procedural::CityRoadSplineKnot &knot, String &error )
    {
        if( !m_hasDraft ) { error = "Begin a road draft first."; return false; }
        if( !finite( knot.position ) || !finite( knot.inHandle ) || !finite( knot.outHandle ) )
        { error = "The draft knot and handles must be finite world-space metres."; return false; }
        for( auto &road : m_draft.roads ) if( matches( road.id, roadId ) )
        {
            if( road.knots.size() >= 256 ) { error = "The draft knot limit is 256 per road."; return false; }
            try { road.knots.push_back( knot ); error.clear(); return true; }
            catch( const std::exception &e ) { error = e.what(); return false; }
        }
        error = "The selected draft road no longer exists."; return false;
    }

    bool CityRoadAuthoring::moveDraftKnot( const String &roadId, const String &knotId,
                                          Vector3F position, Vector3F inHandle, Vector3F outHandle,
                                          String &error )
    {
        if( !m_hasDraft ) { error = "Begin a road draft first."; return false; }
        if( !finite( position ) || !finite( inHandle ) || !finite( outHandle ) )
        { error = "The draft knot and handles must be finite world-space metres."; return false; }
        for( auto &road : m_draft.roads ) if( matches( road.id, roadId ) )
            for( auto &knot : road.knots ) if( matches( knot.id, knotId ) )
            { knot.position = position; knot.inHandle = inHandle; knot.outHandle = outHandle; error.clear(); return true; }
        error = "The selected draft control point no longer exists."; return false;
    }

    bool CityRoadAuthoring::commitDraft( String &error )
    {
        if( !m_hasDraft ) { error = "There is no draft to commit."; return false; }
        if( m_draftRevision != getSourceRevision() )
        { error = "The road source changed while the draft was open; cancel or restart the draft."; return false; }
        try { if( !applySource( m_draft, m_draftRevision, false, error ) ) return false; }
        catch( const std::exception &e ) { error = m_error = e.what(); return false; }
        cancelDraft(); return true;
    }

    void CityRoadAuthoring::cancelDraft()
    {
        procedural::CityRoadSplineSource empty;
        m_draft.swap( empty ); m_hasDraft = false; m_draftRevision = 0;
    }
    void CityRoadAuthoring::setPublishCallback( PublishCallback callback )
    { m_publishCallback = std::move( callback ); }
}
