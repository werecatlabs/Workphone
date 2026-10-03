#ifndef CVideoManager_h__
#define CVideoManager_h__



#include <video/IVideoManager.hpp>
#include <unordered_map>



typedef struct _GMainContext GMainContext;



namespace fb
{

	

	//---------------------------------------------------------------------------------------------------
	class CVideoManager : public IVideoManager
	{
	public:	
		CVideoManager();
		~CVideoManager();

		void update( u32 taskId, f64 t, f64 dt );

		VideoPtr getVideoById( u32 id ) const;

		VideoPtr addVideo( const stringc& fileName );
		VideoPtr addVideo( u32 id, const stringc& fileName );

		VideoTexturePtr createVideoTexture(const stringc& textureName);
		bool removeVideoTexture(VideoTexturePtr videoTexture);
		bool removeVideoTexture(const stringc& textureName);

		VideoStreamPtr createVideoStream() const;

		virtual void startCapture();

		virtual void stopCapture();

		virtual void setOutputFilePath( const stringc& filePath );

		virtual bool isCapturing() const;

		virtual stringc getOutputFilePath() const;

	protected:
		typedef stdext::hash_map<u32, VideoPtr> Videos;
		Videos m_videos;

		GMainContext *ctx;
	};



} // end namespace fb



#endif // CVideoManager_h__


