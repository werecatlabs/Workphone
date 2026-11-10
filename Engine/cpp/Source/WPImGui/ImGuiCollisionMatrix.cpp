#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiCollisionMatrix.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Physics/CollisionLayerMatrix.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <imgui.h>
#include <algorithm>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiCollisionMatrix, IUIElement );

    namespace
    {
        constexpr auto collisionLayersProperty = "physics.collisionLayers";
        constexpr size_t maxLayers = 32;
        constexpr size_t maxSettingLength = 65536;

        void appendName( String &text, const String &name )
        {
            if( !text.empty() )
                text += ',';
            text += name;
        }

        String encodeLayers( const Array<String> &layers )
        {
            String text;
            for( const auto &layer : layers )
                appendName( text, layer );
            return text;
        }

        String encodePairs( const std::set<StringPair> &pairs )
        {
            String text;
            for( const auto &[first, second] : pairs )
            {
                appendName( text, first );
                appendName( text, second );
            }
            return text;
        }

        StringPair orderedPair( const String &first, const String &second )
        {
            const auto pair = std::minmax( first, second );
            return { pair.first, pair.second };
        }

        int resizeLayerNameInput( ImGuiInputTextCallbackData *data )
        {
            auto &name = *static_cast<String *>( data->UserData );
            name.resize( static_cast<size_t>( data->BufTextLen ) );
            data->Buf = name.data();
            return 0;
        }
    }  // namespace

    SmartPtr<Properties> ImGuiCollisionMatrix::getProperties() const
    {
        ScopedLock lock( this, false );
        auto properties = make_ptr<Properties>();
        properties->setProperty( collisionLayersProperty, m_layersText );
        properties->setProperty( physics::disabledCollisionPairsProperty, m_disabledPairsText );
        return properties;
    }

    void ImGuiCollisionMatrix::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;

        auto layersText = properties->getProperty( collisionLayersProperty );
        auto pairsText = properties->getProperty( physics::disabledCollisionPairsProperty );
        Array<String> layers;
        StringPairs pairs;
        physics::parseCollisionLayerNames( layersText, layers );
        const bool validPairs = physics::parseDisabledCollisionPairs( pairsText, pairs );
        std::set<StringPair> disabledPairs;
        for( auto &[first, second] : pairs )
        {
            if( second < first )
                std::swap( first, second );
            disabledPairs.emplace( std::move( first ), std::move( second ) );
        }

        ScopedLock lock( this );
        if( layers != m_layers )
        {
            m_selectedLayer = -1;
            m_scrollToSelectedLayer = false;
        }
        m_layerEdit = LayerEdit::None;
        m_layerError.clear();
        m_layers = std::move( layers );
        m_layersText = std::move( layersText );
        m_disabledPairs = std::move( disabledPairs );
        m_disabledPairsText = std::move( pairsText );
        m_validPairs = validPairs;
        m_editCollisionPairs = properties->getPropertyAsBool( "editCollisionPairs", true );
    }

    void ImGuiCollisionMatrix::update()
    {
        bool changed = false;
        {
            ScopedLock lock( this );
            changed = drawLayerList();
            changed |= drawMatrix();
        }
        if( changed )
        {
            for( auto listener : getObjectListeners() )
                listener->handleEvent( EventType::UI, IEvent::handleValueChanged, {},
                                       this, this, nullptr );
        }
    }

    bool ImGuiCollisionMatrix::drawLayerList()
    {
        ImGui::TextUnformatted( "Collision Layers" );
        const int count = static_cast<int>( m_layers.size() );
        const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        const float listHeight = rowHeight * std::clamp( count, 2, 6 ) +
                                 ImGui::GetStyle().FramePadding.y * 2.0f;
        if( ImGui::BeginListBox( "##CollisionLayers", ImVec2( -1.0f, listHeight ) ) )
        {
            for( int index = 0; index < count; ++index )
            {
                ImGui::PushID( index );
                const auto position = ImGui::GetCursorScreenPos();
                if( ImGui::Selectable( "##Layer", index == m_selectedLayer, 0,
                                       ImVec2( 0, ImGui::GetTextLineHeight() ) ) &&
                    index != m_selectedLayer )
                {
                    m_selectedLayer = index;
                    m_layerEdit = LayerEdit::None;
                    m_layerError.clear();
                    m_scrollToSelectedLayer = false;
                }
                if( index == m_selectedLayer && m_scrollToSelectedLayer )
                {
                    ImGui::SetScrollHereY( 1.0f );
                    m_scrollToSelectedLayer = false;
                }
                // Draw the literal name even if it contains ImGui's label delimiters.
                const auto &layer = m_layers[index];
                ImGui::GetWindowDrawList()->AddText( position, ImGui::GetColorU32( ImGuiCol_Text ),
                                                     layer.c_str() );
                if( ImGui::IsItemHovered() )
                    ImGui::SetTooltip( "%s", layer.c_str() );
                ImGui::PopID();
            }
            ImGui::EndListBox();
        }
        if( m_layers.empty() )
            ImGui::TextWrapped( "The saved layer list is invalid. Add a layer or reset the settings." );

        ImGui::BeginDisabled( !isEnabled() );
        ImGui::BeginDisabled( m_layers.size() >= maxLayers );
        const bool add = ImGui::Button( "Add Layer" );
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled( m_selectedLayer < 0 );
        const bool rename = ImGui::Button( "Rename" );
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled( m_selectedLayer < 0 || count <= 1 );
        const bool remove = ImGui::Button( "Remove" );
        ImGui::EndDisabled();

        bool changed = false;
        if( remove )
        {
            removeSelectedLayer();
            changed = true;
        }
        if( add || rename )
        {
            m_layerEdit = add ? LayerEdit::Add : LayerEdit::Rename;
            m_layerName = add ? String() : m_layers[m_selectedLayer];
            m_layerError.clear();
        }
        if( m_layerEdit != LayerEdit::None )
        {
            if( add || rename )
                ImGui::SetKeyboardFocusHere();
            ImGui::SetNextItemWidth( 240.0f );
            const auto flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll |
                               ImGuiInputTextFlags_CallbackResize;
            const auto *hint = m_layerEdit == LayerEdit::Add ? "New layer name" : "Rename layer";
            const bool enter = ImGui::InputTextWithHint( "##CollisionLayerName", hint, m_layerName.data(),
                                                        m_layerName.capacity() + 1, flags,
                                                        resizeLayerNameInput, &m_layerName );
            ImGui::SameLine();
            const bool confirm = ImGui::Button( "Done" );
            ImGui::SameLine();
            if( ImGui::Button( "Cancel" ) )
            {
                m_layerEdit = LayerEdit::None;
                m_layerError.clear();
            }
            else if( enter || confirm )
                changed |= commitLayerName();
            if( !m_layerError.empty() )
                ImGui::TextWrapped( "%s", m_layerError.c_str() );
        }
        ImGui::EndDisabled();
        ImGui::TextWrapped( "Rigidbody assignments use layer names. Update assignments after renaming or removing a layer." );
        return changed;
    }

    bool ImGuiCollisionMatrix::commitLayerName()
    {
        const auto name = StringUtil::trim( m_layerName );
        const bool rename = m_layerEdit == LayerEdit::Rename;
        const auto previous = rename ? m_layers[m_selectedLayer] : String();
        if( rename && name == previous )
        {
            m_layerEdit = LayerEdit::None;
            return false;
        }
        if( name.empty() || name.find( ',' ) != String::npos )
        {
            m_layerError = "Enter a non-empty layer name without commas.";
            return false;
        }
        if( std::find( m_layers.begin(), m_layers.end(), name ) != m_layers.end() )
        {
            m_layerError = "A layer with that name already exists.";
            return false;
        }

        auto layers = m_layers;
        if( rename )
            layers[m_selectedLayer] = name;
        else
            layers.push_back( name );

        // A name that was absent from the layer list may have dormant rules from an older profile.
        // Drop those rules before adding the name or transferring the renamed layer's rules.
        std::set<StringPair> pairs;
        for( const auto &[first, second] : m_disabledPairs )
        {
            if( first != name && second != name )
                pairs.insert( orderedPair( first == previous ? name : first,
                                           second == previous ? name : second ) );
        }

        auto layersText = encodeLayers( layers );
        const bool pairsChanged = pairs != m_disabledPairs;
        auto pairsText = pairsChanged ? encodePairs( pairs ) : m_disabledPairsText;
        if( layersText.size() > maxSettingLength || pairsText.size() > maxSettingLength )
        {
            m_layerError = "Use a shorter layer name to fit the project settings.";
            return false;
        }

        m_layers = std::move( layers );
        m_layersText = std::move( layersText );
        m_disabledPairs = std::move( pairs );
        m_disabledPairsText = std::move( pairsText );
        if( !rename )
        {
            m_selectedLayer = static_cast<int>( m_layers.size() ) - 1;
            m_scrollToSelectedLayer = true;
        }
        m_layerEdit = LayerEdit::None;
        m_layerError.clear();
        return true;
    }

    void ImGuiCollisionMatrix::removeSelectedLayer()
    {
        const auto layer = m_layers[m_selectedLayer];
        const auto pairCount = m_disabledPairs.size();
        for( auto pair = m_disabledPairs.begin(); pair != m_disabledPairs.end(); )
        {
            if( pair->first == layer || pair->second == layer )
                pair = m_disabledPairs.erase( pair );
            else
                ++pair;
        }
        if( m_disabledPairs.size() != pairCount )
            m_disabledPairsText = encodePairs( m_disabledPairs );
        m_layers.erase( static_cast<size_t>( m_selectedLayer ) );
        m_layersText = encodeLayers( m_layers );
        m_selectedLayer = -1;
        m_layerEdit = LayerEdit::None;
        m_layerError.clear();
        m_scrollToSelectedLayer = false;
    }

    bool ImGuiCollisionMatrix::drawMatrix()
    {
        if( m_layers.empty() )
            return false;
        if( !m_validPairs )
        {
            ImGui::TextDisabled( "The saved collision pairs are invalid. Reset to edit the matrix." );
            return false;
        }

        ImGui::TextWrapped( "Checked pairs can collide if their Rigidbody masks also allow it." );
        const auto count = static_cast<int>( m_layers.size() );
        const auto rowHeight = ImGui::GetFrameHeight() + 2.0f * ImGui::GetStyle().CellPadding.y;
        const auto tableHeight = rowHeight * std::min( count + 1, 12 );
        constexpr auto flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                               ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders |
                               ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings;
        if( !ImGui::BeginTable( "##CollisionMatrix", count + 1, flags, ImVec2( 0, tableHeight ) ) )
            return false;

        ImGui::TableSetupScrollFreeze( 1, 1 );
        ImGui::TableSetupColumn( "Layer", ImGuiTableColumnFlags_WidthFixed, 116.0f );
        for( const auto &layer : m_layers )
        {
            const auto width = std::clamp( ImGui::CalcTextSize( layer.c_str() ).x, 32.0f, 140.0f );
            ImGui::TableSetupColumn( nullptr, ImGuiTableColumnFlags_WidthFixed, width );
        }

        ImGui::TableNextRow( ImGuiTableRowFlags_Headers );
        ImGui::TableSetColumnIndex( 0 );
        ImGui::TextUnformatted( "Layer" );
        for( int column = 0; column < count; ++column )
        {
            if( ImGui::TableSetColumnIndex( column + 1 ) )
                drawLayerName( m_layers[column] );
        }

        bool changed = false;
        ImGui::BeginDisabled( !isEnabled() || !m_editCollisionPairs );
        for( int row = 0; row < count; ++row )
        {
            ImGui::PushID( row );
            ImGui::TableNextRow();
            if( ImGui::TableSetColumnIndex( 0 ) )
                drawLayerName( m_layers[row] );
            for( int column = row; column < count; ++column )
            {
                if( !ImGui::TableSetColumnIndex( column + 1 ) )
                    continue;
                const auto &first = m_layers[row];
                const auto &second = m_layers[column];
                const auto pair = orderedPair( first, second );
                bool enabled = m_disabledPairs.find( pair ) == m_disabledPairs.end();
                ImGui::PushID( column );
                if( ImGui::Checkbox( "##Pair", &enabled ) )
                {
                    if( enabled )
                        m_disabledPairs.erase( pair );
                    else
                        m_disabledPairs.insert( pair );
                    changed = true;
                }
                if( ImGui::IsItemHovered() )
                    ImGui::SetTooltip( "%s and %s", first.c_str(), second.c_str() );
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::EndDisabled();
        ImGui::EndTable();

        if( changed )
            m_disabledPairsText = encodePairs( m_disabledPairs );
        return changed;
    }

    void ImGuiCollisionMatrix::drawLayerName( const String &layer )
    {
        ImGui::TextUnformatted( layer.c_str() );
        if( ImGui::IsItemHovered() )
            ImGui::SetTooltip( "%s", layer.c_str() );
    }
}  // namespace workphone::ui
