#ifndef ISequenceDetector_h__
#define ISequenceDetector_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /** Detects an input sequence.*/
    class WPCore_API ISequenceDetector : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~ISequenceDetector() override;

        /** Feed an input event to the detector. */
        virtual void feedInput( const SmartPtr<IInputEvent> &event ) = 0;

        /** Reset the sequence detector to its initial state. */
        virtual void reset() = 0;

        /** Returns true if the full sequence has been detected. */
        virtual bool isSequenceDetected() const = 0;

        /** Returns true if the current input partially matches the sequence. */
        virtual bool isPartialMatch() const = 0;

        /** Set the target input sequence to detect. */
        virtual void setSequence( const Array<SmartPtr<IInputEvent>> &sequence ) = 0;

        /** Get the target input sequence. */
        virtual Array<SmartPtr<IInputEvent>> getSequence() const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // ISequenceDetector_h__
