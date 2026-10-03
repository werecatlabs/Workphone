#include "CVideoManager.hpp"
#include "CVideo.hpp"
#include <Workphone.hpp>
#include <video\IVideoStream.hpp>

#include <gst/gst.hpp>
#include <gst/app/gstappsink.hpp>



namespace fb
{



	//-------------------------------------------------------------------------------------------------------------------------------
	CVideoManager::CVideoManager()
	{
		GError *error;
		gboolean bInit = gst_init_check(NULL, NULL, &error);
		if(!bInit)
		{
			LOG_MESSAGE("Video", "Could not initialise video manager. ");
		}

		FBSystem* fbSystem = FBSystem::getSingletonPtr();
		FileSystemPtr fileSystem = fbSystem->getFileSystem();
		stringc workingDir = fileSystem->getWorkingDirectory();
		stringc pluginDir = workingDir + stringc("/plugins");
		GstRegistry* registry = gst_registry_get_default();
		//gst_registry_add_path(registry, "./plugins");
		gboolean ret = gst_registry_scan_path(registry, pluginDir.c_str());

				// I'm not entirely sure how threading works in GStreamer,
		// I just hope that it does :P.
		if (!g_thread_supported())
		{
			g_thread_init(0);
		}

		ctx = g_main_context_get_thread_default ();
	}



	//-------------------------------------------------------------------------------------------------------------------------------
	CVideoManager::~CVideoManager()
	{
	}
	


	//-------------------------------------------------------------------------------------------------------------------------------
	void CVideoManager::update( u32 taskId, f64 t, f64 dt )
	{
		if(taskId == TI_GraphicsTaskId)
		{
			g_main_context_iteration (ctx, FALSE);

			Videos::iterator it = m_videos.begin();
			for(; it!=m_videos.end(); ++it)
			{
				it->second->update(taskId, t, dt);
			}
		}
	}



	//-------------------------------------------------------------------------------------------------------------------------------
	VideoPtr CVideoManager::getVideoById( u32 id ) const
	{
		Videos::const_iterator it = m_videos.find(id);
		if(it != m_videos.end())
		{
			return it->second;
		}

		return VideoPtr::NULL_PTR;
	}



	//-------------------------------------------------------------------------------------------------------------------------------
	VideoPtr CVideoManager::addVideo( u32 id, const stringc& fileName )
	{
		CVideoPtr video(new CVideo(id), true);
		video->initialise(fileName);
		m_videos[video->getId()] = video;

		return video;
	}



	//-------------------------------------------------------------------------------------------------------------------------------
	VideoPtr CVideoManager::addVideo( const stringc& fileName )
	{
		CVideoPtr video(new CVideo, true);
		video->initialise(fileName);
		m_videos[video->getId()] = video;

		return video;
	}



	//-------------------------------------------------------------------------------------------------------------------------------
	VideoTexturePtr CVideoManager::createVideoTexture( const stringc& textureName )
	{
		FBSystem* fbSystem = FBSystem::getSingletonPtr();
		GraphicsSystemPtr gfxSystem = fbSystem->getGraphicsSystem();
		TextureManagerPtr textureManager = gfxSystem->getTextureManager();
		
		VideoTexturePtr videoTexture = textureManager->createVideoTexture(textureName);
		return videoTexture;
	}

	VideoStreamPtr CVideoManager::createVideoStream() const
	{
		return VideoStreamPtr::NULL_PTR;
	}

	void CVideoManager::startCapture()
	{
		throw std::exception("The method or operation is not implemented.");
	}

	void CVideoManager::stopCapture()
	{
		throw std::exception("The method or operation is not implemented.");
	}

	void CVideoManager::setOutputFilePath( const stringc& filePath )
	{
		throw std::exception("The method or operation is not implemented.");
	}

	bool CVideoManager::isCapturing() const
	{
		throw std::exception("The method or operation is not implemented.");
	}

	fb::stringc CVideoManager::getOutputFilePath() const
	{
		throw std::exception("The method or operation is not implemented.");
	}

	bool CVideoManager::removeVideoTexture( VideoTexturePtr videoTexture )
	{
		return false;
	}

	bool CVideoManager::removeVideoTexture( const stringc& textureName )
	{
		return false;
	}



} // end namespace fb
