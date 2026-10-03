// ---------------------------------------------------------------------------
//  CopyDataWindow.hpp
//
//  Mirrors Tools/csharp/AssetDatabaseTool/CopyDataWindow.  The C# class
//  copies wing attributes between a source and destination model object.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorCopyDataWindow_h__
#define AssetDatabaseEditorCopyDataWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        class CopyDataWindow : public ISharedObject
        {
        public:
            CopyDataWindow();
            ~CopyDataWindow() override;

            void setSourceConnection( SmartPtr<AssetDatabaseEditorDatabase> database );
            void setDestinationConnection( SmartPtr<AssetDatabaseEditorDatabase> database );
            void setModelObjectType( const String &type );
            void show();

            WP_CLASS_REGISTER_DECL;

        protected:
            void performCopy();

            SmartPtr<AssetDatabaseEditorDatabase> m_source;
            SmartPtr<AssetDatabaseEditorDatabase> m_destination;
            String m_modelObjectType;
            SmartPtr<ui::IUIWindow> m_window;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorCopyDataWindow_h__
