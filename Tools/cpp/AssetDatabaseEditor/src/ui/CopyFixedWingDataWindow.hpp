// ---------------------------------------------------------------------------
//  CopyFixedWingDataWindow.hpp
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorCopyFixedWingDataWindow_h__
#define AssetDatabaseEditorCopyFixedWingDataWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Port of the C# CopyFixedWingDataWindow.  Allows the user
         *        to copy a set of model objects (wing_data, engine_data,
         *        control_surface_data, ...) from one model to another.
         */
        class CopyFixedWingDataWindow : public ISharedObject
        {
        public:
            CopyFixedWingDataWindow();
            ~CopyFixedWingDataWindow() override;

            void setSourceConnection( SmartPtr<AssetDatabaseEditorDatabase> database );
            void setDestinationConnection( SmartPtr<AssetDatabaseEditorDatabase> database );
            void setIncludeWingData( bool value ) { m_wingData = value; }
            void setIncludeWheelData( bool value ) { m_wheelData = value; }
            void setIncludeEngineData( bool value ) { m_engineData = value; }
            void setIncludePropwashData( bool value ) { m_propwashData = value; }
            void setIncludeModelData( bool value ) { m_modelData = value; }
            void setIncludeControlSurfaceData( bool value ) { m_controlSurfaceData = value; }
            void show();

            WP_CLASS_REGISTER_DECL;

        protected:
            void performCopy();

            SmartPtr<AssetDatabaseEditorDatabase> m_source;
            SmartPtr<AssetDatabaseEditorDatabase> m_destination;
            bool m_wingData = true;
            bool m_wheelData = true;
            bool m_engineData = true;
            bool m_propwashData = true;
            bool m_modelData = true;
            bool m_controlSurfaceData = true;
            SmartPtr<ui::IUIWindow> m_window;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorCopyFixedWingDataWindow_h__
