#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIDropdownCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/State/States/UIDropdownStateData.hpp>
#include <Workphone/Workphone.hpp>

extern "C" {
#include <workphone.h>
#include <workphone_layout.h>
#include <workphone_types.h>
}

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIDropdownCore, UIElementCore<UIDropdown> );

        namespace
        {
            const String itemHeightStr = "itemHeight";
            const String dropdownWidthStr = "dropdownWidth";
            const String dropdownHeightStr = "dropdownHeight";
            const String selectedOptionStr = "selectedOption";
            const String optionStr = "Option";
            const String textStr = "text";
        }  // namespace

        UIDropdownCore::UIDropdownCore()
        {
            createStateContext();
        }

        UIDropdownCore::~UIDropdownCore()
        {
            unload( nullptr );
        }

        void UIDropdownCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                UIElementCore<UIDropdown>::load( data );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIDropdownCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    setLoadingState( LoadingState::Unloading );

                    UIElementCore<UIDropdown>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIDropdownCore::update()
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

            auto ui = (UIManagerCore *)applicationManager->getRenderUIPtr();
            if( !ui )
            {
                return;
            }

            auto *ctx = ui->getContext();
            if( !ctx )
            {
                return;
            }

            auto stateContext = getStateContext();
            if( !stateContext )
            {
                return;
            }

            auto options = getOptions();

            if( options.empty() )
            {
                UIElementCore<UIDropdown>::update();
                return;
            }

            // Build a contiguous array of C-string pointers for wp_combo.
            Array<const c8 *> itemPtrs;
            itemPtrs.reserve( options.size() );

            for( const auto &opt : options )
            {
                itemPtrs.push_back( reinterpret_cast<const c8 *>( opt.c_str() ) );
            }

            auto position = getPosition();
            auto size = getSize();

            struct wp_rect bounds;
            UIUtilCore::calculateBounds( position, size, &bounds );

            wp_layout_space_push( ctx, bounds );

            const struct wp_vec2f dropSize = wp_make_vec2( m_dropdownWidth, m_dropdownHeight );
            auto selectedOption = getSelectedOption();

            if( selectedOption >= options.size() )
            {
                WP_LOG_ERROR( "Dropdown selected option is out of range. Resetting to option 0." );
                selectedOption = 0u;
                if( auto writeState =
                        stateContext->invalidateStateDataById<UIDropdownStateData>( getId(), false ) )
                {
                    writeState->selectedOption = selectedOption;
                }
            }

            const wp_s32 prevSelected = static_cast<wp_s32>( selectedOption );

            const auto itemsPtr = reinterpret_cast<const wp_c8 **>( itemPtrs.data() );
            const wp_s32 newSelected =
                wp_combo( ctx, itemsPtr, static_cast<wp_s32>( itemPtrs.size() ), prevSelected,
                          static_cast<wp_s32>( m_itemHeight ), dropSize );

            if( newSelected != prevSelected )
            {
                if( newSelected < 0 || newSelected >= static_cast<wp_s32>( options.size() ) )
                {
                    WP_LOG_ERROR( "Dropdown returned an invalid selected option index." );
                    UIElementCore<UIDropdown>::update();
                    return;
                }

                if( auto writeState =
                        stateContext->invalidateStateDataById<UIDropdownStateData>( getId(), false ) )
                {
                    writeState->selectedOption = static_cast<u32>( newSelected );
                }

                Array<Parameter> args;
                args.push_back( Parameter( static_cast<u32>( newSelected ) ) );

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

            UIElementCore<UIDropdown>::update();
        }

        f32 UIDropdownCore::getItemHeight() const
        {
            return m_itemHeight;
        }

        void UIDropdownCore::setItemHeight( f32 itemHeight )
        {
            m_itemHeight = MathF::max( itemHeight, 1.0f );
        }

        f32 UIDropdownCore::getDropdownWidth() const
        {
            return m_dropdownWidth;
        }

        void UIDropdownCore::setDropdownWidth( f32 dropdownWidth )
        {
            m_dropdownWidth = MathF::max( dropdownWidth, 1.0f );
        }

        f32 UIDropdownCore::getDropdownHeight() const
        {
            return m_dropdownHeight;
        }

        void UIDropdownCore::setDropdownHeight( f32 dropdownHeight )
        {
            m_dropdownHeight = MathF::max( dropdownHeight, 1.0f );
        }

        SmartPtr<Properties> UIDropdownCore::getProperties() const
        {
            auto properties = UIElementCore<UIDropdown>::getProperties();
            properties->setProperty( itemHeightStr, m_itemHeight );
            properties->setProperty( dropdownWidthStr, m_dropdownWidth );
            properties->setProperty( dropdownHeightStr, m_dropdownHeight );
            properties->setProperty( selectedOptionStr, getSelectedOption() );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                auto factoryManager = applicationManager->getFactoryManagerPtr();
                if( factoryManager )
                {
                    for( const auto &option : getOptions() )
                    {
                        auto optionProperties = factoryManager->make_ptr<Properties>();
                        optionProperties->setName( optionStr );
                        optionProperties->setProperty( textStr, option );
                        properties->addChild( optionProperties );
                    }
                }
                else
                {
                    WP_LOG_ERROR(
                        "Factory manager is not available while serialising dropdown options." );
                }
            }
            else
            {
                WP_LOG_ERROR(
                    "Application manager is not available while serialising dropdown options." );
            }

            return properties;
        }

        void UIDropdownCore::setProperties( SmartPtr<Properties> properties )
        {
            UIElementCore<UIDropdown>::setProperties( properties );

            properties->getPropertyValue( itemHeightStr, m_itemHeight );
            properties->getPropertyValue( dropdownWidthStr, m_dropdownWidth );
            properties->getPropertyValue( dropdownHeightStr, m_dropdownHeight );

            u32 selectedOption = getSelectedOption();
            if( properties->getPropertyValue( selectedOptionStr, selectedOption ) )
            {
                setSelectedOption( selectedOption );
            }

            auto optionProperties = properties->getChildrenByName( optionStr );
            if( !optionProperties.empty() )
            {
                auto options = Array<String>();
                options.reserve( optionProperties.size() );

                for( auto optionProperty : optionProperties )
                {
                    if( !optionProperty )
                    {
                        WP_LOG_ERROR( "Skipping null dropdown option properties." );
                        continue;
                    }

                    String option;
                    optionProperty->getPropertyValue( textStr, option );
                    options.push_back( option );
                }

                setOptions( options );
            }

            setItemHeight( m_itemHeight );
            setDropdownWidth( m_dropdownWidth );
            setDropdownHeight( m_dropdownHeight );

            const auto optionCount = getOptions().size();
            if( optionCount > 0 && getSelectedOption() >= optionCount )
            {
                WP_LOG_ERROR( "Dropdown selected option is out of range. Resetting to option 0." );
                setSelectedOption( 0u );
            }
        }

        void UIDropdownCore::createStateContext()
        {
            WP_ASSERT( getStateContext() == nullptr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            auto transformState = factoryManager->make_ptr<State>();
            transformState->setId( getId() );
            transformState->setOwner( this );
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transformState->setData( transformStateData );

            auto dropdownState = factoryManager->make_ptr<State>();
            dropdownState->setId( getId() );
            dropdownState->setOwner( this );
            stateContext->addState( dropdownState );

            auto dropdownStateData = factoryManager->make_ptr<UIDropdownStateData>();
            dropdownState->setData( dropdownStateData );

            setStateContext( stateContext );
            setStateListener( listener );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

    }  // namespace ui
}  // namespace workphone
