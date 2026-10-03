#include "CVideo.hpp"
#include <Workphone.hpp>

#include <iostream>

#include <gst/gst.hpp>
#include <gst/app/gstappsrc.hpp>
#include <gst/app/gstappbuffer.hpp>
#include <gst/app/gstappsink.hpp>
#include <gst/video/video.hpp>

#include <messages\StateMessagePlay.hpp>
#include <messages\StateMessageStop.hpp>



GST_DEBUG_CATEGORY (appsrc_playbin_debug);
#define GST_CAT_DEFAULT appsrc_playbin_debug



namespace fb
{



	//--------------------------------------------------------------------------
	void eos(GstAppSink *sink, gpointer user_data)
	{
	}



	//--------------------------------------------------------------------------
	GstFlowReturn new_preroll(GstAppSink *sink, gpointer user_data)
	{
		return GST_FLOW_OK;
	}



	//--------------------------------------------------------------------------
	GstFlowReturn new_buffer(GstAppSink *sink, gpointer user_data)
	{
		return GST_FLOW_OK;
	}



	//--------------------------------------------------------------------------
	GstFlowReturn new_buffer_list(GstAppSink *sink, gpointer user_data)
	{
		return GST_FLOW_OK;
	}



	//--------------------------------------------------------------------------
	static void need_data (GstAppSrc *src, guint size, gpointer user_data) 
	{ 
		CVideo* video = (CVideo*)user_data;

		GstFlowReturn ret; 
		GstBuffer *buffer = gst_buffer_try_new_and_alloc (size); 
		if(buffer)
		{
			video->getStream()->read(GST_BUFFER_DATA (buffer), size);  

			GstAppSrc* appsrc = video->getAppsrc();
			g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret); 
		}
	}



	//--------------------------------------------------------------------------
	static void enough_data (GstAppSrc *src, gpointer user_data) 
	{ 
		CVideo* video = (CVideo*)user_data;

		int halt = 0;
		halt = 0;
	}



	//--------------------------------------------------------------------------
	gboolean seek_data(GstAppSrc *src, guint64 offset, gpointer user_data)
	{
		CVideo* video = (CVideo*)user_data;
		video->getStream()->seek(offset);
		return true;
	}



	//--------------------------------------------------------------------------
	void destroyNotify(gpointer user_data)
	{
	}



	//--------------------------------------------------------------------------
	static void found_source (GObject * object, GObject * orig, GParamSpec * pspec, CVideo* video) 
	{ 
		GstAppSrc* appsrc = nullptr;

		/* get a handle to the appsrc */ 
		g_object_get (orig, pspec->name, &appsrc, NULL); 

		/* get the appsrc */
		//appsrc = GST_APP_SRC( gst_bin_get_by_name (GST_BIN(app->pipeline), "mysource") );
		gst_app_src_set_size(appsrc, G_MAXINT64);
		//gst_app_src_set_max_bytes (appsrc, framesize);

		video->setAppsrc(appsrc);

		GST_DEBUG ("got appsrc %p", appsrc); 

		GstAppSrcCallbacks appSrcCallbacks;
		appSrcCallbacks.need_data = need_data;
		appSrcCallbacks.enough_data = enough_data;
		appSrcCallbacks.seek_data = seek_data;
		gst_app_src_set_callbacks(appsrc, &appSrcCallbacks, video, destroyNotify);

		gst_app_src_set_stream_type (appsrc, GST_APP_STREAM_TYPE_RANDOM_ACCESS);
	}



	//--------------------------------------------------------------------------
	static GstElement* getPlayer(gpointer userData)
	{
		return reinterpret_cast<GstElement*>(userData);
	}



	//--------------------------------------------------------------------------
	static void* getUserData(GstElement* player)
	{
		return reinterpret_cast<void*>(player);
	}
	


	//--------------------------------------------------------------------------
	// Called by GStreamer when a new frame is available.
	// Data is processed within FrameListener::frameRenderingQueued.
	static GstFlowReturn onNewBuffer(GstAppSink *sink, gpointer userData)
	{
		CVideo* video = reinterpret_cast<CVideo*>(userData);
		u32 nextUpdateTime = video->getNextUpdateTime();
		video->setNextUpdateTime(++nextUpdateTime);

		return GST_FLOW_OK;
	}



	//--------------------------------------------------------------------------
	static gboolean onBusMessage(GstBus* bus, GstMessage* message, gpointer userData)
	{
		CVideo* video = (CVideo*)userData;
		GstElement* player = video->getPlayer();

		switch (GST_MESSAGE_TYPE(message))
		{
		case GST_MESSAGE_EOS:
			{
				if(video->getLoop())
				{
					if (!gst_element_seek(player, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 
						GST_SEEK_TYPE_SET, 0,  GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) 
					{
					}
				}
			}
			break;

		case GST_MESSAGE_ERROR:
			std::cout << "Error" << std::endl;
			gst_element_set_state(GST_ELEMENT(player), GST_STATE_NULL);
			break;

		default:
			break;
		}

		return true;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	CVideo::CVideo()
		: m_id(0)
	{
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	CVideo::CVideo( u32 id )
		: 
		m_id(id), 
		m_loop(false),
		mPlayer(NULL),
		mAppSink(NULL),
		m_nextUpdateTime(0), 
		m_sourceId(0)
	{
		FBSystem* fbSystem = FBSystem::getSingletonPtr();
		PlatformManagerPtr& platformManager = fbSystem->getPlatformManager();
		m_stateObject = platformManager->createStateObject();

		m_buffer = new BYTE[2048*2048*4];
		memset(m_buffer, 0, sizeof(m_buffer));
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	CVideo::~CVideo()
	{
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::initialise( const stringc& fileName )
	{
		FBSystem* fbSystem = FBSystem::getSingletonPtr();
		FileSystemPtr fileSystem = fbSystem->getFileSystem();
		m_stream = fileSystem->open(fileName);
		if(!m_stream)
		{
			LOG_MESSAGE("Video", stringc("Could not open file: ") + fileName);
			return;
		}

		ctx = g_main_context_get_thread_default ();

		setLength(m_stream->getSize());

		GST_DEBUG_CATEGORY_INIT (appsrc_playbin_debug, "appsrc-playbin", 0, "appsrc playbin example");

		// playbin2 is an all-in-one audio and video player.
		// It should be good enough for most uses.
		mPlayer = gst_element_factory_make("playbin2", "player");

		// Listen for messages on the playbin pipeline bus.
		GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(mPlayer));

		g_main_context_push_thread_default (ctx);
		gst_bus_add_watch(bus, (GstBusFunc)onBusMessage, this);
		g_main_context_pop_thread_default (ctx);

		//gst_object_unref(bus);

		// By default, playbin creates its own window to which it streams
		// video output.  We create an appsink element to allow video to be
		// streamed to an Ogre texture instead.
		mAppSink = gst_element_factory_make("appsink", "app_sink");

		// Set the appsink to emit signals (so we know when a new frame has
		// arrived), and to drop frames instead of buffer them (we want a
		// realtime video player).
		g_object_set(G_OBJECT(mAppSink), "emit-signals", true, NULL);
		g_object_set(G_OBJECT(mAppSink), "max-buffers", 1, NULL);
		g_object_set(G_OBJECT(mAppSink), "drop", true, NULL);

		// Listen for new-buffer signals.
		g_signal_connect(G_OBJECT(mAppSink), "new-buffer", G_CALLBACK(onNewBuffer), this);

		// Create a filter to produce simple rgb (actually, bgra) data.
		GstCaps* caps = gst_caps_new_simple("video/x-raw-rgb", 0);
		GstElement* rgbFilter = gst_element_factory_make("capsfilter", "rgb_filter");
		g_object_set(G_OBJECT(rgbFilter), "caps", caps, NULL);
		//gst_caps_unref(caps);

		// Create a bin to combine the rgb conversion with the appsink.
		GstElement* appBin = gst_bin_new("app_bin");

		// Add the filter to the bin, then attach a ghostpad to allow the
		// output of the filter to connect to the input of the appsink.
		gst_bin_add(GST_BIN(appBin), rgbFilter);
		GstPad* rgbSinkPad = gst_element_get_static_pad(rgbFilter, "sink");
		GstPad* ghostPad = gst_ghost_pad_new("app_bin_sink", rgbSinkPad);
		//gst_object_unref(rgbSinkPad);
		gst_element_add_pad(appBin, ghostPad);

		// Add the appsink to the bin.
		gst_bin_add_many(GST_BIN(appBin), mAppSink, NULL);
		gst_element_link_many(rgbFilter, mAppSink, NULL);

		GstAppSinkCallbacks callbacks;
		callbacks.eos = eos;
		callbacks.new_preroll = new_preroll;
		callbacks.new_buffer = new_buffer;
		callbacks.new_buffer_list = new_buffer_list;
		gst_app_sink_set_callbacks((GstAppSink*)mAppSink, &callbacks, this, destroyNotify);
		
		// Replace the default window sink with our appsink, and set the
		// media file to play.
		g_object_set(G_OBJECT(mPlayer), "video-sink", appBin, NULL);

		/* set to read from appsrc */ 
		g_object_set (mPlayer, "uri", "appsrc://", NULL);

		/* get notification when the source is created so that we get a handle to it 
		and can configure it */
		g_signal_connect (mPlayer, "deep-notify::source", (GCallback) found_source, this);

		//gst_element_set_state(GST_ELEMENT(mPlayer), GST_STATE_PLAYING);
		//gst_element_set_state(GST_ELEMENT(mPlayer), GST_STATE_PAUSED);

		m_bFlipVertical = TRUE;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	VideoTexturePtr CVideo::getVideoTexture() const
	{
		return m_videoTexture;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	bool CVideo::getAutoUpdate() const
	{
		return true;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::update( u32 taskId, f64 t, f64 dt )
	{
		//if(m_nextUpdateTime > 0)
		{
			// Get the new buffer.
			GstBuffer* buffer = nullptr;
			g_signal_emit_by_name(mAppSink, "pull-buffer", &buffer);

			if(buffer)
			{
				GstPad* pad = gst_element_get_static_pad(mAppSink, "sink");
				if(pad)
				{
					int width, height;
					if(gst_video_get_size(GST_PAD(pad), &width, &height))
					{
						m_size = vector2i(width, height);

						m_lVidHeight = m_size.Y();
						m_lVidWidth = m_size.X();
						m_lVidPitch = m_size.X()*4;
					}
				}

				void* pBuffer =  (void*)GST_BUFFER_DATA(buffer); 
				int size = GST_BUFFER_SIZE(buffer);
				writeToBuffer((BYTE*)pBuffer, size);

				m_videoTexture->_copyFrameData(m_buffer, getSize());

				gst_buffer_unref(buffer);
			}
		}
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	LONG CVideo::calculateNearest2Pow(LONG input)
	{
		if (input <= 32)   return 32;
		if (input <= 64)   return 64;
		if (input <= 128)  return 128;
		if (input <= 256)  return 256;
		if (input <= 512)  return 512;
		if (input <= 1024) return 1024;
		if (input <= 2048) return 2048;
		return input;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::writeToBuffer(BYTE * pSampleBuffer, int size)
	{
		LONG biSourceHeight = m_lVidHeight;//pviBmp->bmiHeader.biHeight;
		LONG biSourceWidth  = m_lVidWidth;//pviBmp->bmiHeader.biWidth;
		LONG biSourcePitch  = m_lVidPitch;//biWidth * 4;
		BOOL bFlip    = TRUE;
		LONG biTargetHeight = calculateNearest2Pow(biSourceHeight);
		LONG biTargetWidth  = calculateNearest2Pow(biSourceWidth);
		LONG biTargetPitch  = biTargetWidth*4;
		double vScale = biSourceHeight*1.0/biTargetHeight;
		double hScale = biSourceWidth*1.0/biTargetWidth;

		if (biSourceWidth < 0)
		{
			biSourceWidth = -biSourceWidth;
			biSourcePitch = -biSourcePitch;
			bFlip = FALSE;
		}

		BYTE * pTargetLine = m_buffer;
		BYTE * pSourceLine = (BYTE *)pSampleBuffer;

		if(m_bFlipVertical == TRUE)
			bFlip = !bFlip;

		if (bFlip)
		{
			pSourceLine = pSampleBuffer+biSourcePitch*(biSourceHeight-1);
		}

		int i;
		for (i=0; i<biTargetHeight; i++)
		{
			if (!bFlip)
				pSourceLine = pSampleBuffer + ((int)(i*vScale))*biSourcePitch;
			else
				pSourceLine = pSampleBuffer + biSourcePitch*(biSourceHeight-1) - ((int)(i*vScale))*biSourcePitch;

			pTargetLine = m_buffer + i*biTargetPitch;

#if 1
			for (int j=0; j<biTargetWidth; j++)
			{
				pTargetLine[j*4+0] = pSourceLine[((int)(j*hScale))*4+0]; //
				pTargetLine[j*4+1] = pSourceLine[((int)(j*hScale))*4+1]; //
				pTargetLine[j*4+2] = pSourceLine[((int)(j*hScale))*4+2]; //
				pTargetLine[j*4+3] = pSourceLine[((int)(j*hScale))*4+3]; //
			}
#else
			for (int j=0; j<biTargetWidth; j++)
			{
				SystemUtil::Memcpy(&pTargetLine[j*4], &pSourceLine[((int)(j*hScale))*4], sizeof(BYTE)*4);
			}
#endif

		}
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::setLoop( bool loop )
	{
		m_loop = loop;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::stop()
	{
		if(CURRENT_TASK_ID == TI_GraphicsTaskId)
		{
			gst_element_set_state(GST_ELEMENT(mPlayer), GST_STATE_PAUSED);
		}
		else
		{
			StateMessageStopPtr message(new StateMessageStop, true);
			m_stateObject->addMessage(TI_GraphicsTaskId, message);
		}
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::setVideoTexture( VideoTexturePtr videoTexture )
	{
		m_videoTexture = videoTexture;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	u32 CVideo::getId() const
	{
		return m_id;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	vector2i CVideo::getSize() const
	{
		return m_size;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::setSize(const vector2i& size)
	{
		m_size = size;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::setAutoUpdate( bool autoUpdate )
	{

	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	bool CVideo::getLoop() const
	{
		return m_loop;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::_getObject( void** object )
	{
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void* CVideo::getCurrentFrameBuffer() const
	{
		return NULL;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::play()
	{
		if(CURRENT_TASK_ID == TI_GraphicsTaskId)
		{
			gst_element_set_state(GST_ELEMENT(mPlayer), GST_STATE_PLAYING);
		}
		else
		{
			StateMessagePlayPtr message(new StateMessagePlay, true);
			m_stateObject->addMessage(TI_GraphicsTaskId, message);
		}
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	StreamPtr& CVideo::getStream()
	{
		return m_stream;
	}

	

	//-----------------------------------------------------------------------------------------------------------------------------------
	const StreamPtr& CVideo::getStream() const
	{
		return m_stream;
	}



	//-----------------------------------------------------------------------------------------------------------------------------------
	void CVideo::VideoStateListener::OnStateChanged( const StateMessagePtr& message )
	{
		if(message->isExactly(StateMessagePlay::TYPE_INFO))
		{
			m_video->play();
		}
		else if(message->isExactly(StateMessageStop::TYPE_INFO))
		{
			m_video->stop();
		}
	}



} // end namespace fb


