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

    SmartPtr<Properties> ImGuiCollisionMatrix::getProperties() const
    {
        ScopedLock lock( this, false );
        auto properties = make_ptr<Properties>();
        properties->setProperty( physics::disabledCollisionPairsProperty, m_disabledPairsText );
        return properties;
    }

    void ImGuiCollisionMatrix::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;

        auto text = properties->getProperty( physics::disabledCollisionPairsProperty );
        Array<String> layers;
        StringPairs pairs;
        physics::parseCollisionLayerNames( properties->getProperty( "physics.collisionLayers" ),
                                           layers );
        const bool validPairs = physics::parseDisabledCollisionPairs( text, pairs );
        std::set<StringPair> disabledPairs;
        for( auto &[first, second] : pairs )
        {
            if( second < first )
                std::swap( first, second );
            disabledPairs.emplace( std::move( first ), std::move( second ) );
        }

        ScopedLock lock( this );
        m_layers = std::move( layers );
        m_disabledPairs = std::move( disabledPairs );
        m_disabledPairsText = std::move( text );
        m_validPairs = validPairs;
    }

    void ImGuiCollisionMatrix::update()
    {
        bool changed = false;
        {
            ScopedLock lock( this );
            changed = drawMatrix();
        }
        if( changed )
        {
            for( auto listener : getObjectListeners() )
                listener->handleEvent( EventType::UI, IEvent::handleValueChanged, {},
                                       this, this, nullptr );
        }
    }

    bool ImGuiCollisionMatrix::drawMatrix()
    {
        if( m_layers.empty() )
        {
            ImGui::TextDisabled( "Enter 1 to 32 distinct, comma-separated layer names." );
            return false;
        }
        if( !m_validPairs )
        {
            ImGui::TextDisabled( "The saved collision pairs are invalid. Reset to edit the matrix." );
            return false;
        }

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
        ImGui::BeginDisabled( !isEnabled() );
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
                auto ordered = std::minmax( first, second );
                const StringPair pair{ ordered.first, ordered.second };
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
        {
            m_disabledPairsText.clear();
            for( const auto &[first, second] : m_disabledPairs )
            {
                if( !m_disabledPairsText.empty() )
                    m_disabledPairsText += ",";
                m_disabledPairsText += first + "," + second;
            }
        }
        return changed;
    }

    void ImGuiCollisionMatrix::drawLayerName( const String &layer )
    {
        ImGui::TextUnformatted( layer.c_str() );
        if( ImGui::IsItemHovered() )
            ImGui::SetTooltip( "%s", layer.c_str() );
    }
}  // namespace workphone::ui
