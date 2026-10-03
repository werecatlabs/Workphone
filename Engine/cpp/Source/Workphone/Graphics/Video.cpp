#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Video.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>

// Platform-specific includes
#if defined( _WIN32 ) || defined( WP_PLATFORM_WIN32 )
#elif defined( __linux__ )
#elif defined( __ANDROID__ )
// #include <Android/MediaCodecVideo.hpp> // Placeholder
#elif defined( __APPLE__ )
// #include <iOS/AVFoundationVideo.hpp> // Placeholder
#endif

namespace workphone
{
    namespace render
    {

        Video::Video()
        {
            // Default: no fileName, so just create the backend with default constructor
#if defined( _WIN32 ) || defined( WP_PLATFORM_WIN32 )
#elif defined( __linux__ )
#elif defined( __ANDROID__ )
#elif defined( __APPLE__ )
#else
#endif
        }

        Video::Video( const String &fileName )
        {
#if defined( _WIN32 ) || defined( WP_PLATFORM_WIN32 )
#elif defined( __linux__ )
#elif defined( __ANDROID__ )
#elif defined( __APPLE__ )
#else
#endif
        }

        Video::~Video() = default;

        void Video::play()
        {
            if( m_impl )
                m_impl->play();
        }

        void Video::stop()
        {
            if( m_impl )
                m_impl->stop();
        }

        Vector2I Video::getSize() const
        {
            return m_impl ? m_impl->getSize() : Vector2I();
        }
        void Video::setSize( const Vector2I &size )
        {
            if( m_impl )
                m_impl->setSize( size );
        }

        void *Video::getCurrentFrameBuffer() const
        {
            return m_impl ? m_impl->getCurrentFrameBuffer() : nullptr;
        }

        void Video::setLoop( bool loop )
        {
            if( m_impl )
                m_impl->setLoop( loop );
        }

        bool Video::getLoop() const
        {
            return m_impl ? m_impl->getLoop() : false;
        }

        void Video::setAutoUpdate( bool autoUpdate )
        {
            if( m_impl )
                m_impl->setAutoUpdate( autoUpdate );
        }

        bool Video::getAutoUpdate() const
        {
            return m_impl ? m_impl->getAutoUpdate() : false;
        }

        SmartPtr<IVideoTexture> Video::getVideoTexture() const
        {
            return m_impl ? m_impl->getVideoTexture() : nullptr;
        }

        void Video::setVideoTexture( SmartPtr<IVideoTexture> videoTexture )
        {
            if( m_impl )
                m_impl->setVideoTexture( videoTexture );
        }

        void Video::_getObject( void **object )
        {
            if( m_impl )
                m_impl->_getObject( object );
        }

    }  // namespace render
}  // namespace workphone
