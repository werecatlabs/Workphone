#ifndef IAnimationControllerListener_h__
#define IAnimationControllerListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API IAnimationControllerListener : public ISharedObject
        {
        public:
            ~IAnimationControllerListener() override;

            virtual void handleAnimationEnd( const String &name ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IAnimationControllerListener_h__
