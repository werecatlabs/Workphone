#ifndef ObjectWindow_h__
#define ObjectWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <wx/dialog.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class ObjectWindow : public CSharedObject<ISharedObject>
		{
		public:
			enum
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};

			ObjectWindow();
			ObjectWindow(wxWindow* parent);
			~ObjectWindow();

			void load(SmartPtr<ISharedObject> data) override;

			wxWindow* getParent() const;
			void setParent(wxWindow* parent);

			wxWindow* getWindow() const;
			void setWindow(wxWindow* parent);

		protected:
			wxWindow* m_parent = nullptr;
			wxWindow* m_window = nullptr;
			SmartPtr<ActorWindow> m_actorWindow;
			SmartPtr<PropertiesWindow> m_propertiesWindow;
		};



	} // end namespace editor
} // end namespace fb



#endif // ObjectWindow_h__


