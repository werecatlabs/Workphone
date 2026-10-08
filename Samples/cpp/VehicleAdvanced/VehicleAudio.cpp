#include "VehicleAudio.h"
#include <Workphone/Workphone.hpp>

namespace workphone::advanced
{
    bool VehicleAudio::load( SmartPtr<ISoundManager> manager )
    {
        unload();
        if( !manager || !manager->isLoaded() )
            return false;
        m_manager = manager;
        for( size_t i = 0; i < m_sounds.size(); ++i )
        {
            const auto name = i < 6    ? "engine_" + StringUtil::toString( i )
                              : i == 6 ? String( "tyre_roll" )
                                       : String( "tyre_squeal" );
            auto sound = dynamic_pointer_cast<ISound>(
                manager->createOrRetrieve( "Audio/VehicleAdvanced/" + name + ".wav" ).first );
            m_sounds[i] = sound;
            if( !sound || !sound->isLoaded() )
            {
                WP_LOG_WARNING( "Vehicle audio: cannot load " + name );
                unload();
                return false;
            }
            sound->setVolume( 0 );
            sound->setLoop( true );
            sound->play();
        }
        WP_LOG( "Vehicle audio: loaded six engine loops, tyre rolling and squeal." );
        return true;
    }

    void VehicleAudio::unload()
    {
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                sound->stop();
                sound->unload( nullptr );
                if( m_manager )
                    m_manager->destroyResource( sound );
                sound = nullptr;
            }
        }
        m_manager = nullptr;
        m_gains = {};
    }

    void VehicleAudio::update( const VehicleAudioInput &input, float dt )
    {
        smoothVehicleAudio( m_gains, vehicleAudioTargets( input ), dt );
        for( size_t i = 0; i < m_sounds.size(); ++i )
            if( m_sounds[i] )
                m_sounds[i]->setVolume( i < 6    ? m_gains.engine[i]
                                        : i == 6 ? m_gains.rolling
                                                 : m_gains.squeal );
    }

    bool VehicleAudio::isPlaying() const
    {
        for( const auto &sound : m_sounds )
            if( !sound || !sound->isPlaying() || !sound->getLoop() )
                return false;
        return true;
    }
}  // namespace workphone::advanced
