#include <GameEditorPCH.hpp>
#include "ui/NewComponentDialog.hpp"
#include "ui/NewPropertyDialog.hpp"
#include "ui/NewEventDialog.hpp"

#include "editor/EditorManager.hpp"
#include "editor/EditorMessages.hpp"

#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBObjectTemplates/ScriptTemplate.hpp>

#include <FBCore/FBCoreHeaders.hpp>


#define ID_BTN_ADD_PROPERTY wxID_HIGHEST + 1
#define ID_BTN_REMOVE_PROPERTY ID_BTN_ADD_PROPERTY + 1
#define ID_BTN_ADD_EVENT ID_BTN_REMOVE_PROPERTY + 1
#define ID_BTN_REMOVE_EVENT ID_BTN_ADD_EVENT + 1
#define ID_LIST_EVENTS ID_BTN_REMOVE_EVENT + 1
#define ID_POPMENU_ADD_PROPERTY ID_LIST_EVENTS + 1
#define ID_POPMENU_REMOVE_PROPERTY ID_POPMENU_ADD_PROPERTY + 1
#define ID_POPMENU_ADD_EVENT ID_POPMENU_ADD_PROPERTY + 1
#define ID_POPMENU_REMOVE_EVENT ID_POPMENU_ADD_EVENT + 1


BEGIN_EVENT_TABLE(fb::editor::EditComponentDialog, wxDialog)  
	EVT_PG_RIGHT_CLICK(wxID_ANY,EditComponentDialog::OnPropertyGridItemRightClick )
END_EVENT_TABLE()


namespace fb
{
	namespace editor
	{
	
	
	
		EditComponentDialog::EditComponentDialog( wxWindow *parent, bool edit )
			: 
			wxDialog(parent, wxID_ANY, _T("Add/Modify Component"),
	               wxDefaultPosition, wxSize(-1, 400),
	               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			m_lastSelectedComponentTemplate = nullptr;
	
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			wxBoxSizer *compDataBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(compDataBox, 1, wxEXPAND);
	
			wxBoxSizer *labelBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(labelBox, 0, wxEXPAND);
			m_labelTxt = new wxTextCtrl(this, -1);
			labelBox->Add( new wxStaticText( this, -1, wxT("Label:") ));
			labelBox->Add(m_labelTxt, 1, wxEXPAND);
	
			wxBoxSizer *typeBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(typeBox, 0, wxEXPAND);
			m_typeTxt = new wxTextCtrl(this, -1);
			typeBox->Add( new wxStaticText( this, -1, wxT("Type:") ));
			typeBox->Add(m_typeTxt, 1, wxEXPAND);
	
			wxBoxSizer *propertiesBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(propertiesBox, 1, wxEXPAND);
			wxBoxSizer *propBox = new wxBoxSizer( wxHORIZONTAL );
			propBox->Add( new wxStaticText( this, -1, wxT("Properties") ));
			propBox->Add( new wxButton(this, ID_BTN_ADD_PROPERTY, "Add new property"));
			propBox->Add( new wxButton(this, ID_BTN_REMOVE_PROPERTY, "Remove property"));
			propertiesBox->Add(propBox);
			m_propertiesWindow = new PropertiesWindow(this, -1);
			propertiesBox->Add(m_propertiesWindow->getWindow(), 1, wxEXPAND);
	
			wxBoxSizer *eventsBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(eventsBox, 1, wxEXPAND);
			wxBoxSizer *evBox = new wxBoxSizer( wxHORIZONTAL );
			evBox->Add( new wxStaticText( this, -1, wxT("Events") ));
			evBox->Add( new wxButton(this, ID_BTN_ADD_EVENT, "Add/Edit event"));
			evBox->Add( new wxButton(this, ID_BTN_REMOVE_EVENT, "Remove event"));
			eventsBox->Add(evBox);
			m_events = new wxListBox(this, ID_LIST_EVENTS);
			eventsBox->Add(m_events, 1, wxEXPAND);
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Save");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
	
			Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, 
				wxCommandEventHandler(EditComponentDialog::onSelect));
	
