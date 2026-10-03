#ifndef IFrameGrabber_h__
#define IFrameGrabber_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** An interface to record a frame. */
    class IFrameGrabber : public ISharedObject
    {
    public:
        ~IFrameGrabber() override = default;

        virtual void addFrame( SmartPtr<IStateMessage> message ) = 0;
        virtual SmartPtr<IStateMessage> popFrame() const = 0;
    };

}  // namespace workphone

#endif  // IFrameGrabber_h__
