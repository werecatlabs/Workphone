#ifndef __WP_SoundManager_h__
#define __WP_SoundManager_h__

#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{
    /** Sound manager base implementation.
     */
    class WPCore_API SoundManager : public ISoundManager
    {
    public:
        /** The state listener class. */
        class StateListener : public IStateListener
        {
        public:
            /** Constructor. */
            StateListener();

            /** Destructor. */
            ~StateListener() override;

            /** @copydoc IStateListener::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Gets the owner. */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /** Gets the owner. */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /** Gets the owner. */
            SmartPtr<SoundManager> getOwner() const;

            /** Sets the owner. */
            void setOwner( SmartPtr<SoundManager> owner );

        protected:
            /** The owner. */
            AtomicWeakPtr<SoundManager> m_owner;
        };

        /** Constructor. */
        SoundManager();

        /** Destructor. */
        ~SoundManager() override;

        /** @copydoc ISoundManager::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISoundManager::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISoundManager::update */
        void update() override;

        /** @copydoc ISoundManager::addListener3 */
        SmartPtr<ISoundListener3> addListener3(
            const String &name, const Vector3<real_Num> &position = Vector3<real_Num>::zero() ) override;

        /** @copydoc ISoundManager::findListener3 */
        SmartPtr<ISoundListener3> findListener3( const String &name ) override;

        /** @copydoc ISoundManager::setVolume */
        void setVolume( f32 volume ) override;

        /** @copydoc ISoundManager::getVolume */
        f32 getVolume() const override;

        /** @copydoc ISoundManager::startRecording */
        void startRecording() override;

        /** @copydoc ISoundManager::stopRecording */
        void stopRecording() override;

        /** @copydoc ISoundManager::getBufferSize */
        u32 getBufferSize() const override;

        /** @copydoc ISoundManager::copyContentsToMemory */
        void copyContentsToMemory( void *buffer, u32 size ) override;

        /** @copydoc ISoundManager::isRealtime */
        bool isRealtime() const override;

        /** @copydoc ISoundManager::isMute */
        bool isMute() const override;

        /** @copydoc ISoundManager::setMute */
        void setMute( bool mute ) override;

        /** @copydoc ISoundManager::loadObject */
        void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

        /** @copydoc ISoundManager::unloadObject */
        void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

        /** @copydoc ISoundManager::create */
        SmartPtr<IResource> create( const String &name ) override;

        /** @copydoc ISoundManager::create */
        SmartPtr<IResource> create( const String &uuid, const String &name ) override;

        /** @copydoc ISoundManager::createOrRetrieve */
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                          const String &type ) override;

        /** @copydoc ISoundManager::createOrRetrieve */
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

        /** @copydoc ISoundManager::destroyResource */
        void destroyResource( SmartPtr<IResource> resource ) override;

        /** @copydoc ISoundManager::destroyAll */
        void destroyAll() override;

        /** @copydoc ISoundManager::saveToFile */
        void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

        /** @copydoc ISoundManager::loadFromFile */
        SmartPtr<IResource> loadFromFile( const String &filePath ) override;

        /** @copydoc ISoundManager::loadResource */
        SmartPtr<IResource> loadResource( const String &name ) override;

        /** @copydoc ISoundManager::getByName */
        SmartPtr<IResource> getByName( const String &name ) override;

        /** @copydoc ISoundManager::getById */
        SmartPtr<IResource> getById( const String &uuid ) override;

        /** @copydoc ISoundManager::getFactoryManager */
        SmartPtr<IFactoryManager> getFactoryManager() const override;

        /** @copydoc ISoundManager::setFactoryManager */
        void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) override;

        /** @copydoc ISoundManager::cloneResource */
        SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                           const String &clonedResourceName ) override;

        /** @copydoc ISoundManager::cloneResource */
        SmartPtr<IResource> cloneResource( const String &name,
                                           const String &clonedResourceName ) override;

        /** @copydoc ISoundManager::lock */
        void lock() override;

        /** @copydoc ISoundManager::try_lock */
        bool try_lock() override;

        /** @copydoc ISoundManager::unlock */
        void unlock() override;

        /** @copydoc ISoundManager::_getObject */
        void _getObject( void **ppObject ) const override;

        /**
         * @brief Get the raw pointer to the attached IStateContext.
         * Useful when C-style pointer access is required. The returned pointer is not
         * accompanied by ownership guarantees; prefer getStateContext() for ownership.
         * @return Raw IStateContext pointer or nullptr if none set.
         */
        virtual IStateContext *getStateContextPtr() const;

        /**
         * @brief Gets the state object associated with this scene node.
         * @return The state object.
         */
        SmartPtr<IStateContext> getStateContext() const;

        /**
         * @brief Sets the state object associated with this scene node.
         * @param stateContext The state object.
         */
        void setStateContext( SmartPtr<IStateContext> stateContext );

        /**
         * @brief Gets the state listener associated with this scene node.
         * @return The state listener.
         */
        SmartPtr<IStateListener> getStateListener() const;

        /**
         * @brief Sets the state listener associated with this scene node.
         * @param stateListener The state listener.
         */
        void setStateListener( SmartPtr<IStateListener> stateListener );

        /** @copydoc ISoundManager::getFlags */
        u32 getFlags() const override;

        /** @copydoc ISoundManager::setFlags */
        void setFlags( u32 flags ) override;

        /** @copydoc ISoundManager::setFlag */
        void setFlag( u32 flags, bool value ) override;

        /** @copydoc ISoundManager::getFlag */
        bool getFlag( u32 flags ) const override;

        /** Handle state changed. */
        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

        /** Handle state changed. */
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** The factory manager. */
        AtomicSmartPtr<IFactoryManager> m_factoryManager;

        /**< The state object associated with this object. */
        AtomicSmartPtr<IStateContext> m_stateContext;

        /**< The state listener associated with this object. */
        AtomicSmartPtr<IStateListener> m_stateListener;

        /** The array of sounds. */
        ConcurrentArray<SmartPtr<ISound>> m_sounds;

        /** The sound listeners. */
        ConcurrentArray<SmartPtr<ISoundListener3>> m_listeners;

        /** Load queue. */
        ConcurrentQueue<SmartPtr<ISharedObject>> m_loadQueue;

        /** Unload queue. */
        ConcurrentQueue<SmartPtr<ISharedObject>> m_unloadQueue;

        /** The mutex. */
        mutable RecursiveSpinMutex m_mutex;
    };

    inline IStateContext *SoundManager::getStateContextPtr() const
    {
        return m_stateContext.get();
    }

}  // namespace workphone

#endif  // CSoundManager_h__
