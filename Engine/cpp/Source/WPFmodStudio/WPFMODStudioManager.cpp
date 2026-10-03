#include <WPFMODStudio/WPFMODStudioManager.hpp>
#include <WPFMODStudio/WPFMODStudioListener3.hpp>
#include <WPFMODStudio/WPFMODStudioSound.hpp>
#include <Workphone/Workphone.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_studio.hpp>

namespace workphone
{
    const f32 DISTANCEFACTOR =
        1.0f;  // Units per meter.  I.e feet would = 3.28.  centimeters would = 100.

    WP_CLASS_REGISTER_DERIVED( workphone::scene, WPFMODStudioManager, SoundManager );

    static void ERRCHECK( FMOD_RESULT result )
    {
        if( result != FMOD_OK )
        {
            if( result == FMOD_ERR_FILE_NOTFOUND )
            {
                WP_LOG_INFO( "FMOD error! FMOD_ERR_FILE_NOTFOUND" );
            }
            else
            {
                String message = String( "FMOD error: " ) + FMOD_ErrorString( result );
                WP_LOG_INFO( message.c_str() );
            }
        }
    }

    WPFMODStudioManager::WPFMODStudioManager()
    {
    }

    WPFMODStudioManager::~WPFMODStudioManager()
    {
        unload( nullptr );
    }

    void WPFMODStudioManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        SoundManager::load( data );

        auto applicationManager = core::IApplicationManager::instance();

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( data );
        setFactoryManager( factoryManager );

        FactoryUtil::addFactory<WPFMODStudioSound>( factoryManager );
        FactoryUtil::addFactory<FMODSoundListener3>( factoryManager );

        ScopedLock lock( this );

        unsigned int version;
        void *extradriverdata = nullptr;

        FMOD_RESULT result = FMOD::Studio::System::create( &m_pSystem );
        ERRCHECK( result );

        result = m_pSystem.initialize( 32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extradriverdata );
        ERRCHECK( result );

        setLoadingState( LoadingState::Loaded );
    }

    void WPFMODStudioManager::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        if( auto factoryManager = getFactoryManager() )
        {
            FactoryUtil::removeFactory<WPFMODStudioSound>( factoryManager );
        }

        for( auto sound : m_sounds )
        {
            if( sound )
            {
                sound->unload( data );
            }
        }

        if( m_pSystem.isValid() )
        {
            m_pSystem.release();
        }

        SoundManager::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    void WPFMODStudioManager::update()
    {
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::GarbageCollect:
        {
        }
        break;
        case TaskId::Primary:
        {
            SoundManager::update();

            ScopedLock lock( this );

            FMOD_RESULT result;

            int counter = 0;  //hack
            //SoundListenerMap::iterator iter;
            //for( iter = m_listeners.begin(); iter != m_listeners.end(); ++iter )
            //{
            //    SmartPtr<FMODSoundListener3> pListener3D;  // = iter->second;

            //    Vector3F vListenerPosition = pListener3D->getPosition();
            //    Vector3F vListenerForward = pListener3D->getForwardVector();
            //    Vector3F vListenerVelocity = pListener3D->getVelocity();

            //    FMOD_VECTOR listener_pos = { vListenerPosition.X(), vListenerPosition.Y(),
            //                                 vListenerPosition.Z() };
            //    FMOD_VECTOR listener_forward = { vListenerForward.X(), vListenerForward.Y(),
            //                                     vListenerForward.Z() };
            //    FMOD_VECTOR listener_up = { 0.0f, 1.0f, 0.0f };
            //    FMOD_VECTOR listener_vel = { vListenerVelocity.X(), vListenerVelocity.Y(),
            //                                 vListenerVelocity.Z() };

            //    //result = m_pSystem->set3DListenerAttributes((int)counter, &listener_pos, &listener_vel, &listener_forward, &listener_up);     // update 'ears'
            //    ERRCHECK( result );

            //    counter++;
            //}

            //m_pEventSystem->update();   // needed to update 3d engine, once per frame.

            //
            //delete sound that are no long needed
            //

            //static f64 nextRemoveUpdate = 0;
            //if( nextRemoveUpdate < t )
            //{
            //    //for( u32 i = 0; i < m_sounds2d.size(); ++i )
            //    //{
            //    //    WPFMODStudioSound2Ptr pSoundEffect2D = m_sounds2d[i];
            //    //    if( pSoundEffect2D->GetDeleteWhenFinished() && !pSoundEffect2D->isPlaying() )
            //    //    {
            //    //        //m_sounds2d.erase_element_index(i);
            //    //        //pSoundEffect2D->removeReference();
            //    //        --i;
            //    //    }
            //    //}

            //    for( auto pSoundEffect3D : m_sounds )
            //    {
            //        //if( pSoundEffect3D->GetDeleteWhenFinished() && !pSoundEffect3D->isPlaying() )
            //        //{
            //        //    //m_sounds3d.erase_element_index(i);
            //        //    //pSoundEffect3D->removeReference();
            //        //    --i;
            //        //}
            //    }

            //    nextRemoveUpdate = t + 1.0f;
            //}
        }
        break;
        default:
        {
        }
        }
    }

    void *WPFMODStudioManager::GetEventSystem() const
    {
        ScopedLock lock( this );
        return nullptr;
    }

    void WPFMODStudioManager::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
    }

    void WPFMODStudioManager::handleStateChanged( SmartPtr<IState> &state )
    {
    }

    void WPFMODStudioManager::setVolume( f32 fVolume )
    {
        ScopedLock lock( this );

        FMOD_RESULT result;
        FMOD::ChannelGroup *pChannelGroup;

        //m_pSystem.getMasterChannelGroup(&pChannelGroup);
        //result = pChannelGroup->setVolume(fVolume);
        //ERRCHECK(result);
    }

    f32 WPFMODStudioManager::getVolume() const
    {
        ScopedLock lock( this );

        FMOD_RESULT result;
        FMOD::ChannelGroup *pChannelGroup;

        //m_pSystem->getMasterChannelGroup(&pChannelGroup);

        f32 fVolume;
        //result = pChannelGroup->getVolume(&fVolume);
        //ERRCHECK(result);

        return fVolume;
    }

    void WPFMODStudioManager::removeAll()
    {
        m_sounds.clear();
        m_listeners.clear();
    }

    void WPFMODStudioManager::startRecording()
    {
    }

    void WPFMODStudioManager::stopRecording()
    {
    }

    u32 WPFMODStudioManager::getBufferSize() const
    {
        return 0;
    }

    void WPFMODStudioManager::copyContentsToMemory( void *buffer, u32 size )
    {
    }

    void WPFMODStudioManager::_getObject( void **ppObject ) const
    {
        *ppObject = (void *)&m_pSystem;
    }
}  // namespace workphone
