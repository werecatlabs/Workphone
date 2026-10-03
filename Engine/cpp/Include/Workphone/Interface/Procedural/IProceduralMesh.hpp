#ifndef IProceduralMesh_h__
#define IProceduralMesh_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for mutable mesh data produced by procedural model rules.
         *
         * This class provides a way to define 3D mesh geometry consisting of vertices,
         * normals, and indices. Positions, normals and indices are stored in separate arrays.
         * A valid normal array is either empty (normals have not been generated yet)
         * or contains one normal per vertex. Indices describe a triangle list and
         * are therefore grouped in sets of three.
         */
        class WPCore_API IProceduralMesh : public ISharedObject
        {
        public:
            ~IProceduralMesh() override;

            /**
             * @brief Removes all vertices, normals and indices from the mesh.
             *
             * Resets the mesh to an empty state.
             */
            virtual void clear() = 0;

            /**
             * @brief Adds a vertex to the mesh.
             * @param position The 3D position of the vertex.
             * @return The zero-based index of the newly added vertex.
             */
            virtual u32 addVertex( const Vector3F &position ) = 0;

            /**
             * @brief Appends one triangle to the index list.
             * @param a Index of the first vertex.
             * @param b Index of the second vertex.
             * @param c Index of the third vertex.
             */
            virtual void addTriangle( u32 a, u32 b, u32 c ) = 0;

            /**
             * @brief Appends another mesh, translating its vertices by the specified offset.
             * @param mesh The mesh to append.
             * @param offset The translation offset for the appended mesh's vertices.
             */
            virtual void append( SmartPtr<IProceduralMesh> mesh,
                                 const Vector3F &offset = Vector3F::ZERO ) = 0;

            /**
             * @brief Rebuilds smooth, per-vertex normals based on the indexed triangle list.
             *
             * This method calculates surface normals by averaging the normals of all
             * triangles sharing a vertex.
             */
            virtual void computeNormals() = 0;

            /** @return The array of vertex positions. */
            virtual Array<Vector3F> getVertices() const = 0;
            /** @param vertices The array of vertex positions to set. */
            virtual void setVertices( const Array<Vector3F> &vertices ) = 0;

            /** @return The array of vertex normals. */
            virtual Array<Vector3F> getNormals() const = 0;
            /** @param normals The array of vertex normals to set. */
            virtual void setNormals( const Array<Vector3F> &normals ) = 0;

            /** @return The array of triangle indices. */
            virtual Array<u32> getIndices() const = 0;
            /** @param indices The array of triangle indices to set. */
            virtual void setIndices( const Array<u32> &indices ) = 0;

            /** @return The number of vertices currently in the mesh. */
            virtual u32 getNumVertices() const = 0;
            /** @return The number of indices currently in the mesh. */
            virtual u32 getNumIndices() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // IProceduralMesh_h__
