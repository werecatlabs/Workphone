#ifndef __WPWxWindow_H
#define __WPWxWindow_H

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        class wxApplicationWindow : public ISharedObject
        {
        public:
            wxApplicationWindow();
            ~wxApplicationWindow();

            String getName() const;
            void setName( const String &name );

            wxWindow *getWindow() const;
            void setWindow( wxWindow *window );

            wxWindow *getParent() const;
            void setParent( wxWindow *parent );

            wxAuiNotebook *getNotebook() const;
            void setNotebook( wxAuiNotebook *notebook );

            s32 getNotebookIndex() const;
            void setNotebookIndex( s32 notebookIndex );

            void addNotebookPage();
            void removeNotebookPage();

            void show();
            void hide();

        private:
            String m_name;

            wxWindow *m_parent = nullptr;

            wxWindow *m_window = nullptr;

            wxAuiNotebook *m_notebook = nullptr;

            s32 m_notebookIndex = 0;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
