#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/BoundingBox.hpp>
#include <Workphone/Core/Blob.hpp>

namespace workphone
{

    // Forward declarations
    class INavmeshDataListener;

    /**
     * @class NavmeshData
     * @brief Navmesh resource data containing the navigation graph.
     */
    class WPNETWORK_API NavmeshData : public Workphone::RefCounted
    {
    public:
        NavmeshData();
        virtual ~NavmeshData();

        // Serialization
        bool LoadFromFile( const NavPath &filePath );
        bool SaveToFile( const WPCore::FileSystem &fileSystem ) const;

        // Validity
        bool IsValid() const
        {
            return !m_graphImage.IsEmpty();
        }

        // Accessors
        const WPCore::Blob &GetGraphImage() const
        {
            return m_graphImage;
        }
        void SetGraphImage( const WPCore::Blob &image )
        {
            m_graphImage = image;
        }

        const WPCore::BoundingBox &GetBounds() const
        {
            return m_bounds;
        }
        void SetBounds( const WPCore::BoundingBox &bounds )
        {
            m_bounds = bounds;
        }

        uint32_t GetLayerIndex() const
        {
            return m_layerIndex;
        }
        void SetLayerIndex( uint32_t index )
        {
            m_layerIndex = index;
        }

        // Listeners
        void AddListener( INavmeshDataListener *listener );
        void RemoveListener( INavmeshDataListener *listener );

    private:
        WPCore::Blob m_graphImage;
        WPCore::BoundingBox m_bounds;
        uint32_t m_layerIndex = 0;
        WPCore::Array<INavmeshDataListener *> m_listeners;
    };

    /**
     * @class INavmeshDataListener
     * @brief Interface for listening to navmesh data changes.
     */
    class INavmeshDataListener
    {
    public:
        virtual ~INavmeshDataListener() = default;
        virtual void OnNavmeshDataChanged( NavmeshData *pData ) = 0;
    };

    typedef Workphone::SmartPtr<NavmeshData> NavmeshDataPtr;
    typedef Workphone::WeakPtr<NavmeshData> NavmeshDataWeakPtr;

}  // namespace workphone
