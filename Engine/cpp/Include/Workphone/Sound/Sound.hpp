#ifndef __WP_Sound_h__
#define __WP_Sound_h__

#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/System/Resource.hpp>

namespace workphone
{
    /** Sound resource. */
    class WPCore_API Sound : public Resource<ISound>
    {
    public:
        // Static property key strings (defined in Sound.cpp)
        static const String saveStr;
        static const String importStr;
        static const String playStr;
        static const String stopStr;

        /** Constructor. */
        Sound();

        /** Destructor. */
        ~Sound() override;

        /** @copydoc ISound::play */
        void play() override;

        /** @copydoc ISound::pause */
        void pause() override;

        /** @copydoc ISound::stop */
        void stop() override;

        /** @copydoc ISound::isPlaying */
        bool isPlaying() const override;

        /** @copydoc IObject::isValid */
        bool isValid() const override;

        /** @copydoc ISound::setVolume */
        void setVolume( f32 volume ) override;

        /** @copydoc ISound::getVolume */
        f32 getVolume() const override;

        /** @copydoc ISound::setLoop */
        void setLoop( bool loop ) override;

        /** @copydoc ISound::getLoop */
        bool getLoop() const override;

        /** @copydoc ISound::getSpectrum */
        void getSpectrum( Array<f32> &spectrum, u32 numValues ) const override;

        /** @copydoc ISound::setPan */
        void setPan( f32 pan ) override;

        /** @copydoc ISound::getPan */
        f32 getPan() const;

        /** @copydoc ISound::setPosition */
        void setPosition( const Vector3<real_Num> &position ) override;

        /** @copydoc ISound::getPosition */
        Vector3<real_Num> getPosition() const override;

        /** @copydoc ISound::setMinMaxDistance */
        void setMinMaxDistance( f32 minDistance, f32 maxDistance ) override;

        /** @copydoc ISound::getMinMaxDistance */
        void getMinMaxDistance( f32 &minDistance, f32 &maxDistance ) override;

        /** @copydoc ISound::getOwner */
        SmartPtr<ISoundManager> getOwner() const override;

        /** @copydoc ISound::setOwner */
        void setOwner( SmartPtr<ISoundManager> owner ) override;

        /** @copydoc ISound::getFlags */
        u32 getFlags() const override;

        /** @copydoc ISound::setFlags */
        void setFlags( u32 flags ) override;

        /** @copydoc ISound::setFlag */
        void setFlag( u32 flags, bool value ) override;

        /** @copydoc ISound::getFlag */
        bool getFlag( u32 flags ) const override;

        /** @copydoc ISound::getProperties */
        SmartPtr<Properties> getProperties() const override;

        /** @copydoc ISound::setProperties */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc ISound::handleEvent */
        virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

        /** @copydoc ISound::handleStateChanged */
        virtual bool handleStateChanged( SmartPtr<IState> &state );

        WP_CLASS_REGISTER_DECL;

    protected:
        /** The sound manager that owns this sound. */
        WeakPtr<ISoundManager> m_owner;

        /** The 3D position of the sound. */
        Vector3<real_Num> m_position = Vector3<real_Num>::zero();

        /** The sound flags. */
        u32 m_flags = 0;

        /** The volume level (0.0 to 1.0). */
        f32 m_volume = 1.0f;

        /** The stereo pan (-1.0 = left, 0.0 = center, 1.0 = right). */
        f32 m_pan = 0.0f;

        /** The minimum distance for 3D attenuation. */
        f32 m_minDistance = 1.0f;

        /** The maximum distance for 3D attenuation. */
        f32 m_maxDistance = 10000.0f;

        /** Whether the sound is currently playing. */
        bool m_isPlaying = false;
    };
}  // namespace workphone

#endif  // CSound_h__
