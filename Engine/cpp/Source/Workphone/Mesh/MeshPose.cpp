#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshPose.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MeshPose, IMeshPose );

    MeshPose::MeshPose( u16 target, const String &name ) :
        m_poseName( name ),
        m_target( target ),
        m_includesNormals( false )
    {
    }

    MeshPose::~MeshPose() = default;

    u16 MeshPose::getTarget() const
    {
        return m_target;
    }

    void MeshPose::setTarget( u16 target )
    {
        m_target = target;
    }

    bool MeshPose::getIncludesNormals() const
    {
        return m_includesNormals;
    }

    void MeshPose::setIncludesNormals( bool includesNormals )
    {
        m_includesNormals = includesNormals;
    }

    void MeshPose::addVertex( u32 vertexIndex, const Vector3F &offset )
    {
        m_vertexOffsets[vertexIndex] = offset;
    }

    void MeshPose::addVertex( u32 vertexIndex, const Vector3F &offset, const Vector3F &normal )
    {
        m_vertexOffsets[vertexIndex] = offset;
        m_normalOffsets[vertexIndex] = normal;
        m_includesNormals = true;
    }

    u32 MeshPose::getNumVertexOffsets() const
    {
        return static_cast<u32>( m_vertexOffsets.size() );
    }

    const HashMap<u32, Vector3F> &MeshPose::getVertexOffsets() const
    {
        return m_vertexOffsets;
    }

    const HashMap<u32, Vector3F> &MeshPose::getNormalOffsets() const
    {
        return m_normalOffsets;
    }

    SmartPtr<IMeshPose> MeshPose::clone() const
    {
        auto clonePose = workphone::make_ptr<MeshPose>( m_target, m_poseName );
        clonePose->m_includesNormals = m_includesNormals;
        clonePose->m_vertexOffsets = m_vertexOffsets;
        clonePose->m_normalOffsets = m_normalOffsets;
        return clonePose;
    }

}  // namespace workphone
