#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/TabView.hpp>
#include <Workphone/Scene/Components/UI/TabPage.hpp>
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
    const String TabView::tabCountStr = String( "tabCount" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, TabView, UIComponent );

    const String TabView::orientationStr = "orientation";
    const String TabView::tabPositionStr = "tabPosition";
    const String TabView::tabSizeStr = "tabSize";
    const String TabView::showTabHeadersStr = "showTabHeaders";

    TabView::TabView() :
        m_orientation( Orientation::Horizontal ),
        m_tabPosition( TabPosition::Top ),
        m_tabSize( Vector2F( 100.0f, 30.0f ) ),
        m_showTabHeaders( true )
    {
    }

    TabView::~TabView()
    {
    }

    void TabView::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            UIComponent::load( data );

            // Initialize the tab view after loading
            createUI();
            updateElementState();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabView::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            // Clear tab pages
            m_tabPages.clear();
            m_headerArea = nullptr;
            m_contentArea = nullptr;

            UIComponent::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> TabView::getChildObjects() const
    {
        auto objects = UIComponent::getChildObjects();

        // Add tab pages to child objects
        for( auto &tabPage : m_tabPages )
        {
            if( tabPage )
            {
                objects.push_back( tabPage );
            }
        }

        // Add header and content areas if they exist
        if( m_headerArea )
        {
            objects.push_back( m_headerArea );
        }
        if( m_contentArea )
        {
            objects.push_back( m_contentArea );
        }

        return objects;
    }

    SmartPtr<Properties> TabView::getProperties() const
    {
        auto properties = UIComponent::getProperties();
        if( properties )
        {
            properties->setProperty( orientationStr, static_cast<s32>( m_orientation ) );
            properties->setProperty( tabPositionStr, static_cast<s32>( m_tabPosition ) );
            properties->setProperty( tabSizeStr, m_tabSize );
            properties->setProperty( showTabHeadersStr, m_showTabHeaders );

            // Store tab count for display purposes
            properties->setProperty( TabView::tabCountStr, static_cast<s32>( m_tabPages.size() ) );

            return properties;
        }

        return nullptr;
    }

    void TabView::setProperties( SmartPtr<Properties> properties )
    {
        if( properties )
        {
            s32 orientation = static_cast<s32>( m_orientation );
            if( properties->getPropertyValue( orientationStr, orientation ) )
            {
                setOrientation( static_cast<Orientation>( orientation ) );
            }

            s32 tabPosition = static_cast<s32>( m_tabPosition );
            if( properties->getPropertyValue( tabPositionStr, tabPosition ) )
            {
                setTabPosition( static_cast<TabPosition>( tabPosition ) );
            }

            Vector2F tabSize = m_tabSize;
            if( properties->getPropertyValue( tabSizeStr, tabSize ) )
            {
                setTabSize( tabSize );
            }

            bool showTabHeaders = m_showTabHeaders;
            if( properties->getPropertyValue( showTabHeadersStr, showTabHeaders ) )
            {
                setShowTabHeaders( showTabHeaders );
            }
        }

        UIComponent::setProperties( properties );
    }

    bool TabView::isValid() const
    {
        // A TabView is valid if it has been properly loaded
        return getLoadingState() == LoadingState::Loaded;
    }

    void TabView::createUI()
    {
        try
        {
            auto actor = getActor();
            if( !actor )
            {
                return;
            }

            // Create header area for tab buttons if showing headers
            if( m_showTabHeaders && !m_headerArea )
            {
                auto headerAreaActor = actor->findChild( "TabHeader" );
                if( headerAreaActor )
                {
                    m_headerArea = headerAreaActor->getComponent<UIComponent>();
                }
            }

            // Create content area for displaying tab content
            if( !m_contentArea )
            {
                auto contentAreaActor = actor->findChild( "TabContent" );
                if( contentAreaActor )
                {
                    m_contentArea = contentAreaActor->getComponent<UIComponent>();
                }
            }

            // Initialize child tab pages
            auto children = actor->getChildren();
            for( auto &child : children )
            {
                if( auto tabPage = child->getComponent<TabPage>() )
                {
                    if( std::find( m_tabPages.begin(), m_tabPages.end(), tabPage ) == m_tabPages.end() )
                    {
                        m_tabPages.push_back( tabPage );
                    }
                }
            }

            // Make first tab page visible, hide the rest
            for( u32 i = 0; i < static_cast<u32>( m_tabPages.size() ); ++i )
            {
                if( auto &page = m_tabPages[i] )
                {
                    if( auto pageActor = page->getActor() )
                    {
                        pageActor->setVisible( i == 0 );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabView::addTabPage( SmartPtr<TabPage> tabPage )
    {
        if( !tabPage )
        {
            WP_LOG( "Cannot add null tab page to TabView" );
            return;
        }

        if( std::find( m_tabPages.begin(), m_tabPages.end(), tabPage ) != m_tabPages.end() )
        {
            return;
        }

        // Hide new page unless it's the first
        if( !m_tabPages.empty() )
        {
            if( auto pageActor = tabPage->getActor() )
            {
                pageActor->setVisible( false );
            }
        }

        m_tabPages.push_back( tabPage );

        // Update the UI to reflect the new tab page
        updateElementState();
    }

    void TabView::removeTabPage( u32 index )
    {
        if( index >= m_tabPages.size() )
        {
            WP_LOG( "Tab page index out of range: " + StringUtil::toString( index ) );
            return;
        }

        m_tabPages.erase( m_tabPages.begin() + static_cast<ptrdiff_t>( index ) );
        updateElementState();
    }

    void TabView::removeTabPage( SmartPtr<TabPage> tabPage )
    {
        auto it = std::find( m_tabPages.begin(), m_tabPages.end(), tabPage );
        if( it != m_tabPages.end() )
        {
            m_tabPages.erase( it );
            updateElementState();
        }
    }

    SmartPtr<TabPage> TabView::getTabPage( u32 index ) const
    {
        if( index < m_tabPages.size() )
        {
            return m_tabPages[index];
        }
        return nullptr;
    }

    Array<SmartPtr<TabPage>> TabView::getTabPages() const
    {
        return m_tabPages;
    }

    u32 TabView::getTabPageCount() const
    {
        return static_cast<u32>( m_tabPages.size() );
    }

    void TabView::setOrientation( Orientation orientation )
    {
        if( m_orientation != orientation )
        {
            m_orientation = orientation;
            updateElementState();
        }
    }

    TabView::Orientation TabView::getOrientation() const
    {
        return m_orientation;
    }

    void TabView::setTabPosition( TabPosition position )
    {
        if( m_tabPosition != position )
        {
            m_tabPosition = position;
            updateElementState();
        }
    }

    TabView::TabPosition TabView::getTabPosition() const
    {
        return m_tabPosition;
    }

    void TabView::setTabSize( const Vector2F &size )
    {
        if( m_tabSize != size )
        {
            m_tabSize = size;
            updateElementState();
        }
    }

    Vector2F TabView::getTabSize() const
    {
        return m_tabSize;
    }

    void TabView::setShowTabHeaders( bool showHeaders )
    {
        if( m_showTabHeaders != showHeaders )
        {
            m_showTabHeaders = showHeaders;
            updateElementState();
        }
    }

    bool TabView::getShowTabHeaders() const
    {
        return m_showTabHeaders;
    }

    void TabView::updateElementState()
    {
        try
        {
            // Update the layout and visibility of tab components
            updateTabLayout();
            updateVisibility();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabView::updateTabLayout()
    {
        try
        {
            auto actor = getActor();
            if( !actor )
            {
                return;
            }

            // Update header area visibility
            if( m_headerArea )
            {
                if( auto headerActor = m_headerArea->getActor() )
                {
                    headerActor->setVisible( m_showTabHeaders );
                }
            }

            // Update tab page layouts based on orientation and position
            for( auto &tabPage : m_tabPages )
            {
                if( tabPage )
                {
                    tabPage->updateElementState();
                }
            }

            // Adjust layout based on orientation and tab position
            if( auto layoutTransform = actor->getComponent<LayoutTransform>() )
            {
                layoutTransform->updateTransform();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Parameter TabView::handleEvent( EventType eventType, hash_type eventValue,
                                    const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                    SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventType == EventType::UI )
        {
            if( eventValue == StringUtil::getHash( "TabChanged" ) )
            {
                // A child TabPage's active tab changed — forward as a TabView-level event
                if( auto actor = getActor() )
                {
                    UIComponent::handleEvent( EventType::UI, StringUtil::getHash( "TabViewChanged" ),
                                              arguments, actor, this, nullptr );
                }
            }
        }

        return UIComponent::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

}  // namespace workphone::scene
