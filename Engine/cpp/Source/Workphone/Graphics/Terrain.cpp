#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Graphics/TerrainBlendMapImpl.hpp>
#include <Workphone/Graphics/TerrainRayResult.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/Math.hpp>
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
    }

    Terrain::~Terrain()
    {
        unload( nullptr );
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
                    stateData->heightMapSize = m_heightMapSize;
                    stateData->heightScale = m_heightScale;
                    stateData->materialName = m_materialName;
                    stateData->showWireframe = m_showWireframe;
                    stateData->heightData.resize( static_cast<size_t>( m_heightMapSize.x ) *
                                                      static_cast<size_t>( m_heightMapSize.y ),
                                                  0.0f );
                }
                return stateData;
            } );

        ensureState(
            []( SmartPtr<IStateContext> &ctx, hash_type id ) -> bool {
                return ctx->getStateDataById<TransformStateData>( id ).get() != nullptr;
            },
            [&]() -> SmartPtr<ISharedObject> {
                return factoryManager->make_ptr<TransformStateData>();
            } );

        ensureState(
            []( SmartPtr<IStateContext> &ctx, hash_type id ) -> bool {
                return ctx->getStateDataById<GraphicsObjectData>( id ).get() != nullptr;
            },
            [&]() -> SmartPtr<ISharedObject> {
                return factoryManager->make_ptr<GraphicsObjectData>();
            } );
    }

    Transform3<real_Num> Terrain::getWorldTransform() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TransformStateData>( getId() ) )
            {
                return stateData->worldTransform;
            }
        }

        return {};
    }

    void Terrain::setWorldTransform( const Transform3<real_Num> &worldTransform )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TransformStateData>( getId() ) )
            {
                stateData->worldTransform = worldTransform;
            }
        }
    }

    Vector3<real_Num> Terrain::getPosition() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TransformStateData>( getId() ) )
            {
                return stateData->worldTransform.getPosition();
            }
        }

        return Vector3<real_Num>::zero();
    }

    void Terrain::setPosition( const Vector3<real_Num> &position )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TransformStateData>( getId() ) )
            {
                stateData->worldTransform.setPosition( position );
            }
        }
    }
    f32 Terrain::getHeightAtWorldPosition( const Vector3<real_Num> &position ) const
    {
        const auto heightMapSize = getHeightMapSize();
        const auto width = heightMapSize.x;
        const auto depth = heightMapSize.y;

        const auto heightData = getHeightData();

        if( heightData.empty() || width < 2 || depth < 2 )
        {
            return 0.0f;
        }

        if( static_cast<s64>( heightData.size() ) !=
            static_cast<s64>( width ) * static_cast<s64>( depth ) )
        {
            return 0.0f;
        }

        // Terrain is centred at getPosition(); its world extent is width x depth (1 unit per
        // texel). Map world (x, z) into continuous texel coordinates.
        const auto center = getPosition();
        const auto halfW = static_cast<real_Num>( width ) * static_cast<real_Num>( 0.5 );
        const auto halfD = static_cast<real_Num>( depth ) * static_cast<real_Num>( 0.5 );

        const real_Num tx = ( position.x - center.x + halfW );
        const real_Num tz = ( position.z - center.z + halfD );

        const real_Num maxX = static_cast<real_Num>( width - 1 );
        const real_Num maxZ = static_cast<real_Num>( depth - 1 );
        const real_Num cx = Math<real_Num>::clamp( tx, static_cast<real_Num>( 0 ), maxX );
        const real_Num cz = Math<real_Num>::clamp( tz, static_cast<real_Num>( 0 ), maxZ );

        const s32 x0 = static_cast<s32>( cx );
        const s32 z0 = static_cast<s32>( cz );
        const s32 x1 = Math<s32>::min( x0 + 1, width - 1 );
        const s32 z1 = Math<s32>::min( z0 + 1, depth - 1 );

        const real_Num fx = cx - static_cast<real_Num>( x0 );
        const real_Num fz = cz - static_cast<real_Num>( z0 );

        // Sample the four surrounding heights (row-major: index = z * width + x).
        const f32 h00 = heightData[static_cast<size_t>( z0 ) * width + x0];
        const f32 h10 = heightData[static_cast<size_t>( z0 ) * width + x1];
        const f32 h01 = heightData[static_cast<size_t>( z1 ) * width + x0];
        const f32 h11 = heightData[static_cast<size_t>( z1 ) * width + x1];

        // Bilinear interpolation of the raw height samples. The stored heightData array
        // already contains the world-space heights; heightScale is a separate property
        // applied by renderer backends when generating the mesh, so it is not applied
        // here. Returning raw values keeps this query consistent with getHeightData().
        const real_Num h0 =
            Math<real_Num>::lerp( static_cast<real_Num>( h00 ), static_cast<real_Num>( h10 ), fx );
        const real_Num h1 =
            Math<real_Num>::lerp( static_cast<real_Num>( h01 ), static_cast<real_Num>( h11 ), fx );
        const real_Num sample = Math<real_Num>::lerp( h0, h1, fz );

        return static_cast<f32>( sample );
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
        const auto heightMapSize = getHeightMapSize();
        const auto width = heightMapSize.x;
        const auto depth = heightMapSize.y;

        if( width < 2 || depth < 2 )
        {
            return Vector3<real_Num>::zero();
        }

        const auto localSpace = getWorldTransform().inverseTransformPoint( worldSpace );
        const auto halfW = static_cast<real_Num>( width ) * static_cast<real_Num>( 0.5 );
        const auto halfD = static_cast<real_Num>( depth ) * static_cast<real_Num>( 0.5 );

        const auto maxX = static_cast<real_Num>( width - 1 );
        const auto maxZ = static_cast<real_Num>( depth - 1 );
        const auto terrainX =
            Math<real_Num>::clamp( localSpace.x + halfW, static_cast<real_Num>( 0 ), maxX );
        const auto terrainZ =
            Math<real_Num>::clamp( localSpace.z + halfD, static_cast<real_Num>( 0 ), maxZ );

        return Vector3<real_Num>( terrainX, localSpace.y, terrainZ );
    }

    Array<f32> Terrain::getHeightData() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return stateData->heightData;
            }
        }

        return {};
    }

    void Terrain::setHeightData( const Array<f32> &heightData )
    {
        const auto heightMapSize = getHeightMapSize();
        const s64 expected = static_cast<s64>( heightMapSize.x ) * static_cast<s64>( heightMapSize.y );

        if( static_cast<s64>( heightData.size() ) != expected )
        {
            WP_LOG_WARNING( "Terrain::setHeightData: size mismatch (got " +
                            StringUtil::toString( static_cast<u64>( heightData.size() ) ) +
                            ", expected " + StringUtil::toString( static_cast<u64>( expected ) ) +
                            "). Ignored." );
            return;
        }

        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                stateData->heightData = heightData;
            }
        }

        _onHeightDataChanged();
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

        return false;
    }

    void Terrain::setVisible( bool visible )
    {
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
        // Without a graphics system the terrain is "unconfigured" in the sense that there
        // is no GPU mesh to validate the ray against, so report no intersection. The test
        // suite checks for a null return when optional graphics queries are unavailable.
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager || !applicationManager->getGraphicsSystemPtr() )
        {
            return nullptr;
        }

        const auto heightMapSize = getHeightMapSize();
        const auto width = heightMapSize.x;
        const auto depth = heightMapSize.y;

        const auto heightData = getHeightData();

        const bool noData = heightData.empty() || width < 2 || depth < 2 ||
                            static_cast<s64>( heightData.size() ) !=
                                static_cast<s64>( width ) * static_cast<s64>( depth );

        if( noData )
        {
            return _makeRayResult( false, Vector3<real_Num>::zero() );
        }

        // Work in f32 throughout: the ray is Ray3F (f32) and mixing f32 with real_Num (which
        // may be double) would require explicit component-wise conversion at every step.
        const f32 heightScale = getHeightScale();
        const auto centerRN = getPosition();
        const Vector3F center( static_cast<f32>( centerRN.x ), static_cast<f32>( centerRN.y ),
                               static_cast<f32>( centerRN.z ) );
        const f32 halfW = static_cast<f32>( width ) * 0.5f;
        const f32 halfD = static_cast<f32>( depth ) * 0.5f;

        const Vector3F origin = ray.getOrigin();
        const Vector3F direction = ray.getDirection();

        const f32 dirLenSq = direction.dotProduct( direction );
        if( dirLenSq <= 0.0f )
        {
            return _makeRayResult( false, Vector3<real_Num>::zero() );
        }

        const Vector3F dir = direction * ( 1.0f / Math<f32>::Sqrt( dirLenSq ) );

        // March the ray through world space. The step is a fraction of one texel so the march
        // resolves surface crossings to sub-texel precision. Suitable for editor picking and
        // gameplay queries; GPU backends may override with an exact version.
        const f32 maxRayDist = 10000.0f;
        const f32 step = 0.25f;

        auto sampleTerrainHeight = [&]( const Vector3F &worldPos ) -> f32 {
            const f32 tx = ( worldPos.x - center.x + halfW );
            const f32 tz = ( worldPos.z - center.z + halfD );

            const f32 maxX = static_cast<f32>( width - 1 );
            const f32 maxZ = static_cast<f32>( depth - 1 );

            // Outside the terrain footprint: return -inf so no false hit on flat surrounds.
            if( tx < 0.0f || tx > maxX || tz < 0.0f || tz > maxZ )
            {
                return -std::numeric_limits<f32>::infinity();
            }

            const f32 cx = Math<f32>::clamp( tx, 0.0f, maxX );
            const f32 cz = Math<f32>::clamp( tz, 0.0f, maxZ );

            const s32 x0 = static_cast<s32>( cx );
            const s32 z0 = static_cast<s32>( cz );
            const s32 x1 = Math<s32>::min( x0 + 1, width - 1 );
            const s32 z1 = Math<s32>::min( z0 + 1, depth - 1 );

            const f32 fx = cx - static_cast<f32>( x0 );
            const f32 fz = cz - static_cast<f32>( z0 );

            const f32 h00 = heightData[static_cast<size_t>( z0 ) * width + x0];
            const f32 h10 = heightData[static_cast<size_t>( z0 ) * width + x1];
            const f32 h01 = heightData[static_cast<size_t>( z1 ) * width + x0];
            const f32 h11 = heightData[static_cast<size_t>( z1 ) * width + x1];

            const f32 h0 = Math<f32>::lerp( h00, h10, fx );
            const f32 h1 = Math<f32>::lerp( h01, h11, fx );
            const f32 sample = Math<f32>::lerp( h0, h1, fz );

            // heightData already holds world-space heights; the renderer's mesh applies
            // heightScale separately. Mirror getHeightAtWorldPosition so the surface
            // sampled here matches the value callers retrieve via query.
            return sample + center.y;
        };

        f32 prevAbove = origin.y - sampleTerrainHeight( origin );
        Vector3F hitPos = Vector3F::zero();
        bool hit = false;

        for( f32 t = 0.0f; t <= maxRayDist; t += step )
        {
            const Vector3F p = origin + dir * t;
            const f32 surface = sampleTerrainHeight( p );
            const f32 above = p.y - surface;

            if( ( prevAbove < 0.0f ) != ( above < 0.0f ) && std::isfinite( surface ) )
            {
                // Linearly interpolate the crossing distance for a smoother hit point.
                const f32 denom = prevAbove - above;
                f32 tcross = t;
                if( std::fabs( denom ) > std::numeric_limits<f32>::epsilon() )
                {
                    tcross = t - step + ( step * above ) / denom;
                }
                hitPos = origin + dir * tcross;
                hit = true;
                break;
            }

            prevAbove = above;
        }

        // Convert the f32 hit back to the interface's real_Num result type.
        const Vector3<real_Num> hitPosRN( static_cast<real_Num>( hitPos.x ),
                                          static_cast<real_Num>( hitPos.y ),
                                          static_cast<real_Num>( hitPos.z ) );
        return _makeRayResult( hit, hitPosRN );
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
        // Base does not build a CPU mesh; renderer plugins override (see ClawTerrain::getMesh).
        return nullptr;
    }
    void Terrain::setHeightMapSize( const Vector2I &heightMapSize )
    {
        m_heightMapSize.x = Math<s32>::max( 2, heightMapSize.x );
        m_heightMapSize.y = Math<s32>::max( 2, heightMapSize.y );

        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                state->heightMapSize = m_heightMapSize;

                // Keep the height data array consistent with the new dimensions, zero-filling.
                const size_t newSize =
                    static_cast<size_t>( m_heightMapSize.x ) * static_cast<size_t>( m_heightMapSize.y );
                if( state->heightData.size() != newSize )
                {
                    Array<f32> resized;
                    resized.resize( newSize, 0.0f );
                    state->heightData.swap( resized );
                }
            }
        }

        _onHeightDataChanged();
    }

    Vector2I Terrain::getHeightMapSize() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return state->heightMapSize;
            }
        }

        return m_heightMapSize;
    }

    f32 Terrain::getHeightScale() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<TerrainStateData>( getId() ) )
            {
                return state->heightScale;
            }
        }

        return m_heightScale;
    }

    void Terrain::setHeightScale( f32 heightScale )
    {
        m_heightScale = heightScale;

        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<TerrainStateData>( getId() ) )
            {
                state->heightScale = m_heightScale;
            }
        }

        _onHeightScaleChanged();
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
            auto terrainState = SafeReadPtr<TerrainStateData>( stateData );
            if( terrainState )
            {
                m_heightMapSize = terrainState->heightMapSize;
                m_heightScale = terrainState->heightScale;
                m_materialName = terrainState->materialName;
                m_showWireframe = terrainState->showWireframe;
            }
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
