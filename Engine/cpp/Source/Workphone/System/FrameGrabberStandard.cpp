#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FrameGrabberStandard.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Graphics/IVideoStream.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/Messages/StateFrameData.hpp>

namespace workphone
{

    FrameGrabberStandard::FrameGrabberStandard()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto videoManager = applicationManager->getVideoManager();
        m_videoStream = videoManager->createVideoStream();
    }

    FrameGrabberStandard::~FrameGrabberStandard() = default;

    void FrameGrabberStandard::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto window = graphicsSystem->getRenderWindow();

        auto windowSize = window->getSize();
        u32 w = windowSize.X();
        u32 h = windowSize.Y();
        u32 colourDepth = window->getColourDepth();
        u32 size = w * h * colourDepth;

        auto data = workphone::make_ptr<StateFrameData>();
        data->setVideoBufferSize( size );

        window->copyContentsToMemory( data->getVideoBuffer(), size );

        //auto soundMgr = engine->getSoundManager();
        //u32 soundBufferSize = soundMgr->getBufferSize();
        //data->setSoundBufferSize(soundBufferSize);
        //soundMgr->copyContentsToMemory(data->getSoundBuffer(), soundBufferSize);

        m_videoStream->addFrame( data );
    }

    void FrameGrabberStandard::addFrame( SmartPtr<IStateMessage> message )
    {
        m_videoStream->addFrame( message );
    }

    auto FrameGrabberStandard::popFrame() const -> SmartPtr<IStateMessage>
    {
        return nullptr;
    }
}  // namespace workphone
