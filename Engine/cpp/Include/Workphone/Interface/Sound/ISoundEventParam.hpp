#ifndef ISoundEventParam_h__
#define ISoundEventParam_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for a sound event parameter. */
    class WPCore_API ISoundEventParam : public ISharedObject
    {
    public:
        /** Destructor. */
        ~ISoundEventParam() override;

        /** Gets the parameter value.
         * @return The parameter value.
         */
        virtual f32 getValue() const = 0;

        /** Sets the parameter value.
         * @param value The new value.
         */
        virtual void setValue( f32 value ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // ISoundEventParam_h__
