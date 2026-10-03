#ifndef __IPhysicsSoftBody2__H
#define __IPhysicsSoftBody2__H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include "Workphone/Math/AABB2.hpp"
#include "Workphone/Math/Vector2.hpp"
#include "Workphone/Math/Quaternion.hpp"
#include "Workphone/Core/StringTypes.hpp"

namespace workphone
{
    namespace physics
    {

        class WPCore_API IPhysicsSoftBody2 : public ISharedObject
        {
        public:
            ~IPhysicsSoftBody2() override;

            /** */
            virtual void setPosition( const Vector2<real_Num> &position ) = 0;

            /** */
            virtual const Vector2<real_Num> &getPosition() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif
