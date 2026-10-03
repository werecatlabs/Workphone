#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/TabPage.hpp>
#include <Workphone/Scene/Components/UI/TabView.hpp>
#include <Workphone/Scene/Components/UI/TabItem.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, TabPage, UIComponent );

    const String TabPage::activeTabIndexStr = "activeTabIndex";
    const String TabPage::tabsStr = "tabs";
    const String TabPage::contentAreaStr = "contentArea";

    TabPage::TabPage() : m_activeTabIndex( 0 )
    {
    }

    TabPage::~TabPage()
    {
    }

    void TabPage::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            UIComponent::load( data );

            // Initialize the tab page after loading
            createUI();
            updateElementState();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabPage::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            // Clear tab items
            m_tabItems.clear();
            m_contentArea = nullptr;

            UIComponent::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> TabPage::getChildObjects() const
    {
        auto objects = UIComponent::getChildObjects();

        // Add tab items to child objects
        for( auto &tabItem : m_tabItems )
        {
            if( tabItem )
            {
                objects.push_back( tabItem );
            }
        }

        // Add content area if it exists
        if( m_contentArea )
        {
            objects.push_back( m_contentArea );
        }

        return objects;
    }

    SmartPtr<Properties> TabPage::getProperties() const
    {
        auto properties = UIComponent::getProperties();
        if( properties )
        {
            properties->setProperty( activeTabIndexStr, static_cast<s32>( m_activeTabIndex ) );

            // Store tab count for display purposes
            properties->setProperty( tabsStr, static_cast<s32>( m_tabItems.size() ) );

            return properties;
        }

        return nullptr;
    }

    void TabPage::setProperties( SmartPtr<Properties> properties )
    {
        if( properties )
        {
            s32 activeIndex = 0;
            if( properties->getPropertyValue( activeTabIndexStr, activeIndex ) )
            {
                setActiveTabIndex( static_cast<u32>( activeIndex ) );
            }
        }

        UIComponent::setProperties( properties );
    }

    bool TabPage::isValid() const
    {
        // A TabPage is valid if it has been properly loaded and has at least one tab
        return getLoadingState() == LoadingState::Loaded && !m_tabItems.empty();
    }

    void TabPage::createUI()
    {
        try
        {
            auto actor = getActor();
            if( !actor )
            {
                return;
            }

            // Create content area for displaying active tab content
            if( !m_contentArea )
            {
                auto contentAreaActor = actor->findChild( "ContentArea" );
                if( !contentAreaActor )
                {
                    // Content area will be managed by the layout system
                    auto layoutTransform = actor->getComponent<LayoutTransform>();
                    if( layoutTransform )
                    {
                        // The content area is the TabPage itself - it displays the active tab's content
                        updateActiveTabContent();
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabPage::addTabItem( SmartPtr<TabItem> tabItem )
    {
        if( !tabItem )
        {
            WP_LOG( "Cannot add null tab item to TabPage" );
            return;
        }

        m_tabItems.push_back( tabItem );

        // If this is the first tab, make it active
        if( m_tabItems.size() == 1 )
        {
            setActiveTabIndex( 0 );
        }

        // Update the UI to reflect the new tab
        updateElementState();
    }

    void TabPage::removeTabItem( u32 index )
    {
        if( index >= m_tabItems.size() )
        {
            WP_LOG( "Tab index out of range: " + StringUtil::toString( index ) );
            return;
        }

        m_tabItems.erase( m_tabItems.begin() + index );

        // Adjust active tab index if necessary
        if( m_activeTabIndex >= m_tabItems.size() && !m_tabItems.empty() )
        {
            m_activeTabIndex = (s32)m_tabItems.size() - 1;
        }
        else if( m_tabItems.empty() )
        {
            m_activeTabIndex = 0;
        }

        updateActiveTabContent();
        updateElementState();
    }

    void TabPage::removeTabItem( SmartPtr<TabItem> tabItem )
    {
        auto it = std::find( m_tabItems.begin(), m_tabItems.end(), tabItem );
        if( it != m_tabItems.end() )
        {
            u32 index = static_cast<u32>( std::distance( m_tabItems.begin(), it ) );
            removeTabItem( index );
        }
    }

    SmartPtr<TabItem> TabPage::getTabItem( u32 index ) const
    {
        if( index < m_tabItems.size() )
        {
            return m_tabItems[index];
        }
        return nullptr;
    }

    Array<SmartPtr<TabItem>> TabPage::getTabItems() const
    {
        return m_tabItems;
    }

    u32 TabPage::getTabCount() const
    {
        return static_cast<u32>( m_tabItems.size() );
    }

    void TabPage::setActiveTabIndex( u32 index )
    {
        if( index >= m_tabItems.size() )
        {
            WP_LOG( "Tab index out of range: " + StringUtil::toString( index ) );
            return;
        }

        if( m_activeTabIndex != index )
        {
            m_activeTabIndex = index;
            updateActiveTabContent();
            updateElementState();

            // Notify listeners about tab change
            if( auto actor = getActor() )
            {
                handleEvent( EventType::UI, StringUtil::getHash( "TabChanged" ), Array<Parameter>(),
                             actor, this, nullptr );
            }
        }
    }

    u32 TabPage::getActiveTabIndex() const
    {
        return m_activeTabIndex;
    }

    SmartPtr<TabItem> TabPage::getActiveTab() const
    {
        if( m_activeTabIndex < m_tabItems.size() )
        {
            return m_tabItems[m_activeTabIndex];
        }
        return nullptr;
    }

    void TabPage::updateActiveTabContent()
    {
        try
        {
            auto activeTab = getActiveTab();
            if( !activeTab )
            {
                return;
            }

            // Get the content from the active tab
            auto content = activeTab->getContent();
            if( !content )
            {
                return;
            }

            // If content is an actor, make sure it's visible and positioned correctly
            if( auto contentActor = workphone::dynamic_pointer_cast<IGameActor>( content ) )
            {
                // Hide content of other tabs
                for( u32 i = 0; i < m_tabItems.size(); ++i )
                {
                    if( i != m_activeTabIndex )
                    {
                        if( auto otherTab = m_tabItems[i] )
                        {
                            if( auto otherContent = otherTab->getContent() )
                            {
                                if( auto otherActor =
                                        workphone::dynamic_pointer_cast<IGameActor>( otherContent ) )
                                {
                                    otherActor->setVisible( false );
                                }
                            }
                        }
                    }
                }

                // Show active tab content
                contentActor->setVisible( true );
                contentActor->setEnabled( true );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabPage::updateElementState()
    {
        try
        {
            // Update the display of tab headers and content
            updateActiveTabContent();

            // Ensure proper layout and visibility
            updateVisibility();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Parameter TabPage::handleEvent( EventType eventType, hash_type eventValue,
                                    const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                    SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        // Handle tab selection events
        if( eventType == EventType::UI )
        {
            if( eventValue == IEvent::CLICK_HASH )
            {
                // Check if the click was on a tab header
                if( auto tabItem = workphone::dynamic_pointer_cast<TabItem>( sender ) )
                {
                    // Find which tab was clicked and make it active
                    for( u32 i = 0; i < m_tabItems.size(); ++i )
                    {
                        if( m_tabItems[i] == tabItem )
                        {
                            setActiveTabIndex( i );
                            break;
                        }
                    }
                }
            }
        }

        return UIComponent::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

}  // namespace workphone::scene
