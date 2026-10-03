#ifndef ICharacterController2_h__
#define ICharacterController2_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {

        class WPCore_API ICharacterController2 : public ISharedObject
        {
        public:
            ~ICharacterController2() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // ICharacterController2_h__
