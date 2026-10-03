#include <GameEditorPCH.hpp>
#include "wxGUIUtil.hpp"

#include "PropGridCommon.hpp"


#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include "script/ScriptManager.hpp"
#include <wx/propgrid/advprops.hpp>
#include <wx/listctrl.hpp>

class wxEventProperty : public wxStringProperty
{
	WX_PG_DECLARE_PROPERTY_CLASS(wxEventProperty)
public:

	wxEventProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxString& value = wxString());
	~wxEventProperty();

	bool OnEvent(wxPropertyGrid* propgrid,
		wxWindow* primary, wxEvent& event);

protected:
};

WX_PG_IMPLEMENT_PROPERTY_CLASS(wxEventProperty, wxStringProperty,
	wxString, const wxString&, TextCtrlAndButton);

wxEventProperty::wxEventProperty(const wxString& label,
	const wxString& name,
	const wxString& value)
	: wxStringProperty(label, name, value)
{
}

wxEventProperty::~wxEventProperty()
{
}

bool wxEventProperty::OnEvent(wxPropertyGrid* propgrid,
	wxWindow* primary, wxEvent& event)
{
	if (propgrid->IsMainButtonEvent(event))
	{
		fb::editor::EditorManager* appRoot = fb::editor::EditorManager::getSingletonPtr();
		fb::editor::EditorScriptManagerPtr scriptMgr = appRoot->getScriptManager();
		scriptMgr->createEvent("test", "test");
		return true;
	}

	return false;
}



class CustomPGArrayStringEditorDialog : public wxPGArrayStringEditorDialog
{
public:
	void OnEndLabelEdit(wxListEvent& event)
	{
		/*
		wxString str = event.GetLabel();

		if ( m_itemPendingAtIndex >= 0 )
		{
			// Add a new item
			if ( ArrayInsert(str, m_itemPendingAtIndex) )
			{
				m_modified = true;
			}
			else
			{
				// Editable list box doesn't really respect Veto(), but
				// it recognizes if no text was added, so we simulate
				// Veto() using it.
				event.m_item.setText(wxEmptyString);
				m_elb->GetListCtrl()->setItemText(m_itemPendingAtIndex,
					wxEmptyString);

				event.Veto();
			}
		}
		else
		{
			// Change an existing item
			int index = GetSelection();
			wxASSERT( index != wxNOT_FOUND );
			if(ArrayGetCount() > index)
			{
				if ( Arrayset(index, str) )
					m_modified = true;
				else
					event.Veto();
			}
			else
			{
				if ( ArrayInsert(str, index) )
					m_modified = true;
				else
					event.Veto();
			}
		}
		*/

		event.Skip();
	}
};



// -----------------------------------------------------------------------
// Dirs Property
// -----------------------------------------------------------------------
//WX_PG_IMPLEMENT_ARRAYSTRING_PROPERTY_WITH_VALIDATOR(wxDirsProperty, ',', "Browse")
class wxDirsProperty : public wxArrayStringProperty
{
	WX_PG_DECLARE_PROPERTY_CLASS(wxDirsProperty)
public:

	wxDirsProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		wxArrayString wxArray = wxArrayString());
	~wxDirsProperty();

	bool OnEvent(wxPropertyGrid* propgrid,
		wxWindow* primary, wxEvent& event);

	bool OnCustomStringEdit(wxWindow* parent, wxString& value);

	void OnsetValue()
	{
		GenerateValueAsString();
	}

	bool OnButtonClick(wxPropertyGrid* propGrid,
		wxWindow* WXUNUSED(primaryCtrl),
		const wxChar* cbt)
	{
		// Update the value
		wxVariant useValue = propGrid->GetUncommittedPropertyValue();

		if (!propGrid->EditorValidate())
			return false;

		// Create editor dialog.
		wxPGArrayEditorDialog* dlg = CreateEditorDialog();
#if wxUSE_VALIDATORS
		//wxValidator* validator = GetValidator();
		//wxPGInDialogValidator dialogValidator;
#endif

		wxPGArrayStringEditorDialog* strEdDlg = wxDynamicCast(dlg, wxPGArrayStringEditorDialog);

		if (strEdDlg)
			strEdDlg->SetCustomButton(cbt, this);

		dlg->SetDialogValue(useValue);
		dlg->Create(propGrid, wxEmptyString, m_label);

#if !wxPG_SMALL_SCREEN
		dlg->Move(propGrid->GetGoodEditorDialogPosition(this, dlg->GetSize()));
#endif

		bool retVal;

		for (;;)
		{
			retVal = false;

			int res = dlg->ShowModal();

			if (res == wxID_OK && dlg->IsModified())
			{
				wxVariant value = dlg->GetDialogValue();
				if (!value.IsNull())
				{
					wxArrayString actualValue = value.GetArrayString();
					wxString tempStr;
					ConvertArrayToString(actualValue, &tempStr, m_delimiter);
#if wxUSE_VALIDATORS
					//if ( dialogValidator.DoValidate(propGrid, validator,
					//	tempStr) )
#endif
					{
						SetValueInEvent(actualValue);
						retVal = true;
						break;
					}
				}
				else
					break;
			}
			else
				break;
		}

		delete dlg;

		return retVal;
	}

protected:
};

WX_PG_IMPLEMENT_PROPERTY_CLASS(wxDirsProperty, wxArrayStringProperty,
	wxString, const wxString&, TextCtrlAndButton);


wxDirsProperty::wxDirsProperty(const wxString& label,
	const wxString& name,
	wxArrayString wxArray)
	: wxArrayStringProperty(label, name, wxArray)
{
}

wxDirsProperty::~wxDirsProperty()
{
}

bool wxDirsProperty::OnEvent(wxPropertyGrid* propgrid,
	wxWindow* primary, wxEvent& event)
{
	if (propgrid->IsMainButtonEvent(event))
	{
		try
		{
			wxButton* button = dynamic_cast<wxButton*>(primary);
			if (button)
				return OnButtonClick(propgrid, primary, (const wxChar*)"Browse");
		}
		catch (...)
		{
		}
	}

	return false;
}

bool wxDirsProperty::OnCustomStringEdit(wxWindow* parent, wxString& value)
{
	wxDirDialog dlg(parent,
		_("Select a directory to be added to the list:"),
		value,
		0);

	if (dlg.ShowModal() == wxID_OK)
	{
		fb::editor::ProjectPtr project = fb::editor::EditorManager::getSingletonPtr()->getProject();
		fb::String relPath = fb::String(dlg.GetPath().c_str());
		value = fb::Path::getRelativePath(project->getProjectDirectory(), relPath).c_str();
		return TRUE;
	}
	return FALSE;
}



void populateWxArray(const fb::String& dataString, wxArrayString& example_Array)
{
	using namespace fb;

	Array<String> list;
	StringUtil::parseArray(dataString, list);

	for (u32 elemIdx = 0; elemIdx < list.size(); ++elemIdx)
	{
		const String& element = list[elemIdx];
		example_Array.Add(element.c_str());
	}
}



namespace fb
{
	namespace editor
	{


