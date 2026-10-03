#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/States/CameraStateData.hpp>
#include <Workphone/State/States/FrustumStateData.hpp>
#include <Workphone/State/States/State.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsCamera, GraphicsObject<IGraphicsCamera> );

    GraphicsCamera::GraphicsCamera() = default;

    GraphicsCamera::~GraphicsCamera() = default;

    void GraphicsCamera::load( SmartPtr<ISharedObject> data )
    {
        GraphicsObject<IGraphicsCamera>::load( data );
    }

    void GraphicsCamera::unload( SmartPtr<ISharedObject> data )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId(), false ) )
            {
                state->targetTexture = nullptr;
                state->viewport = nullptr;
            }
        }

        GraphicsObject<IGraphicsCamera>::unload( data );
    }

    Vector3<real_Num> GraphicsCamera::getDirection() const
    {
        if( auto owner = getOwner() )
        {
            return owner->getOrientation() * -Vector3<real_Num>::unitZ();
        }

        return {};
    }

    Vector3<real_Num> GraphicsCamera::getUp() const
    {
        if( auto owner = getOwner() )
        {
            return owner->getOrientation() * Vector3<real_Num>::unitY();
        }

        return {};
    }

    Vector3<real_Num> GraphicsCamera::getRight() const
    {
        if( auto owner = getOwner() )
        {
            return owner->getOrientation() * Vector3<real_Num>::unitX();
        }

        return {};
    }

    void GraphicsCamera::setLodBias( f32 factor /*= 1.0 */ )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->lodBias = factor;
            }
        }
    }

    f32 GraphicsCamera::getLodBias() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<CameraStateData>( getId() ) )
            {
                return state->lodBias;
            }
        }

        // Renderers without camera state still need the neutral LOD multiplier.
        return 1.0f;
    }

    Ray3<real_Num> GraphicsCamera::getRay( f32 screenx, f32 screeny ) const
    {
        auto viewMatrix = getViewMatrix();
        auto projectionMatrix = getProjectionMatrix();
        auto screenWidth = getScreenWidth();
        auto screenHeight = getScreenHeight();

        // Step 1: Convert screen coordinates to normalized device coordinates (NDC)
        real_Num ndcX = ( 2.0f * screenx ) / screenWidth - 1.0f;
        real_Num ndcY = 1.0f - ( 2.0f * screeny ) / screenHeight;  // Flip Y

        // Step 2: Convert NDC to clip space
        Vector4<real_Num> clipSpacePos( ndcX, ndcY, -1.0f, 1.0f );

        // Step 3: Convert from clip space to view space (inverse projection)
        Matrix4<real_Num> invProj = projectionMatrix.inverse();
        Vector4<real_Num> viewSpacePos = invProj * clipSpacePos;
        viewSpacePos /= viewSpacePos.w;  // Perspective divide

        // Step 4: Convert from view space to world space (inverse view)
        Matrix4<real_Num> invView = viewMatrix.inverse();
        Vector4<real_Num> worldSpacePos =
            invView * Vector4<real_Num>( viewSpacePos.x, viewSpacePos.y, viewSpacePos.z, 0.0f );

        // Step 5: Normalize direction and create the ray
        Vector3<real_Num> rayOrigin = invView.getTranslation();  // Camera position

        auto worldSpacePos3 = Vector3<real_Num>( worldSpacePos.x, worldSpacePos.y, worldSpacePos.z );
        Vector3<real_Num> rayDirection = worldSpacePos3.normaliseCopy();  // Normalize direction

        return Ray3<real_Num>( rayOrigin, rayDirection );
    }

    Vector2<real_Num> GraphicsCamera::getScreenPosition( const Vector3<real_Num> &position )
    {
        auto viewMatrix = getViewMatrix();
        auto projectionMatrix = getProjectionMatrix();
        auto screenWidth = getScreenWidth();
        auto screenHeight = getScreenHeight();

        // Convert world position to clip space: clipPos = projection * view * worldPos
        Vector4<real_Num> clipPos =
            projectionMatrix *
            ( viewMatrix * Vector4<real_Num>( position.X(), position.Y(), position.Z(), 1.0 ) );

        // Perform perspective division to convert from homogeneous coordinates to NDC
        if( clipPos.w == 0 )
            return {};  // Avoid division by zero

        Vector3<real_Num> ndc = Vector3<real_Num>( clipPos.x, clipPos.y, clipPos.z ) / clipPos.w;

        // Convert from NDC (-1 to 1) to screen coordinates
        Vector2<real_Num> screenPos;
        screenPos.x = ( ndc.x * 0.5f + 0.5f ) * screenWidth;
        screenPos.y = ( 1.0f - ( ndc.y * 0.5f + 0.5f ) ) *
                      screenHeight;  // Flip Y since screen space is top-left origin

        return screenPos;
    }

    void GraphicsCamera::setWindow( f32 left, f32 top, f32 right, f32 bottom )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->windowDimensions = { left, top, right, bottom };
            }
        }
    }

    f32 GraphicsCamera::getFOVy() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->fovy;
            }
        }

        return 0.0f;
    }

    void GraphicsCamera::setFOVy( f32 fov )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<FrustumStateData>( getId() ) )
            {
                state->fovy = fov;
            }
        }
    }

    void GraphicsCamera::setAutoAspectRatio( bool autoratio )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->flags =
                    BitUtil::setFlagValue( state->flags, CameraFlagAutoAspectRatio, autoratio );
            }
        }
    }

    bool GraphicsCamera::getAutoAspectRatio() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<CameraStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, CameraFlagAutoAspectRatio );
            }
        }

        return false;
    }

    void GraphicsCamera::setNearClipDistance( f32 nearDist )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<FrustumStateData>( getId() ) )
            {
                state->nearClipDistance = nearDist;
            }
        }
    }

    f32 GraphicsCamera::getNearClipDistance() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->nearClipDistance;
            }
        }

        return 0.0f;
    }

    void GraphicsCamera::setFarClipDistance( f32 farDist )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<FrustumStateData>( getId() ) )
            {
                state->farClipDistance = farDist;
            }
        }
    }

    f32 GraphicsCamera::getFarClipDistance() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->farClipDistance;
            }
        }

        return 0.0f;
    }

    void GraphicsCamera::setAspectRatio( f32 ratio )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<FrustumStateData>( getId() ) )
            {
                state->aspectRatio = ratio;
            }
        }
    }

    f32 GraphicsCamera::getAspectRatio() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->aspectRatio;
            }
        }

        return 0.0f;
    }

    SmartPtr<IViewport> GraphicsCamera::getViewport() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<CameraStateData>( getId() ) )
            {
                return state->viewport.lock();
            }
        }

        return nullptr;
    }

    void GraphicsCamera::setViewport( SmartPtr<IViewport> viewport )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->viewport = viewport;
            }
        }
    }

    Matrix4<real_Num> GraphicsCamera::getViewMatrix() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->viewMatrix;
            }
        }

        return {};
    }

    Matrix4<real_Num> GraphicsCamera::getProjectionMatrix() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->projectionMatrix;
            }
        }

        return {};
    }

    void *GraphicsCamera::getRenderViewMatrix() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->viewMatrix.ptr();
            }
        }

        return nullptr;
    }

    void *GraphicsCamera::getRenderProjectionMatrix() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->projectionMatrix.ptr();
            }
        }

        return nullptr;
    }

    SmartPtr<ITexture> GraphicsCamera::getTargetTexture() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<CameraStateData>( getId() ) )
            {
                return state->targetTexture;
            }
        }

        return nullptr;
    }

    void GraphicsCamera::setTargetTexture( SmartPtr<ITexture> targetTexture )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->targetTexture = targetTexture;
            }
        }
    }

    auto GraphicsCamera::getPostProcessSettings() const -> PostProcessSettings
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<CameraStateData>( getId() ) )
            {
                return state->postProcessSettings;
            }
        }

        return {};
    }

    void GraphicsCamera::setPostProcessSettings( const PostProcessSettings &settings )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->postProcessSettings = settings;
            }
        }
    }

    auto GraphicsCamera::getCompositeLayers() const -> Array<CompositeLayer>
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<CameraStateData>( getId() ) )
            {
                return state->compositeLayers;
            }
        }

        return {};
    }

    void GraphicsCamera::setCompositeLayers( const Array<CompositeLayer> &layers )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->compositeLayers = layers;
            }
        }
    }

    void GraphicsCamera::setRenderUI( bool enabled )
    {
        m_renderUI = enabled;
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<CameraStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, CameraFlagRenderUI, enabled );
            }
        }
    }

    bool GraphicsCamera::getRenderUI() const
    {
        return m_renderUI;
    }

    bool GraphicsCamera::isObjectVisible( const AABB3<real_Num> &bound ) const
    {
        auto viewMatrix = getViewMatrix();
        auto projectionMatrix = getProjectionMatrix();

        // Compute the view-projection matrix
        Matrix4<real_Num> viewProj = projectionMatrix * viewMatrix;

        // Extract frustum planes from the view-projection matrix
        Vector4<real_Num> planes[6];
        planes[0] = viewProj.getRow( 3 ) + viewProj.getRow( 0 );  // Left Plane
        planes[1] = viewProj.getRow( 3 ) - viewProj.getRow( 0 );  // Right Plane
        planes[2] = viewProj.getRow( 3 ) + viewProj.getRow( 1 );  // Bottom Plane
        planes[3] = viewProj.getRow( 3 ) - viewProj.getRow( 1 );  // Top Plane
        planes[4] = viewProj.getRow( 3 ) + viewProj.getRow( 2 );  // Near Plane
        planes[5] = viewProj.getRow( 3 ) - viewProj.getRow( 2 );  // Far Plane

        // Normalize the plane equations
        for( int i = 0; i < 6; i++ )
        {
            planes[i] /= planes[i].xyz().length();
        }

        // Get AABB corners
        Vector3<real_Num> corners[8];
        bound.getEdges( corners );  // Assumes AABB3F has a function to compute its 8 corners

        // Check if any corner is inside the frustum
        for( int i = 0; i < 8; i++ )
        {
            bool inside = true;
            for( int j = 0; j < 6; j++ )
            {
                real_Num distance = planes[j].xyz().dotProduct( corners[i] ) + planes[j].w;
                if( distance < 0 )
                {
                    inside = false;
                    break;
                }
            }

            if( inside )
            {
                return true;  // At least one corner is inside the frustum
            }
        }

        return false;  // No corners are inside the frustum
    }

    bool GraphicsCamera::isObjectVisible( const Vector3<real_Num> &vert ) const
    {
        auto viewMatrix = getViewMatrix();
        auto projectionMatrix = getProjectionMatrix();

        // Compute the view-projection matrix
        Matrix4<real_Num> viewProj = projectionMatrix * viewMatrix;

        // Transform the vertex into clip space
        Vector4<real_Num> clipPos = viewProj * Vector4<real_Num>( vert, 1.0 );

        // Perform perspective divide to get normalized device coordinates (NDC)
        if( clipPos.w == 0 )
            return false;

        Vector3<real_Num> ndc = clipPos.xyz() / clipPos.w;

        // Check if the point is inside the NDC cube (-1 to 1 range)
        return ( ndc.x >= -1 && ndc.x <= 1 ) && ( ndc.y >= -1 && ndc.y <= 1 ) &&
               ( ndc.z >= -1 && ndc.z <= 1 );
    }

    bool GraphicsCamera::isObjectVisible( const Sphere3<real_Num> &bound ) const
    {
        // Compute the combined view-projection matrix
        Matrix4<real_Num> viewProj = getProjectionMatrix() * getViewMatrix();

        // Extract frustum planes from the matrix
        Vector4<real_Num> planes[6] = {
            viewProj.getRow( 3 ) + viewProj.getRow( 0 ),  // Left Plane
            viewProj.getRow( 3 ) - viewProj.getRow( 0 ),  // Right Plane
            viewProj.getRow( 3 ) + viewProj.getRow( 1 ),  // Bottom Plane
            viewProj.getRow( 3 ) - viewProj.getRow( 1 ),  // Top Plane
            viewProj.getRow( 3 ) + viewProj.getRow( 2 ),  // Near Plane
            viewProj.getRow( 3 ) - viewProj.getRow( 2 )   // Far Plane
        };

        // Normalize the planes
        for( auto &plane : planes )
        {
            plane /= plane.xyz().length();
        }

        // Check if the sphere is outside any of the frustum planes
        for( const auto &plane : planes )
        {
            real_Num distance = plane.xyz().dotProduct( bound.getCenter() ) + plane.w;
            if( distance < -bound.getRadius() )
            {
                return false;  // Completely outside
            }
        }

        return true;  // Sphere is visible
    }

    Vector3<real_Num> GraphicsCamera::getDirection( const Vector2<real_Num> &screenPosition,
                                                    Vector3<real_Num> &worldPosition ) const
    {
        auto viewMatrix = getViewMatrix();
        auto projectionMatrix = getProjectionMatrix();
        auto screenWidth = getScreenWidth();
        auto screenHeight = getScreenHeight();

        // Step 1: Convert screen coordinates to normalized device coordinates (NDC)
        real_Num ndcX = ( 2.0f * screenPosition.x ) / screenWidth - 1.0f;
        real_Num ndcY = 1.0f - ( 2.0f * screenPosition.y ) / screenHeight;  // Flip Y

        // Step 2: Convert NDC to clip space (assuming z = -1 for near plane)
        Vector4<real_Num> clipSpacePos( ndcX, ndcY, -1.0f, 1.0f );

        // Step 3: Convert from clip space to view space (inverse projection)
        Matrix4<real_Num> invProj = projectionMatrix.inverse();
        Vector4<real_Num> viewSpacePos = invProj * clipSpacePos;
        viewSpacePos /= viewSpacePos.w;  // Perspective divide

        // Step 4: Convert from view space to world space (inverse view)
        Matrix4<real_Num> invView = viewMatrix.inverse();
        Vector4<real_Num> worldSpaceDir =
            invView * Vector4<real_Num>( viewSpacePos.x, viewSpacePos.y, viewSpacePos.z, 0.0f );

        // Step 5: Normalize direction
        Vector3<real_Num> direction = worldSpaceDir.xyz().normaliseCopy();

        // Step 6: Get camera position as world origin
        worldPosition = invView.getTranslation();

        return direction;
    }

    Vector3<real_Num> GraphicsCamera::getDirection( const Vector2<real_Num> &screenPosition ) const
    {
        auto viewMatrix = getViewMatrix();
        auto projectionMatrix = getProjectionMatrix();
        auto screenWidth = getScreenWidth();
        auto screenHeight = getScreenHeight();

        // Step 1: Convert screen coordinates to normalized device coordinates (NDC)
        real_Num ndcX = ( 2.0f * screenPosition.x ) / screenWidth - 1.0f;
        real_Num ndcY = 1.0f - ( 2.0f * screenPosition.y ) / screenHeight;  // Flip Y

        // Step 2: Convert NDC to clip space (assuming z = -1 for near plane)
        Vector4<real_Num> clipSpacePos( ndcX, ndcY, -1.0f, 1.0f );

        // Step 3: Convert from clip space to view space (inverse projection)
        Matrix4<real_Num> invProj = projectionMatrix.inverse();
        Vector4<real_Num> viewSpacePos = invProj * clipSpacePos;
        viewSpacePos /= viewSpacePos.w;  // Perspective divide

        // Step 4: Convert from view space to world space (inverse view)
        Matrix4<real_Num> invView = viewMatrix.inverse();
        Vector4<real_Num> worldSpaceDir =
            invView * Vector4<real_Num>( viewSpacePos.x, viewSpacePos.y, viewSpacePos.z, 0.0f );

        // Step 5: Normalize direction and return
        auto worldSpaceDir3 = Vector3<real_Num>( worldSpaceDir.x, worldSpaceDir.y, worldSpaceDir.z );
        return worldSpaceDir3.normaliseCopy();
    }

    Array<SmartPtr<ISharedObject>> GraphicsCamera::getChildObjects() const
    {
        auto objects = GraphicsObject<IGraphicsCamera>::getChildObjects();
        objects.push_back( getViewport() );
        objects.push_back( getTargetTexture() );

        return objects;
    }

    s32 GraphicsCamera::getScreenWidth() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->screenWidth;
            }
        }

        return 0;
    }

    void GraphicsCamera::setScreenWidth( s32 width )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<FrustumStateData>( getId() ) )
            {
                state->screenWidth = width;
            }
        }
    }

    s32 GraphicsCamera::getScreenHeight() const
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<FrustumStateData>( getId() ) )
            {
                return state->screenHeight;
            }
        }
        return 0;
    }

    void GraphicsCamera::setScreenHeight( s32 height )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<FrustumStateData>( getId() ) )
            {
                state->screenHeight = height;
            }
        }
    }

}  // namespace workphone::render
