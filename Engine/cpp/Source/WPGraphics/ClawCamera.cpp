#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawSceneNode.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/State/States/CameraStateData.hpp>
#include <boost/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace workphone
{
    namespace render
    {
        namespace
        {
            auto resolvePostProcessWorkspace( const IGraphicsCamera::PostProcessSettings &settings )
                -> IGraphicsCamera::PostProcessSettings
            {
                if( !settings.enabled || settings.workspace.empty() )
                    return settings;

                try
                {
                    auto app = core::IApplicationManager::instancePtr();
                    const auto projectPath = app ? app->getProjectPath() : String();
                    const auto relative = std::filesystem::u8path( settings.workspace.c_str() );
                    if( projectPath.empty() || relative.has_root_path() || relative.extension() != ".json" )
                        throw std::runtime_error( "Expected a project-relative JSON preset path." );

                    const auto root = std::filesystem::canonical(
                        std::filesystem::u8path( projectPath.c_str() ) );
                    const auto file = std::filesystem::canonical( root / relative );
                    const auto inProject = file.lexically_relative( root );
                    if( inProject.empty() || *inProject.begin() == ".." ||
                        !std::filesystem::is_regular_file( file ) )
                        throw std::runtime_error( "The preset must be a file in the project." );

                    const auto size = std::filesystem::file_size( file );
                    if( size > 64 * 1024 )
                        throw std::runtime_error( "The preset exceeds 64 KiB." );

                    std::string source( static_cast<size_t>( size ), '\0' );
                    std::ifstream input( file, std::ios::binary );
                    if( !input.read( source.data(), static_cast<std::streamsize>( size ) ) )
                        throw std::runtime_error( "Could not read the preset." );
                    const auto document = boost::json::parse( source );
                    const auto &values = document.as_object();
                    auto resolved = settings;
                    const auto readNumber = [&]( const char *key, f32 &destination ) {
                        if( const auto *value = values.if_contains( key ) )
                            destination = boost::json::value_to<f32>( *value );
                    };
                    if( const auto *value = values.if_contains( "fxaa" ) )
                        resolved.fxaa = value->as_bool();
                    if( const auto *value = values.if_contains( "bloom" ) )
                        resolved.bloom = value->as_bool();
                    readNumber( "exposure", resolved.exposure );
                    readNumber( "gamma", resolved.gamma );
                    readNumber( "contrast", resolved.contrast );
                    readNumber( "saturation", resolved.saturation );
                    readNumber( "bloomIntensity", resolved.bloomIntensity );
                    readNumber( "bloomThreshold", resolved.bloomThreshold );
                    readNumber( "vignette", resolved.vignette );
                    return resolved;
                }
                catch( const std::exception &error )
                {
                    WP_LOG_ERROR( String( "Camera post-processing preset " ) + settings.workspace +
                                  ": " + error.what() );
                    return settings;
                }
            }
        }

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawCamera, GraphicsCamera );

        ClawCamera::ClawCamera() : m_camera( wp_camera_create() )
        {
        }

        ClawCamera::~ClawCamera()
        {
            wp_camera_destroy( m_camera );
        }

        void ClawCamera::unload( SmartPtr<ISharedObject> data )
        {
            // Generic graphics unload clears the weak owner. Detach the native
            // camera first so a retained RTT camera cannot read a freed node
            // after its scene node is unloaded on the render task.
            wp_camera_attach_to_node( m_camera, nullptr );
            GraphicsCamera::unload( data );
        }

        f32 ClawCamera::getFOVy() const
        {
            return MathF::RadToDeg( wp_camera_get_fov_y( m_camera ) );
        }

        void ClawCamera::setFOVy( f32 fov )
        {
            // The public graphics camera API uses degrees; the C renderer uses radians.
            wp_camera_set_fov_y( m_camera, MathF::DegToRad( fov ) );
        }

        void ClawCamera::setOrthoWindow( f32 width, f32 height )
        {
            const auto halfWidth = std::isfinite( width ) && width > 0.0f ? width * 0.5f : 0.0f;
            const auto halfHeight = std::isfinite( height ) && height > 0.0f ? height * 0.5f : 0.0f;
            wp_camera_set_ortho_width( m_camera, halfWidth );
            wp_camera_set_ortho_height( m_camera, halfHeight );
            wp_camera_set_projection_type( m_camera, halfWidth > 0.0f || halfHeight > 0.0f ?
                    WORKPHONE_PROJECTION_ORTHOGRAPHIC : WORKPHONE_PROJECTION_PERSPECTIVE );
        }

        f32 ClawCamera::getNearClipDistance() const
        {
            return wp_camera_get_near_clip_distance( m_camera );
        }

        void ClawCamera::setNearClipDistance( f32 nearDist )
        {
            wp_camera_set_near_clip_distance( m_camera, nearDist );
        }

        f32 ClawCamera::getFarClipDistance() const
        {
            return wp_camera_get_far_clip_distance( m_camera );
        }

        void ClawCamera::setFarClipDistance( f32 farDist )
        {
            wp_camera_set_far_clip_distance( m_camera, farDist );
        }

        f32 ClawCamera::getAspectRatio() const
        {
            return wp_camera_get_aspect_ratio( m_camera );
        }

        void ClawCamera::setAspectRatio( f32 ratio )
        {
            if( ratio > MathF::epsilon() )
            {
                wp_camera_set_aspect_ratio( m_camera, ratio );
            }
        }

        Matrix4<real_Num> ClawCamera::getViewMatrix() const
        {
            // Editor transforms are written directly to the native Claw scene node.
            // Use its current world transform rather than the C++ node's cached state.
            wp_mat4f nativeView;
            wp_camera_get_view_matrix( m_camera, &nativeView );
            Matrix4<real_Num> view;
            for( u32 row = 0; row < 4; ++row )
            {
                for( u32 column = 0; column < 4; ++column )
                {
                    view[row][column] = nativeView.m[row][column];
                }
            }
            return view;
        }

        Matrix4<real_Num> ClawCamera::getProjectionMatrix() const
        {
            // Match the native matrix used by scene culling.
            wp_mat4f nativeProjection;
            wp_camera_get_projection_matrix( m_camera, &nativeProjection );
            Matrix4<real_Num> projection;
            for( u32 row = 0; row < 4; ++row )
            {
                for( u32 column = 0; column < 4; ++column )
                {
                    projection[row][column] = nativeProjection.m[row][column];
                }
            }
            return projection;
        }

        void ClawCamera::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            GraphicsCamera::attachToParent( parent );
            auto clawNode = dynamic_pointer_cast<ClawSceneNode>( parent );
            wp_camera_attach_to_node( m_camera, clawNode ? clawNode->getNativeNode() : nullptr );
        }

        void ClawCamera::detachFromParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            wp_camera_attach_to_node( m_camera, nullptr );
            GraphicsCamera::detachFromParent( parent );
        }

        wp_camera *ClawCamera::getNativeCamera() const
        {
            return m_camera;
        }

        void ClawCamera::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = m_camera;
            }
        }

        void ClawCamera::setPostProcessSettings( const PostProcessSettings &settings )
        {
            const auto resolved = resolvePostProcessWorkspace( settings );
            if( auto context = getStateContextPtr() )
            {
                if( auto state = context->invalidateStateDataById<CameraStateData>( getId() ) )
                {
                    state->postProcessSettings = settings;
                    state->renderPostProcessSettings = resolved;
                }
            }
        }

        auto ClawCamera::getRenderPostProcessSettings() const -> PostProcessSettings
        {
            if( auto context = getStateContextPtr() )
            {
                if( auto state = context->getStateDataById<CameraStateData>( getId() ) )
                    return state->renderPostProcessSettings;
            }
            return {};
        }

        SmartPtr<IGraphicsObject> ClawCamera::clone( const String &name ) const
        {
            auto creator = getCreator();
            if( !creator )
            {
                return nullptr;
            }
            SmartPtr<IGraphicsCamera> camera;
            try
            {
                // Reuse the creator's existing camera/context lifecycle. A raw
                // constructor has no inherited visibility/camera state records.
                camera = creator->addGraphicsObjectByType<IGraphicsCamera>();
                if( !camera )
                {
                    return nullptr;
                }
                if( !name.empty() )
                {
                    camera->setName( name );
                }
                camera->setFOVy( getFOVy() );
                if( wp_camera_get_projection_type( m_camera ) == WORKPHONE_PROJECTION_ORTHOGRAPHIC )
                {
                    camera->setOrthoWindow( 2.0f * wp_camera_get_ortho_width( m_camera ),
                                            2.0f * wp_camera_get_ortho_height( m_camera ) );
                }
                camera->setNearClipDistance( getNearClipDistance() );
                camera->setFarClipDistance( getFarClipDistance() );
                camera->setAspectRatio( getAspectRatio() );
                camera->setAutoAspectRatio( getAutoAspectRatio() );
                camera->setLodBias( getLodBias() );
                camera->setRenderUI( getRenderUI() );
                camera->setScreenWidth( getScreenWidth() );
                camera->setScreenHeight( getScreenHeight() );
                camera->setTargetTexture( getTargetTexture() );
                camera->setPostProcessSettings( getPostProcessSettings() );
                camera->setCompositeLayers( getCompositeLayers() );
                camera->setVisible( isVisible() );
                camera->setCastShadows( getCastShadows() );
                camera->setVisibilityFlags( getVisibilityFlags() );
                return camera;
            }
            catch( ... )
            {
                if( camera )
                {
                    try
                    {
                        camera->unload( nullptr );
                    }
                    catch( ... )
                    {
                    }
                    try
                    {
                        creator->removeGraphicsObject( camera );
                    }
                    catch( ... )
                    {
                    }
                }
                return nullptr;
            }
        }
    }  // namespace render
}  // namespace workphone
