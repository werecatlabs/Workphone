#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Toggle.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Toggle, UIComponent );

    const String Toggle::toggleStr = String( "toggle" );
    const String Toggle::toggleBgTransformStr = String( "toggleBgTransform" );
    const String Toggle::toggleTransformStr = String( "toggleTransform" );
    const String Toggle::toggledColourStr = String( "toggledColour" );
    const String Toggle::untoggledColourStr = String( "untoggledColour" );
    const String Toggle::labelStr = String( "label" );
    const String Toggle::textSizeStr = String( "textSize" );
    const String Toggle::showLabelStr = String( "showLabel" );
    const String Toggle::toggleTypeStr = String( "toggleType" );
    const String Toggle::toggleStateStr = String( "toggleState" );
    const String Toggle::toggledPositionFactorStr = String( "toggledPositionFactor" );
    const String Toggle::untoggledPositionFactorStr = String( "untoggledPositionFactor" );

    namespace
    {
        const Array<String> toggleTypeNames = { "CheckBox", "RadioButton", "ToggleSwitch" };
        const Array<String> toggleStateNames = { "Off", "On", "Indeterminate" };

        bool isValidToggleType( s32 value )
        {
            return value >= 0 && value < static_cast<s32>( toggleTypeNames.size() );
        }

        bool isValidToggleState( s32 value )
        {
            return value >= 0 && value < static_cast<s32>( toggleStateNames.size() );
        }
    }  // namespace

    Toggle::Toggle() = default;

    Toggle::~Toggle()
    {
    }

    void Toggle::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            createUI();
            UIComponent::load( data );

            setHandleInputEvents( true );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Toggle::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                m_toggleBgTransform = nullptr;
                m_toggleTransform = nullptr;

                UIComponent::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> Toggle::getProperties() const
    {
        auto properties = UIComponent::getProperties();
        properties->setProperty( toggleStr, m_isToggled );
        properties->setPropertyAsType( toggleBgTransformStr, m_toggleBgTransform );
        properties->setPropertyAsType( toggleTransformStr, m_toggleTransform );
        properties->setProperty( toggledColourStr, m_toggledColour );
        properties->setProperty( untoggledColourStr, m_untoggledColour );
        properties->setProperty( labelStr, m_label );
        properties->setProperty( textSizeStr, m_textSize );
        properties->setProperty( showLabelStr, m_showLabel );
        properties->setPropertyAsEnum( toggleTypeStr, static_cast<s32>( m_toggleType ),
                                       toggleTypeNames );
        properties->setPropertyAsEnum( toggleStateStr, static_cast<s32>( m_toggleState ),
                                       toggleStateNames );
        properties->setProperty( toggledPositionFactorStr, m_toggledPositionFactor );
        properties->setProperty( untoggledPositionFactorStr, m_untoggledPositionFactor );
        return properties;
    }

    void Toggle::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "Toggle::setProperties received null properties." );
            return;
        }

        UIComponent::setProperties( properties );

        auto toggled = isToggled();
        auto label = getLabel();

        properties->getPropertyValue( toggleStr, toggled );
        properties->getPropertyAsType( toggleBgTransformStr, m_toggleBgTransform );
        properties->getPropertyAsType( toggleTransformStr, m_toggleTransform );
        properties->getPropertyValue( toggledColourStr, m_toggledColour );
        properties->getPropertyValue( untoggledColourStr, m_untoggledColour );
        properties->getPropertyValue( labelStr, label );
        properties->getPropertyValue( textSizeStr, m_textSize );
        properties->getPropertyValue( showLabelStr, m_showLabel );
        properties->getPropertyValue( toggledPositionFactorStr, m_toggledPositionFactor );
        properties->getPropertyValue( untoggledPositionFactorStr, m_untoggledPositionFactor );

        setLabel( label );

        s32 toggleType = static_cast<s32>( m_toggleType );
        if( properties->getPropertyValue( toggleTypeStr, toggleType ) )
        {
            if( isValidToggleType( toggleType ) )
            {
                m_toggleType = static_cast<ToggleType>( toggleType );
            }
            else
            {
                WP_LOG_ERROR( "Toggle properties contained an invalid toggle type." );
            }
        }

        s32 toggleState = static_cast<s32>( m_toggleState );
        const auto hasToggleStateProperty = properties->getPropertyValue( toggleStateStr, toggleState );
        if( hasToggleStateProperty )
        {
            if( isValidToggleState( toggleState ) )
            {
                m_toggleState = static_cast<ToggleState>( toggleState );
                toggled = m_toggleState == ToggleState::On;
            }
            else
            {
                WP_LOG_ERROR( "Toggle properties contained an invalid toggle state." );
            }
        }

        setLabel( m_label );
        setTextSize( m_textSize );
        setShowLabel( m_showLabel );
        setToggleType( m_toggleType );
        setToggledPositionFactor( m_toggledPositionFactor );
        setUntoggledPositionFactor( m_untoggledPositionFactor );
        if( hasToggleStateProperty )
        {
            setToggleState( m_toggleState );
        }
        else
        {
            setToggled( toggled );
        }
    }

    bool Toggle::isToggled() const
    {
        return m_isToggled;
    }

    void Toggle::setToggled( bool toggled )
    {
        m_isToggled = toggled;
        m_toggleState = toggled ? ToggleState::On : ToggleState::Off;

        if( auto element = workphone::dynamic_pointer_cast<ui::IUIToggle>( getElement() ) )
        {
            element->setToggled( m_isToggled );
            element->setToggleState( m_toggleState );
        }

        updateTransform();
        updateColour();
    }

    SmartPtr<LayoutTransform> Toggle::getToggleTransform() const
    {
        return m_toggleTransform;
    }

    void Toggle::setToggleTransform( SmartPtr<LayoutTransform> toggleTransform )
    {
        m_toggleTransform = toggleTransform;
        updateTransform();
    }

    FSMReturnType Toggle::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        UIComponent::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                createUI();
                updateElementState();
                updateVisibility();
                updateColour();
            }
            break;
            }
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    Parameter Toggle::handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::CLICK_HASH )
        {
            if( !arguments.empty() && arguments.front().type == ParameterType::PARAM_TYPE_BOOL )
            {
                setToggled( arguments.front().getBool() );
            }
            else
            {
                auto toggled = isToggled();
                setToggled( !toggled );
            }
        }

        return UIComponent::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    void Toggle::createUI()
    {
        try
        {
            auto state = getState();
            switch( state )
            {
            case State::Edit:
            case State::Play:
            {
                if( auto actor = getActor() )
                {
                    auto enabled = isEnabled() && actor->isEnabledInScene();
                    if( enabled )
                    {
                        if( !getElement() )
                        {
                            auto applicationManager = core::IApplicationManager::instance();
                            WP_ASSERT( applicationManager );

                            auto renderUI = applicationManager->getRenderUI();
                            if( !renderUI )
                            {
                                return;
                            }

                            if( auto parentActor = actor->getSceneRoot() )
                            {
                                auto toggle = renderUI->addElementByType<ui::IUIToggle>();
                                setElement( toggle );

                                updateElementState();
                                updateVisibility();
                            }
                        }
                    }
                }
            }
            break;
            default:
            {
            }
            break;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Toggle::updateElementState()
    {
        ScopedLock lock( this );

        auto element = getElement();
        if( auto toggle = workphone::dynamic_pointer_cast<ui::IUIToggle>( element ) )
        {
            toggle->setLabel( m_label );
            toggle->setTextSize( m_textSize );
            toggle->setShowLabel( m_showLabel );
            toggle->setToggleType( m_toggleType );
            toggle->setToggled( m_isToggled );
            toggle->setToggleState( m_toggleState );

            auto cascadeInput = getCascadeInput();
            toggle->setHandleInputEvents( cascadeInput );
        }
        else if( element )
        {
            WP_LOG_ERROR( "Toggle has a UI element that is not an IUIToggle." );
        }

        updateColour();
    }

    void Toggle::updateTransform()
    {
        auto toggled = isToggled();

        if( auto toggleTransform = getToggleTransform() )
        {
            auto actor = getActor();
            if( !actor )
            {
                WP_LOG_ERROR( "Toggle cannot update transform without an actor." );
                return;
            }

            auto layoutTransform = actor->getComponent<LayoutTransform>();
            if( !layoutTransform )
            {
                WP_LOG_ERROR( "Toggle cannot update transform without a LayoutTransform." );
                return;
            }

            auto size = layoutTransform->getSize();
            const auto factor = toggled ? m_toggledPositionFactor : m_untoggledPositionFactor;
            const auto pos = Vector2<real_Num>( size.x * factor.x, size.y * factor.y );
            toggleTransform->setPosition( pos );
        }
        else if( getToggleType() != ToggleType::ToggleButton )
        {
            WP_LOG_ERROR( "Toggle transform is not assigned; skipping thumb transform update." );
        }
    }

    void Toggle::updateColour()
    {
        if( auto bgTransform = getToggleBgTransform() )
        {
            if( auto actor = bgTransform->getActor() )
            {
                if( auto image = actor->getComponent<Image>() )
                {
                    if( auto element = image->getElement() )
                    {
                        auto toggled = isToggled();
                        auto toggledColour = getToggledColour();
                        auto untoggledColour = getUntoggledColour();

                        auto colour = toggled ? toggledColour : untoggledColour;
                        element->setColour( colour );
                    }
                    else
                    {
                        WP_LOG_ERROR( "Toggle background image has no UI element." );
                    }
                }
                else
                {
                    WP_LOG_ERROR( "Toggle background transform actor has no Image component." );
                }
            }
            else
            {
                WP_LOG_ERROR( "Toggle background transform has no actor." );
            }
        }
        else if( getToggleType() != ToggleType::ToggleButton )
        {
            WP_LOG_ERROR( "Toggle background transform is not assigned; skipping colour update." );
        }
    }

    void Toggle::setUntoggledColour( const ColourF &untoggledColour )
    {
        m_untoggledColour = untoggledColour;
        updateColour();
    }

    ColourF Toggle::getUntoggledColour() const
    {
        return m_untoggledColour;
    }

    void Toggle::setToggledColour( const ColourF &toggledColour )
    {
        m_toggledColour = toggledColour;
        updateColour();
    }

    ColourF Toggle::getToggledColour() const
    {
        return m_toggledColour;
    }

    void Toggle::setToggleBgTransform( SmartPtr<LayoutTransform> toggleBgTransform )
    {
        m_toggleBgTransform = toggleBgTransform;
        updateColour();
    }

    SmartPtr<LayoutTransform> Toggle::getToggleBgTransform() const
    {
        return m_toggleBgTransform;
    }

    String Toggle::getLabel() const
    {
        return m_label;
    }

    void Toggle::setLabel( const String &label )
    {
        m_label = label;
        if( auto toggle = workphone::dynamic_pointer_cast<ui::IUIToggle>( getElement() ) )
        {
            toggle->setLabel( m_label );
        }
    }

    f32 Toggle::getTextSize() const
    {
        return m_textSize;
    }

    void Toggle::setTextSize( f32 textSize )
    {
        m_textSize = MathF::max( textSize, 0.0f );
        if( auto toggle = workphone::dynamic_pointer_cast<ui::IUIToggle>( getElement() ) )
        {
            toggle->setTextSize( m_textSize );
        }
    }

    bool Toggle::getShowLabel() const
    {
        return m_showLabel;
    }

    void Toggle::setShowLabel( bool showLabel )
    {
        m_showLabel = showLabel;
        if( auto toggle = workphone::dynamic_pointer_cast<ui::IUIToggle>( getElement() ) )
        {
            toggle->setShowLabel( m_showLabel );
        }
    }

    Toggle::ToggleType Toggle::getToggleType() const
    {
        return m_toggleType;
    }

    void Toggle::setToggleType( ToggleType toggleType )
    {
        const auto value = static_cast<s32>( toggleType );
        if( !isValidToggleType( value ) )
        {
            WP_LOG_ERROR( "Attempted to set an invalid Toggle type." );
            return;
        }

        m_toggleType = toggleType;
        if( auto toggle = workphone::dynamic_pointer_cast<ui::IUIToggle>( getElement() ) )
        {
            toggle->setToggleType( m_toggleType );
        }
    }

    Toggle::ToggleState Toggle::getToggleState() const
    {
        return m_toggleState;
    }

    void Toggle::setToggleState( ToggleState toggleState )
    {
        const auto value = static_cast<s32>( toggleState );
        if( !isValidToggleState( value ) )
        {
            WP_LOG_ERROR( "Attempted to set an invalid Toggle state." );
            return;
        }

        m_toggleState = toggleState;
        m_isToggled = m_toggleState == ToggleState::On;
        if( auto toggle = workphone::dynamic_pointer_cast<ui::IUIToggle>( getElement() ) )
        {
            toggle->setToggleState( m_toggleState );
        }

        updateTransform();
        updateColour();
    }

    Vector2<real_Num> Toggle::getToggledPositionFactor() const
    {
        return m_toggledPositionFactor;
    }

    void Toggle::setToggledPositionFactor( const Vector2<real_Num> &factor )
    {
        m_toggledPositionFactor = factor;
        updateTransform();
    }

    Vector2<real_Num> Toggle::getUntoggledPositionFactor() const
    {
        return m_untoggledPositionFactor;
    }

    void Toggle::setUntoggledPositionFactor( const Vector2<real_Num> &factor )
    {
        m_untoggledPositionFactor = factor;
        updateTransform();
    }
}  // namespace workphone::scene
