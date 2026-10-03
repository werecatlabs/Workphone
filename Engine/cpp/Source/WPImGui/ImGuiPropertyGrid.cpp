#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiPropertyGrid.hpp>
#include <WPImGui/ImGuiUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiPropertyGrid, IUIPropertyGrid );
    u32 ImGuiPropertyGrid::m_childWindowCount = 0;

    ImGuiPropertyGrid::ImGuiPropertyGrid()
    {
        auto name = "PropertyGrid_" + StringUtil::toString( m_childWindowCount++ );
        setName( name );
    }

    ImGuiPropertyGrid::~ImGuiPropertyGrid() = default;

    SmartPtr<Properties> ImGuiPropertyGrid::getProperties() const
    {
        return m_properties;
    }

    void ImGuiPropertyGrid::setProperties( SmartPtr<Properties> properties )
    {
        m_properties = properties;
    }

    void ImGuiPropertyGrid::createElement( SmartPtr<Properties> properties, SmartPtr<Properties> parent )
    {
        if( !properties )
        {
            return;
        }

        auto categories = Array<String>();
        auto hasUncategorizedProperties = false;
        for( const auto &property : properties->getPropertiesAsArray() )
        {
            if( !matchesFilter( property ) )
            {
                continue;
            }

            auto category = property.getAttribute( "category" );
            if( StringUtil::isNullOrEmpty( category ) )
            {
                hasUncategorizedProperties = true;
            }
            else if( std::find( categories.begin(), categories.end(), category ) == categories.end() )
            {
                categories.push_back( category );
            }
        }

        ImGui::PushID( properties.get() );

        if( hasUncategorizedProperties )
        {
            drawPropertyTable( properties, parent, String(), true );
        }

        for( const auto &category : categories )
        {
            if( ImGui::CollapsingHeader( category.c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                drawPropertyTable( properties, parent, category, false );
            }
        }

        for( auto &child : properties->getChildren() )
        {
            if( child && hasMatchingProperties( child ) )
            {
                auto label = child->getName();
                if( StringUtil::isNullOrEmpty( label ) )
                {
                    label = "Properties";
                }

                ImGui::PushID( child.get() );
                if( ImGui::CollapsingHeader( label.c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
                {
                    createElement( child, properties );
                }
                ImGui::PopID();
            }
        }

        ImGui::PopID();
    }

    void ImGuiPropertyGrid::createElement( SmartPtr<IUIElement> element )
    {
        if( element )
        {
            auto propertyGrid = workphone::dynamic_pointer_cast<IUIPropertyGrid>( element );
            WP_ASSERT( propertyGrid );

            if( propertyGrid )
            {
                if( auto properties = propertyGrid->getProperties() )
                {
                    auto imGuiPropertyGrid =
                        workphone::dynamic_pointer_cast<ImGuiPropertyGrid>( propertyGrid );
                    if( !imGuiPropertyGrid )
                    {
                        return;
                    }

                    ImGui::PushID( imGuiPropertyGrid.get() );
                    char filterBuffer[256] = {};
                    StringUtil::toBuffer( imGuiPropertyGrid->m_filter, filterBuffer,
                                          sizeof( filterBuffer ) );
                    ImGui::SetNextItemWidth( -FLT_MIN );
                    if( ImGui::InputTextWithHint( "##PropertyFilter", "Search properties...",
                                                  filterBuffer, sizeof( filterBuffer ) ) )
                    {
                        imGuiPropertyGrid->m_filter = String( filterBuffer );
                    }

                    ImGui::Separator();
                    imGuiPropertyGrid->createElement( properties, nullptr );
                    ImGui::PopID();
                }
            }
        }
    }

    bool ImGuiPropertyGrid::matchesFilter( const Property &property ) const
    {
        if( StringUtil::isNullOrEmpty( m_filter ) )
        {
            return true;
        }

        auto filter = StringUtil::make_lower( m_filter );
        auto searchableText = property.getName() + " " + property.getAttribute( "label" ) + " " +
                              property.getAttribute( "category" ) + " " +
                              property.getAttribute( "description" );
        searchableText = StringUtil::make_lower( searchableText );
        return StringUtil::contains( searchableText, filter );
    }

    bool ImGuiPropertyGrid::hasMatchingProperties( SmartPtr<Properties> properties ) const
    {
        if( !properties )
        {
            return false;
        }

        for( const auto &property : properties->getPropertiesAsArray() )
        {
            if( matchesFilter( property ) )
            {
                return true;
            }
        }

        for( const auto &child : properties->getChildren() )
        {
            if( hasMatchingProperties( child ) )
            {
                return true;
            }
        }

        return false;
    }

    void ImGuiPropertyGrid::drawPropertyTable( SmartPtr<Properties> properties,
                                               SmartPtr<Properties> parent,
                                               const String &category, bool uncategorized )
    {
        ImGui::PushID( uncategorized ? "Uncategorized" : category.c_str() );

        auto tableFlags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable |
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
        if( ImGui::BeginTable( "Properties", 2, tableFlags ) )
        {
            ImGui::TableSetupColumn( "Property", ImGuiTableColumnFlags_WidthStretch, 0.45f );
            ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch, 0.55f );

            for( auto &property : properties->getPropertiesAsArray() )
            {
                if( !matchesFilter( property ) )
                {
                    continue;
                }

                auto propertyCategory = property.getAttribute( "category" );
                if( uncategorized != StringUtil::isNullOrEmpty( propertyCategory ) ||
                    ( !uncategorized && propertyCategory != category ) )
                {
                    continue;
                }

                auto name = property.getName();
                auto value = property.getValue();
                auto label = property.getAttribute( "label" );
                if( StringUtil::isNullOrEmpty( label ) )
                {
                    label = name;
                }

                ImGui::PushID( name.c_str() );
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex( 0 );
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted( label.c_str() );

                auto description = property.getAttribute( "description" );
                if( !StringUtil::isNullOrEmpty( description ) && ImGui::IsItemHovered() )
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos( ImGui::GetFontSize() * 32.0f );
                    ImGui::TextUnformatted( description.c_str() );
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }

                ImGui::TableSetColumnIndex( 1 );
                ImGui::SetNextItemWidth( -FLT_MIN );
                createProperty( properties, property, this );

                if( auto dropTarget = getDropTarget() )
                {
                    if( ImGui::BeginDragDropTarget() )
                    {
                        if( auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" ) )
                        {
                            auto data = String( static_cast<const char *>( payload->Data ),
                                                payload->DataSize );
                            auto args = Array<Parameter>( 5 );
                            args[0].setStr( data );
                            args[1].setStr( name );
                            args[2].setStr( value );
                            args[3].setObject( properties );
                            args[4].setObject( parent );
                            dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, args, this,
                                                     this, nullptr );
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::PopID();
    }

    void ImGuiPropertyGrid::handlePropertyButtonClicked( SmartPtr<IUIPropertyGrid> propertyGrid,
                                                         const String &name, const String &value )
    {
        WP_ASSERT( propertyGrid );

        auto listeners = propertyGrid->getObjectListeners();
        for( auto &listener : listeners )
        {
            auto args = Array<Parameter>();
            args.resize( 3 );

            args[0].setStr( name );
            args[1].setStr( value );
            args[2].setObject( propertyGrid->getProperties() );

            listener->handleEvent( EventType::Object, IEvent::handlePropertyButtonClick, args,
                                   propertyGrid, propertyGrid, nullptr );
        }
    }

    void ImGuiPropertyGrid::handlePropertyChanged( SmartPtr<IUIPropertyGrid> propertyGrid,
                                                   SmartPtr<Properties> properties, const String &name,
                                                   const String &str )
    {
        auto listeners = propertyGrid->getObjectListeners();
        for( auto &listener : listeners )
        {
            auto args = Array<Parameter>();
            args.resize( 3 );

            args[0].setStr( name );
            args[1].setStr( str );
            args[2].setObject( properties );

            listener->handleEvent( EventType::Object, IEvent::handlePropertyChanged, args, propertyGrid,
                                   propertyGrid, nullptr );
        }
    }

    void ImGuiPropertyGrid::createProperty( SmartPtr<Properties> properties, Property &property,
                                            SmartPtr<IUIPropertyGrid> propertyGrid )
    {
        auto name = property.getName();
        if( StringUtil::isNullOrEmpty( name ) )
        {
            name = "UnnamedProperty";
        }

        const auto value = property.getValue();
        const auto type = property.getTypeName();
        const auto widgetId = "##value";
        const auto readOnly = property.isReadOnly();

        const auto minAttribute = property.getAttribute( "min" );
        const auto maxAttribute = property.getAttribute( "max" );
        const auto stepAttribute = property.getAttribute( "step" );
        const auto hasMinimum = !StringUtil::isNullOrEmpty( minAttribute );
        const auto hasMaximum = !StringUtil::isNullOrEmpty( maxAttribute );

        auto clampFloat = [&]( f32 v ) {
            if( hasMinimum )
            {
                v = std::max( v, StringUtil::parseFloat( minAttribute ) );
            }
            if( hasMaximum )
            {
                v = std::min( v, StringUtil::parseFloat( maxAttribute ) );
            }
            return v;
        };

        auto commit = [&]( const String &newValue ) {
            properties->setProperty( name, newValue );
            handlePropertyChanged( propertyGrid, properties, name, newValue );
        };

        ImGui::BeginDisabled( readOnly );

        if( StringUtil::isEqual( type, Util::enumTypeStr, true ) )
        {
            auto enumArray = Array<String>();
            StringUtil::parseArray( property.getAttribute( Util::enumTypeStr ), enumArray );

            auto selectedIndex = 0;
            const auto isNumericValue = StringUtil::isNumber( value );
            if( isNumericValue )
            {
                selectedIndex = StringUtil::parseInt( value );
            }
            else
            {
                auto it = std::find( enumArray.begin(), enumArray.end(), value );
                if( it != enumArray.end() )
                {
                    selectedIndex = static_cast<s32>( std::distance( enumArray.begin(), it ) );
                }
            }

            if( !enumArray.empty() )
            {
                selectedIndex = std::max( 0, std::min( selectedIndex,
                                                       static_cast<s32>( enumArray.size() - 1 ) ) );
                auto enumItems = String();
                for( const auto &entry : enumArray )
                {
                    enumItems += entry;
                    enumItems += '\0';
                }
                enumItems += '\0';

                if( ImGui::Combo( widgetId, &selectedIndex, enumItems.c_str() ) )
                {
                    commit( isNumericValue ? StringUtil::toString( selectedIndex ) :
                                             enumArray[selectedIndex] );
                }
            }
            else
            {
                ImGui::TextUnformatted( value.c_str() );
            }
        }
        else if( StringUtil::isEqual( type, Util::vector2iStr, true ) )
        {
            auto vector = StringUtil::parseVector2<s32>( value );
            s32 values[2] = { vector.X(), vector.Y() };
            if( ImGui::InputInt2( widgetId, values ) )
            {
                auto newValue = Vector2I( values[0], values[1] );
                properties->setProperty( name, newValue );
                handlePropertyChanged( propertyGrid, properties, name,
                                       StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::vector2Str, true ) ||
                 StringUtil::isEqual( type, Util::vector2fStr, true ) ||
                 StringUtil::isEqual( type, Util::vector2dStr, true ) )
        {
            auto vector = StringUtil::parseVector2<f32>( value );
            f32 values[2] = { vector.X(), vector.Y() };
            if( ImGui::InputFloat2( widgetId, values ) )
            {
                auto newValue = Vector2F( clampFloat( values[0] ), clampFloat( values[1] ) );
                properties->setProperty( name, newValue );
                handlePropertyChanged( propertyGrid, properties, name,
                                       StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::vector3iStr, true ) )
        {
            auto vector = StringUtil::parseVector3<s32>( value );
            s32 values[3] = { vector.X(), vector.Y(), vector.Z() };
            if( ImGui::InputInt3( widgetId, values ) )
            {
                auto newValue = Vector3I( values[0], values[1], values[2] );
                properties->setProperty( name, newValue );
                handlePropertyChanged( propertyGrid, properties, name,
                                       StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::vector3Str, true ) ||
                 StringUtil::isEqual( type, Util::vector3fStr, true ) ||
                 StringUtil::isEqual( type, Util::vector3dStr, true ) )
        {
            auto vector = StringUtil::parseVector3<f32>( value );
            f32 values[3] = { vector.X(), vector.Y(), vector.Z() };
            if( ImGui::InputFloat3( widgetId, values ) )
            {
                auto newValue = Vector3F( clampFloat( values[0] ), clampFloat( values[1] ),
                                          clampFloat( values[2] ) );
                properties->setProperty( name, newValue );
                handlePropertyChanged( propertyGrid, properties, name,
                                       StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::colourStr, true ) )
        {
            auto colour = StringUtil::parseColourf( value );
            f32 values[4] = { colour.r, colour.g, colour.b, colour.a };
            if( ImGui::ColorEdit4( widgetId, values ) )
            {
                auto newValue = ColourF( values[0], values[1], values[2], values[3] );
                properties->setProperty( name, newValue );
                handlePropertyChanged( propertyGrid, properties, name,
                                       StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::resourceStr, true ) )
        {
            if( ImGui::Button( StringUtil::isNullOrEmpty( value ) ? "None" : value.c_str() ) )
            {
                handlePropertyButtonClicked( propertyGrid, name, value );
            }
        }
        else if( StringUtil::isEqual( type, Util::buttonTypeStr, true ) )
        {
            auto label = property.getAttribute( "label" );
            if( StringUtil::isNullOrEmpty( label ) )
            {
                label = name;
            }
            if( ImGui::Button( label.c_str() ) )
            {
                properties->setButtonPressed( name, true );
                handlePropertyButtonClicked( propertyGrid, name, value );
            }
        }
        else if( StringUtil::isEqual( type, Util::intStr, true ) )
        {
            auto oldValue = StringUtil::parseInt( value );
            auto newValue = oldValue;
            auto step = StringUtil::isNullOrEmpty( stepAttribute ) ? 1 :
                                                                    StringUtil::parseInt( stepAttribute );
            if( ImGui::InputInt( widgetId, &newValue, std::max( step, 1 ) ) && newValue != oldValue )
            {
                if( hasMinimum )
                    newValue = std::max( newValue, StringUtil::parseInt( minAttribute ) );
                if( hasMaximum )
                    newValue = std::min( newValue, StringUtil::parseInt( maxAttribute ) );
                commit( StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Properties::u32Str, true ) )
        {
            auto oldValue = StringUtil::parseUInt( value );
            auto newValue = oldValue;
            auto step = StringUtil::isNullOrEmpty( stepAttribute ) ? 1u :
                                                                    StringUtil::parseUInt( stepAttribute );
            if( ImGui::InputScalar( widgetId, ImGuiDataType_U32, &newValue, &step ) &&
                newValue != oldValue )
            {
                if( hasMinimum )
                    newValue = std::max( newValue, StringUtil::parseUInt( minAttribute ) );
                if( hasMaximum )
                    newValue = std::min( newValue, StringUtil::parseUInt( maxAttribute ) );
                commit( StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::floatStr, true ) )
        {
            auto oldValue = StringUtil::parseFloat( value );
            auto newValue = oldValue;
            auto step = StringUtil::isNullOrEmpty( stepAttribute ) ? 0.1f :
                                                                    StringUtil::parseFloat( stepAttribute );
            if( ImGui::InputFloat( widgetId, &newValue, step ) && newValue != oldValue )
            {
                newValue = clampFloat( newValue );
                commit( StringUtil::toString( newValue ) );
            }
        }
        else if( StringUtil::isEqual( type, Util::boolStr, true ) )
        {
            auto oldValue = StringUtil::parseBool( value );
            auto newValue = oldValue;
            if( ImGuiUtil::ToggleButton( widgetId, &newValue ) && newValue != oldValue )
            {
                commit( StringUtil::toString( newValue ) );
            }
        }
        else
        {
            constexpr auto BufferSize = 4096;
            char buffer[BufferSize] = {};
            StringUtil::toBuffer( value, buffer, BufferSize );
            const auto submitted =
                ImGui::InputText( widgetId, buffer, BufferSize,
                                  ImGuiInputTextFlags_EnterReturnsTrue );
            const auto editFinished = ImGui::IsItemDeactivatedAfterEdit();
            if( submitted || editFinished )
            {
                auto newValue = String( buffer );
                if( value != newValue )
                {
                    commit( newValue );
                }
            }
        }

        ImGui::EndDisabled();
    }
}  // namespace workphone::ui
