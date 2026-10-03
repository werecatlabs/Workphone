#ifndef IMeshPose_h__
#define IMeshPose_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @class IMeshPose
     * @brief Interface for a mesh pose, representing a set of vertex offsets for a mesh.
     *
     * A mesh pose is a collection of vertex offsets that can be applied to a mesh to create
     * different shapes or animations. This interface provides methods to manipulate and query
     * the vertex offsets and other properties of the pose.
     */
    class IMeshPose : public ISharedObject
    {
    public:
        ~IMeshPose() override;

        /**
         * @brief Gets the target index for this pose.
         * @return The target index (usually referring to submesh or vertex buffer index).
         */
        virtual u16 getTarget() const = 0;

        /**
         * @brief Sets the target index for this pose.
         * @param target The target index to set.
         */
        virtual void setTarget( u16 target ) = 0;

        /**
         * @brief Checks if the pose includes normal offsets.
         * @return True if normal offsets are included, false otherwise.
         */
        virtual bool getIncludesNormals() const = 0;

        /**
         * @brief Sets whether the pose includes normal offsets.
         * @param includesNormals True to include normal offsets, false otherwise.
         */
        virtual void setIncludesNormals( bool includesNormals ) = 0;

        /**
         * @brief Adds a vertex offset to the pose.
         * @param vertexIndex The index of the vertex to modify.
         * @param offset The position offset to apply.
         */
        virtual void addVertex( u32 vertexIndex, const Vector3F &offset ) = 0;

        /**
         * @brief Adds a vertex offset with normal to the pose.
         * @param vertexIndex The index of the vertex to modify.
         * @param offset The position offset to apply.
         * @param normal The normal offset to apply.
         */
        virtual void addVertex( u32 vertexIndex, const Vector3F &offset, const Vector3F &normal ) = 0;

        /**
         * @brief Gets the number of vertex offsets in the pose.
         * @return The number of vertex offsets.
         */
        virtual u32 getNumVertexOffsets() const = 0;

        virtual SmartPtr<IMeshPose> clone() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IMeshPose_h__
