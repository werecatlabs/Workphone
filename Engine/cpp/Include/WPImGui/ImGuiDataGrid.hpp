#ifndef ImGuiDataGrid_h__
#define ImGuiDataGrid_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIDataGrid.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiDataGrid : public ImGuiElement<IUIDataGrid>
        {
        public:
            ImGuiDataGrid();
            ~ImGuiDataGrid() override;

            void update() override;

            /** @copydoc IUIDataGrid::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IUIDataGrid::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            static void createElement( SmartPtr<IUIElement> element );
            void createElement( SmartPtr<Properties> properties, SmartPtr<Properties> parent );

            static void handleDataChanged( SmartPtr<IUIDataGrid> dataGrid,
                                           SmartPtr<Properties> properties, const String &name,
                                           const String &value );

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicSmartPtr<Properties> m_properties;
            static u32 m_childWindowCount;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiDataGrid_h__
