#include <GameEditorPCH.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <wx/wx.hpp>



namespace fb
{
	namespace editor
	{


		ObjectWindow::ObjectWindow()
		{

		}

		ObjectWindow::ObjectWindow(wxWindow* parent)
		{
		}


		ObjectWindow::~ObjectWindow()
		{
		}

		void ObjectWindow::load(SmartPtr<ISharedObject> data)
		{
			auto parent = getParent();

			auto parentWindow = new wxScrolledWindow(parent, -1);
			setWindow(parentWindow);

			auto baseSizer = new wxBoxSizer(wxVERTICAL);
			parentWindow->SetSizer(baseSizer);

			m_actorWindow = new ActorWindow(parentWindow, -1);
			m_propertiesWindow = new PropertiesWindow(parentWindow, -1);

			auto actorSizerFlags = wxSizerFlags().Expand().Proportion(30);
			auto propertiesSizerFlags = wxSizerFlags().Expand().Proportion(70);

			baseSizer->Add(m_actorWindow->getWindow(), actorSizerFlags);
			baseSizer->Add(m_propertiesWindow->getWindow(), propertiesSizerFlags);

			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();

			uiManager->setActorWindow(m_actorWindow.get());
			uiManager->setPropertiesWindow(m_propertiesWindow.get());

			baseSizer->Layout();

			parentWindow->SetAutoLayout(true);
		}

		wxWindow* ObjectWindow::getParent() const
		{
			return m_parent;
		}

		void ObjectWindow::setParent(wxWindow* parent)
		{
			m_parent = parent;
		}

		wxWindow* ObjectWindow::getWindow() const
		{
			return m_window;
		}

		void ObjectWindow::setWindow(wxWindow* window)
		{
			m_window = window;
		}

	} // end namespace editor	
} // end namespace fb


