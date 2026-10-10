#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Graphics/TerrainBlendMapImpl.hpp>
#include <Workphone/Graphics/TerrainRayResult.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Interface/Memory/IObject.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>
#include <Workphone/State/States/TerrainStateData.hpp>
#include <Workphone/State/States/TransformStateData.hpp>

#include <cmath>
#include <limits>
#include <atomic>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Terrain, SharedGraphicsObject<IGraphicsTerrain> );

    const String Terrain::HeightMapSizeStr = "heightMapSize";
    const String Terrain::HeightScaleStr = "heightScale";
    const String Terrain::ShowWireframeStr = "showWireframe";
    const String Terrain::MaterialNameStr = "materialName";
    const String Terrain::VisibleStr = "visible";
    const String Terrain::HeightMapStr = "heightMap";

    u32 Terrain::m_idExt = 0;

    Terrain::Terrain()
    {
        auto name = String( "Terrain" ) + StringUtil::toString( m_idExt++ );
        auto id = StringUtil::getHash( name );

        setName( name );
        setId( id );

        setEventTaskFlags( Thread::Render_Flag );

        auto initial = std::make_shared<TerrainData>();
        initial->dimensions = m_heightMapSize;
        initial->origin = Vector2F( -m_heightMapSize.x * 0.5f, -m_heightMapSize.y * 0.5f );
        initial->heightScale = m_heightScale;
        initial->heights.resize( size_t( m_heightMapSize.x ) * m_heightMapSize.y, 0.0f );
        initial->revision = 1;
        m_terrainSnapshot = std::move( initial );
    }

    Terrain::~Terrain()
    {
        unload( nullptr );
    }

    TerrainSnapshot Terrain::getTerrainSnapshot() const
    {
        return std::atomic_load( &m_terrainSnapshot );
    }

    u64 Terrain::getTerrainRevision() const
    {
        return getTerrainSnapshot()->revision;
    }

    bool Terrain::applyTerrainData( const TerrainData &data, String &error, u64 expectedRevision )
    {
        if( !validateTerrainData( data, error ) )
            return false;
        try
        {
            auto candidate = std::make_shared<TerrainData>( data );
            {
                std::lock_guard<std::mutex> lock( m_terrainDataMutex );
                const auto current = getTerrainSnapshot();
                if( expectedRevision && expectedRevision != current->revision )
                {
                    error = "Terrain changed before the edit could be applied";
                    return false;
                }
                if( !validateTerrainPlacement( *candidate, m_terrainWorldTransform, error ) )
                    return false;
                if( current->dimensions == candidate->dimensions &&
                    current->spacing == candidate->spacing && current->origin == candidate->origin &&
                    current->heightScale == candidate->heightScale &&
                    current->heights == candidate->heights )
                    return true;
                if( current->revision == std::numeric_limits<u64>::max() )
                {
                    error = "Terrain revision exhausted";
                    return false;
                }
                candidate->revision = current->revision + 1;
                m_heightMapSize = candidate->dimensions;
                m_heightScale = candidate->heightScale;
                std::atomic_store( &m_terrainSnapshot, TerrainSnapshot( candidate ) );
                m_cpuTerrainMesh = nullptr;
                m_cpuTerrainMeshRevision = 0;
            }
            // The retained snapshot is authoritative. Mirror legacy state for existing
            // property/renderer listeners without making queries depend on a StateManager.
            try
            {
                if( auto context = getStateContext() )
                    if( auto state = context->invalidateStateDataById<TerrainStateData>( getId() ) )
                    {
                        state->heightMapSize = candidate->dimensions;
                        state->heightScale = candidate->heightScale;
                        state->heightData = candidate->heights;
                    }
            }
            catch( const std::exception &exception )
            {
                WP_LOG_EXCEPTION( exception );
            }
            try
            {
                _onHeightDataChanged();
                _onHeightScaleChanged();
            }
            catch( const std::exception &exception )
            {
                WP_LOG_EXCEPTION( exception );
            }
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    void Terrain::load( SmartPtr<ISharedObject> data )
    {
        SharedGraphicsObject<IGraphicsTerrain>::load( data );

        try
        {
            _ensureStateData();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Terrain::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            m_blendMaps.clear();

            if( auto stateContext = getStateContext() )
            {
                auto states = stateContext->getStates();
                for( auto &state : states )
                {
                    if( state && state->getOwnerPtr() == this )
                    {
                        state->unload( nullptr );
                        stateContext->removeState( state );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        SharedGraphicsObject<IGraphicsTerrain>::unload( data );
    }
    void Terrain::_ensureStateData()
    {
        auto stateContext = getStateContext();
        if( !stateContext )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            return;
        }

        const auto terrainId = getId();

        // Helper that creates a State wrapping StateData of type T when no matching state
        // already exists for this terrain id AND type. The state's id is set to getId() so the
        // getStateDataById<T>( getId() ) lookups used throughout this class succeed. Each state
        // type is checked independently because several state data types share the same id.
        auto ensureState = [&]( auto typeProbe, auto makeData ) -> void {
            if( typeProbe( stateContext, terrainId ) )
            {
                return;
            }

            auto state = factoryManager->make_ptr<State>();
            if( !state )
            {
                return;
            }

            state->setId( terrainId );
            state->setOwner( this );
            state->setData( makeData() );

            stateContext->addState( state );
        };

        ensureState(
            []( SmartPtr<IStateContext> &ctx, hash_type id ) -> bool {
                return ctx->getStateDataById<TerrainStateData>( id ).get() != nullptr;
            },
            [&]() -> SmartPtr<ISharedObject> {
                auto stateData = factoryManager->make_ptr<TerrainStateData>();
                if( stateData )
                {
                    const auto snapshot = getTerrainSnapshot();
                    stateData->heightMapSize = snapshot->dimensions;
                    stateData->heightScale = snapshot->heightScale;
                    stateData->materialName = m_materialName;
                    stateData->showWireframe = m_showWireframe;
                    stateData->heightData = snapshot->heights;
                }
                return stateData;
            } );

        ensureState(
            []( SmartPtr<IStateContext> &ctx, hash_type id ) -> bool {
                return ctx->getStateDataById<TransformStateData>( id ).get() != nullptr;
            },
            [&]() -> SmartPtr<ISharedObject> {
                auto state = factoryManager->make_ptr<TransformStateData>();
                if( state )
                    state->worldTransform = getWorldTransform();
                return state;
            } );

        ensureState(
            []( SmartPtr<IStateContext> &ctx, hash_type id ) -> bool {
                return ctx->getStateDataById<GraphicsObjectData>( id ).get() != nullptr;
            },
            [&]() -> SmartPtr<ISharedObject> {
                auto state = factoryManager->make_ptr<GraphicsObjectData>();
                if( state )
                    state->flags = BitUtil::setFlagValue( state->flags, IGraphicsObject::visibleFlag,
                                                          m_terrainVisible );
                return state;
            } );
    }

    Transform3<real_Num> Terrain::getWorldTransform() const
    {
        std::lock_guard<std::mutex> lock( m_terrainDataMutex );
        return m_terrainWorldTransform;
    }

    void Terrain::setWorldTransform( const Transform3<real_Num> &worldTransform )
    {
        String error;
        if( !validateTerrainTransform( worldTransform, error ) )
        {
            WP_LOG_WARNING( error );
            return;
        }
        {
            std::lock_guard<std::mutex> lock( m_terrainDataMutex );
            if( m_terrainWorldTransform == worldTransform )
                return;
            if( !validateTerrainPlacement( *getTerrainSnapshot(), worldTransform, error ) )
            {
                WP_LOG_WARNING( error );
                return;
            }
            m_terrainWorldTransform = worldTransform;
        }
        if( auto context = getStateContext() )
            if( auto state = context->invalidateStateDataById<TransformStateData>( getId() ) )
                state->worldTransform = worldTransform;
    }

    Vector3<real_Num> Terrain::getPosition() const
    {
        return getWorldTransform().getPosition();
    }

    void Terrain::setPosition( const Vector3<real_Num> &position )
    {
        if( !position.isFinite() )
        {
            WP_LOG_WARNING( "Terrain position must be finite" );
            return;
        }
        auto transform = getWorldTransform();
        transform.setPosition( position );
        setWorldTransform( transform );
    }

    f32 Terrain::getHeightAtWorldPosition( const Vector3<real_Num> &position ) const
    {
        TerrainSnapshot snapshot;
        Transform3<real_Num> transform;
        {
            std::lock_guard<std::mutex> lock( m_terrainDataMutex );
            snapshot = getTerrainSnapshot();
            transform = m_terrainWorldTransform;
        }
        f32 height = 0;
        sampleTerrainHeight( *snapshot, transform, position, height );
        return height;
    }

    u16 Terrain::getSize() const
    {
        // Vertex count along one edge of the (square) grid. Callers use (getSize() - 1) as the
        // quad count and getSize() as the row stride, matching the Ogre terrain convention.
        const auto heightMapSize = getHeightMapSize();
        const s32 size = Math<s32>::min( heightMapSize.x, heightMapSize.y );
        return static_cast<u16>(
            Math<s32>::min( size, static_cast<s32>( std::numeric_limits<u16>::max() ) ) );
    }

    Vector3<real_Num> Terrain::getTerrainSpacePosition( const Vector3<real_Num> &worldSpace ) const
    {
        const auto snapshot = getTerrainSnapshot();
        const auto local = getWorldTransform().inverseTransformPoint( worldSpace );
        if( !std::isfinite( local.x ) || !std::isfinite( local.y ) || !std::isfinite( local.z ) )
            return Vector3<real_Num>::zero();
        return { std::clamp( ( local.x - snapshot->origin.x ) / snapshot->spacing.x, real_Num( 0 ),
                             real_Num( snapshot->dimensions.x - 1 ) ),
                 local.y,
                 std::clamp( ( local.z - snapshot->origin.y ) / snapshot->spacing.y, real_Num( 0 ),
                             real_Num( snapshot->dimensions.y - 1 ) ) };
    }

    Array<f32> Terrain::getHeightData() const
    {
        return getTerrainSnapshot()->heights;
    }

    void Terrain::setHeightData( const Array<f32> &heightData )
    {
        const auto snapshot = getTerrainSnapshot();
        auto candidate = *snapshot;
        candidate.heights = heightData;
        String error;
        if( !applyTerrainData( candidate, error, snapshot->revision ) )
            WP_LOG_WARNING( error );
    }

    bool Terrain::isVisible() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<GraphicsObjectData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, IGraphicsObject::visibleFlag );
            }
        }

        return m_terrainVisible;
    }

    void Terrain::setVisible( bool visible )
    {
        m_terrainVisible = visible;
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<GraphicsObjectData>( getId() ) )
            {
                state->flags =
                    BitUtil::setFlagValue( state->flags, IGraphicsObject::visibleFlag, visible );
            }
        }
    }

    bool Terrain::getShowWireframe() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return stateData->showWireframe;
            }
        }

        return m_showWireframe;
    }

    void Terrain::setShowWireframe( bool showWireframe )
    {
        m_showWireframe = showWireframe;

        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                stateData->showWireframe = m_showWireframe;
            }
        }
    }

    String Terrain::getMaterialName() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return state->materialName;
            }
        }

        return m_materialName;
    }

    void Terrain::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;

        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                state->materialName = m_materialName;
            }
        }
    }

    void Terrain::updateMaterial()
    {
        // Base only marks the terrain state dirty so renderer plugins that poll the state
        // system pick up material-name/texture changes. GPU rebuilds are the backend's job.
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                WP_UNUSED( state );
            }
        }
    }
    SmartPtr<ITerrainBlendMap> Terrain::getBlendMap( u32 index )
    {
        // Grow the CPU-side blend map cache lazily. Renderer plugins with a native blend map
        // (e.g. CTerrainOgreBlendMap) override this and never touch this cache.
        // When no graphics system is configured, return null so callers can detect the
        // unconfigured state without having to track a separate flag.
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager || !applicationManager->getGraphicsSystemPtr() )
        {
            return nullptr;
        }

        if( index >= m_blendMaps.size() )
        {
            m_blendMaps.resize( index + 1 );
        }

        auto &slot = m_blendMaps[index];
        if( !slot )
        {
            const u32 blendSize = static_cast<u32>( getLayerBlendMapSize() );
            slot = SmartPtr<ITerrainBlendMap>(
                new TerrainBlendMapImpl( SmartPtr<IGraphicsTerrain>( this ), blendSize, index ) );
        }

        return slot;
    }

    u16 Terrain::getLayerBlendMapSize() const
    {
        // Default to 0 when no blend maps have been configured. The renderer plugin is
        // responsible for setting a non-zero size once its GPU blend maps are ready.
        if( m_blendMaps.empty() )
        {
            return 0;
        }
        const auto heightMapSize = getHeightMapSize();
        const s32 edge = Math<s32>::min( heightMapSize.x, heightMapSize.y );
        if( edge < 2 )
        {
            return 0;
        }
        const s32 size = Math<s32>::max( 16, edge / 4 );
        return static_cast<u16>(
            Math<s32>::min( size, static_cast<s32>( std::numeric_limits<u16>::max() ) ) );
    }

    SmartPtr<ITerrainRayResult> Terrain::intersects( const Ray3F &ray ) const
    {
        TerrainSnapshot snapshot;
        Transform3<real_Num> transform;
        {
            std::lock_guard<std::mutex> lock( m_terrainDataMutex );
            snapshot = getTerrainSnapshot();
            transform = m_terrainWorldTransform;
        }
        Vector3<real_Num> hit;
        const auto intersects = intersectTerrain( *snapshot, transform, ray, hit );
        return _makeRayResult( intersects, hit );
    }

    SmartPtr<ITerrainRayResult> Terrain::_makeRayResult( bool hit, const Vector3<real_Num> &pos ) const
    {
        auto result = SmartPtr<ITerrainRayResult>( new TerrainRayResult() );
        if( result )
        {
            result->setIntersected( hit );
            result->setTerrain( SmartPtr<IGraphicsTerrain>( const_cast<Terrain *>( this ) ) );
            result->setPosition( pos );
        }
        return result;
    }

    SmartPtr<IMesh> Terrain::getMesh() const
    {
        // Mesh's existing synchronization is owned by the engine mesh manager.
        // Renderer-free callers can always use buildTerrainMeshData directly.
        auto application = core::IApplicationManager::instancePtr();
        if( !application || !application->getMeshManager() )
        {
            WP_LOG_WARNING( "Terrain CPU mesh requires the engine mesh manager" );
            return nullptr;
        }
        const auto snapshot = getTerrainSnapshot();
        {
            std::lock_guard<std::mutex> lock( m_terrainDataMutex );
            if( m_cpuTerrainMesh && m_cpuTerrainMeshRevision == snapshot->revision )
                return m_cpuTerrainMesh;
        }
        TerrainMeshData geometry;
        String error;
        if( !buildTerrainMeshData( *snapshot, geometry, error ) )
        {
            WP_LOG_WARNING( error );
            return nullptr;
        }
        try
        {
            Array<Vector3<real_Num>> positions, normals;
            Array<Vector2<real_Num>> uvs;
            positions.reserve( geometry.positions.size() );
            normals.reserve( geometry.normals.size() );
            uvs.reserve( geometry.uvs.size() );
            for( const auto &p : geometry.positions )
                positions.push_back( { p.x, p.y, p.z } );
            for( const auto &n : geometry.normals )
                normals.push_back( { n.x, n.y, n.z } );
            for( const auto &uv : geometry.uvs )
                uvs.push_back( { uv.x, uv.y } );
            auto mesh = MeshUtil::createMesh( positions, normals, uvs, geometry.indices );
            if( mesh )
                mesh->updateAABB();
            std::lock_guard<std::mutex> lock( m_terrainDataMutex );
            if( getTerrainRevision() == snapshot->revision )
            {
                m_cpuTerrainMesh = mesh;
                m_cpuTerrainMeshRevision = snapshot->revision;
            }
            return mesh;
        }
        catch( const std::exception &exception )
        {
            WP_LOG_EXCEPTION( exception );
            return nullptr;
        }
    }

    void Terrain::setHeightMapSize( const Vector2I &heightMapSize )
    {
        const Vector2I dimensions( std::max( heightMapSize.x, 2 ), std::max( heightMapSize.y, 2 ) );
        if( dimensions.x > terrainMaximumDimension || dimensions.y > terrainMaximumDimension ||
            u64( dimensions.x ) * dimensions.y > terrainMaximumSamples )
        {
            WP_LOG_WARNING( "Terrain dimensions exceed the supported allocation limit" );
            return;
        }
        const auto previous = getTerrainSnapshot();
        if( previous->dimensions == dimensions )
            return;
        auto candidate = *previous;
        candidate.dimensions = dimensions;
        // Preserve the actual legacy Claw convention for existing separate size/data setters.
        candidate.origin = { -dimensions.x * candidate.spacing.x * 0.5f,
                             -dimensions.y * candidate.spacing.y * 0.5f };
        candidate.heights.assign( size_t( dimensions.x ) * dimensions.y, 0.0f );
        for( s32 z = 0; z < std::min( dimensions.y, previous->dimensions.y ); ++z )
            for( s32 x = 0; x < std::min( dimensions.x, previous->dimensions.x ); ++x )
                candidate.heights[size_t( z ) * dimensions.x + x] =
                    previous->heights[size_t( z ) * previous->dimensions.x + x];
        String error;
        if( !applyTerrainData( candidate, error, previous->revision ) )
            WP_LOG_WARNING( error );
    }

    Vector2I Terrain::getHeightMapSize() const
    {
        return getTerrainSnapshot()->dimensions;
    }

    f32 Terrain::getHeightScale() const
    {
        return getTerrainSnapshot()->heightScale;
    }

    void Terrain::setHeightScale( f32 heightScale )
    {
        const auto previous = getTerrainSnapshot();
        auto candidate = *previous;
        candidate.heightScale = heightScale;
        String error;
        if( !applyTerrainData( candidate, error, previous->revision ) )
            WP_LOG_WARNING( error );
    }

    SmartPtr<IGraphicsScene> Terrain::getSceneManager() const
    {
        auto p = m_sceneManager.load();
        return p.lock();
    }

    void Terrain::setSceneManager( SmartPtr<IGraphicsScene> sceneManager )
    {
        m_sceneManager = sceneManager;
    }

    void Terrain::_getObject( void **ppObject ) const
    {
        // Base has no native object; renderer plugins override to expose their handle.
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }

    void Terrain::setTextureLayer( s32 layer, const String &textureName )
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( !resourceDatabase )
        {
            return;
        }

        auto texture = resourceDatabase->loadResourceByType<ITexture>( textureName );
        setTexture( static_cast<u32>( layer ), texture );
    }

    SmartPtr<ITexture> Terrain::getHeightMap() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return state->heightMap;
            }
        }

        return {};
    }

    void Terrain::setHeightMap( SmartPtr<ITexture> heightMap )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                state->heightMap = heightMap;
            }
        }
    }

    void Terrain::setTexture( u32 index, SmartPtr<ITexture> texture )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                if( index >= stateData->textures.size() )
                {
                    stateData->textures.resize( index + 1 );
                }
                stateData->textures[index] = texture;
            }
        }
    }

    SmartPtr<ITexture> Terrain::getTexture( u32 index ) const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                if( index < stateData->textures.size() )
                {
                    return stateData->textures[index];
                }
            }
        }

        return nullptr;
    }

    void Terrain::setTextures( const Array<SmartPtr<ITexture>> &textures )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                stateData->textures = textures;
            }
        }
    }

    Array<SmartPtr<ITexture>> Terrain::getTextures() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return stateData->textures;
            }
        }

        return {};
    }
    SmartPtr<Properties> Terrain::getProperties() const
    {
        auto properties = SharedGraphicsObject<IGraphicsTerrain>::getProperties();
        if( !properties )
        {
            return properties;
        }

        properties->setProperty( HeightMapSizeStr, getHeightMapSize() );
        properties->setProperty( HeightScaleStr, getHeightScale() );
        properties->setProperty( ShowWireframeStr, getShowWireframe() );
        properties->setProperty( MaterialNameStr, getMaterialName() );
        properties->setProperty( VisibleStr, isVisible() );

        return properties;
    }

    void Terrain::setProperties( SmartPtr<Properties> properties )
    {
        SharedGraphicsObject<IGraphicsTerrain>::setProperties( properties );

        if( !properties )
        {
            return;
        }

        auto heightMapSize = getHeightMapSize();
        auto heightScale = getHeightScale();
        auto showWireframe = getShowWireframe();
        auto materialName = getMaterialName();
        auto visible = isVisible();

        properties->getPropertyValue( HeightMapSizeStr, heightMapSize );
        properties->getPropertyValue( HeightScaleStr, heightScale );
        properties->getPropertyValue( ShowWireframeStr, showWireframe );
        properties->getPropertyValue( MaterialNameStr, materialName );
        properties->getPropertyValue( VisibleStr, visible );

        setHeightMapSize( heightMapSize );
        setHeightScale( heightScale );
        setShowWireframe( showWireframe );
        setMaterialName( materialName );
        setVisible( visible );
    }

    bool Terrain::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( !message )
        {
            return false;
        }

        // React to load/reload messages so the terrain state can be (re)seeded from editor or
        // gameplay code. Renderer plugins may override to handle additional messages.
        if( message->isDerived<StateMessageLoad>() )
        {
            auto loadMsg = workphone::static_pointer_cast<StateMessageLoad>( message );
            auto type = loadMsg->getType();

            if( type == StateMessageLoad::LOAD_HASH || type == StateMessageLoad::RELOAD_HASH )
            {
                _ensureStateData();
                return true;
            }
        }

        return false;
    }

    bool Terrain::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !state )
        {
            return false;
        }

        auto stateData = state->getData();
        if( !stateData )
        {
            return false;
        }

        // Pull TerrainStateData changes back into the local cache so query methods stay
        // consistent without repeatedly touching the state system.
        if( stateData->isDerived<TerrainStateData>() )
        {
            const auto previous = getTerrainSnapshot();
            auto candidate = *previous;
            {
                auto terrainState = SafeReadPtr<TerrainStateData>( stateData );
                if( terrainState )
                {
                    candidate.dimensions = terrainState->heightMapSize;
                    candidate.heightScale = terrainState->heightScale;
                    candidate.heights = terrainState->heightData;
                    m_materialName = terrainState->materialName;
                    m_showWireframe = terrainState->showWireframe;
                }
            }
            if( candidate.dimensions != previous->dimensions )
                candidate.origin = { -candidate.dimensions.x * candidate.spacing.x * 0.5f,
                                     -candidate.dimensions.y * candidate.spacing.y * 0.5f };
            String error;
            if( !applyTerrainData( candidate, error, previous->revision ) )
                WP_LOG_WARNING( error );
            return true;
        }

        return false;
    }

    void Terrain::lock()
    {
        SharedGraphicsObject<IGraphicsTerrain>::lock();
    }

    void Terrain::unlock()
    {
        SharedGraphicsObject<IGraphicsTerrain>::unlock();
    }

    void Terrain::_onHeightDataChanged()
    {
        // Default: nothing. Renderer plugins override to mark their GPU mesh dirty.
    }

    void Terrain::_onHeightScaleChanged()
    {
        // Default: nothing. Renderer plugins override to mark their GPU mesh dirty.
    }

}  // namespace workphone::render
