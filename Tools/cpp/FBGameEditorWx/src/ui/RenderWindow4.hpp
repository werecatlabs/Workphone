#ifndef __RenderWindow4_h__
#define __RenderWindow4_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxWidgets.hpp>
#include <wx/wx.hpp>




namespace fb
{	
	namespace editor
	{
		
	
	
		//---------------------------------------------------------------------
		class RenderWindow4 : public wxScrolledWindow
		{
		public:
			RenderWindow4(wxWindow* parent, wxWindowID id);
			~RenderWindow4();

			ui::wxViewWindow* getWindow(u32 index) const;

			SmartPtr<render::ICamera> getCamera() const;
			void setCamera(SmartPtr<render::ICamera> val);

			SmartPtr<render::ISceneNode> getCameraNode() const;
			void setCameraNode(SmartPtr<render::ISceneNode> val);

			SmartPtr<render::IWindow> getWindow() const;
			void setWindow(SmartPtr<render::IWindow> val);
			
		protected:
			void initialiseRenderWindows();
	
			ui::wxFourWaySplitter* mSplitter;
			ui::wxViewWindow* mViewWindows[4];

			SmartPtr<render::ICamera> m_camera;
			SmartPtr<render::ISceneNode> m_cameraNode;
			SmartPtr<render::IViewport> m_viewport;
			SmartPtr<render::IWindow> m_window;

		};
	
	
	
	} // end namespace editor	
} // end namespace fb



#endif // __RenderWindow4_h__


