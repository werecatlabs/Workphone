#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DynamicMesh.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, DynamicMesh, GraphicsObject<IDynamicMesh> );

    DynamicMesh::DynamicMesh() = default;
    DynamicMesh::~DynamicMesh() = default;

    void DynamicMesh::setMesh( SmartPtr<IMesh> mesh )
    {
        m_mesh = std::move( mesh );
        setDirty( true );
    }

    SmartPtr<IMesh> DynamicMesh::getMesh() const
    {
        return m_mesh;
    }

    void DynamicMesh::setSubMesh( SmartPtr<ISubMesh> subMesh )
    {
        m_subMesh = std::move( subMesh );
        setDirty( true );
    }

    SmartPtr<ISubMesh> DynamicMesh::getSubMesh() const
    {
        return m_subMesh;
    }

    void DynamicMesh::setDirty( bool dirty )
    {
        m_dirty = dirty;
    }
}  // namespace workphone::render
