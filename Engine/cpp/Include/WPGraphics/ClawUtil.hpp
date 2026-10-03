#ifndef ClawUtil_h__
#define ClawUtil_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include "workphone_color.h"
#include "workphone_matrix.h"
#include "workphone_math.h"
#include "workphone_graphics_pipeline.h"

namespace workphone
{
    namespace render
    {

        /**
         * @class ClawUtil
         * @brief Utility class providing conversion functions between Workphone graphics types and ClawHammer C-style types.
         */
        class ClawUtil
        {
        public:
            static wp_mat4f toNativeMatrix( const Matrix4F &matrix );

            static wp_render_debug_view toNativeDebugView( IGraphicsPipeline::DebugView view );

            static wp_vec3f toCVector( const Vector3<real_Num> &vector );

            static Vector3<real_Num> fromCVector( wp_vec3f vector );

            /**
             * @brief Converts a Workphone ColourF to a ClawHammer wp_colour_f.
             * @param colour The source Workphone colour.
             * @return The equivalent ClawHammer colour.
             */
            static wp_colour_f toCColour( const ColourF &colour );

            /**
             * @brief Converts a ClawHammer wp_colour_f to a Workphone ColourF.
             * @param colour The source ClawHammer colour.
             * @return The equivalent Workphone colour.
             */
            static ColourF fromCColour( const wp_colour_f &colour );

            /**
             * @brief Converts a Workphone blend mode identifier to a ClawHammer wp_blend_mode.
             * @param mode The source blend mode as a u32.
             * @return The equivalent ClawHammer blend mode.
             */
            static wp_blend_mode toCBlendMode( u32 mode );

            /**
             * @brief Converts a ClawHammer wp_blend_mode to a Workphone blend mode identifier.
             * @param mode The source ClawHammer blend mode.
             * @return The equivalent blend mode as a u32.
             */
            static u32 fromCBlendMode( wp_blend_mode mode );

            /**
             * @brief Converts a Workphone cull mode identifier to a ClawHammer wp_cull_mode.
             * @param mode The source cull mode as a u32.
             * @return The equivalent ClawHammer cull mode.
             */
            static wp_cull_mode toCCullMode( u32 mode );

            /**
             * @brief Converts a ClawHammer wp_cull_mode to a Workphone cull mode identifier.
             * @param mode The source ClawHammer cull mode.
             * @return The equivalent cull mode as a u32.
             */
            static u32 fromCCullMode( wp_cull_mode mode );

            static void calculatePositionAndSize( const Vector2F &position, const Vector2F &size,
                                           Vector2F &outAbsolutePosition, Vector2F &outAbsoluteSize );

            static void calculateBounds( const Vector2F &position, const Vector2F &size, wp_rect *outBounds );
           
            static wp_color toWpColor( const ColourF &c );

            static wp_style_item solidItem( const ColourF &c );
        };

    }  // namespace render
}  // namespace workphone

#endif  // ClawUtil_h__
