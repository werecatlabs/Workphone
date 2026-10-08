#pragma once

#include "VehicleAudioMix.h"
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>

namespace workphone::advanced
{
    class VehicleAudio
    {
    public:
        bool load( SmartPtr<ISoundManager> manager );
        void unload();
        void update( const VehicleAudioInput &input, float dt );
        bool isPlaying() const;
        const VehicleAudioGains &gains() const
        {
            return m_gains;
        }

    private:
        SmartPtr<ISoundManager> m_manager;
        std::array<SmartPtr<ISound>, 8> m_sounds;
        VehicleAudioGains m_gains;
    };
}  // namespace workphone::advanced
