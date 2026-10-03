#ifndef ISharedObjectListener_h__
#define ISharedObjectListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /** An interface for a shared object listener. */
    class WPCore_API ISharedObjectListener : public ISharedObject
    {
    public:
        /** Constructor.
         */
        ISharedObjectListener();

        /** Virtual destructor.
         */
        ~ISharedObjectListener() override;

        /** Function to destroy shared object listener.
        @param ptr A pointer to the object.
        @return True if the object was destroyed, false otherwise.
        */
        virtual bool destroy( void *ptr ) = 0;
    };
}  // namespace workphone

#endif  // ISharedObjectListener_h__
