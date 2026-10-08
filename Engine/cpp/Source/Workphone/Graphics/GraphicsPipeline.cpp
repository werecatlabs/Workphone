#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Math/Frustum3.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Exception.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::render
{
    using ScopedLock = RecursiveMutex::ScopedLock;

    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsPipeline, IGraphicsPipeline );

    // ========================================================================
    // CONSTRUCTION & DESTRUCTION
    // ========================================================================

    namespace
    {
        struct PipelineToggle
        {
            const char *name;
            bool ( IGraphicsPipeline::*get )() const;
            void ( IGraphicsPipeline::*set )( bool );
        };

        const PipelineToggle pipelineToggles[] = {
            { "Temporal anti-aliasing", &IGraphicsPipeline::isTAAEnabled,
              &IGraphicsPipeline::enableTAA },
            { "Ambient occlusion", &IGraphicsPipeline::isGTAOEnabled, &IGraphicsPipeline::enableGTAO },
            { "Screen-space reflections", &IGraphicsPipeline::isSSREnabled,
              &IGraphicsPipeline::enableSSR },
            { "Contact shadows", &IGraphicsPipeline::isContactShadowsEnabled,
              &IGraphicsPipeline::enableContactShadows },
            { "Depth of field", &IGraphicsPipeline::isDOFEnabled, &IGraphicsPipeline::enableDOF },
            { "Bloom", &IGraphicsPipeline::isBloomEnabled, &IGraphicsPipeline::enableBloom },
            { "Auto exposure", &IGraphicsPipeline::isExposureEnabled,
              &IGraphicsPipeline::enableExposure },
            { "Motion blur", &IGraphicsPipeline::isMotionBlurEnabled,
              &IGraphicsPipeline::enableMotionBlur },
            { "Cascaded shadows", &IGraphicsPipeline::isCSMEnabled, &IGraphicsPipeline::enableCSM }
        };

        template <class T>
        bool readPipelineValue( const Properties &properties, const char *name, T &value, T minimum,
                                T maximum )
        {
            T candidate = value;
            if( !properties.getPropertyValue( name, candidate ) || !std::isfinite( candidate ) )
                return false;
            candidate = std::clamp( candidate, minimum, maximum );
            if( candidate == value )
                return false;
            value = candidate;
            return true;
        }
    }  // namespace

    SmartPtr<Properties> GraphicsPipeline::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( "Applies to", "Software renderer main window", true );
        properties->setProperty( "Pipeline support", "Software main window only", true );
        properties->setProperty( "Quality presets", "Changing quality resets effect options", true );
        properties->setPropertyAsEnum( "Quality", static_cast<s32>( getQualityLevel() ),
                                       { "Low", "Medium", "High", "Ultra", "Cinematic" } );
        for( const auto &toggle : pipelineToggles )
            properties->setProperty( toggle.name, ( this->*toggle.get )() );
        properties->setProperty( "TAA feedback", getTaaSettings().m_feedback );
        properties->setProperty( "AO radius", getTaoaSettings().m_radius );
        properties->setProperty( "AO intensity", getTaoaSettings().m_intensity );
        properties->setProperty( "Bloom threshold", getHdrSettings().m_bloomThreshold );
        properties->setProperty( "Bloom intensity", getHdrSettings().m_bloomIntensity );
        properties->setProperty( "Shadow map size", getCsmSettings().m_shadowMapSize );
        return properties;
    }

    void GraphicsPipeline::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;
        s32 quality = static_cast<s32>( getQualityLevel() );
        if( readPipelineValue( *properties, "Quality", quality, 0, 4 ) )
        {
            setQualityLevel( static_cast<QualityLevel>( quality ) );
            // The inspector sends a full snapshot. Do not overwrite a new preset
            // with the old snapshot's toggles and parameters.
            return;
        }
        for( const auto &toggle : pipelineToggles )
        {
            bool enabled = ( this->*toggle.get )();
            if( properties->getPropertyValue( toggle.name, enabled ) &&
                enabled != ( this->*toggle.get )() )
                ( this->*toggle.set )( enabled );
        }
        auto taa = getTaaSettings();
        if( readPipelineValue( *properties, "TAA feedback", taa.m_feedback, 0.0f, 0.99f ) )
            setTaaSettings( taa );
        auto ao = getTaoaSettings();
        const bool radiusChanged =
            readPipelineValue( *properties, "AO radius", ao.m_radius, 0.01f, 100.0f );
        const bool intensityChanged =
            readPipelineValue( *properties, "AO intensity", ao.m_intensity, 0.0f, 10.0f );
        if( radiusChanged || intensityChanged )
            setTaoaSettings( ao );
        auto hdr = getHdrSettings();
        const bool thresholdChanged =
            readPipelineValue( *properties, "Bloom threshold", hdr.m_bloomThreshold, 0.0f, 100.0f );
        const bool bloomChanged =
            readPipelineValue( *properties, "Bloom intensity", hdr.m_bloomIntensity, 0.0f, 10.0f );
        if( thresholdChanged || bloomChanged )
            setHdrSettings( hdr );
        auto csm = getCsmSettings();
        if( readPipelineValue( *properties, "Shadow map size", csm.m_shadowMapSize, 128, 4096 ) )
            setCsmSettings( csm );
    }

    GraphicsPipeline::GraphicsPipeline()
    {
        // Set object flags for event handling
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        // Initialize with default name
        auto name = String( "GraphicsPipeline" );
        setName( name );
        setId( StringUtil::getHash( name ) );

        WP_LOG_INFO( "GraphicsPipeline: Constructor called" );
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        // Ensure shutdown is called if not already
        if( m_initialized )
        {
            shutdown();
        }

        WP_LOG_INFO( "GraphicsPipeline: Destructor called" );
    }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    bool GraphicsPipeline::initialize( s32 width, s32 height )
    {
        ScopedLock lock( m_mutex );

        if( m_initialized )
        {
            WP_LOG_WARNING( "GraphicsPipeline: Already initialized" );
            return true;
        }

        try
        {
            m_width = width;
            m_height = height;

            // Initialize output buffers
            m_outputBuffer.resize( static_cast<size_t>( width ) * height * 4, 0.0f );
            m_gBufferNormal.resize( static_cast<size_t>( width ) * height * 4, 0.0f );
            m_gBufferDepth.resize( static_cast<size_t>( width ) * height, 0.0f );
            m_gBufferVelocity.resize( static_cast<size_t>( width ) * height * 2, 0.0f );

            // Reserve space for objects and lights
            m_objects.reserve( 1024 );
            m_lights.reserve( 256 );
            m_culledObjects.reserve( 512 );
            m_culledLights.reserve( 64 );
            m_visibleNodes.reserve( 512 );

            // Apply initial quality presets
            applyQualityPresets();

            m_initialized = true;

            WP_LOG_INFO( "GraphicsPipeline: Initialized." );
            return true;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR( "GraphicsPipeline: Failed to initialize" );
            return false;
        }
    }

    void GraphicsPipeline::shutdown()
    {
        ScopedLock lock( m_mutex );

        if( !m_initialized )
        {
            return;
        }

        try
        {
            // Clear all objects and lights
            m_objects.clear();
            m_lights.clear();
            m_culledObjects.clear();
            m_culledLights.clear();
            m_visibleNodes.clear();

            // Clear buffers
            m_outputBuffer.clear();
            m_gBufferNormal.clear();
            m_gBufferDepth.clear();
            m_gBufferVelocity.clear();

            // Reset state
            m_scene = nullptr;
            m_camera = nullptr;
            m_width = 0;
            m_height = 0;
            m_initialized = false;

            WP_LOG_INFO( "GraphicsPipeline: Shutdown complete" );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR( "GraphicsPipeline: Error during shutdown" );
        }
    }

    // ========================================================================
    // SCENE
    // ========================================================================

    void GraphicsPipeline::setScene( IGraphicsScene *scene )
    {
        ScopedLock lock( m_mutex );
        m_scene = scene;
    }

    IGraphicsScene *GraphicsPipeline::getScene() const
    {
        ScopedLock lock( m_mutex );
        return m_scene;
    }

    // ========================================================================
    // FRAME
    // ========================================================================

    void GraphicsPipeline::beginFrame()
    {
        ScopedLock lock( m_mutex );

        if( !m_initialized )
        {
            return;
        }

        ++m_frameCount;

        // Update matrices from camera if available
        if( m_camera )
        {
            m_viewMatrix = m_camera->getViewMatrix();
            m_projectionMatrix = m_camera->getProjectionMatrix();
        }

        // Update view frustum
        updateViewFrustum();

        // Perform culling for this frame
        performFrustumCulling();

        // Clear output buffers
        if( !m_outputBuffer.empty() )
        {
            std::fill( m_outputBuffer.begin(), m_outputBuffer.end(), 0.0f );
        }
    }

    void GraphicsPipeline::endFrame()
    {
        ScopedLock lock( m_mutex );

        if( !m_initialized )
        {
            return;
        }

        // Clear culled lists for next frame
        m_culledObjects.clear();
        m_culledLights.clear();
        m_visibleNodes.clear();
    }

    void GraphicsPipeline::render()
    {
        ScopedLock lock( m_mutex );

        if( !m_initialized || !m_camera )
        {
            return;
        }

        // The actual rendering would be implemented by a derived class or renderer
        // This base implementation performs culling and prepares data

        // All culled objects are considered visible for the base implementation
        m_visibleNodes = m_culledObjects;
    }

    // ========================================================================
    // CAMERA & MATRICES
    // ========================================================================

    void GraphicsPipeline::setCamera( IGraphicsCamera *camera )
    {
        ScopedLock lock( m_mutex );
        m_camera = camera;

        if( camera )
        {
            m_viewMatrix = camera->getViewMatrix();
            m_projectionMatrix = camera->getProjectionMatrix();
        }
    }

    IGraphicsCamera *GraphicsPipeline::getCamera() const
    {
        ScopedLock lock( m_mutex );
        return m_camera;
    }

    void GraphicsPipeline::setViewMatrix( const Matrix4F &view )
    {
        ScopedLock lock( m_mutex );
        m_viewMatrix = view;
    }

    void GraphicsPipeline::setProjectionMatrix( const Matrix4F &projection )
    {
        ScopedLock lock( m_mutex );
        m_projectionMatrix = projection;
    }

    const Matrix4F &GraphicsPipeline::getViewMatrix() const
    {
        return m_viewMatrix;
    }

    const Matrix4F &GraphicsPipeline::getProjectionMatrix() const
    {
        return m_projectionMatrix;
    }

    // ========================================================================
    // CULLING
    // ========================================================================

    const Array<IGraphicsSceneNode *> &GraphicsPipeline::getCulledObjects() const
    {
        return m_culledObjects;
    }

    const Array<IGraphicsLight *> &GraphicsPipeline::getCulledLights() const
    {
        return m_culledLights;
    }

    const Array<IGraphicsSceneNode *> &GraphicsPipeline::getVisibleNodes() const
    {
        return m_visibleNodes;
    }

    void GraphicsPipeline::performFrustumCulling()
    {
        // Cull objects
        for( auto object : m_objects )
        {
            if( object && isInFrustum( object ) )
            {
                m_culledObjects.push_back( object );
            }
        }

        // Cull lights
        for( auto light : m_lights )
        {
            if( light && isInFrustum( light ) )
            {
                m_culledLights.push_back( light );
            }
        }
    }

    void GraphicsPipeline::updateViewFrustum()
    {
        // Update frustum from view-projection matrix
        Matrix4F viewProj = m_projectionMatrix * m_viewMatrix;
        m_frustum.setViewProjection( viewProj );
    }

    bool GraphicsPipeline::isInFrustum( IGraphicsSceneNode *object ) const
    {
        if( !object )
        {
            return false;
        }

        // Get world AABB of the object
        auto worldAABB = object->getWorldAABB();

        // Get center and half-size for frustum test
        Vector3F center = worldAABB.getCenter();
        Vector3F halfSize = worldAABB.getSize() * 0.5f;

        // Check if AABB intersects with frustum
        return m_frustum.intersects( center, halfSize );
    }

    bool GraphicsPipeline::isInFrustum( IGraphicsLight *light ) const
    {
        if( !light )
        {
            return false;
        }

        // Get light position from owner
        Vector3F lightPos;
        if( auto owner = light->getOwner() )
        {
            lightPos = owner->getWorldPosition();
        }

        // Directional lights always affect the scene (affect everything)
        if( light->getType() == LightTypes::LT_DIRECTIONAL )
        {
            return true;
        }

        // For point and spot lights, approximate with bounding sphere
        f32 range = light->getAttenuationRange();
        if( range <= 0.0f )
        {
            range = 100.0f;  // Default range for lights without explicit range
        }

        // Use a sphere intersection test - check if the light's sphere intersects the frustum
        // Since Frustum3 only has box intersection, approximate the sphere as a box
        Vector3F extent( range, range, range );
        return m_frustum.intersects( lightPos, extent );
    }

    // ========================================================================
    // RAYCASTING
    // ========================================================================

    bool GraphicsPipeline::raycastClosest( const Ray3F &ray, IGraphicsSceneNode *&outHit,
                                           Vector3F &outPoint, Vector3F &outNormal ) const
    {
        outHit = nullptr;
        outPoint = {};
        outNormal = {};

        if( !ray.isValid() )
        {
            return false;
        }

        f32 closestDistance = std::numeric_limits<f32>::max();
        bool foundHit = false;

        // Test against all objects
        for( auto object : m_objects )
        {
            if( !object )
            {
                continue;
            }

            // Get world AABB for quick rejection
            auto worldAABB = object->getWorldAABB();

            // Ray-AABB intersection using MathUtil
            auto result = MathUtil<real_Num>::intersects( ray, worldAABB );
            if( !result.first )
            {
                continue;
            }

            f32 tMin = result.second;

            // Only accept hits in front of the camera
            if( tMin > 0 && tMin < closestDistance )
            {
                closestDistance = tMin;
                outHit = object;
                outPoint = ray.getOrigin() + ray.getDirection() * tMin;

                // Approximate normal as face normal of AABB hit
                // A production implementation would compute actual triangle normal
                Vector3F center = worldAABB.getCenter();
                Vector3F diff = outPoint - center;
                if( !diff.isZeroLength() )
                {
                    outNormal = diff.normaliseCopy();
                }
                else
                {
                    outNormal = -ray.getDirection();
                }

                foundHit = true;
            }
        }

        return foundHit;
    }

    void GraphicsPipeline::raycastAll( const Ray3F &ray, Array<IGraphicsSceneNode *> &outHits ) const
    {
        outHits.clear();

        if( !ray.isValid() )
        {
            return;
        }

        // Test against all objects
        for( auto object : m_objects )
        {
            if( !object )
            {
                continue;
            }

            // Get world AABB
            auto worldAABB = object->getWorldAABB();

            // Ray-AABB intersection using MathUtil
            auto result = MathUtil<real_Num>::intersects( ray, worldAABB );
            if( result.first )
            {
                // Only include hits in front of the camera
                if( result.second > 0 )
                {
                    outHits.push_back( object );
                }
            }
        }
    }

    // ========================================================================
    // CULLING PARAMETERS
    // ========================================================================

    void GraphicsPipeline::setCullingParams( f32 nearClip, f32 farClip )
    {
        ScopedLock lock( m_mutex );

        m_nearClip = nearClip;
        m_farClip = farClip;

        // Update projection matrix if using manual settings
        if( !m_camera )
        {
            m_projectionMatrix.makePerspective( m_fov, m_aspect, m_nearClip, m_farClip );
        }
    }

    void GraphicsPipeline::setFOV( f32 fov )
    {
        ScopedLock lock( m_mutex );
        m_fov = fov;

        // Update projection matrix if using manual settings
        if( !m_camera )
        {
            m_projectionMatrix.makePerspective( m_fov, m_aspect, m_nearClip, m_farClip );
        }
    }

    void GraphicsPipeline::setAspectRatio( f32 aspect )
    {
        ScopedLock lock( m_mutex );
        m_aspect = aspect;

        // Update projection matrix if using manual settings
        if( !m_camera )
        {
            m_projectionMatrix.makePerspective( m_fov, m_aspect, m_nearClip, m_farClip );
        }
    }

    // ========================================================================
    // OBJECT MANAGEMENT
    // ========================================================================

    void GraphicsPipeline::addObject( IGraphicsSceneNode *object )
    {
        if( !object )
        {
            return;
        }

        ScopedLock lock( m_mutex );

        // Check if already added
        auto it = std::find( m_objects.begin(), m_objects.end(), object );
        if( it == m_objects.end() )
        {
            m_objects.push_back( object );
        }
    }

    void GraphicsPipeline::removeObject( IGraphicsSceneNode *object )
    {
        if( !object )
        {
            return;
        }

        ScopedLock lock( m_mutex );

        auto it = std::find( m_objects.begin(), m_objects.end(), object );
        if( it != m_objects.end() )
        {
            m_objects.erase( it );
        }

        // Also remove from culled lists
        m_culledObjects.erase( std::remove( m_culledObjects.begin(), m_culledObjects.end(), object ),
                               m_culledObjects.end() );
        m_visibleNodes.erase( std::remove( m_visibleNodes.begin(), m_visibleNodes.end(), object ),
                              m_visibleNodes.end() );
    }

    const Array<IGraphicsSceneNode *> &GraphicsPipeline::getObjects() const
    {
        return m_objects;
    }

    // ========================================================================
    // LIGHT MANAGEMENT
    // ========================================================================

    void GraphicsPipeline::addLight( IGraphicsLight *light )
    {
        if( !light )
        {
            return;
        }

        ScopedLock lock( m_mutex );

        // Check if already added
        auto it = std::find( m_lights.begin(), m_lights.end(), light );
        if( it == m_lights.end() )
        {
            m_lights.push_back( light );
        }
    }

    void GraphicsPipeline::removeLight( IGraphicsLight *light )
    {
        if( !light )
        {
            return;
        }

        ScopedLock lock( m_mutex );

        auto it = std::find( m_lights.begin(), m_lights.end(), light );
        if( it != m_lights.end() )
        {
            m_lights.erase( it );
        }

        // Also remove from culled list
        m_culledLights.erase( std::remove( m_culledLights.begin(), m_culledLights.end(), light ),
                              m_culledLights.end() );
    }

    const Array<IGraphicsLight *> &GraphicsPipeline::getLights() const
    {
        return m_lights;
    }

    // ========================================================================
    // QUALITY SETTINGS
    // ========================================================================

    void GraphicsPipeline::setQualityLevel( QualityLevel level )
    {
        ScopedLock lock( m_mutex );
        m_qualityLevel = level;
        applyQualityPresets();
    }

    QualityLevel GraphicsPipeline::getQualityLevel() const
    {
        ScopedLock lock( m_mutex );
        return m_qualityLevel;
    }

    void GraphicsPipeline::applyQualityPresets()
    {
        m_exposureEnabled = m_qualityLevel >= QualityLevel::Medium;
        switch( m_qualityLevel )
        {
        case QualityLevel::Low:
        {
            m_taaEnabled = false;
            m_gtaoEnabled = false;
            m_ssrEnabled = false;
            m_contactShadowsEnabled = false;
            m_dofEnabled = false;
            m_bloomEnabled = false;
            m_motionBlurEnabled = false;
            m_csmEnabled = false;

            m_csmSettings.m_shadowMapSize = 512;
            m_taaSettings.m_quality = 0;
            break;
        }
        case QualityLevel::Medium:
        {
            m_taaEnabled = true;
            m_gtaoEnabled = false;
            m_ssrEnabled = false;
            m_contactShadowsEnabled = true;
            m_dofEnabled = false;
            m_bloomEnabled = true;
            m_motionBlurEnabled = false;
            m_csmEnabled = true;

            m_csmSettings.m_shadowMapSize = 1024;
            m_taaSettings.m_quality = 1;
            break;
        }
        case QualityLevel::High:
        {
            m_taaEnabled = true;
            m_gtaoEnabled = true;
            m_ssrEnabled = false;
            m_contactShadowsEnabled = true;
            m_dofEnabled = false;
            m_bloomEnabled = true;
            m_motionBlurEnabled = false;
            m_csmEnabled = true;

            m_csmSettings.m_shadowMapSize = 2048;
            m_taaSettings.m_quality = 2;
            break;
        }
        case QualityLevel::Ultra:
        {
            m_taaEnabled = true;
            m_gtaoEnabled = true;
            m_ssrEnabled = true;
            m_contactShadowsEnabled = true;
            m_dofEnabled = true;
            m_bloomEnabled = true;
            m_motionBlurEnabled = false;
            m_csmEnabled = true;

            m_csmSettings.m_shadowMapSize = 2048;
            m_taaSettings.m_quality = 3;
            break;
        }
        case QualityLevel::Cinematic:
        {
            m_taaEnabled = true;
            m_gtaoEnabled = true;
            m_ssrEnabled = true;
            m_contactShadowsEnabled = true;
            m_dofEnabled = true;
            m_bloomEnabled = true;
            m_motionBlurEnabled = true;
            m_csmEnabled = true;

            m_csmSettings.m_shadowMapSize = 4096;
            m_taaSettings.m_quality = 4;
            break;
        }
        }
    }

    // ========================================================================
    // QUALITY SETTINGS SETTERS
    // ========================================================================

    void GraphicsPipeline::setTaaSettings( const TaaSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_taaSettings = settings;
    }

    void GraphicsPipeline::setTaoaSettings( const TaoaSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_taoaSettings = settings;
    }

    void GraphicsPipeline::setSsrSettings( const SsrSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_ssrSettings = settings;
    }

    void GraphicsPipeline::setContactShadowSettings( const ContactShadowSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_contactShadowSettings = settings;
    }

    void GraphicsPipeline::setDofSettings( const DofSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_dofSettings = settings;
    }

    void GraphicsPipeline::setHdrSettings( const HdrSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_hdrSettings = settings;
    }

    void GraphicsPipeline::setCsmSettings( const CsmSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_csmSettings = settings;
    }

    void GraphicsPipeline::setMotionBlurSettings( const MotionBlurSettings &settings )
    {
        ScopedLock lock( m_mutex );
        m_motionBlurSettings = settings;
    }

    // ========================================================================
    // QUALITY SETTINGS GETTERS
    // ========================================================================

    const TaaSettings &GraphicsPipeline::getTaaSettings() const
    {
        return m_taaSettings;
    }

    const TaoaSettings &GraphicsPipeline::getTaoaSettings() const
    {
        return m_taoaSettings;
    }

    const SsrSettings &GraphicsPipeline::getSsrSettings() const
    {
        return m_ssrSettings;
    }

    const ContactShadowSettings &GraphicsPipeline::getContactShadowSettings() const
    {
        return m_contactShadowSettings;
    }

    const DofSettings &GraphicsPipeline::getDofSettings() const
    {
        return m_dofSettings;
    }

    const HdrSettings &GraphicsPipeline::getHdrSettings() const
    {
        return m_hdrSettings;
    }

    const CsmSettings &GraphicsPipeline::getCsmSettings() const
    {
        return m_csmSettings;
    }

    const MotionBlurSettings &GraphicsPipeline::getMotionBlurSettings() const
    {
        return m_motionBlurSettings;
    }

    // ========================================================================
    // EFFECT TOGGLES
    // ========================================================================

    void GraphicsPipeline::enableTAA( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_taaEnabled = enabled;
    }

    void GraphicsPipeline::enableGTAO( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_gtaoEnabled = enabled;
    }

    void GraphicsPipeline::enableSSR( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_ssrEnabled = enabled;
    }

    void GraphicsPipeline::enableContactShadows( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_contactShadowsEnabled = enabled;
    }

    void GraphicsPipeline::enableDOF( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_dofEnabled = enabled;
    }

    void GraphicsPipeline::enableBloom( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_bloomEnabled = enabled;
    }

    void GraphicsPipeline::enableExposure( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_exposureEnabled = enabled;
    }

    void GraphicsPipeline::enableMotionBlur( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_motionBlurEnabled = enabled;
    }

    void GraphicsPipeline::enableCSM( bool enabled )
    {
        ScopedLock lock( m_mutex );
        m_csmEnabled = enabled;
    }

    // ========================================================================
    // EFFECT TOGGLES QUERY
    // ========================================================================

    bool GraphicsPipeline::isTAAEnabled() const
    {
        return m_taaEnabled;
    }

    bool GraphicsPipeline::isGTAOEnabled() const
    {
        return m_gtaoEnabled;
    }

    bool GraphicsPipeline::isSSREnabled() const
    {
        return m_ssrEnabled;
    }

    bool GraphicsPipeline::isContactShadowsEnabled() const
    {
        return m_contactShadowsEnabled;
    }

    bool GraphicsPipeline::isDOFEnabled() const
    {
        return m_dofEnabled;
    }

    bool GraphicsPipeline::isBloomEnabled() const
    {
        return m_bloomEnabled;
    }

    bool GraphicsPipeline::isExposureEnabled() const
    {
        return m_exposureEnabled;
    }

    bool GraphicsPipeline::isMotionBlurEnabled() const
    {
        return m_motionBlurEnabled;
    }

    bool GraphicsPipeline::isCSMEnabled() const
    {
        return m_csmEnabled;
    }

    // ========================================================================
    // OUTPUT BUFFERS
    // ========================================================================

    const f32 *GraphicsPipeline::getOutputBuffer( s32 *outWidth, s32 *outHeight ) const
    {
        if( outWidth )
        {
            *outWidth = m_width;
        }
        if( outHeight )
        {
            *outHeight = m_height;
        }
        return m_outputBuffer.empty() ? nullptr : m_outputBuffer.data();
    }

    const f32 *GraphicsPipeline::getGBufferNormal() const
    {
        return m_gBufferNormal.empty() ? nullptr : m_gBufferNormal.data();
    }

    const f32 *GraphicsPipeline::getGBufferDepth() const
    {
        return m_gBufferDepth.empty() ? nullptr : m_gBufferDepth.data();
    }

    const f32 *GraphicsPipeline::getGBufferVelocity() const
    {
        return m_gBufferVelocity.empty() ? nullptr : m_gBufferVelocity.data();
    }

    // ========================================================================
    // DEBUG
    // ========================================================================

    void GraphicsPipeline::setDebugView( DebugView view )
    {
        ScopedLock lock( m_mutex );
        m_debugView = view;
    }

    IGraphicsPipeline::DebugView GraphicsPipeline::getDebugView() const
    {
        ScopedLock lock( m_mutex );
        return m_debugView;
    }

    // ========================================================================
    // STATE
    // ========================================================================

    bool GraphicsPipeline::isInitialized() const
    {
        ScopedLock lock( m_mutex );
        return m_initialized;
    }

    s32 GraphicsPipeline::getWidth() const
    {
        ScopedLock lock( m_mutex );
        return m_width;
    }

    s32 GraphicsPipeline::getHeight() const
    {
        ScopedLock lock( m_mutex );
        return m_height;
    }

    // ========================================================================
    // PRIVATE HELPERS
    // ========================================================================

    IGraphicsSceneNode *GraphicsPipeline::validateObject( IGraphicsSceneNode *object )
    {
        return object;
    }

    IGraphicsLight *GraphicsPipeline::validateLight( IGraphicsLight *light )
    {
        return light;
    }

}  // namespace workphone::render
