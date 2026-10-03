#ifndef IInstancedObject_h__
#define IInstancedObject_h__

#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for an instanced object.
         */
        class WPCore_API IInstancedObject : public IGraphicsObject
        {
        public:
            static const hash_type RENDER_QUEUE_HASH;
            static const hash_type VISIBILITY_FLAGS_HASH;
            static const hash_type CUSTOM_PARAMETER_HASH;
            static const u32 MAX_CUSTOM_PARAMS;

            /** Virtual destructor. */
            ~IInstancedObject() override;

            /** Sets the position.
             * @param position The position to set.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /** Gets the position.
             * @return The position of the instance.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /** Sets the orientation of this node via Quaternion parameters.
             * @param orientation The orientation to set.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /** Returns a Quaternion representing the nodes orientation.
             * @return The orientation of the instance.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /** Sets the scaling factor applied to this node.
             * @param scale The scaling factor to set.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /** Gets the scaling factor of this node.
             * @return The scaling factor of the instance.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /** Sets the custom parameter for this instance.
             * @param idx The index of the parameter to set.
             * @param newParam The value of the parameter to set.
             */
            virtual void setCustomParam( u8 idx, const Vector4F &newParam ) = 0;

            /** Gets the custom parameter for this instance.
             * @param idx The index of the parameter to get.
             * @return The value of the parameter.
             */
            virtual Vector4F getCustomParam( u8 idx ) = 0;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IInstancedObject_h__
