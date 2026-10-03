#ifndef IDebug_h__
#define IDebug_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/OBB3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for debug rendering.
         */
        class WPCore_API IDebug : public ISharedObject
        {
        public:
            /** Destructor. */
            ~IDebug() override;

            /**
             * Clears the debug output.
             */
            virtual void clear() = 0;

            /**
             * Draws a debug point.
             * @param id The unique identifier for the point.
             * @param positon The position of the point in 3D space.
             * @param color The color of the point.
             */
            virtual void drawPoint( hash_type id, const Vector3<real_Num> &positon, u32 color ) = 0;

            /**
             * Draws a debug line.
             * @param id The unique identifier for the line.
             * @param start The start position of the line in 3D space.
             * @param end The end position of the line in 3D space.
             * @param colour The color of the line.
             */
            virtual SmartPtr<IDebugLine> drawLine( hash_type id, const Vector3<real_Num> &start,
                                                   const Vector3<real_Num> &end, u32 colour ) = 0;

            /**
             * Draws the twelve edges of an axis-aligned bounding box.
             *

             * * The supplied id is used as a stable primitive id; the individual edge ids are
 * derived
             * from it. Calling this method again with the same id updates the existing
             *
             * representation instead of allocating another box.
             *
             * @param id
             * Stable identifier for the box.
             * @param box World-space axis-aligned bounding
             * box.
             * @param colour Packed RGBA colour.
             */
            void drawAABB( hash_type id, const AABB3<real_Num> &box, u32 colour );

            /**
             * Draws the twelve edges of an oriented bounding box.
             *

             * * @param id Stable identifier for the box.
             * @param box World-space oriented
             * bounding box.
             * @param colour Packed RGBA colour.
             */
            void drawOBB( hash_type id, const OBB3<real_Num> &box, u32 colour );

            /**
             * Draws a vector as a shaft with a two-line arrow head.
             *

             * * @param id Stable identifier for the arrow.
             * @param start World-space start
             * of the vector.
             * @param vector Direction and length of the vector.
 * @param
             * colour Packed RGBA colour.
             * @param headScale Arrow-head length as a fraction
             * of the vector length.
             */
            void drawArrow( hash_type id, const Vector3<real_Num> &start,
                            const Vector3<real_Num> &vector, u32 colour,
                            real_Num headScale = static_cast<real_Num>( 0.2 ) );

            /** Draws a debug circle.
             * @param id The unique identifier for the circle.
             * @param position The position of the circle in 3D space.
             * @param radius The radius of the circle.
             * @param color The color of the circle.
             */
            virtual SmartPtr<IDebugCircle> drawCircle( hash_type id, const Vector3<real_Num> &position,
                                                       const Quaternion<real_Num> &orientation,
                                                       real_Num radius, u32 color ) = 0;

            /**
             * Draws text.
             * @param id The unique identifier for the text.
             * @param position The position of the text in 2D space.
             * @param text The text to draw.
             * @param color The color of the text.
             */
            virtual void drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                                   u32 color ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IDebug_h__
