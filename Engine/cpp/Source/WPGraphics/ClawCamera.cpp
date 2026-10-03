#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawSceneNode.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawCamera, GraphicsCamera );

        ClawCamera::ClawCamera() : m_camera( wp_camera_create() )
        {
        }

        ClawCamera::~ClawCamera()
        {
            wp_camera_destroy( m_camera );
        }

        f32 ClawCamera::getFOVy() const
        {
            return wp_camera_get_fov_y( m_camera );
        }

        void ClawCamera::setFOVy( f32 fov )
        {
            wp_camera_set_fov_y( m_camera, fov );
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
            const auto nearClip = std::max( getNearClipDistance(), MathF::epsilon() );
            const auto farClip = std::max( getFarClipDistance(), nearClip + 1.0f );
            const auto aspect = std::max( getAspectRatio(), MathF::epsilon() );
            Matrix4<real_Num> projection;
            projection.makePerspective( getFOVy(), aspect, nearClip, farClip );
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

        SmartPtr<IGraphicsObject> ClawCamera::clone( const String &name ) const
        {
            auto camera = workphone::make_ptr<ClawCamera>();
            camera->setFOVy( getFOVy() );
            camera->setNearClipDistance( getNearClipDistance() );
            camera->setFarClipDistance( getFarClipDistance() );
            camera->setAspectRatio( getAspectRatio() );
            camera->setAutoAspectRatio( getAutoAspectRatio() );
            camera->setLodBias( getLodBias() );
            camera->setRenderUI( getRenderUI() );
            camera->setScreenWidth( getScreenWidth() );
            camera->setScreenHeight( getScreenHeight() );
            camera->setVisible( isVisible() );
            camera->setCastShadows( getCastShadows() );
            camera->setVisibilityFlags( getVisibilityFlags() );
            return camera;
        }
    }  // namespace render
}  // namespace workphone
