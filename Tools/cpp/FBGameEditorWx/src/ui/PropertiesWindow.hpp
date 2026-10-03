#ifndef __PropertiesWindow_h__
#define __PropertiesWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <core/IMessageListener.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <wx/scrolwin.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class PropertiesWindow : public ui::wxApplicationWindow
		{
		public:
			enum
			{
				PropertiesId = wxID_HIGHEST,
			};
	
			PropertiesWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = 0, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl");
			~PropertiesWindow();

			void update();

			void objectSelected();
	
			void handleMessage(SmartPtr<IMessage> message);

			void actorSelected(SmartPtr<IEditableObject> editable);

			void updateSelection();

			bool isDirty() const;
			void setDirty(bool val);

		protected:
			class MessageListener : public CSharedObject<IMessageListener>
			{
			public:
				MessageListener(PropertiesWindow* propertiesWindow);
				~MessageListener();

				void handleMessage(SmartPtr<IMessage> message);

				PropertiesWindow* m_propertiesWindow;
			};

			void propertyChange(wxPropertyGridEvent& event);
	
			IMessageListener* m_messageListener = nullptr;
			wxPropertyGrid*	m_pg = nullptr;
	
			wxButton*	m_batchAll = nullptr;
			wxButton*	m_batchMaterials = nullptr;
			wxButton*	m_batchScene = nullptr;
	
			SmartPtr<IEditableObject> m_selectedEditable;

			bool m_isDirty = false;
	
			DECLARE_EVENT_TABLE()
		};
	
	
	
	} // end namespace editor
	
}


#endif // PropertiesWindow_h__


