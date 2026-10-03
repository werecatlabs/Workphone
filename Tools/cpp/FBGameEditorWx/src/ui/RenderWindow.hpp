#ifndef __RenderWindow_h__
#define __RenderWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxWidgets.hpp>
#include <wx/wx.hpp>



namespace fb
{
	namespace editor
	{



		//---------------------------------------------------------------------
		class RenderWindow : public wxScrolledWindow
		{
		public:
			RenderWindow(wxWindow* parent, wxWindowID id);
			~RenderWindow();

			ui::wxViewWindow* getWxWindow() const;

			SmartPtr<render::ICamera> getCamera() const;
			void setCamera(SmartPtr<render::ICamera> val);

			SmartPtr<render::ISceneNode> getCameraNode() const;
			void setCameraNode(SmartPtr<render::ISceneNode> val);

			SmartPtr<render::IWindow> getWindow() const;
			void setWindow(SmartPtr<render::IWindow> val);

		protected:
			void initialiseRenderWindows();

			ui::wxViewWindow* m_viewWindow = nullptr;

			SmartPtr<render::ICamera> m_camera;
			SmartPtr<render::ISceneNode> m_cameraNode;
			SmartPtr<render::IViewport> m_viewport;
			SmartPtr<render::IWindow> m_window;
		};



	} // end namespace editor	
} // end namespace fb



#endif // GUIRenderWindow_h__


