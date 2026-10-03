#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include <Workphone/Math/Transform.hpp>

namespace workphone
{

    /**
     * @class NavmeshTesterComponent
     * @brief Component for testing navmesh queries in the editor.
     *        Allows specifying start/end transforms for pathfinding tests.
     */
    class WPNETWORK_API NavmeshTesterComponent
    {
    public:
        NavmeshTesterComponent();
        virtual ~NavmeshTesterComponent();

        // Transform accessors
        const WPCore::Transform &GetStartTransform() const
        {
            return m_startTransform;
        }
        void SetStartTransform( const WPCore::Transform &transform )
        {
            m_startTransform = transform;
        }

        const WPCore::Transform &GetEndTransform() const
        {
            return m_endTransform;
        }
        void SetEndTransform( const WPCore::Transform &transform )
        {
            m_endTransform = transform;
        }

        // Position shortcuts
        WPCore::Vector3 GetStartPosition() const
        {
            return m_startTransform.GetPosition();
        }
        void SetStartPosition( const WPCore::Vector3 &pos )
        {
            m_startTransform.SetPosition( pos );
        }

        WPCore::Vector3 GetEndPosition() const
        {
            return m_endTransform.GetPosition();
        }
        void SetEndPosition( const WPCore::Vector3 &pos )
        {
            m_endTransform.SetPosition( pos );
        }

        // Test control
        bool IsTestActive() const
        {
            return m_testActive;
        }
        void SetTestActive( bool active )
        {
            m_testActive = active;
        }

        // Entity binding
        void SetOwnerEntity( void *entity )
        {
            m_ownerEntity = entity;
        }
        void *GetOwnerEntity() const
        {
            return m_ownerEntity;
        }

        // Component ID
        void SetID( uint64_t id )
        {
            m_componentID = id;
        }
        uint64_t GetID() const
        {
            return m_componentID;
        }

    private:
        void *m_ownerEntity = nullptr;
        uint64_t m_componentID = 0;

        WPCore::Transform m_startTransform;
        WPCore::Transform m_endTransform;
        bool m_testActive = false;
    };

}  // namespace workphone
