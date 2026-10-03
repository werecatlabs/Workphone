#ifndef ImGuiPropertyGrid_h__
#define ImGuiPropertyGrid_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIPropertyGrid.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiPropertyGrid : public ImGuiElement<IUIPropertyGrid>
        {
        public:
            ImGuiPropertyGrid();
            ~ImGuiPropertyGrid() override;

            /** @copydoc IUIPropertyGrid::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IUIPropertyGrid::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            static void createElement( SmartPtr<IUIElement> element );
            void createElement( SmartPtr<Properties> properties, SmartPtr<Properties> parent );

            static void createProperty( SmartPtr<Properties> properties, Property &property,
                                        SmartPtr<IUIPropertyGrid> propertyGrid );

            static void handlePropertyButtonClicked( SmartPtr<IUIPropertyGrid> propertyGrid,
                                                     const String &name, const String &value );

            static void handlePropertyChanged( SmartPtr<IUIPropertyGrid> propertyGrid,
                                               SmartPtr<Properties> properties, const String &name,
                                               const String &str );

            WP_CLASS_REGISTER_DECL;

        protected:
            bool matchesFilter( const Property &property ) const;
            bool hasMatchingProperties( SmartPtr<Properties> properties ) const;
            void drawPropertyTable( SmartPtr<Properties> properties, SmartPtr<Properties> parent,
                                    const String &category, bool uncategorized );

            AtomicSmartPtr<Properties> m_properties;
            String m_filter;
            static u32 m_childWindowCount;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiPropertyGrid_h__