			Connect(ID_BTN_ADD_PROPERTY, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(EditComponentDialog::onAddNewProperty));
	
			Connect(ID_BTN_REMOVE_PROPERTY, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(EditComponentDialog::onRemoveProperty));
	
			Connect(ID_BTN_ADD_EVENT, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(EditComponentDialog::onAddNewEvent));
	
			Connect(ID_BTN_REMOVE_EVENT, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(EditComponentDialog::onRemoveEvent));
	
			Connect(ID_LIST_EVENTS, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, 
				wxCommandEventHandler(EditComponentDialog::onEventListDblCLick));
	
			m_events->Connect(wxEVT_RIGHT_DOWN,
				wxMouseEventHandler(EditComponentDialog::onListboxRightMouseBtnUp), NULL, this);
	
			Connect(ID_POPMENU_ADD_PROPERTY, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(EditComponentDialog::onPopMenuAddProperty));
	
			Connect(ID_POPMENU_REMOVE_PROPERTY, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(EditComponentDialog::onPopMenuRemoveProperty));
	
			Connect(ID_POPMENU_ADD_EVENT, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(EditComponentDialog::onPopMenuAddEvent));
	
			Connect(ID_POPMENU_REMOVE_EVENT, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(EditComponentDialog::onPopMenuRemoveEvent));
	
	
			// add existing components in combo box
			if(!edit)
			{
				m_componentTemplate = SmartPtr<ComponentTemplate>(new ComponentTemplate);
				m_componentTemplate->setLabel("New Component");
				m_componentTemplate->setName("NewComponent");
				m_componentTemplate->setType("NewComponent");
				populateWithComponentTemplate( m_componentTemplate );
			}
		}
	
	
	
	
		EditComponentDialog::~EditComponentDialog()
		{
			if(m_propertiesWindow)
				delete m_propertiesWindow;
			if(m_events)
				delete m_events;
			if(m_labelTxt)
				delete m_labelTxt; 
			if(m_typeTxt)
				delete m_typeTxt;
			if(m_cancelBtn)
				delete m_cancelBtn;
			if(m_okBtn)
				delete m_okBtn;
		}
	
	
	
	
		void EditComponentDialog::onAddNewProperty(wxCommandEvent& event) 
		{
			EditPropertyDialog dlg(this);
			if(dlg.ShowModal() == wxID_OK)
			{
				Property customProperty;
				customProperty.setName(dlg.getName());
				customProperty.setType(dlg.getType());
				customProperty.setLabel(dlg.getLabel());
				customProperty.setValue(dlg.getValue());
				customProperty.setReadOnly(dlg.getReadOnly());
	
				if(m_lastSelectedComponentTemplate)
				{
					// get property group
					Properties propertyGroup;
					m_lastSelectedComponentTemplate->getProperties( propertyGroup );
					propertyGroup.addProperty(customProperty);
					m_lastSelectedComponentTemplate->setProperties( propertyGroup );
	
					// show properties
					ComponentItemSelectedPtr msg(new ComponentItemSelected);
					SmartPtr<IEditableObject> obj;// = (IEditableObject*)m_lastSelectedComponentTemplate.get();
					msg->setSelectedObject(obj);
					m_propertiesWindow->handleMessage(msg);
				}
			}
		}
	
	
	
	
		void EditComponentDialog::onRemoveProperty(wxCommandEvent& event) 
		{
			if(m_lastSelectedComponentTemplate)
			{
				// delete current selected property
				ComponentDeleteCurrentPropertyPtr msg(new ComponentDeleteCurrentProperty);
				SmartPtr<IEditableObject> obj;// = (IEditableObject*)m_lastSelectedComponentTemplate.get();
				msg->setSelectedObject(obj);
				m_propertiesWindow->handleMessage(msg);
			}
		}
	
	
		void EditComponentDialog::onAddNewEvent(wxCommandEvent& event) 
		{
			EditEventDialog dlg(this);
	
			if(m_lastSelectedComponentTemplate)
			{
				// get selected line
				String eventName = String(m_events->GetStringSelection());
				if(eventName != "")
				{
					const Array<SmartPtr<EventTemplate>>& eventsOriginal = m_lastSelectedComponentTemplate->getEventTemplates();
					Array<SmartPtr<EventTemplate>> events = eventsOriginal;
					for(u32 eventIdx=0; eventIdx<events.size(); ++eventIdx)
					{
						const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
						if(eventName == eventTemplate->getLabel())
						{
							dlg.setClassValue(eventTemplate->getClassName());
							dlg.setLabelValue(eventTemplate->getLabel());
							dlg.setFunctionValue(eventTemplate->getFunction());
							dlg.setTypeValue(eventTemplate->getType());
						}
					}
				}
			}
	
			if(dlg.ShowModal() == wxID_OK)
			{
				if(dlg.getLabel() != "")
				{
					SmartPtr<EventTemplate> eventTemplate(new EventTemplate);
					eventTemplate->setClassName(dlg.getClass());
					eventTemplate->setType(dlg.getType());
					eventTemplate->setLabel(dlg.getLabel());
					eventTemplate->setFunction(dlg.getFunction());
	
					if(m_lastSelectedComponentTemplate)
					{
	
						// if we already have the event we delete it before added again
						if(m_lastSelectedComponentTemplate->hasEventTemplate(dlg.getLabel()))
							m_lastSelectedComponentTemplate->removeEventTemplate(dlg.getLabel());
	
						// add event to component
						m_lastSelectedComponentTemplate->addEventTemplate( eventTemplate);
	
						// refresh list
						m_events->Clear();
						const Array<SmartPtr<EventTemplate>>& events = m_lastSelectedComponentTemplate->getEventTemplates();
						for(u32 eventIdx=0; eventIdx<events.size(); ++eventIdx)
						{
							const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
							m_events->Append(eventTemplate->getLabel().c_str());
						}
					}
				}
			}
		}
	
	
	
