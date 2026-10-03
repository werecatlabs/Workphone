#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Thumbnail.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    const String Thumbnail::thumbStr = String( "thumb" );
    const String Thumbnail::labelTextStr = String( "labelText" );
    const String Thumbnail::highlightObjectStr = String( "highlightObject" );
    const String Thumbnail::highlightImageStr = String( "highlightImage" );
    const String Thumbnail::highlightColorStr = String( "highlightColor" );
    const String Thumbnail::normalColorStr = String( "normalColor" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Thumbnail, UIComponent );

    Thumbnail::Thumbnail() = default;

    Thumbnail::~Thumbnail() = default;

    void Thumbnail::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            UIComponent::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Thumbnail::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );
            UIComponent::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> Thumbnail::getChildObjects() const
    {
        Array<SmartPtr<ISharedObject>> objects;
        objects.push_back( m_thumb );
        objects.push_back( m_labelText );
        objects.push_back( m_highlightObject );
        objects.push_back( m_highlightImage );

        return objects;
    }

    SmartPtr<Properties> Thumbnail::getProperties() const
    {
        try
        {
            if( auto properties = UIComponent::getProperties() )
            {
                properties->setPropertyAsType( Thumbnail::thumbStr, m_thumb );
                properties->setPropertyAsType( Thumbnail::labelTextStr, m_labelText );
                properties->setPropertyAsType( Thumbnail::highlightObjectStr, m_highlightObject );
                properties->setPropertyAsType( Thumbnail::highlightImageStr, m_highlightImage );
                properties->setProperty( Thumbnail::highlightColorStr, m_highlightColor );
                properties->setProperty( Thumbnail::normalColorStr, m_normalColor );
                return properties;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void Thumbnail::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( properties )
            {
                UIComponent::setProperties( properties );

                properties->getPropertyAsType( Thumbnail::thumbStr, m_thumb );
                properties->getPropertyAsType( Thumbnail::labelTextStr, m_labelText );
                properties->getPropertyAsType( Thumbnail::highlightObjectStr, m_highlightObject );
                properties->getPropertyAsType( Thumbnail::highlightImageStr, m_highlightImage );
                properties->getPropertyValue( Thumbnail::highlightColorStr, m_highlightColor );
                properties->getPropertyValue( Thumbnail::normalColorStr, m_normalColor );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Thumbnail::setNormalColor( const ColourF &normalColor )
    {
        m_normalColor = normalColor;
    }

    ColourF Thumbnail::getNormalColor() const
    {
        return m_normalColor;
    }

    void Thumbnail::setHighlightColor( const ColourF &highlightColor )
    {
        m_highlightColor = highlightColor;
    }

    ColourF Thumbnail::getHighlightColor() const
    {
        return m_highlightColor;
    }

    void Thumbnail::setHighlightImage( SmartPtr<Image> highlightImage )
    {
        m_highlightImage = highlightImage;
    }

    SmartPtr<Image> Thumbnail::getHighlightImage() const
    {
        return m_highlightImage;
    }

    void Thumbnail::setHighlightObject( SmartPtr<IGameActor> highlightObject )
    {
        m_highlightObject = highlightObject;
    }

    SmartPtr<IGameActor> Thumbnail::getHighlightObject() const
    {
        return m_highlightObject;
    }

    void Thumbnail::setLabelText( SmartPtr<Text> labelText )
    {
        m_labelText = labelText;
    }

    SmartPtr<Text> Thumbnail::getLabelText() const
    {
        return m_labelText;
    }

    void Thumbnail::setThumb( SmartPtr<Image> thumb )
    {
        m_thumb = thumb;
    }

    SmartPtr<Image> Thumbnail::getThumb() const
    {
        return m_thumb;
    }
}  // namespace workphone::scene
