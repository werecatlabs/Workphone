#include <GameEditorPCH.hpp>
#include <ui/ObjectBrowserDialog.hpp>
#include <ui/ProjectTreeData.hpp>
#include <editor/EditorManager.hpp>
#include <editor/EditorMessages.hpp>
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplate.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/PhysicsBodyTemplate.hpp>
#include <wx/wx.hpp>



namespace fb
{
	namespace editor
	{



		ObjectBrowserDialog::ObjectBrowserDialog(wxWindow* parent)
			: wxDialog(parent, wxID_ANY, _T("Create Physics Body"),
				wxDefaultPosition, wxDefaultSize,
				wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
		{
			wxBoxSizer* baseSizer = new wxBoxSizer(wxVERTICAL);
			SetSizer(baseSizer);

			m_tree = new wxTreeCtrl(this, -1);
			baseSizer->Add(m_tree, 1, wxEXPAND);

			m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &ObjectBrowserDialog::handleTreeSelectionChanged, this, -1);
			m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &ObjectBrowserDialog::handleTreeSelectionActivated, this, -1);

			populate();
		}




		ObjectBrowserDialog::~ObjectBrowserDialog()
		{
		}



		void ObjectBrowserDialog::populate()
		{
			auto applicationManager = IApplicationManager::instance();
			auto factoryManager = applicationManager->getFactoryManager();
			auto factories = factoryManager->getFactories();

			auto rootId = m_tree->AddRoot("Objects", -1, -1, nullptr);

			for (auto factory : factories)
			{
				if (factory->isObjectDerivedFrom<component::IComponent>())
				{
					auto name = factory->getObjectType();

					ProjectTreeData data("factory", "factory", factory, factory);
					m_tree->AppendItem(rootId, name, -1, -1, new ProjectTreeData(data));
				}
			}

			m_tree->ExpandAll();
		}



		String ObjectBrowserDialog::getSelectedObject() const
		{
			return m_selectedObject;
		}

		void ObjectBrowserDialog::setSelectedObject(const String& val)
		{
			m_selectedObject = val;
		}

		void ObjectBrowserDialog::handleTreeSelectionChanged(wxTreeEvent& event)
		{
			auto selectedId = event.GetItem();
			auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
			if (data)
			{
				auto factory = fb::static_pointer_cast<IFactory>(data->getOwnerData());
				auto selectedObject = factory->getObjectType();
				setSelectedObject(selectedObject);
			}
		}

		void ObjectBrowserDialog::handleTreeSelectionActivated(wxTreeEvent& event)
		{
			auto selectedId = event.GetItem();
			auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
			if (data)
			{
				auto factory = fb::static_pointer_cast<IFactory>(data->getOwnerData());
				auto selectedObject = factory->getObjectType();
				setSelectedObject(selectedObject);
			}

			EndModal(wxID_OK);
		}



	} // end namespace editor	
} // end namespace fb


