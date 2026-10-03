#ifndef IBoxShape2_h__
#define IBoxShape2_h__

#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Math/AABB2.hpp>

namespace workphone
{
    namespace physics
    {

        class WPCore_API IBoxShape2 : public IPhysicsShape2
        {
        public:
            ~IBoxShape2() override;

            virtual void setAABB( const AABB2<real_Num> &box ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IBoxShape2_h__
