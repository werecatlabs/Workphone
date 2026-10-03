#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/Component_Navmesh.hpp"
#include <Workphone/Core/Path.hpp>

namespace workphone
{

    NavmeshComponent::NavmeshComponent()
    {
    }

    NavmeshComponent::NavmeshComponent( const NavPath &navmeshResourcePath ) :
        m_resourcePath( navmeshResourcePath )
    {
    }

    NavmeshComponent::~NavmeshComponent()
    {
        m_navmeshData = nullptr;
    }

    bool NavmeshComponent::HasNavmeshData() const
    {
        return m_navmeshData && m_navmeshData->IsValid();
    }

    void NavmeshComponent::SetNavmeshData( NavmeshDataPtr navmeshData )
    {
        m_navmeshData = navmeshData;
    }

    void NavmeshComponent::SetNavmeshResourcePath( const NavPath &path )
    {
        m_resourcePath = path;
        // TODO: Trigger async resource loading here
    }

    void NavmeshComponent::AddAdditionalLayer( const NavmeshLayerBuildSettings &layer )
    {
        m_buildSettings.m_additionalLayers.Add( layer );
    }

    void NavmeshComponent::RemoveAdditionalLayer( uint32_t index )
    {
        if( index < m_buildSettings.m_additionalLayers.GetSize() )
        {
            m_buildSettings.m_additionalLayers.RemoveAt( index );
        }
    }

}  // namespace workphone
