#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateFrameData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <memory>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateFrameData, StateMessage );

    StateFrameData::StateFrameData() = default;

    StateFrameData::StateFrameData( s32 videoBufferSize, s32 soundBufferSize )
    {
        m_videoBuffer = static_cast<u8 *>( malloc( videoBufferSize ) );
        m_videoBufferSize = videoBufferSize;

        m_soundBuffer = new u8[soundBufferSize];
        m_soundBufferSize = soundBufferSize;
    }

    StateFrameData::~StateFrameData()
    {
        if( m_videoBuffer )
        {
            free( m_videoBuffer );

            m_videoBuffer = nullptr;
        }

        if( m_soundBuffer )
        {
            delete[] m_soundBuffer;
            m_soundBuffer = nullptr;
        }
    }

    auto StateFrameData::getVideoBuffer() const -> u8 *
    {
        return m_videoBuffer;
    }

    void StateFrameData::setVideoBuffer( u8 *value )
    {
        m_videoBuffer = value;
    }

    auto StateFrameData::getVideoBufferSize() const -> s32
    {
        return m_videoBufferSize;
    }

    void StateFrameData::setVideoBufferSize( s32 value )
    {
        m_videoBufferSize = value;

        if( m_videoBuffer )
        {
            free( m_videoBuffer );

            m_videoBuffer = nullptr;
        }

        m_videoBuffer = static_cast<u8 *>( malloc( m_videoBufferSize ) );

        if( !m_videoBuffer )
        {
            WP_EXCEPTION( "Error: could not allocate frame." );
        }
    }

    auto StateFrameData::getSoundBuffer() const -> u8 *
    {
        return m_soundBuffer;
    }

    void StateFrameData::setSoundBuffer( u8 *value )
    {
        m_soundBuffer = value;
    }

    auto StateFrameData::getSoundBufferSize() const -> s32
    {
        return m_soundBufferSize;
    }

    void StateFrameData::setSoundBufferSize( s32 value )
    {
        m_soundBufferSize = value;

        if( m_soundBuffer )
        {
            delete[] m_soundBuffer;
            m_soundBuffer = nullptr;
        }

        m_soundBuffer = new unsigned char[m_soundBufferSize];
    }
}  // namespace workphone
