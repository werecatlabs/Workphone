#ifndef __UITabViewCore_h__
#define __UITabViewCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/Interface/UI/IUITabBar.hpp>
#include <Workphone/Interface/UI/IUITabItem.hpp>
#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {
        class UITabItemCore : public UIElementCore<UIElement<IUITabItem>>
        {
        public:
            UITabItemCore();
            ~UITabItemCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;
        };

        class UITabViewCore : public UIElementCore<UIElement<IUITabBar>>
        {
        public:
            UITabViewCore();
            ~UITabViewCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            void addChild( SmartPtr<IUIElement> child ) override;

            SmartPtr<IUITabItem> addTabItem() override;
            void removeTabItem( SmartPtr<IUITabItem> tabItem ) override;

            Array<SmartPtr<IUITabItem>> getTabItems() const;
            void setTabItems( const Array<SmartPtr<IUITabItem>> &tabItems );

            u32 getSelectedIndex() const;
            void setSelectedIndex( u32 selectedIndex );

            f32 getTabHeight() const;
            void setTabHeight( f32 tabHeight );

            f32 getTabSpacing() const;
            void setTabSpacing( f32 tabSpacing );

            f32 getMinTabWidth() const;
            void setMinTabWidth( f32 minTabWidth );

            Vector2F getPadding() const;
            void setPadding( const Vector2F &padding );

            ColourF getNormalColour() const;
            void setNormalColour( const ColourF &colour );

            ColourF getHoverColour() const;
            void setHoverColour( const ColourF &colour );

            ColourF getActiveColour() const;
            void setActiveColour( const ColourF &colour );

            ColourF getSelectedNormalColour() const;
            void setSelectedNormalColour( const ColourF &colour );

            ColourF getSelectedHoverColour() const;
            void setSelectedHoverColour( const ColourF &colour );

            ColourF getSelectedActiveColour() const;
            void setSelectedActiveColour( const ColourF &colour );

            ColourF getTextNormalColour() const;
            void setTextNormalColour( const ColourF &colour );

            ColourF getTextHoverColour() const;
            void setTextHoverColour( const ColourF &colour );

            ColourF getTextActiveColour() const;
            void setTextActiveColour( const ColourF &colour );

            ColourF getBorderColour() const;
            void setBorderColour( const ColourF &colour );

            f32 getBorderWidth() const;
            void setBorderWidth( f32 borderWidth );

            f32 getRounding() const;
            void setRounding( f32 rounding );

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            struct wp_style_button createTabStyle( struct wp_context *ctx, bool selected ) const;
            void notifySelectionChanged();

            Array<SmartPtr<IUITabItem>> m_tabItems;

            u32 m_selectedIndex = 0u;
            f32 m_tabHeight = 32.0f;
            f32 m_tabSpacing = 2.0f;
            f32 m_minTabWidth = 96.0f;

            Vector2F m_padding{ 8.0f, 4.0f };

            ColourF m_normalColour{ 0.18f, 0.18f, 0.18f, 1.0f };
            ColourF m_hoverColour{ 0.26f, 0.26f, 0.26f, 1.0f };
            ColourF m_activeColour{ 0.14f, 0.14f, 0.14f, 1.0f };

            ColourF m_selectedNormalColour{ 0.26f, 0.52f, 0.96f, 1.0f };
            ColourF m_selectedHoverColour{ 0.34f, 0.60f, 1.0f, 1.0f };
            ColourF m_selectedActiveColour{ 0.20f, 0.42f, 0.80f, 1.0f };

            ColourF m_textNormalColour{ 0.90f, 0.90f, 0.90f, 1.0f };
            ColourF m_textHoverColour{ 1.0f, 1.0f, 1.0f, 1.0f };
            ColourF m_textActiveColour{ 1.0f, 1.0f, 1.0f, 1.0f };

            ColourF m_borderColour{ 0.50f, 0.50f, 0.50f, 1.0f };
            f32 m_borderWidth = 1.0f;
            f32 m_rounding = 3.0f;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // __UITabViewCore_h__
