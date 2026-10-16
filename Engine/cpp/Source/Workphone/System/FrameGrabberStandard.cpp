#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FrameGrabberStandard.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Graphics/IVideoStream.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/Messages/StateFrameData.hpp>
#include <limits>

namespace workphone
{

    FrameGrabberStandard::FrameGrabberStandard()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto videoManager = applicationManager ? applicationManager->getVideoManager() : nullptr;
        m_videoStream = videoManager ? videoManager->createVideoStream() : nullptr;
    }

    FrameGrabberStandard::~FrameGrabberStandard() = default;

    void FrameGrabberStandard::update()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        auto window = graphicsSystem ? graphicsSystem->getDefaultWindow() : nullptr;
        if( !window )
        {
            return;
        }
        const auto windowSize = window->getSize();
        if( windowSize.x <= 0 || windowSize.y <= 0 )
        {
            return;
        }
        const u64 byteCount = static_cast<u64>( windowSize.x ) * windowSize.y * 4u;
        if( byteCount > static_cast<u64>( std::numeric_limits<s32>::max() ) )
        {
            return;
        }
        const auto size = static_cast<u32>( byteCount );

        auto data = workphone::make_ptr<StateFrameData>();
        data->setVideoBufferSize( static_cast<s32>( size ) );
        if( !window->tryCopyContentsToMemory( data->getVideoBuffer(), size ) )
        {
            return;
        }

        //auto soundMgr = engine->getSoundManager();
        //u32 soundBufferSize = soundMgr->getBufferSize();
        //data->setSoundBufferSize(soundBufferSize);
        //soundMgr->copyContentsToMemory(data->getSoundBuffer(), soundBufferSize);

        addFrame( data );
    }

    void FrameGrabberStandard::addFrame( SmartPtr<IStateMessage> message )
    {
        if( !message )
        {
            return;
        }
        constexpr size_t maxBufferedFrames = 3;
        if( m_frames.size() == maxBufferedFrames )
        {
            m_frames.pop_front();
        }
        m_frames.push_back( message );
        if( m_videoStream )
        {
            m_videoStream->addFrame( message );
        }
    }

    auto FrameGrabberStandard::popFrame() const -> SmartPtr<IStateMessage>
    {
        if( m_frames.empty() )
        {
            return nullptr;
        }
        auto message = m_frames.front();
        m_frames.pop_front();
        return message;
    }
}  // namespace workphone
