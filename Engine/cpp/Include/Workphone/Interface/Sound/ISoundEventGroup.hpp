#ifndef ISoundEventGroup_h__
#define ISoundEventGroup_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for a sound event group. */
    class WPCore_API ISoundEventGroup : public ISharedObject
    {
    public:
        /** Destructor. */
        ~ISoundEventGroup() override;

        /**
         * Set the volume of the sound event group.
         * @param volume The volume to set.
         */
        virtual void setVolume( f32 volume ) = 0;

        /**
         * Set the mute state of the sound event group.
         * @param mute The mute state to set.
         */
        virtual void setMute( bool mute ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ISoundEventGroup_h__