		//--------------------------------------------
		void wxGUIUtil::populateProperties(const Properties& propertyGroup, wxPropertyGrid* pg)
		{
			if (pg)
			{
				pg->Clear();

				const auto& properties = propertyGroup.getPropertiesAsArray();
				for (u32 i = 0; i < properties.size(); ++i)
				{
					const auto& prop = properties[i];

					auto label = prop.getLabel();
					if (StringUtil::isNullOrEmpty(label))
					{
						label = prop.getName();
					}

					auto name = prop.getName();

					wxPGProperty* wxProperty = nullptr;

					const String& propertyType = prop.getType();

					if (propertyType == ("file"))
					{
						auto applicationManager = IApplicationManager::instance();

						auto propertyValue = prop.getValue();
						propertyValue = StringUtil::cleanupPath(propertyValue);

						auto fileProperty = new wxFileProperty(name.c_str(), wxPG_LABEL, propertyValue.c_str());

						auto initialPath = Path::getFilePath(propertyValue);
						if (StringUtil::isNullOrEmpty(initialPath))
						{
							initialPath = applicationManager->getProjectPath();
						}

						wxVariant initialPathVariant(initialPath.c_str());
						fileProperty->DoSetAttribute(wxPG_FILE_INITIAL_PATH, initialPathVariant);

						wxProperty = pg->Append(fileProperty);
					}
					else if (propertyType == ("folder"))
					{
						auto folder = StringUtil::cleanupPath(prop.getValue());
						wxProperty = pg->Append(new wxDirProperty(name.c_str(), wxPG_LABEL, folder.c_str()));
					}
					else if (propertyType == ("integer") || propertyType == ("int"))
					{
						int value = 0.0f;
						sscanf(prop.getValue().c_str(), "%i", &value);
						wxProperty = pg->Append(new wxIntProperty(name.c_str(), wxPG_LABEL, value));
					}
					else if (propertyType == ("float"))
					{
						f32 value = 0.0f;
						sscanf(prop.getValue().c_str(), "%f", &value);
						wxProperty = pg->Append(new wxFloatProperty(name.c_str(), wxPG_LABEL, value));
					}
					else if (propertyType == ("bool") || propertyType == ("boolean"))
					{
						bool value = true;
						if (prop.getValue() == ("false"))
						{
							value = false;
						}

						wxProperty = pg->Append(new wxBoolProperty(name.c_str(), wxPG_LABEL, value));
					}
					else if (propertyType == ("enum") || propertyType == ("choices"))
					{
						auto choiceStr = prop.getValue();
						auto dataString = prop.getAttribute("enum");

						Array<String> list;
						StringUtil::parseArray(dataString, list);

						u32 curChoiceIdx = 0;
						wxPGChoices choices;

						for (u32 elemIdx = 0; elemIdx < list.size(); ++elemIdx)
						{
							const String& element = list[elemIdx];
							choices.Add(element.c_str(), elemIdx);

							if (element == (choiceStr))
							{
								curChoiceIdx = elemIdx - 1;
							}
						}

						wxProperty = pg->Append(new wxEditEnumProperty(name.c_str(), wxPG_LABEL, choices, choiceStr.c_str()));
					}
					else if (propertyType == ("list") || propertyType == ("Array"))
					{
						wxArrayString wxArray;
						const String& dataString = prop.getValue();
						populateWxArray(dataString, wxArray);
						wxProperty = pg->Append(new wxArrayStringProperty(name.c_str(), wxPG_LABEL, wxArray));
					}
					else if (propertyType == ("dirs") || propertyType == ("folders"))
					{
						wxArrayString wxArray;
						const String& dataString = prop.getValue();

						Array<String> list;
						StringUtil::parseArray(dataString, list);

						for (u32 elemIdx = 0; elemIdx < list.size(); ++elemIdx)
						{
							auto element = list[elemIdx];
							element = StringUtil::cleanupPath(element);
							wxArray.Add(element.c_str());
						}

						wxProperty = pg->Append(new wxDirsProperty(name.c_str(), wxPG_LABEL, wxArray));
					}
					else if (propertyType == ("MultiChoice"))
					{
						Array<String> gfxObjects;
						Array<String> selectedGfxObjs;
						StringUtil::parseMultiChoice(prop.getValue(), gfxObjects, selectedGfxObjs);

						wxArrayString tchoices;
						for (u32 choiceIdx = 0; choiceIdx < gfxObjects.size(); ++choiceIdx)
							tchoices.Add(gfxObjects[choiceIdx].c_str());

						wxArrayString tchoicesValues;
						for (u32 choiceIdx = 0; choiceIdx < selectedGfxObjs.size(); ++choiceIdx)
							tchoicesValues.Add(selectedGfxObjs[choiceIdx].c_str());

						wxProperty = pg->Append(new wxMultiChoiceProperty(name.c_str(), wxPG_LABEL, tchoices, tchoicesValues));
					}
					else if (propertyType == ("vector2") || propertyType == ("Vector2F") || propertyType == ("vector2d"))
					{
						Vector2F vector;
						propertyGroup.getPropertyValue(prop.getName(), vector);
						wxVector3f value(vector.X(), vector.Y(), 0.0);
						wxProperty = pg->Append(new wxVectorProperty(name.c_str(), wxPG_LABEL, value));
					}
					else if (propertyType == ("vector3") || propertyType == ("Vector3F") || propertyType == ("vector3d"))
					{
						Vector3F vector = prop.getValueAsVector3f();
						wxVector3f wxVector(vector.X(), vector.Y(), vector.Z());
						wxProperty = pg->Append(new wxVectorProperty(name.c_str(), wxPG_LABEL, wxVector));
					}
					else if (propertyType == ("quaternion") || propertyType == ("QuaternionF") || propertyType == ("QuaternionD"))
					{
						Vector3F vector = prop.getValueAsVector3f();
						wxVector3f wxVector(vector.X(), vector.Y(), vector.Z());
						wxProperty = pg->Append(new wxVectorProperty(name.c_str(), wxPG_LABEL, wxVector));
					}
					else if (propertyType == ("vector4") || propertyType == ("Vector4F") || propertyType == ("vector4d"))
					{
						auto vector = prop.getValueAsVector4f();
						wxVector3f wxVector(vector.X(), vector.Y(), vector.Z());
						wxProperty = pg->Append(new wxVectorProperty(name.c_str(), wxPG_LABEL, wxVector));
					}
					else if (propertyType == ("colourf") || propertyType == ("colour"))
					{
						auto colourStr = prop.getValue();
						auto colourF = StringUtil::parseColourf(colourStr);
						auto color = wxColour(colourF.r * 255.0f, colourF.g * 255.0f, colourF.b * 255.0f, colourF.a * 255.0f);
						wxProperty = pg->Append(new wxColourProperty(name.c_str(), wxPG_LABEL, color));
					}
					else if (propertyType == ("colouri"))
					{
						ColourI colourI = StringUtil::parseColour(prop.getValue());
						wxColour color(colourI.getRed(), colourI.getGreen(), colourI.getBlue(), colourI.getAlpha());
						wxProperty = pg->Append(new wxColourProperty(name.c_str(), wxPG_LABEL, color));
						//wxProperty->setFlag(wxPG_PROP_AUTO_UNSPECIFIED);
					}
					else if (propertyType == ("string"))
					{
						wxProperty = pg->Append(new wxStringProperty(name.c_str(), wxPG_LABEL, prop.getValue().c_str()));
					}
					else if (propertyType == ("event"))
					{
						//wxProperty = pg->Append(new wxStringProperty(name.c_str(), wxPG_LABEL, prop.getValue().c_str()));
						wxProperty = pg->Append(new wxEventProperty(label.c_str(), name.c_str(), prop.getValue().c_str()));

					}
					else
					{
						wxProperty = pg->Append(new wxStringProperty(name.c_str(), wxPG_LABEL, prop.getValue().c_str()));
					}

					if (wxProperty)
					{
						wxProperty->SetLabel(label.c_str());
					}

					//if(wxProperty && prop.isReadOnly())
					//	wxProperty->ChangeFlag(wxPG_PROP_READONLY);
				}

				pg->Refresh();
			}
		}




