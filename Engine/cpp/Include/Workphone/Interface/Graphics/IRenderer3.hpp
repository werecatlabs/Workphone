#ifndef __IRenderer3_h__
#define __IRenderer3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderer.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for 3D rendering.
         *
         * Extends IRenderer with 3D-specific capabilities including camera management,
         * mesh and graphics object rendering, and 3D primitive drawing.
         */
        class WPCore_API IRenderer3 : public IRenderer
        {
        public:
            /** Virtual destructor. */
            ~IRenderer3() override;

            /**
             * Draws a 3D line segment in world space.
             *
             * @param start     The start position in world space.
             * @param end       The end position in world space.
             * @param colour    The colour of the line.
             */
            virtual void drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                   const ColourF &colour ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __IRenderer3_h__
