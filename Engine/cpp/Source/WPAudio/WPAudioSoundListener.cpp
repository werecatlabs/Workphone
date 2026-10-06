#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/WPAudioSoundListener.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
#        include <x3daudio.h>
#    endif
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPAudioSoundListener, ISoundListener3 );

    WPAudioSoundListener::WPAudioSoundListener() :
        m_vCurPosition( Vector3F::zero() ),
        m_vVelocity( Vector3F::zero() ),
        m_vForwardVector( Vector3F( 0.0f, 0.0f, 1.0f ) ),
        m_vUpVector( Vector3F( 0.0f, 1.0f, 0.0f ) )
    {
#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        // Initialize X3DAudio listener with default values
        updateNativeListener();
#    endif
#endif
    }

    WPAudioSoundListener::~WPAudioSoundListener() = default;

    void WPAudioSoundListener::setPosition( const Vector3F &position )
    {
        m_vCurPosition = position;

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        // Update the native X3DAudio listener structure
        updateNativeListener();
#    endif
#endif
    }

    Vector3F WPAudioSoundListener::getPosition() const
    {
        return m_vCurPosition;
    }

    void WPAudioSoundListener::setForwardVector( const Vector3F &vForwardVector )
    {
        m_vForwardVector = vForwardVector;

        // Normalize to ensure proper orientation
        f32 length = m_vForwardVector.length();
        if( length > 0.0001f )
        {
            m_vForwardVector /= length;
        }

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        updateNativeListener();
#    endif
#endif
    }

    Vector3F WPAudioSoundListener::getForwardVector() const
    {
        return m_vForwardVector;
    }

    void WPAudioSoundListener::setVelocity( const Vector3F &velocity )
    {
        m_vVelocity = velocity;

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        updateNativeListener();
#    endif
#endif
    }

    Vector3F WPAudioSoundListener::getVelocity() const
    {
        return m_vVelocity;
    }

    void WPAudioSoundListener::setUpVector( const Vector3F &vUpVector )
    {
        m_vUpVector = vUpVector;

        // Normalize to ensure proper orientation
        f32 length = m_vUpVector.length();
        if( length > 0.0001f )
        {
            m_vUpVector /= length;
        }

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        updateNativeListener();
#    endif
#endif
    }

    Vector3F WPAudioSoundListener::getUpVector() const
    {
        return m_vUpVector;
    }

    void WPAudioSoundListener::setOrientation( const Vector3F &forward, const Vector3F &up )
    {
        m_vForwardVector = forward;
        m_vUpVector = up;

        // Normalize vectors
        f32 forwardLength = m_vForwardVector.length();
        if( forwardLength > 0.0001f )
        {
            m_vForwardVector /= forwardLength;
        }

        f32 upLength = m_vUpVector.length();
        if( upLength > 0.0001f )
        {
            m_vUpVector /= upLength;
        }

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        updateNativeListener();
#    endif
#endif
    }

    void WPAudioSoundListener::getOrientation( Vector3F &forward, Vector3F &up ) const
    {
        forward = m_vForwardVector;
        up = m_vUpVector;
    }

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
    void WPAudioSoundListener::updateNativeListener()
    {
        // Update X3DAUDIO_LISTENER structure
        // Note: X3DAudio uses a left-handed coordinate system

        m_nativeListener.Position.x = m_vCurPosition.x;
        m_nativeListener.Position.y = m_vCurPosition.y;
        m_nativeListener.Position.z = m_vCurPosition.z;

        m_nativeListener.Velocity.x = m_vVelocity.x;
        m_nativeListener.Velocity.y = m_vVelocity.y;
        m_nativeListener.Velocity.z = m_vVelocity.z;

        m_nativeListener.OrientFront.x = m_vForwardVector.x;
        m_nativeListener.OrientFront.y = m_vForwardVector.y;
        m_nativeListener.OrientFront.z = m_vForwardVector.z;

        m_nativeListener.OrientTop.x = m_vUpVector.x;
        m_nativeListener.OrientTop.y = m_vUpVector.y;
        m_nativeListener.OrientTop.z = m_vUpVector.z;

        // Initialize cone if not already set
        // The listener cone defines the direction and angle of the listener's attention
        if( m_nativeListener.pCone == nullptr )
        {
            // Default listener cone - omnidirectional
            m_listenerCone.InnerAngle = X3DAUDIO_2PI;
            m_listenerCone.OuterAngle = X3DAUDIO_2PI;
            m_listenerCone.InnerVolume = 1.0f;
            m_listenerCone.OuterVolume = 1.0f;
            m_listenerCone.InnerLPF = 0.0f;
            m_listenerCone.OuterLPF = 0.0f;
            m_listenerCone.InnerReverb = 1.0f;
            m_listenerCone.OuterReverb = 1.0f;

            m_nativeListener.pCone = &m_listenerCone;
        }
    }

    const X3DAUDIO_LISTENER &WPAudioSoundListener::getNativeListener() const
    {
        return m_nativeListener;
    }

    X3DAUDIO_LISTENER &WPAudioSoundListener::getNativeListener()
    {
        return m_nativeListener;
    }
#    endif
#endif

}  // namespace workphone
