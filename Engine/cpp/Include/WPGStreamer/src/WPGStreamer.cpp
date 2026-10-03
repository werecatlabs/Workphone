#include "WPGStreamer.hpp"
#include "CVideoManager.hpp"
#include <system\IStateObject.hpp>


namespace fb
{



	FB_GSTREAMER_API VideoManagerPtr FBCALLCONV createGStreamerVideoManager()
	{
		CVideoManager* videoManager = new CVideoManager;
		return VideoManagerPtr(videoManager, true);
	}



} // end namespace fb


