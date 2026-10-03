#ifndef IMassData2_h__
#define IMassData2_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace physics
    {

        /// This holds the mass data computed for a shape.
        class WPCore_API IMassData2 : public ISharedObject
        {
        public:
            ~IMassData2() override;

            /// The mass of the shape, usually in kilograms.
            virtual void setMass( f32 mass ) = 0;

            /// The mass of the shape, usually in kilograms.
            virtual f32 getMass() const = 0;

            /// The position of the shape's centroid relative to the shape's origin.
            virtual void setCenter( Vector2<real_Num> center ) = 0;

            /// The position of the shape's centroid relative to the shape's origin.
            virtual Vector2<real_Num> getCenter() const = 0;

            /// The rotational inertia of the shape about the local origin.
            virtual void setInertia( f32 inertia ) = 0;

            /// The rotational inertia of the shape about the local origin.
            virtual f32 getInertia() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IMassData2_h__
