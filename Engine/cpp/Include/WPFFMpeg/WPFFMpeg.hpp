#ifndef WPFFMpeg_h__
#define WPFFMpeg_h__

#include "Workphone/WorkphoneAutolink.hpp"

//#ifdef _DEBUG
//	#pragma comment(lib, "WPFFMpeg_d.lib")
//	#pragma comment(lib, "swscale.lib")
//	#pragma comment(lib, "avutil.lib")
//	#pragma comment(lib, "avdevice.lib")
//	#pragma comment(lib, "avcodec.lib")
//	#pragma comment(lib, "avfilter.lib")
//	#pragma comment(lib, "avformat.lib")
//#elif NDEBUG
//	#pragma comment(lib, "WPFFMpeg.lib")
//	#pragma comment(lib, "swscale.lib")
//	#pragma comment(lib, "avutil.lib")
//	#pragma comment(lib, "avdevice.lib")
//	#pragma comment(lib, "avcodec.lib")
//	#pragma comment(lib, "avfilter.lib")
//	#pragma comment(lib, "avformat.lib")
//#else
//	#pragma comment(lib, "WPFFMpeg.lib")
//	#pragma comment(lib, "swscale.lib")
//	#pragma comment(lib, "avutil.lib")
//	#pragma comment(lib, "avdevice.lib")
//	#pragma comment(lib, "avcodec.lib")
//	#pragma comment(lib, "avfilter.lib")
//	#pragma comment(lib, "avformat.lib")
//#endif

#include <Workphone/Interface/Graphics/IVideoManager.hpp>

namespace workphone
{

    SmartPtr<render::IVideoManager> WP_CALL_CONV createFFMpegVideoManager();

}  // namespace workphone

#endif  // WPFFMpeg_h__
