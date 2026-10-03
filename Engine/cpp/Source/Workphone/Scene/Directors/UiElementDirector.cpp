#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/UiElementDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, UiElementDirector, Director );

    const String UiElementDirector::positionStr = String( "Position" );
    const String UiElementDirector::sizeStr = String( "Size" );
    const String UiElementDirector::anchorStr = String( "Anchor" );
    const String UiElementDirector::anchorMinStr = String( "AnchorMin" );
    const String UiElementDirector::anchorMaxStr = String( "AnchorMax" );
    const String UiElementDirector::colourStr = String( "Colour" );
    const String UiElementDirector::zOrderStr = String( "ZOrder" );
    const String UiElementDirector::metricsModeStr = String( "MetricsMode" );
    const String UiElementDirector::ghaStr = String( "GHA" );
    const String UiElementDirector::gvaStr = String( "GVA" );
    const String UiElementDirector::visibleStr = String( "Visible" );
    const String UiElementDirector::handleInputEventsStr = String( "HandleInputEvents" );
    const String UiElementDirector::elementVisibleStr = String( "ElementVisible" );
    const String UiElementDirector::captionStr = String( "Caption" );

    UiElementDirector::UiElementDirector() = default;
    UiElementDirector::~UiElementDirector() = default;

    SmartPtr<Properties> UiElementDirector::getProperties() const
    {
        auto properties = Director::getProperties();
        setupHeaderProperties( properties );
        getButtons( properties );

        properties->setProperty( positionStr, m_position );
        properties->setProperty( sizeStr, m_size );
        properties->setProperty( anchorStr, m_anchor );
        properties->setProperty( anchorMinStr, m_anchorMin );
        properties->setProperty( anchorMaxStr, m_anchorMax );
        properties->setProperty( colourStr, m_colour );
        properties->setProperty( zOrderStr, m_zorder );
        properties->setProperty( metricsModeStr, m_metricsMode );
        properties->setProperty( ghaStr, m_gha );
        properties->setProperty( gvaStr, m_gva );
        properties->setProperty( visibleStr, m_visible );
        properties->setProperty( handleInputEventsStr, m_handleInputEvents );
        properties->setProperty( elementVisibleStr, m_elementVisible );
        properties->setProperty( captionStr, m_caption );

        return properties;
    }

    void UiElementDirector::setProperties( SmartPtr<Properties> properties )
    {
        Director::setProperties( properties );

        if( !properties )
        {
            return;
        }

        setButtons( properties );

        properties->getPropertyValue( positionStr, m_position );
        properties->getPropertyValue( sizeStr, m_size );
        properties->getPropertyValue( anchorStr, m_anchor );
        properties->getPropertyValue( anchorMinStr, m_anchorMin );
        properties->getPropertyValue( anchorMaxStr, m_anchorMax );
        properties->getPropertyValue( colourStr, m_colour );
        properties->getPropertyValue( zOrderStr, m_zorder );
        auto metricsMode = static_cast<u32>( m_metricsMode );
        auto horizontalAlignment = static_cast<u32>( m_gha );
        auto verticalAlignment = static_cast<u32>( m_gva );
        properties->getPropertyValue( metricsModeStr, metricsMode );
        properties->getPropertyValue( ghaStr, horizontalAlignment );
        properties->getPropertyValue( gvaStr, verticalAlignment );
        m_metricsMode = static_cast<u8>( std::min<u32>( metricsMode, 255u ) );
        m_gha = static_cast<u8>( std::min<u32>( horizontalAlignment, 255u ) );
        m_gva = static_cast<u8>( std::min<u32>( verticalAlignment, 255u ) );
        properties->getPropertyValue( visibleStr, m_visible );
        properties->getPropertyValue( handleInputEventsStr, m_handleInputEvents );
        properties->getPropertyValue( elementVisibleStr, m_elementVisible );
        properties->getPropertyValue( captionStr, m_caption );
    }

    void UiElementDirector::setAnchor( const Vector2F &anchor )
    {
        m_anchor = anchor;
    }

    Vector2F UiElementDirector::getAnchor() const
    {
        return m_anchor;
    }

    Vector2<real_Num> UiElementDirector::getPosition() const
    {
        return m_position;
    }

    void UiElementDirector::setPosition( const Vector2<real_Num> &position )
    {
        m_position = position;
    }

    Vector2<real_Num> UiElementDirector::getSize() const
    {
        return m_size;
    }

    void UiElementDirector::setSize( const Vector2<real_Num> &size )
    {
        m_size = size;
    }

    Vector2F UiElementDirector::getAnchorMin() const
    {
        return m_anchorMin;
    }

    void UiElementDirector::setAnchorMin( const Vector2F &anchorMin )
    {
        m_anchorMin = anchorMin;
    }

    Vector2F UiElementDirector::getAnchorMax() const
    {
        return m_anchorMax;
    }

    void UiElementDirector::setAnchorMax( const Vector2F &anchorMax )
    {
        m_anchorMax = anchorMax;
    }

    ColourF UiElementDirector::getColour() const
    {
        return m_colour;
    }

    void UiElementDirector::setColour( const ColourF &colour )
    {
        m_colour = colour;
    }

    u32 UiElementDirector::getZOrder() const
    {
        return m_zorder;
    }

    void UiElementDirector::setZOrder( u32 zOrder )
    {
        m_zorder = zOrder;
    }

    u8 UiElementDirector::getMetricsMode() const
    {
        return m_metricsMode;
    }

    void UiElementDirector::setMetricsMode( u8 metricsMode )
    {
        m_metricsMode = metricsMode;
    }

    u8 UiElementDirector::getHorizontalAlignment() const
    {
        return m_gha;
    }

    void UiElementDirector::setHorizontalAlignment( u8 horizontalAlignment )
    {
        m_gha = horizontalAlignment;
    }

    u8 UiElementDirector::getVerticalAlignment() const
    {
        return m_gva;
    }

    void UiElementDirector::setVerticalAlignment( u8 verticalAlignment )
    {
        m_gva = verticalAlignment;
    }

    u32 UiElementDirector::getFlags() const
    {
        return m_flags;
    }

    void UiElementDirector::setFlags( u32 flags )
    {
        m_flags = flags;
    }

    bool UiElementDirector::isVisible() const
    {
        return m_visible;
    }

    void UiElementDirector::setVisible( bool visible )
    {
        m_visible = visible;
    }

    bool UiElementDirector::getHandleInputEvents() const
    {
        return m_handleInputEvents;
    }

    void UiElementDirector::setHandleInputEvents( bool handleInputEvents )
    {
        m_handleInputEvents = handleInputEvents;
    }

    bool UiElementDirector::isElementVisible() const
    {
        return m_elementVisible;
    }

    void UiElementDirector::setElementVisible( bool elementVisible )
    {
        m_elementVisible = elementVisible;
    }

    String UiElementDirector::getCaption() const
    {
        return m_caption;
    }

    void UiElementDirector::setCaption( const String &caption )
    {
        m_caption = caption;
    }

}  // namespace workphone::scene