		void EditComponentDialog::RemoveEvent(String eventName)
		{
			if(m_lastSelectedComponentTemplate)
			{
				/*const Array<SmartPtr<EventTemplate>>& eventsOriginal = m_lastSelectedComponentTemplate->getEventTemplates();
				Array<SmartPtr<EventTemplate>> events = eventsOriginal;
				for(u32 eventIdx=0; eventIdx<events.size(); ++eventIdx)
				{
					const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
					if(eventName == eventTemplate->getLabel())
					{
						events.erase(eventIdx);
						m_lastSelectedComponentTemplate->setEventTemplates(events);
						break;
					}
				}*/
	
				m_lastSelectedComponentTemplate->removeEventTemplate(eventName);
			}
		}
	
	
		void EditComponentDialog::onRemoveEvent(wxCommandEvent& event) 
		{
			if(m_lastSelectedComponentTemplate)
			{
				// get selected line
				String eventName = String(m_events->GetStringSelection());
				if(eventName != "")
				{
					RemoveEvent(eventName);
	
					// refresh list
					m_events->Clear();
					const Array<SmartPtr<EventTemplate>>& events = m_lastSelectedComponentTemplate->getEventTemplates();
					for(u32 eventIdx=0; eventIdx<events.size(); ++eventIdx)
					{
						const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
						m_events->Append(eventTemplate->getLabel().c_str());
					}
				}
			}
		}
	
	
	
		void EditComponentDialog::onEventListDblCLick(wxCommandEvent& event) 
		{
			onAddNewEvent(event);
		}
	
	
	
		void EditComponentDialog::onSelect(wxCommandEvent& event) 
		{
	
		}
	
	
		void EditComponentDialog::onPopMenuAddProperty(wxCommandEvent& WXUNUSED(event))
		{
			wxCommandEvent event;
			onAddNewProperty(event);
		}
	
		void EditComponentDialog::onPopMenuRemoveProperty(wxCommandEvent& WXUNUSED(event))
		{
			wxCommandEvent event;
			onRemoveProperty(event);
		}
	
