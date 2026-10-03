#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UITabViewCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_button.h>
#include <workphone_layout.h>
#include <workphone_style.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UITabItemCore,
                                   UIElementCore<UIElement<IUITabItem>> );
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UITabViewCore,
                                   UIElementCore<UIElement<IUITabBar>> );

        namespace
        {
            const String labelStr = "label";
            const String tabItemStr = "tabItem";
            const String selectedIndexStr = "selectedIndex";
            const String tabHeightStr = "tabHeight";
            const String tabSpacingStr = "tabSpacing";
            const String minTabWidthStr = "minTabWidth";
            const String paddingStr = "padding";
            const String normalColourStr = "normalColour";
            const String hoverColourStr = "hoverColour";
            const String activeColourStr = "activeColour";
            const String selectedNormalColourStr = "selectedNormalColour";
            const String selectedHoverColourStr = "selectedHoverColour";
            const String selectedActiveColourStr = "selectedActiveColour";
            const String textNormalColourStr = "textNormalColour";
            const String textHoverColourStr = "textHoverColour";
            const String textActiveColourStr = "textActiveColour";
            const String borderColourStr = "borderColour";
            const String borderWidthStr = "borderWidth";
            const String roundingStr = "rounding";

            template <class T>
            void eraseItem( Array<SmartPtr<T>> &items, SmartPtr<T> item )
            {
                auto it = std::remove( items.begin(), items.end(), item );
                if( it != items.end() )
                {
                    items.erase( it, items.end() );
                }
            }
        }  // namespace

        UITabItemCore::UITabItemCore()
        {
            createStateContext();
        }

        UITabItemCore::~UITabItemCore()
        {
            unload( nullptr );
        }

        void UITabItemCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );
                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITabItemCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    setLoadingState( LoadingState::Unloading );
                    UIElementCore<UIElement<IUITabItem>>::unload( data );
                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITabItemCore::update()
        {
            if( !isLoaded() || !isVisible() || !isEnabled() )
            {
                return;
            }

            UIElementCore<UIElement<IUITabItem>>::update();
        }

        String UITabItemCore::getLabel() const
        {
            return m_label;
        }

        void UITabItemCore::setLabel( const String &label )
        {
            m_label = label;
        }

        SmartPtr<Properties> UITabItemCore::getProperties() const
        {
            auto properties = UIElementCore<UIElement<IUITabItem>>::getProperties();
            properties->setProperty( labelStr, m_label );
            return properties;
        }

        void UITabItemCore::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "UITabItemCore::setProperties received null properties." );
                return;
            }

            UIElementCore<UIElement<IUITabItem>>::setProperties( properties );
            properties->getPropertyValue( labelStr, m_label );
        }

        UITabViewCore::UITabViewCore()
        {
            createStateContext();
        }

        UITabViewCore::~UITabViewCore()
        {
            unload( nullptr );
        }

        void UITabViewCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->setDirty( true );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITabViewCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    setLoadingState( LoadingState::Unloading );
                    m_tabItems.clear();
                    UIElementCore<UIElement<IUITabBar>>::unload( data );
                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITabViewCore::update()
        {
            if( !isLoaded() || !isVisible() || !isEnabled() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto ui = workphone::static_pointer_cast<UIManagerCore>( applicationManager->getRenderUI() );
            if( !ui )
            {
                return;
            }

            auto *ctx = ui->getContext();
            if( !ctx )
            {
                return;
            }

            if( m_tabItems.empty() )
            {
                return;
            }

            if( m_selectedIndex >= m_tabItems.size() )
            {
                WP_LOG_ERROR( "UITabViewCore selected index is out of range. Resetting to tab 0." );
                m_selectedIndex = 0u;
            }

            struct wp_rect bounds;
            UIUtilCore::calculateBounds( getPosition(), getSize(), &bounds );

            const auto tabCount = static_cast<f32>( m_tabItems.size() );
            const auto safeSpacing = MathF::max( m_tabSpacing, 0.0f );
            const auto headerHeight = MathF::min( MathF::max( m_tabHeight, 1.0f ), bounds.h );
            const auto availableWidth = MathF::max( bounds.w - safeSpacing * ( tabCount - 1.0f ), 1.0f );
            const auto tabWidth = MathF::max( availableWidth / tabCount, 1.0f );
            const auto actualTabWidth = MathF::max( tabWidth, m_minTabWidth );

            for( size_t i = 0; i < m_tabItems.size(); ++i )
            {
                auto tabItem = m_tabItems[i];
                if( !tabItem )
                {
                    WP_LOG_ERROR( "UITabViewCore has a null tab item. Skipping it." );
                    continue;
                }

                const auto x = bounds.x + static_cast<f32>( i ) * ( actualTabWidth + safeSpacing );
                const auto tabBounds = wp_make_rect( x, bounds.y, actualTabWidth, headerHeight );
                const auto tabStyle = createTabStyle( ctx, i == m_selectedIndex );

                auto label = tabItem->getLabel();
                if( label.empty() )
                {
                    label = "Tab";
                }

                wp_layout_space_push( ctx, tabBounds );
                if( wp_button_label_styled( ctx, &tabStyle, label.c_str() ) )
                {
                    if( m_selectedIndex != i )
                    {
                        m_selectedIndex = static_cast<u32>( i );
                        notifySelectionChanged();
                    }
                }
            }

            if( m_selectedIndex < m_tabItems.size() )
            {
                auto selectedTabItem = m_tabItems[m_selectedIndex];
                if( selectedTabItem && selectedTabItem->isLoaded() )
                {
                    selectedTabItem->update();
                }
            }
        }

        void UITabViewCore::addChild( SmartPtr<IUIElement> child )
        {
            UIElementCore<UIElement<IUITabBar>>::addChild( child );

            if( auto tabItem = workphone::dynamic_pointer_cast<IUITabItem>( child ) )
            {
                if( std::find( m_tabItems.begin(), m_tabItems.end(), tabItem ) == m_tabItems.end() )
                {
                    m_tabItems.push_back( tabItem );
                }
            }
        }

        SmartPtr<IUITabItem> UITabViewCore::addTabItem()
        {
            SmartPtr<IUITabItem> tabItem;

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                if( auto factoryManager = applicationManager->getFactoryManagerPtr() )
                {
                    tabItem = factoryManager->make_ptr<UITabItemCore>();
                }
                else
                {
                    WP_LOG_ERROR( "Factory manager is not available while creating a tab item." );
                }
            }
            else
            {
                WP_LOG_ERROR( "Application manager is not available while creating a tab item." );
            }

            if( !tabItem )
            {
                tabItem = workphone::make_ptr<UITabItemCore>();
            }

            if( tabItem )
            {
                const auto index = static_cast<u32>( m_tabItems.size() + 1u );
                tabItem->setLabel( "Tab " + StringUtil::toString( index ) );
                addChild( tabItem );

                if( !tabItem->isLoaded() )
                {
                    tabItem->load( nullptr );
                }
            }

            return tabItem;
        }

        void UITabViewCore::removeTabItem( SmartPtr<IUITabItem> tabItem )
        {
            if( !tabItem )
            {
                WP_LOG_ERROR( "UITabViewCore::removeTabItem received a null tab item." );
                return;
            }

            eraseItem( m_tabItems, tabItem );
            UIElementCore<UIElement<IUITabBar>>::removeChild( tabItem );

            if( m_selectedIndex >= m_tabItems.size() )
            {
                m_selectedIndex = m_tabItems.empty() ? 0u : static_cast<u32>( m_tabItems.size() - 1u );
            }
        }

        Array<SmartPtr<IUITabItem>> UITabViewCore::getTabItems() const
        {
            return m_tabItems;
        }

        void UITabViewCore::setTabItems( const Array<SmartPtr<IUITabItem>> &tabItems )
        {
            m_tabItems.clear();
            removeAllChildren();

            for( auto tabItem : tabItems )
            {
                if( tabItem )
                {
                    addChild( tabItem );
                }
            }

            setSelectedIndex( m_selectedIndex );
        }

        u32 UITabViewCore::getSelectedIndex() const
        {
            return m_selectedIndex;
        }

        void UITabViewCore::setSelectedIndex( u32 selectedIndex )
        {
            if( m_tabItems.empty() )
            {
                m_selectedIndex = 0u;
                return;
            }

            if( selectedIndex >= m_tabItems.size() )
            {
                WP_LOG_ERROR( "UITabViewCore selected index is out of range. Clamping." );
                selectedIndex = static_cast<u32>( m_tabItems.size() - 1u );
            }

            m_selectedIndex = selectedIndex;
        }

        f32 UITabViewCore::getTabHeight() const
        {
            return m_tabHeight;
        }

        void UITabViewCore::setTabHeight( f32 tabHeight )
        {
            m_tabHeight = MathF::max( tabHeight, 1.0f );
        }

        f32 UITabViewCore::getTabSpacing() const
        {
            return m_tabSpacing;
        }

        void UITabViewCore::setTabSpacing( f32 tabSpacing )
        {
            m_tabSpacing = MathF::max( tabSpacing, 0.0f );
        }

        f32 UITabViewCore::getMinTabWidth() const
        {
            return m_minTabWidth;
        }

        void UITabViewCore::setMinTabWidth( f32 minTabWidth )
        {
            m_minTabWidth = MathF::max( minTabWidth, 1.0f );
        }

        Vector2F UITabViewCore::getPadding() const
        {
            return m_padding;
        }

        void UITabViewCore::setPadding( const Vector2F &padding )
        {
            m_padding = Vector2F( MathF::max( padding.X(), 0.0f ),
                                  MathF::max( padding.Y(), 0.0f ) );
        }

        ColourF UITabViewCore::getNormalColour() const { return m_normalColour; }
        void UITabViewCore::setNormalColour( const ColourF &colour ) { m_normalColour = colour; }

        ColourF UITabViewCore::getHoverColour() const { return m_hoverColour; }
        void UITabViewCore::setHoverColour( const ColourF &colour ) { m_hoverColour = colour; }

        ColourF UITabViewCore::getActiveColour() const { return m_activeColour; }
        void UITabViewCore::setActiveColour( const ColourF &colour ) { m_activeColour = colour; }

        ColourF UITabViewCore::getSelectedNormalColour() const { return m_selectedNormalColour; }
        void UITabViewCore::setSelectedNormalColour( const ColourF &colour )
        {
            m_selectedNormalColour = colour;
        }

        ColourF UITabViewCore::getSelectedHoverColour() const { return m_selectedHoverColour; }
        void UITabViewCore::setSelectedHoverColour( const ColourF &colour )
        {
            m_selectedHoverColour = colour;
        }

        ColourF UITabViewCore::getSelectedActiveColour() const { return m_selectedActiveColour; }
        void UITabViewCore::setSelectedActiveColour( const ColourF &colour )
        {
            m_selectedActiveColour = colour;
        }

        ColourF UITabViewCore::getTextNormalColour() const { return m_textNormalColour; }
        void UITabViewCore::setTextNormalColour( const ColourF &colour )
        {
            m_textNormalColour = colour;
        }

        ColourF UITabViewCore::getTextHoverColour() const { return m_textHoverColour; }
        void UITabViewCore::setTextHoverColour( const ColourF &colour ) { m_textHoverColour = colour; }

        ColourF UITabViewCore::getTextActiveColour() const { return m_textActiveColour; }
        void UITabViewCore::setTextActiveColour( const ColourF &colour )
        {
            m_textActiveColour = colour;
        }

        ColourF UITabViewCore::getBorderColour() const { return m_borderColour; }
        void UITabViewCore::setBorderColour( const ColourF &colour ) { m_borderColour = colour; }

        f32 UITabViewCore::getBorderWidth() const { return m_borderWidth; }
        void UITabViewCore::setBorderWidth( f32 borderWidth )
        {
            m_borderWidth = MathF::max( borderWidth, 0.0f );
        }

        f32 UITabViewCore::getRounding() const { return m_rounding; }
        void UITabViewCore::setRounding( f32 rounding )
        {
            m_rounding = MathF::max( rounding, 0.0f );
        }

        SmartPtr<Properties> UITabViewCore::getProperties() const
        {
            auto properties = UIElementCore<UIElement<IUITabBar>>::getProperties();

            properties->setProperty( selectedIndexStr, m_selectedIndex );
            properties->setProperty( tabHeightStr, m_tabHeight );
            properties->setProperty( tabSpacingStr, m_tabSpacing );
            properties->setProperty( minTabWidthStr, m_minTabWidth );
            properties->setProperty( paddingStr, m_padding );
            properties->setProperty( normalColourStr, m_normalColour );
            properties->setProperty( hoverColourStr, m_hoverColour );
            properties->setProperty( activeColourStr, m_activeColour );
            properties->setProperty( selectedNormalColourStr, m_selectedNormalColour );
            properties->setProperty( selectedHoverColourStr, m_selectedHoverColour );
            properties->setProperty( selectedActiveColourStr, m_selectedActiveColour );
            properties->setProperty( textNormalColourStr, m_textNormalColour );
            properties->setProperty( textHoverColourStr, m_textHoverColour );
            properties->setProperty( textActiveColourStr, m_textActiveColour );
            properties->setProperty( borderColourStr, m_borderColour );
            properties->setProperty( borderWidthStr, m_borderWidth );
            properties->setProperty( roundingStr, m_rounding );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                auto factoryManager = applicationManager->getFactoryManagerPtr();
                if( factoryManager )
                {
                    for( auto tabItem : m_tabItems )
                    {
                        if( tabItem )
                        {
                            auto tabProperties = factoryManager->make_ptr<Properties>();
                            tabProperties->setName( tabItemStr );
                            tabProperties->setProperty( labelStr, tabItem->getLabel() );
                            properties->addChild( tabProperties );
                        }
                    }
                }
                else
                {
                    WP_LOG_ERROR( "Factory manager is not available while serialising tab items." );
                }
            }
            else
            {
                WP_LOG_ERROR( "Application manager is not available while serialising tab items." );
            }

            return properties;
        }

        void UITabViewCore::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "UITabViewCore::setProperties received null properties." );
                return;
            }

            UIElementCore<UIElement<IUITabBar>>::setProperties( properties );

            properties->getPropertyValue( selectedIndexStr, m_selectedIndex );
            properties->getPropertyValue( tabHeightStr, m_tabHeight );
            properties->getPropertyValue( tabSpacingStr, m_tabSpacing );
            properties->getPropertyValue( minTabWidthStr, m_minTabWidth );
            properties->getPropertyValue( paddingStr, m_padding );
            properties->getPropertyValue( normalColourStr, m_normalColour );
            properties->getPropertyValue( hoverColourStr, m_hoverColour );
            properties->getPropertyValue( activeColourStr, m_activeColour );
            properties->getPropertyValue( selectedNormalColourStr, m_selectedNormalColour );
            properties->getPropertyValue( selectedHoverColourStr, m_selectedHoverColour );
            properties->getPropertyValue( selectedActiveColourStr, m_selectedActiveColour );
            properties->getPropertyValue( textNormalColourStr, m_textNormalColour );
            properties->getPropertyValue( textHoverColourStr, m_textHoverColour );
            properties->getPropertyValue( textActiveColourStr, m_textActiveColour );
            properties->getPropertyValue( borderColourStr, m_borderColour );
            properties->getPropertyValue( borderWidthStr, m_borderWidth );
            properties->getPropertyValue( roundingStr, m_rounding );

            setTabHeight( m_tabHeight );
            setTabSpacing( m_tabSpacing );
            setMinTabWidth( m_minTabWidth );
            setPadding( m_padding );
            setBorderWidth( m_borderWidth );
            setRounding( m_rounding );

            auto tabProperties = properties->getChildrenByName( tabItemStr );
            if( !tabProperties.empty() )
            {
                m_tabItems.clear();
                removeAllChildren();

                for( auto tabProperty : tabProperties )
                {
                    if( !tabProperty )
                    {
                        WP_LOG_ERROR( "Skipping null tab item properties." );
                        continue;
                    }

                    auto tabItem = addTabItem();
                    if( tabItem )
                    {
                        String label;
                        tabProperty->getPropertyValue( labelStr, label );
                        tabItem->setLabel( label );
                    }
                }
            }

            setSelectedIndex( m_selectedIndex );
        }

        struct wp_style_button UITabViewCore::createTabStyle( struct wp_context *ctx,
                                                              bool selected ) const
        {
            auto style = ctx ? ctx->style.button : wp_style_button();
            style.normal = UIUtilCore::solidItem( selected ? m_selectedNormalColour : m_normalColour );
            style.hover = UIUtilCore::solidItem( selected ? m_selectedHoverColour : m_hoverColour );
            style.active = UIUtilCore::solidItem( selected ? m_selectedActiveColour : m_activeColour );
            style.text_normal = UIUtilCore::toWpColor( m_textNormalColour );
            style.text_hover = UIUtilCore::toWpColor( m_textHoverColour );
            style.text_active = UIUtilCore::toWpColor( m_textActiveColour );
            style.border_color = UIUtilCore::toWpColor( m_borderColour );
            style.border = m_borderWidth;
            style.rounding = m_rounding;
            style.padding.x = m_padding.X();
            style.padding.y = m_padding.Y();
            return style;
        }

        void UITabViewCore::notifySelectionChanged()
        {
            Array<Parameter> args;
            args.push_back( Parameter( m_selectedIndex ) );

            if( m_selectedIndex < m_tabItems.size() )
            {
                if( auto tabItem = m_tabItems[m_selectedIndex] )
                {
                    args.push_back( Parameter( tabItem->getLabel() ) );
                }
            }

            auto listeners = getObjectListeners();
            for( auto &listener : listeners )
            {
                if( listener )
                {
                    listener->handleEvent( EventType::UI, IEvent::CLICK_HASH, args, this, nullptr,
                                           nullptr );
                }
            }
        }
    }  // namespace ui
}  // namespace workphone
