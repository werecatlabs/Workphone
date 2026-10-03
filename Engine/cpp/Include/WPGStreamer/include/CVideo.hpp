#ifndef CVideo_h__
#define CVideo_h__



#include "video/IVideo.hpp"
#include "io/IStream.hpp"
#include "system/IStateListener.hpp"



typedef struct _GstElement GstElement;
typedef struct _GstAppSrc GstAppSrc;
typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;



namespace fb
{



	//-----------------------------------------------------------------------------------------------------------------------------------
	class CVideo : public IVideo
	{
	public:
		CVideo();
		CVideo(u32 id);
		~CVideo();

		void initialise(const stringc& fileName);

		VideoTexturePtr getVideoTexture() const;

		bool getAutoUpdate() const;

		void update( u32 taskId, f64 t, f64 dt );

		void setLoop( bool loop );

		void stop();

		void setVideoTexture( VideoTexturePtr videoTexture );

		u32 getId() const;

		vector2i getSize() const;
		void setSize(const vector2i& size);

		void setAutoUpdate( bool autoUpdate );

		bool getLoop() const;

		void _getObject( void** object );

		void* getCurrentFrameBuffer() const;

		void play();

		StreamPtr& getStream();
		const StreamPtr& getStream() const;
		void setStream(StreamPtr stream) { m_stream = stream; }

		u32 getSourceId() const { return m_sourceId; }
		void setSourceId(u32 sourceId) { m_sourceId = sourceId; }

		u32 getNextUpdateTime() const { return m_nextUpdateTime; }
		void setNextUpdateTime(u32 nextUpdateTime) { m_nextUpdateTime = nextUpdateTime; }

		u32 getLength() const { return m_length; }
		void setLength(u32 length) { m_length = length; }

		GstAppSrc* getAppsrc() const { return m_appsrc; }
		void setAppsrc(GstAppSrc* appsrc) { m_appsrc = appsrc; }

		GstElement* getPlayer() const { return mPlayer; }
		void setPlayer(GstElement* player) { mPlayer = player; }

		GstElement* getAppSink() const { return mAppSink; }
		void setAppSink(GstElement* appSink) { mAppSink = appSink; }

		StateObjectPtr& getStateObject() { return m_stateObject; }
		const StateObjectPtr& getStateObject() const { return m_stateObject; }

	protected:
		class VideoStateListener : public IStateListener
		{
		public:
			VideoStateListener(CVideo* video) : m_video(video), m_taskId(0) {}
			~VideoStateListener(){}
			
			void OnStateChanged( const StateMessagePtr& message );
			u32 getId() const { return 0; }
			u32 getTaskNotifyId() const { return m_taskId; } 
			void setTaskNotifyId( u32 taskId ) { m_taskId = taskId; }

		protected:
			CVideo* m_video;
			u32 m_taskId;
		};

		void writeToBuffer(BYTE * pSampleBuffer, int size);
		LONG calculateNearest2Pow(LONG input);

		StreamPtr m_stream;

		VideoTexturePtr m_videoTexture;
		vector2i m_size;
		u32 m_id;
		bool m_loop;
		u32 m_nextUpdateTime;
		u32 m_sourceId;

		BOOL m_bFlipVertical;
		LONG m_lVidWidth;   // Video width
		LONG m_lVidHeight;  // Video Height
		LONG m_lVidPitch;   // Video Pitch

		u32 m_length;

		GstAppSrc* m_appsrc;

		GstElement* mPlayer;
		GstElement* mAppSink;
		GMainContext *ctx;
		GMainLoop* main_loop;

		BYTE* m_buffer;

		StateObjectPtr m_stateObject;
	};



	typedef SmartPtr<CVideo> CVideoPtr;



} // end namespace fb



#endif // CVideo_h__