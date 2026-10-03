#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/VertexBoneAssignment.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, VertexBoneAssignment, IVertexBoneAssignment );

    VertexBoneAssignment::VertexBoneAssignment() = default;

    VertexBoneAssignment::VertexBoneAssignment( u32 vertexIndex, u16 boneIndex, f32 weight ) :
        m_vertexIndex( vertexIndex ),
        m_boneIndex( boneIndex ),
        m_weight( weight )
    {
    }

    VertexBoneAssignment::~VertexBoneAssignment() = default;

    u32 VertexBoneAssignment::getVertexIndex() const
    {
        return m_vertexIndex;
    }

    void VertexBoneAssignment::setVertexIndex( u32 vertexIndex )
    {
        m_vertexIndex = vertexIndex;
    }

    u16 VertexBoneAssignment::getBoneIndex() const
    {
        return m_boneIndex;
    }

    void VertexBoneAssignment::setBoneIndex( u16 boneIndex )
    {
        m_boneIndex = boneIndex;
    }

    f32 VertexBoneAssignment::getWeight() const
    {
        return m_weight;
    }

    void VertexBoneAssignment::setWeight( f32 weight )
    {
        m_weight = weight;
    }

}  // namespace workphone
