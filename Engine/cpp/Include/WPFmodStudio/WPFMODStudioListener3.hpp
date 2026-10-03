#ifndef _CSoundListener3D_H
#define _CSoundListener3D_H

#include <Workphone/Sound/SoundListener3.hpp>

namespace workphone
{

    /**
     * @class FMODSoundListener3
     * @brief Sound listener class for FMOD.
     */
    class FMODSoundListener3 : public SoundListener3
    {
    public:
        /** Constructor. */
        FMODSoundListener3();

        /** Destructor. */
        ~FMODSoundListener3() override;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
