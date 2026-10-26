#ifndef __MeshPose_h__
#define __MeshPose_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/IMeshPose.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone
{
    /**
     * @brief Concrete implementation of IMeshPose interface.
     *
     * Represents a collection of vertex offsets that can be applied to a mesh
     * to create shape variations or morph targets for animation.
     */
    class WPCore_API MeshPose : public IMeshPose, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        using VertexOffsetMap =
            std::unordered_map<u32, Vector3F, std::hash<u32>, std::equal_to<u32>,
                               SourceBudgetAllocator<std::pair<const u32, Vector3F>>>;
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
        void unload( SmartPtr<ISharedObject> data ) override;

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
        const VertexOffsetMap &getVertexOffsets() const;

        /**
         * @brief Gets all normal offsets.
         * @return A map of vertex indices to normal offsets.
         */
        const VertexOffsetMap &getNormalOffsets() const;

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
        VertexOffsetMap m_vertexOffsets;

        /** Map of vertex indices to normal offsets. */
        VertexOffsetMap m_normalOffsets;
    };

}  // namespace workphone

#endif  // __MeshPose_h__
