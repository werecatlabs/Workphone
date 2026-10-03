#ifndef __MeshPose_h__
#define __MeshPose_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/IMeshPose.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    /**
     * @brief Concrete implementation of IMeshPose interface.
     *
     * Represents a collection of vertex offsets that can be applied to a mesh
     * to create shape variations or morph targets for animation.
     */
    class WPCore_API MeshPose : public IMeshPose
    {
    public:
        /**
         * @brief Constructor.
         * @param target The target index for this pose.
         * @param name The name of the pose.
         */
        MeshPose( u16 target = 0, const String &name = String() );

        /**
         * @brief Destructor.
         */
        ~MeshPose() override;

        /** @copydoc IMeshPose::getTarget */
        u16 getTarget() const override;

        /** @copydoc IMeshPose::setTarget */
        void setTarget( u16 target ) override;

        /** @copydoc IMeshPose::getIncludesNormals */
        bool getIncludesNormals() const override;

        /** @copydoc IMeshPose::setIncludesNormals */
        void setIncludesNormals( bool includesNormals ) override;

        /** @copydoc IMeshPose::addVertex */
        void addVertex( u32 vertexIndex, const Vector3F &offset ) override;

        /** @copydoc IMeshPose::addVertex */
        void addVertex( u32 vertexIndex, const Vector3F &offset, const Vector3F &normal ) override;

        /** @copydoc IMeshPose::getNumVertexOffsets */
        u32 getNumVertexOffsets() const override;

        /**
         * @brief Gets all vertex offsets.
         * @return A map of vertex indices to position offsets.
         */
        const HashMap<u32, Vector3F> &getVertexOffsets() const;

        /**
         * @brief Gets all normal offsets.
         * @return A map of vertex indices to normal offsets.
         */
        const HashMap<u32, Vector3F> &getNormalOffsets() const;

        SmartPtr<IMeshPose> clone() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** The name of the pose. */
        FixedString<128> m_poseName;

        /** The target index (submesh or vertex buffer index). */
        u16 m_target;

        /** Whether this pose includes normal offsets. */
        bool m_includesNormals;

        /** Map of vertex indices to position offsets. */
        HashMap<u32, Vector3F> m_vertexOffsets;

        /** Map of vertex indices to normal offsets. */
        HashMap<u32, Vector3F> m_normalOffsets;
    };

}  // namespace workphone

#endif  // __MeshPose_h__
