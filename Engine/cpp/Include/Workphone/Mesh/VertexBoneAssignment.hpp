#ifndef __WP_VertexBoneAssignment_H__
#define __WP_VertexBoneAssignment_H__

/**
 * @file VertexBoneAssignment.hpp
 * @brief Defines the VertexBoneAssignment class used to associate a mesh vertex with a skeleton bone.
 *
 * The VertexBoneAssignment class implements IVertexBoneAssignment and stores the mapping
 * between a vertex index and a bone index together with an influence weight. Instances
 * are used when building skinning data for animated meshes.
 */

#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>

namespace workphone
{
    /**
     * @brief Represents a single bone assignment for a vertex.
     *
     * Each assignment maps a single vertex (by index) to a single bone (by index)
     * and carries a weight describing the influence of that bone on the vertex
     * during skinning. Multiple assignments can exist for the same vertex to
     * represent blended influences from several bones.
     */
    class WPCore_API VertexBoneAssignment : public IVertexBoneAssignment
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the vertex index and bone index to 0 and weight to 0.0f.
         */
        VertexBoneAssignment();

        /**
         * @brief Construct a vertex->bone assignment.
         * @param vertexIndex Index of the vertex in the vertex buffer.
         * @param boneIndex Index of the bone in the skeleton.
         * @param weight Influence weight (typically in range [0.0, 1.0]).
         */
        VertexBoneAssignment( u32 vertexIndex, u16 boneIndex, f32 weight );

        /**
         * @brief Virtual destructor.
         */
        ~VertexBoneAssignment() override;

        /**
         * @brief Get the vertex index for this assignment.
         * @return Vertex index.
         */
        u32 getVertexIndex() const override;

        /**
         * @brief Set the vertex index for this assignment.
         * @param vertexIndex New vertex index.
         */
        void setVertexIndex( u32 vertexIndex ) override;

        /**
         * @brief Get the bone index for this assignment.
         * @return Bone index.
         */
        u16 getBoneIndex() const override;

        /**
         * @brief Set the bone index for this assignment.
         * @param boneIndex New bone index.
         */
        void setBoneIndex( u16 boneIndex ) override;

        /**
         * @brief Get the influence weight for this assignment.
         * @return Weight value (higher values mean stronger influence).
         */
        f32 getWeight() const override;

        /**
         * @brief Set the influence weight for this assignment.
         * @param weight New weight value (expected in [0.0, 1.0]).
         */
        void setWeight( f32 weight ) override;

        WP_CLASS_REGISTER_DECL;

        /// Index of the vertex this assignment applies to.
        u32 m_vertexIndex = 0;

        /// Index of the bone influencing the vertex.
        u16 m_boneIndex = 0;

        /// Influence weight of the bone on the vertex (0.0 = no influence, 1.0 = full).
        f32 m_weight = 0.0f;
    };
}  // namespace workphone

#endif
