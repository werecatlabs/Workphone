#include <WPFMODStudio/WPFMODStudioSound.hpp>
#include <fmod_errors.h>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    static void ERRCHECK( FMOD_RESULT result )
    {
        if( result != FMOD_OK )
        {
            String msg = String( "FMOD error!" ) + String( FMOD_ErrorString( result ) );
            WP_LOG_INFO( msg.c_str() );
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone, WPFMODStudioSound, Sound );

    WPFMODStudioSound::WPFMODStudioSound() = default;

    WPFMODStudioSound::WPFMODStudioSound( ISoundManager *pSoundManager, FMOD::Studio::System *system ) :
        m_pSoundManager( pSoundManager ),
        m_pSystem( system ),
        m_pChannel( nullptr )
    {
    }

    WPFMODStudioSound::~WPFMODStudioSound()
    {
        unload( nullptr );
    }

    void WPFMODStudioSound::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        Sound::load( data );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        FMOD_RESULT result;

        FMOD_MODE mode = 0;
        if( getFlag( SOUND_FLAG_3D ) )
        {
            mode |= FMOD_3D;
        }
        else
        {
            mode |= FMOD_2D;
        }

        if( getFlag( SOUND_FLAG_HARDWARE ) )
        {
            mode |= FMOD_HARDWARE;
        }
        else
        {
            mode |= FMOD_SOFTWARE;
        }

        auto soundManager = getOwner();
        soundManager->_getObject( (void **)&m_pSystem );

        FMOD::System *system = nullptr;
        m_pSystem->getLowLevelSystem( &system );

        auto filePath = getFilePath();
        auto stream = fileSystem->open( filePath, true, true, false, false );
        if( !stream )
        {
            stream = fileSystem->open( filePath, true, true, false, true );
        }

        if( stream )
        {
            auto size = stream->size();
            auto buffer = new u8[size];

            stream->read( buffer, size );
            stream->close();

            // Fill out the FMOD_CREATESOUNDEXINFO structure
            FMOD_CREATESOUNDEXINFO createSoundInfo = {};
            createSoundInfo.cbsize = sizeof( FMOD_CREATESOUNDEXINFO );
            createSoundInfo.length = size;  // Length of the sound data buffer
            createSoundInfo.format =
                FMOD_SOUND_FORMAT_PCM16;  // Assuming your WAV file is in PCM format with 16-bit depth

            result = system->createSound( (char *)buffer, FMOD_OPENMEMORY | mode, &createSoundInfo,
                                          &m_pSound );

            //result = system->createSound( filePath.c_str(), mode, nullptr, &m_pSound );
            ERRCHECK( result );

            result = m_pSound->set3DMinMaxDistance( 10.f, 500.f );
            ERRCHECK( result );

            auto bLoop = getLoop();
            if( bLoop )
            {
                result = m_pSound->setMode( FMOD_LOOP_NORMAL );
                ERRCHECK( result );
            }

            delete[] buffer;
        }

        setLoadingState( LoadingState::Loaded );
    }

    void WPFMODStudioSound::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        if( m_pSound )
        {
            m_pSound->release();
            m_pSound = nullptr;
        }

        Sound::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    void WPFMODStudioSound::play()
    {
        FMOD_RESULT result;

        if( !m_pChannel )
        {
            FMOD_VECTOR pos = { m_position.X(), m_position.Y(), m_position.Z() };
            FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

            FMOD::System *system = nullptr;
            m_pSystem->getLowLevelSystem( &system );

            result = system->playSound( m_pSound, nullptr, true, &m_pChannel );
            ERRCHECK( result );
            result = m_pChannel->set3DAttributes( &pos, &vel );
            ERRCHECK( result );
            //result = m_pChannel->setVolume( 1.0f );
            //ERRCHECK(result);
            result = m_pChannel->setPaused( false );
            ERRCHECK( result );
        }
        else
        {
            bool bIsPlaying = false;
            m_pChannel->isPlaying( &bIsPlaying );

            if( !bIsPlaying )
            {
                FMOD::System *system = nullptr;
                m_pSystem->getLowLevelSystem( &system );

                result = system->playSound( m_pSound, nullptr, false, &m_pChannel );
                ERRCHECK( result );
            }
        }
    }

    bool WPFMODStudioSound::isPlaying() const
    {
        bool bIsPlaying = false;
        if( m_pChannel )
        {
            FMOD_RESULT result;
            result = m_pChannel->isPlaying( &bIsPlaying );
            //ERRCHECK(result);
        }

        return bIsPlaying;
    }

    void WPFMODStudioSound::stop()
    {
        if( m_pChannel )
        {
            auto result = m_pChannel->stop();
            ERRCHECK( result );

            m_pChannel = nullptr;
        }
    }

    void WPFMODStudioSound::setPosition( const Vector3F &position )
    {
        m_position = position;

        if( m_pChannel )
        {
            FMOD_RESULT result;
            FMOD_VECTOR pos = { m_position.X(), m_position.Y(), m_position.Z() };
            FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
            result = m_pChannel->set3DAttributes( &pos, &vel );
            ERRCHECK( result );
        }
    }

    Vector3F WPFMODStudioSound::getPosition() const
    {
        return m_position;
    }

    void WPFMODStudioSound::setVolume( f32 fVolume )
    {
        if( m_pChannel )
        {
            FMOD_RESULT result;
            result = m_pChannel->setVolume( fVolume );
            ERRCHECK( result );
        }
    }

    f32 WPFMODStudioSound::getVolume() const
    {
        f32 vol;
        if( m_pChannel )
        {
            FMOD_RESULT result;
            result = m_pChannel->getVolume( &vol );
            ERRCHECK( result );
        }

        return vol;
    }

    void WPFMODStudioSound::setMinMaxDistance( f32 minDistance, f32 maxDistance )
    {
        FMOD_RESULT result;
        result = m_pSound->set3DMinMaxDistance( minDistance, maxDistance );
        ERRCHECK( result );
    }

    void WPFMODStudioSound::getMinMaxDistance( f32 &minDistance, f32 &maxDistance )
    {
        FMOD_RESULT result;
        result = m_pSound->get3DMinMaxDistance( &minDistance, &maxDistance );
        ERRCHECK( result );
    }

    String WPFMODStudioSound::getSoundName() const
    {
        if( m_pSound )
        {
            char soundName[255];
            m_pSound->getName( soundName, 255 );
            return String( soundName );
        }

        return StringUtil::EmptyString;
    }

    void WPFMODStudioSound::getSpectrum( Array<f32> &spectrumData, u32 numvalues ) const
    {
        if( m_pChannel )
        {
            FMOD_RESULT result;
            f32 spectrum[512];
            //result = m_pChannel->getSpectrum( spectrum, 512, 0, FMOD_DSP_FFT_WINDOW_RECT );
            ERRCHECK( result );

            for( u32 i = 0; i < numvalues; i++ )
            {
                spectrumData.push_back( spectrum[i] );
            }
        }
    }
}  // namespace workphone
