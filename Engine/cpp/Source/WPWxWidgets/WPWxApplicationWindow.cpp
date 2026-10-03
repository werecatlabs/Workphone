#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/WPWxApplicationWindow.hpp>
#include <wx/window.h>
#include <wx/aui/framemanager.h>
#include <wx/aui/dockart.h>
#include <wx/aui/auibook.h>
#include <wx/aui/aui.h>

namespace workphone
{
    namespace ui
    {

        wxApplicationWindow::wxApplicationWindow()
        {
        }

        wxApplicationWindow::~wxApplicationWindow()
        {
            m_window = nullptr;
            m_notebook = nullptr;
        }

        String wxApplicationWindow::getName() const
        {
            return m_name;
        }

        void wxApplicationWindow::setName( const String &name )
        {
            m_name = name;
        }

        wxWindow *wxApplicationWindow::getWindow() const
        {
            return m_window;
        }

        void wxApplicationWindow::setWindow( wxWindow *window )
        {
            m_window = window;
        }

        wxWindow *wxApplicationWindow::getParent() const
        {
            return m_parent;
        }

        void wxApplicationWindow::setParent( wxWindow *parent )
        {
            m_parent = parent;
        }

        wxAuiNotebook *wxApplicationWindow::getNotebook() const
        {
            return m_notebook;
        }

        void wxApplicationWindow::setNotebook( wxAuiNotebook *notebook )
        {
            m_notebook = notebook;
        }

        s32 wxApplicationWindow::getNotebookIndex() const
        {
            return m_notebookIndex;
        }

        void wxApplicationWindow::setNotebookIndex( s32 notebookIndex )
        {
            m_notebookIndex = notebookIndex;
        }

        void wxApplicationWindow::addNotebookPage()
        {
            auto window = getWindow();
            m_notebook->AddPage( window, m_name, false );

            m_notebookIndex = m_notebook->GetPageIndex( window );
        }

        void wxApplicationWindow::removeNotebookPage()
        {
            auto window = getWindow();

            WP_ASSERT( m_notebook );
            m_notebookIndex = m_notebook->GetPageIndex( window );
            m_notebook->RemovePage( m_notebookIndex );
            m_notebookIndex = -1;
        }

        void wxApplicationWindow::show()
        {
            auto window = getWindow();
            if( window )
            {
                window->Show();
            }

            addNotebookPage();
        }

        void wxApplicationWindow::hide()
        {
            auto window = getWindow();
            if( window )
            {
                window->Hide();
            }

            removeNotebookPage();
        }

    }  // end namespace ui
}  // namespace workphone
