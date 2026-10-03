#ifndef ITapDetector_h__
#define ITapDetector_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Detects input taps.*/
    class WPCore_API ITapDetector : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~ITapDetector() override;

        /** Feed an input event to the detector. */
        virtual void feedInput( const SmartPtr<IInputEvent> &event ) = 0;

        /** Reset the tap detector to its initial state. */
        virtual void reset() = 0;

        /** Returns true if a tap has been detected. */
        virtual bool isTapDetected() const = 0;

        /** Set the maximum interval (in seconds) between tap down and up for a valid tap. */
        virtual void setTapInterval( f32 interval ) = 0;

        /** Get the current tap interval (in seconds). */
        virtual f32 getTapInterval() const = 0;

        /** Set the number of taps required for detection (e.g., 1 for single tap, 2 for double tap). */
        virtual void setTapCount( u32 count ) = 0;

        /** Get the number of taps required for detection. */
        virtual u32 getTapCount() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ITapDetector_h__
