#ifndef WP_IMGUI_SCENE_VIEWPORT_TOOL_INPUT_HPP
#define WP_IMGUI_SCENE_VIEWPORT_TOOL_INPUT_HPP

#include <Workphone/Scene/ScenePicking.hpp>

namespace workphone::ui
{
    inline const char *terrainViewportBrushPhase( bool cancelRequested, bool hovered,
                                                 bool released, bool clicked, bool held )
    {
        // A release can be the first frame observed outside the image.
        if( cancelRequested || !hovered ) return "cancel";
        if( released ) return "commit";
        if( clicked ) return "begin";
        if( held ) return "update";
        return "hover";
    }

    // The tool protocol uses a world ray through the displayed image, whose
    // pixels need not match the camera's configured screen dimensions.
    template <class Camera>
    bool makeSceneViewportToolArguments( const Camera &camera, Vector2F imageOrigin,
                                         Vector2F imageSize, Vector2F mouse,
                                         const String &phase, Parameters &arguments )
    {
        arguments = { Parameter( String( "cancel" ) ), Parameter( 0.0 ), Parameter( 0.0 ),
                      Parameter( 0.0 ), Parameter( 0.0 ), Parameter( -1.0 ), Parameter( 0.0 ) };
        if( !std::isfinite( imageOrigin.x ) || !std::isfinite( imageOrigin.y ) ||
            !std::isfinite( imageSize.x ) || !std::isfinite( imageSize.y ) ||
            !std::isfinite( mouse.x ) || !std::isfinite( mouse.y ) ||
            imageSize.x <= 0 || imageSize.y <= 0 ) return false;
        Ray3<real_Num> ray;
        real_Num range;
        if( !scene::makeScenePickingRay( camera.getViewMatrix(), camera.getProjectionMatrix(),
                { ( mouse.x - imageOrigin.x ) / imageSize.x,
                  ( mouse.y - imageOrigin.y ) / imageSize.y }, ray, range ) ) return false;
        const auto origin = ray.getOrigin(), direction = ray.getDirection();
        arguments = { Parameter( phase ), Parameter( f64( origin.x ) ), Parameter( f64( origin.y ) ),
                      Parameter( f64( origin.z ) ), Parameter( f64( direction.x ) ),
                      Parameter( f64( direction.y ) ), Parameter( f64( direction.z ) ) };
        return true;
    }
}
#endif