		//--------------------------------------------
		void wxGUIUtil::setPropertyValue(Properties& properties, wxPropertyGrid* pg, wxPGProperty* p)
		{
			auto pPropertyName = p->GetName();
			auto propertyName = String(pPropertyName);

			if (!properties.hasProperty(propertyName))
			{
				return;
			}

			auto &prop = properties.getPropertyObject( propertyName );
			auto propertyType = prop.getType();
			prop.setAttribute("changed", "true");

			if (propertyType == "Array" ||
				propertyType == "list" ||
				propertyType == "folders")
			{
				//format the string
				wxArrayString wxArray = pg->GetPropertyValueAsArrayString(p);
				
				Array<String> fbArray;
				fbArray.reserve(wxArray.size());

				for (u32 i = 0; i < wxArray.size(); ++i)
				{
					String elementValue = String(wxArray[i].c_str());
					fbArray.push_back(elementValue);
				}

				auto valueStr = StringUtil::toString(fbArray);

				if (properties.hasProperty(propertyName))
				{
                    auto &property = properties.getPropertyObject( propertyName );
					property.setValue(valueStr);
				}
				else
				{
					properties.setProperty(propertyName, valueStr);
				}
			}
			else if (propertyType == ("enum") ||
				propertyType == ("choice"))
			{
				String propertyValue = String(pg->GetPropertyValueAsString(p).c_str());
				properties.setProperty(propertyName, propertyValue);

				//String currentValue;
				//properties.getPropertyValue(propertyName, currentValue);

				//Array<String> currentValues;
				//StringUtil::parseArray(currentValue, currentValues);
				//if (!currentValues.empty())
				//	currentValues[0] = propertyValue;

				//if (properties.hasProperty(propertyName))
				//{
				//	Property& property = properties.getProperty(propertyName);
				//	property.setValue(StringUtil::toString(currentValues));
				//}
				//else
				//{
				//	properties.setProperty(propertyName, StringUtil::toString(currentValues));
				//}
			}
			else if (propertyType == ("file"))
			{
				auto applicationManager = IApplicationManager::instance();
				auto projectPath = applicationManager->getProjectPath();
				if (!StringUtil::isNullOrEmpty(projectPath))
				{
					auto propertyValue = pg->GetPropertyValueAsString(p);
					auto propertyValueStr = String(propertyValue.c_str());
					auto relativePath = Path::getRelativePath(projectPath, propertyValueStr);
					relativePath = StringUtil::cleanupPath(relativePath);

					properties.setProperty(propertyName, relativePath);
				}
				else
				{
					auto propertyValue = pg->GetPropertyValueAsString(p);
					auto propertyValueStr = String(propertyValue.c_str());
					propertyValueStr = StringUtil::cleanupPath(propertyValueStr);

					properties.setProperty(propertyName, propertyValueStr);
				}
			}
			else if (
				propertyType == "vector3" ||
				propertyType == "Vector3F" ||
				propertyType == "vector3d")
			{
				String propertyValue = String(pg->GetPropertyValueAsString(p).c_str());
				propertyValue = StringUtil::replace(propertyValue, ';', ',');
				
				if (properties.hasProperty(propertyName))
				{
                    auto &property = properties.getPropertyObject( propertyName );
					property.setValue(propertyValue);
				}
				else
				{
					properties.setProperty(propertyName, propertyValue);
				}
			}
			else if (propertyType == ("colourf") || propertyType == ("colour"))
			{
				//if (propertyValue == "Red")
				//{
				//	properties.setProperty(propertyName, ColourF::Red);
				//}
				//else
				{	
					wxVariant v = pg->GetPropertyValue(p);
					wxColour c;
					c << v;

					auto colourF = ColourF(
						(f32)c.Red() / 255.0f,
						(f32)c.Green() / 255.0f,
						(f32)c.Blue() / 255.0f,
						(f32)c.Alpha() / 255.0f);

					properties.setProperty(propertyName, colourF);
				}
			}
			else
			{
				String propertyValue = String(pg->GetPropertyValueAsString(p).c_str());
				if (properties.hasProperty(propertyName))
				{
                    Property &property = properties.getPropertyObject( propertyName );
					property.setValue(propertyValue);
				}
				else
				{
					properties.setProperty(propertyName, propertyValue);
				}
			}
		}




	} // end namespace editor	
} // end namespace fb
