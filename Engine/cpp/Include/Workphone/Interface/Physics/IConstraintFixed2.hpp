#ifndef IJointFixed2_h__
#define IJointFixed2_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace physics
    {
        class WPCore_API IConstraintFixed2 : public ISharedObject
        {
        public:
            ~IConstraintFixed2() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IJointFixed2_h__