		void EditComponentDialog::OnPropertyGridItemRightClick( wxPropertyGridEvent& event )
		{
			wxMenu *menu;
			wxPoint point;
	
			// get mouse position
			point.x = wxGetMousePosition().x;
			point.y = wxGetMousePosition().y;
			point = this->ScreenToClient(point);
			
			// to get a window location if required use
			//int id = wxFindWindowAtPoint(point);
	
			// create menu 
			menu = new wxMenu();
	
			// add stuff
			menu->Append(ID_POPMENU_ADD_PROPERTY, wxT("Add Property"), wxT(""));
			menu->Append(ID_POPMENU_REMOVE_PROPERTY, wxT("Remove Property"), wxT(""));
			menu->AppendSeparator();
			menu->Append(-1, wxT("Cancel"), wxT(""));
	
			// and then display
			PopupMenu(menu, point.x, point.y);
	
			if(menu)
				delete menu;
		}
	
	
	
		fb::String EditComponentDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}
	
	
	
		fb::String EditComponentDialog::getType() const
		{
			return String(m_typeTxt->GetValue().c_str());
		}
	
		void EditComponentDialog::populateComponentCombo()
		{
		}
	
	
		void EditComponentDialog::onListboxRightMouseBtnUp(wxMouseEvent& event)
		{
			int item = m_events->HitTest(event.GetPosition());
	
			// select item or reset selection
			if(item != wxNOT_FOUND)
				m_events->Select(item);
			else
				m_events->Select(-1);
	
			event.Skip();
	
			wxMenu *menu;
			wxPoint point;
	
			// get mouse position
			point.x = wxGetMousePosition().x;
			point.y = wxGetMousePosition().y;
			point = this->ScreenToClient(point);
	
			// to get a window location if required use
			//int id = wxFindWindowAtPoint(point);
	
			// create menu 
			menu = new wxMenu();
	
			// add stuff
			menu->Append(ID_POPMENU_ADD_EVENT, wxT("Add/Modify Event"), wxT(""));
			menu->Append(ID_POPMENU_REMOVE_EVENT, wxT("Remove Event"), wxT(""));
			menu->AppendSeparator();
			menu->Append(-1, wxT("Cancel"), wxT(""));
	
			// and then display
			PopupMenu(menu, point.x, point.y);
	
			if(menu)
				delete menu;
		}
	
		void EditComponentDialog::onPopMenuAddEvent(wxCommandEvent& WXUNUSED(event))
		{
			wxCommandEvent event;
			onAddNewEvent(event);
		}
	
		void EditComponentDialog::onPopMenuRemoveEvent(wxCommandEvent& WXUNUSED(event))
		{
			wxCommandEvent event;
			onRemoveEvent(event);
		}
	
		void EditComponentDialog::populateWithComponentTemplate( SmartPtr<ComponentTemplate> &componentTemplate )
		{
			m_lastSelectedComponentTemplate = componentTemplate;
	
			String componentType = componentTemplate->getType();
			String componentLabel = componentTemplate->getLabel();
	
			// show properties
			ComponentItemSelectedPtr msg(new ComponentItemSelected);
			SmartPtr<IEditableObject> obj;// = (IEditableObject*)componentTemplate.get();
			msg->setSelectedObject(obj);
			m_propertiesWindow->handleMessage(msg);
	
			// set the label text control
			m_labelTxt->SetValue(componentLabel.c_str());
	
			// set the type text control
			m_typeTxt->SetValue(componentType.c_str());
	
			// clear events list
			m_events->Clear();
			// add events in events list
			const Array<SmartPtr<EventTemplate>>& events = componentTemplate->getEventTemplates();
			for(u32 eventIdx=0; eventIdx<events.size(); ++eventIdx)
			{
				const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
				m_events->Append(eventTemplate->getLabel().c_str());
			}
		}
	
		SmartPtr<ComponentTemplate> EditComponentDialog::getComponentTemplate() const
		{
			return m_componentTemplate;
		}
	
		void EditComponentDialog::setComponentTemplate( SmartPtr<ComponentTemplate> val )
		{
			m_componentTemplate = val;
			populateWithComponentTemplate( m_componentTemplate );
		}
	
	
	
	} // end namespace editor
	
	
}
