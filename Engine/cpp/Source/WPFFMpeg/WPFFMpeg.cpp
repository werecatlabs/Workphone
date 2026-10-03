#include "WPFFMpeg/WPFFMpeg.hpp"
#include "WPFFMpeg/VideoManager.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{

    using namespace workphone::render;

    SmartPtr<IVideoManager> WP_CALL_CONV createFFMpegVideoManager()
    {
        auto videoManager = workphone::make_ptr<VideoManager>();
        return videoManager;
    }

}  // namespace workphone
