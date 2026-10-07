#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Text, UIComponent );

    // Static const property key string definitions
    const String Text::textPropertyStr = String( "text" );
    const String Text::sizePropertyStr = String( "size" );
    const String Text::colourPropertyStr = String( "colour" );
    const String Text::verticalAlignmentPropertyStr = String( "verticalAlignment" );
    const String Text::horizontalAlignmentPropertyStr = String( "horizontalAlignment" );

    Text::Text()
    {
        setColour( ColourF::Black );
    }

    Text::~Text()
    {
    }

    void Text::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            createUI();
            UIComponent::load( data );

            if( data )
            {
                if( data->isExactly<Properties>() )
                {
                    setProperties( data );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Text::createUI()
    {
        try
        {
            if( auto actor = getActorPtr() )
            {
                auto enabled = isEnabled() && actor->isEnabledInScene();
                if( enabled )
                {
                    auto element = getElement();
                    if( !element )
                    {
                        auto applicationManager = core::IApplicationManager::instance();
                        auto renderUI = applicationManager->getRenderUI();
                        if( !renderUI )
                        {
                            return;
                        }

                        if( auto text = renderUI->addElementByType<ui::IUIText>() )
                        {
                            setTextObject( text );
                            setElement( text );

                            auto textStr = getText();
                            text->setText( textStr );
                        }

                        updateVisibility();
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Text::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &state = getLoadingState();
            if( state != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                m_textObject = nullptr;
                UIComponent::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Text::getTextObject() const -> SmartPtr<ui::IUIText>
    {
        return m_textObject;
    }

    void Text::setTextObject( SmartPtr<ui::IUIText> textObject )
    {
        m_textObject = textObject;
    }

    auto Text::getText() const -> String
    {
        return m_text;
    }

    void Text::setText( const String &text )
    {
        m_text = text;

        if( m_textObject )
        {
            m_textObject->setText( text );
        }
    }

    void Text::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActor() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();

                if( !m_textObject )
                {
                    createUI();
                }

                if( m_textObject )
                {
                    m_textObject->setVisible( visible );
                }

                if( auto text = getTextObject() )
                {
                    text->setText( m_text );
                }

                updateTransform();
            }
        }
    }

    auto Text::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = UIComponent::getChildObjects();

        if( m_textObject )
        {
            objects.emplace_back( m_textObject );
        }

        return objects;
    }

    auto Text::getProperties() const -> SmartPtr<Properties>
    {
        if( auto properties = UIComponent::getProperties() )
        {
            properties->setProperty( textPropertyStr, m_text );

            properties->setProperty( sizePropertyStr, m_size );

            properties->setProperty( colourPropertyStr, getColour() );

            auto verticalAlignmentOptions = Array<String>{ "Top", "Bottom", "Center" };
            properties->setPropertyAsEnum( verticalAlignmentPropertyStr,
                                           static_cast<u32>( m_verticalAlignment ),
                                           verticalAlignmentOptions );

            auto horizontalAlignmentOptions = Array<String>{ "Left", "Right", "Center" };
            properties->setPropertyAsEnum( horizontalAlignmentPropertyStr,
                                           static_cast<u32>( m_horizontalAlignment ),
                                           horizontalAlignmentOptions );

            return properties;
        }

        return nullptr;
    }

    void Text::setProperties( SmartPtr<Properties> properties )
    {
        u32 verticalAlignment = 0;
        u32 horizontalAlignment = 0;

        auto text = getText();

        properties->getPropertyValue( textPropertyStr, text );
        properties->getPropertyValue( sizePropertyStr, m_size );
        auto colour = getColour();
        properties->getPropertyValue( colourPropertyStr, colour );
        setColour( colour );
        properties->getPropertyValue( verticalAlignmentPropertyStr, verticalAlignment );
        properties->getPropertyValue( horizontalAlignmentPropertyStr, horizontalAlignment );

        setText( text );

        m_verticalAlignment = verticalAlignment;
        m_horizontalAlignment = horizontalAlignment;

        updateElementState();

        UIComponent::setProperties( properties );
    }

    void Text::updateElementState()
    {
        if( auto text = getTextObject() )
        {
            text->setText( m_text );
            text->setTextSize( static_cast<f32>( m_size ) );
            text->setColour( getColour() );
            text->setVerticalAlignment( m_verticalAlignment );
            text->setHorizontalAlignment( m_horizontalAlignment );
        }
    }

    void Text::setHorizontalAlignment( u8 horizontalAlignment )
    {
        m_horizontalAlignment = horizontalAlignment;
        updateElementState();
    }

    u8 Text::getHorizontalAlignment() const
    {
        return m_horizontalAlignment;
    }

    void Text::setVerticalAlignment( u8 verticalAlignment )
    {
        m_verticalAlignment = verticalAlignment;
        updateElementState();
    }

    u8 Text::getVerticalAlignment() const
    {
        return m_verticalAlignment;
    }

}  // namespace workphone::scene
