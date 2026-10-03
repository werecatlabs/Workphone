#ifndef __WP_IBone_h__
#define __WP_IBone_h__

#include <Workphone/Interface/Graphics/IGraphicsNode.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a bone in a skeletal animation system. */
        class WPCore_API IGraphicsBone : public IGraphicsNode
        {
        public:
            /** Virtual destructor. */
            ~IGraphicsBone() override;

            /** Creates a new bone as a child of this bone. */
            virtual SmartPtr<IGraphicsBone> createChild(
                u16 handle, const Vector3<real_Num> &translate = Vector3<real_Num>::zero(),
                const Quaternion<real_Num> &rotate = Quaternion<real_Num>::identity() ) = 0;

            /**
             * Sets the current position / orientation to be the
             * 'binding pose' ie the layout in which
             *  bones were originally bound to a mesh.
             */
            virtual void setBindingPose() = 0;

            /** Resets the position and orientation of this
             * bone to the original binding position. */
            virtual void reset() = 0;

            /** Sets whether or not this bone is manually controlled.
             * @param manuallyControlled True if this bone is manually controlled, false otherwise.
             */
            virtual void setManuallyControlled( bool manuallyControlled ) = 0;

            /** Getter for mManuallyControlled Flag.
             * @return True if this bone is manually controlled, false otherwise.
             */
            virtual bool isManuallyControlled() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IBone_h__
