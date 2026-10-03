#ifndef _MeshUtil_H
#define _MeshUtil_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Math/LinearSpline3.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Mesh/MeshTransformData.hpp>
#include <Workphone/Mesh/Vertex.hpp>

namespace workphone
{

    /**
     * @brief Utility helpers for creating, analysing and manipulating meshes.
     *
     * MeshUtil provides a collection of static helper functions used across the
     * engine for common mesh tasks such as building primitive geometry, creating
     * vertex/index buffers, merging and cleaning meshes, extracting geometry
     * data and colour/vertex element utilities.
     *
     * All functions operate on the engine's mesh interfaces (IMesh, ISubMesh,
     * IVertexBuffer, IIndexBuffer etc.). Most functions return SmartPtr to
     * engine objects; callers should use the SmartPtr semantics used by the engine.
     */
    class WPCore_API MeshUtil
    {
    public:
        /**
         * @brief Compute a minimal bounding sphere enclosing the provided points.
         * @param points Array of 3D points in the same coordinate space.
         * @return Sphere3<real_Num> Sphere in the points' space that encloses all points.
         *         If the input is empty the returned sphere will be an empty/invalid sphere.
         */
        static Sphere3<real_Num> calculateBoundingSphere( const Array<Vector3<real_Num>> &points );

        /**
         * @brief Calculate a bounding sphere that encloses the provided mesh.
         * @param mesh SmartPtr to the mesh whose bounds will be calculated.
         * @return Sphere3<real_Num> Sphere in mesh local space that encloses the mesh
         *         vertices. Returns an empty sphere when mesh is null or contains no points.
         */
        static Sphere3<real_Num> calculateBoundingSphere( SmartPtr<IMesh> mesh );

        /**
         * @brief Compute axis-aligned bounding box (AABB) for a set of points.
         * @param points Array of 3D points to enclose.
         * @return AABB3<real_Num> Axis-aligned bounding box enclosing the points.
         *         If the input is empty an invalid/empty AABB is returned.
         */
        static AABB3<real_Num> calculateBoundingBox( const Array<Vector3<real_Num>> &points );

        /**
         * @brief Calculate an axis-aligned bounding box (AABB) for the provided mesh.
         * @param mesh SmartPtr to the mesh to compute the AABB for.
         * @return AABB3<real_Num> Axis-aligned bounding box in mesh local space. Returns
         *         an invalid/empty AABB when mesh is null or contains no points.
         */
        static AABB3<real_Num> calculateBoundingBox( SmartPtr<IMesh> mesh );

        /**
         * @brief Create a vertex buffer from an array of interleaved Vertex structures.
         * @param vertices Array of Vertex structures (may include position, normal, uv, colour, etc.)
         * @return SmartPtr<IVertexBuffer> Newly created vertex buffer containing the provided vertices.
         */
        static SmartPtr<IVertexBuffer> createVertexBuffer( const Array<Vertex> &vertices );

        /**
         * @brief Create a vertex buffer from separate attribute arrays.
         * @param positions Array of vertex positions.
         * @param normals Array of vertex normals. May be empty if normals are not required.
         * @param uvs Array of 2D texture coordinates. May be empty if not required.
         * @return SmartPtr<IVertexBuffer> Newly created vertex buffer with interleaved attributes.
         *
         * The function builds a single interleaved buffer from the provided attribute arrays.
         * All arrays are expected to have the same number of elements (vertex count).
         */
        static SmartPtr<IVertexBuffer> createVertexBuffer( const Array<Vector3<real_Num>> &positions,
                                                           const Array<Vector3<real_Num>> &normals,
                                                           const Array<Vector2<real_Num>> &uvs );

        /**
         * @brief Create an index buffer from an array of indices.
         * @param indices Array of 32-bit indices (triangles typically in groups of three).
         * @return SmartPtr<IIndexBuffer> Newly created index buffer populated with the indices.
         */
        static SmartPtr<IIndexBuffer> createIndexBuffer( const Array<u32> &indices );

        /**
         * @brief Create a sub-mesh from a vertex buffer and an index buffer.
         * @param vertexBuffer Vertex buffer that supplies vertex attributes.
         * @param indexBuffer Index buffer that supplies topology.
         * @return SmartPtr<ISubMesh> Newly created submesh that references the given buffers.
         */
        static SmartPtr<ISubMesh> createSubMesh( SmartPtr<IVertexBuffer> vertexBuffer,
                                                 SmartPtr<IIndexBuffer> indexBuffer );

        static SmartPtr<ISubMesh> createSubMesh( Array<Vertex> vertices, Array<u32> indices );

        /**
         * @brief Create a mesh from positions, normals, tangents, uvs and indices.
         * @param vertices Array of positions.
         * @param normals Array of normals.
         * @param tangents Array of tangents (four-component vector where w is handedness).
         * @param uvs Array of uv coordinates (stored in Vector3F for compatibility; z may be unused).
         * @param indices Triangle index list.
         * @return SmartPtr<IMesh> Newly created mesh.
         */
        static SmartPtr<IMesh> createMesh( const Array<Vector3<real_Num>> &vertices,
                                           const Array<Vector3<real_Num>> &normals,
                                           const Array<Vector4F> &tangents,
                                           const Array<Vector3<real_Num>> &uvs,
                                           const Array<u32> &indices );

        /**
         * @brief Create a mesh with two sets of texture coordinates.
         * @param vertices Array of positions.
         * @param normals Array of normals.
         * @param tangents Array of tangents.
         * @param uvs0 Primary texture coordinates.
         * @param uvs1 Secondary texture coordinates.
         * @param indices Triangle index list.
         * @return SmartPtr<IMesh> Newly created mesh with two UV channels.
         */
        static SmartPtr<IMesh> createMesh( const Array<Vector3<real_Num>> &vertices,
                                           const Array<Vector3<real_Num>> &normals,
                                           const Array<Vector4F> &tangents,
                                           const Array<Vector3<real_Num>> &uvs0,
                                           const Array<Vector3<real_Num>> &uvs1,
                                           const Array<u32> &indices );

        /**
         * @brief Create a mesh from positions, normals, single set of uvs and indices.
         * @param vertices Array of positions.
         * @param normals Array of normals.
         * @param uvs Array of 2D texture coordinates.
         * @param indices Triangle index list.
         * @return SmartPtr<IMesh> Newly created mesh.
         */
        static SmartPtr<IMesh> createMesh( const Array<Vector3<real_Num>> &vertices,
                                           const Array<Vector3<real_Num>> &normals,
                                           const Array<Vector2<real_Num>> &uvs,
                                           const Array<u32> &indices );

        /**
         * @brief Create a single quad plane mesh.
         * @param halfExtent Half extents along X, Y and Z (plane size = 2 * halfExtent).
         * @param normal Plane normal direction.
         * @param right Vector pointing to the plane's right direction (tangent).
         * @return SmartPtr<IMesh> Quad mesh lying on the plane defined by the parameters.
         */
        static SmartPtr<IMesh> createPlane( const Vector3<real_Num> &halfExtent,
                                            const Vector3<real_Num> &normal,
                                            const Vector3<real_Num> &right );

        /**
         * @brief Create a box (cuboid) mesh with optional segmentation.
         * @param sizeX Width (X).
         * @param sizeY Height (Y).
         * @param sizeZ Depth (Z).
         * @param numSegX Number of subdivisions along X.
         * @param numSegY Number of subdivisions along Y.
         * @param numSegZ Number of subdivisions along Z.
         * @return SmartPtr<IMesh> Generated box mesh.
         */
        static SmartPtr<IMesh> createBox( f32 sizeX = 1.f, f32 sizeY = 1.f, f32 sizeZ = 1.f,
                                          u32 numSegX = 1, u32 numSegY = 1, u32 numSegZ = 1 );

        /**
         * @brief Create a cylinder mesh.
         * @param radius Cylinder base radius.
         * @param height Cylinder height.
         * @param numSegBase Number of segments around the base (radial subdivisions).
         * @param numSegHeight Number of subdivisions along the height.
         * @param capped True to add end caps, false to leave open ends.
         * @return SmartPtr<IMesh> Generated cylinder mesh.
         */
        static SmartPtr<IMesh> createCylinder( f32 radius = 1.f, f32 height = 1.f, u32 numSegBase = 16,
                                               u32 numSegHeight = 1, bool capped = true );

        /**
         * @brief Create a cylindrical mesh by extruding a circular cross-section along a spline.
         * @param spline Spline that defines the centreline path to extrude along.
         * @param radius Cross-section radius.
         * @param height Total height (used to scale or sample along spline length as applicable).
         * @param numSegBase Radial resolution for the cross-section.
         * @param numSegHeight Longitudinal subdivisions along the spline.
         * @param capped Whether to add end caps at start and end.
         * @return SmartPtr<IMesh> Resulting mesh (useful for pipes, rails, ropes, etc).
         */
        static SmartPtr<IMesh> createCylinderFromSpline( SmartPtr<LinearSpline3<real_Num>> spline,
                                                         f32 radius = 1.f, f32 height = 1.f,
                                                         u32 numSegBase = 16, u32 numSegHeight = 1,
                                                         bool capped = true );

        /**
         * @brief Create a road-like mesh by extruding a profile along a spline.
         * @param spline Spline that represents the road centreline.
         * @param radius Half-width (or profile radius) of the road.
         * @param height Road thickness or vertical profile scale.
         * @param numSegBase Radial/profile resolution (if applicable).
         * @param numSegHeight Longitudinal subdivisions along the spline.
         * @param capped Whether to cap ends.
         * @return SmartPtr<IMesh> Road mesh following the spline.
         */
        static SmartPtr<IMesh> createRoadFromSpline( SmartPtr<LinearSpline3<real_Num>> spline,
                                                     f32 radius = 1.f, f32 height = 1.f,
                                                     u32 numSegBase = 16, u32 numSegHeight = 1,
                                                     bool capped = true );

        /**
         * @brief Create a mesh from a set of splines.
         * @param splines Array of splines; function will generate geometry which connects or represents
         * them.
         * @return SmartPtr<IMesh> Generated mesh representing the provided splines.
         */
        static SmartPtr<IMesh> createMeshFromSplines( const Array<LinearSpline3<real_Num>> &splines );

        /**
         * @brief Remove duplicate / nearly-duplicate vertices and optionally weld vertices within a
         * tolerance.
         * @param mesh Mesh to clean.
         * @param weldTolerance Maximum distance within which two vertices are considered identical and
         * should be welded.
         * @return SmartPtr<IMesh> Cleaned mesh. May return the original mesh or a new mesh instance.
         */
        static SmartPtr<IMesh> clean( SmartPtr<IMesh> mesh, real_Num weldTolerance );

        /**
         * @brief Clean a single submesh by welding near-duplicate vertices.
         * @param submesh Submesh to clean.
         * @param weldTolerance Welding tolerance in mesh units.
         * @return SmartPtr<ISubMesh> Cleaned submesh.
         */
        static SmartPtr<ISubMesh> clean( SmartPtr<ISubMesh> submesh, real_Num weldTolerance );

        /**
         * @brief Extract vertex positions from a mesh.
         * @param mesh Mesh to extract points from.
         * @return Array<Vector3<real_Num>> Array of vertex positions (one vector per vertex).
         */
        static Array<Vector3<real_Num>> getPoints( SmartPtr<IMesh> mesh );

        /**
         * @brief Extract vertex positions from a submesh.
         * @param submesh Submesh to extract points from.
         * @return Array<Vector3<real_Num>> Array of vertex positions.
         */
        static Array<Vector3<real_Num>> getPoints( SmartPtr<ISubMesh> submesh );

        /**
         * @brief Extract index data from a mesh.
         * @param mesh Mesh to extract indices from.
         * @return Array<u32> Flattened index array (triangles in groups of three).
         */
        static Array<u32> getIndices( SmartPtr<IMesh> mesh );

        /**
         * @brief Extract index data from a submesh.
         * @param submesh Submesh to extract indices from.
         * @return Array<u32> Index list for the submesh.
         */
        static Array<u32> getIndices( SmartPtr<ISubMesh> submesh );

        /**
         * @brief Convert mesh geometry into a height map texture-like 2D array.
         * @param mesh Source mesh (world or local space as expected by the caller).
         * @param textureSize Size (width and height) of the height map to generate.
         * @return Array<Array<f32>> 2D array (textureSize.x by textureSize.y) with height values.
         *
         * The function samples the mesh surface to produce height values arranged in rows and columns.
         */
        static Array<Array<f32>> meshToHeightMap( SmartPtr<IMesh> mesh, const Vector2I &textureSize );

        /**
         * @brief Build a mesh from arrays of positions and texture coordinates.
         * @param positions Vertex positions.
         * @param uvs Primary UVs per position.
         * @param uvs2 Secondary UVs per position (may be empty).
         * @return SmartPtr<IMesh> Constructed mesh using positions and uvs. Indices will be generated
         * for triangles.
         */
        static SmartPtr<IMesh> buildMesh( const Array<Vector3<real_Num>> &positions,
                                          const Array<Vector2<real_Num>> &uvs,
                                          const Array<Vector2<real_Num>> &uvs2 );

        /**
         * @brief Build a submesh from arrays of positions and uvs.
         * @param positions Vertex positions.
         * @param uvs Primary UVs.
         * @param uvs2 Secondary UVs (optional).
         * @return SmartPtr<ISubMesh> Created submesh with provided vertex attributes.
         */
        static SmartPtr<ISubMesh> buildSubMesh( const Array<Vector3<real_Num>> &positions,
                                                const Array<Vector2<real_Num>> &uvs,
                                                const Array<Vector2<real_Num>> &uvs2 );

        /**
         * @brief Merge multiple meshes using per-mesh transform data.
         * @param meshTransformData Array of MeshTransformData describing mesh instances and transforms.
         * @return SmartPtr<IMesh> Single merged mesh. Materials and submesh structure may be preserved
         * where possible.
         */
        static SmartPtr<IMesh> mergeMeshes( const Array<MeshTransformData> &meshTransformData );

        /**
         * @brief Merge an array of meshes into one mesh without additional transforms.
         * @param meshes Array of mesh SmartPtrs to merge.
         * @return SmartPtr<IMesh> Merged mesh.
         */
        static SmartPtr<IMesh> mergeMeshes( const Array<SmartPtr<IMesh>> &meshes );

        /**
         * @brief Merge an array of meshes using explicit transform matrices per mesh.
         * @param meshes Array of meshes to merge.
         * @param transformations Array of transformation matrices (one per mesh).
         * @return SmartPtr<IMesh> Merged mesh with each source transformed by the corresponding matrix.
         */
        static SmartPtr<IMesh> mergeMeshes( const Array<SmartPtr<IMesh>> &meshes,
                                            const Array<Matrix4<real_Num>> &transformations );

        /**
         * @brief Merge submeshes of a mesh that share the same material into larger submeshes.
         * @param mesh Mesh whose submeshes will be merged by matching material.
         * @return SmartPtr<IMesh> New or modified mesh with merged submeshes to reduce draw calls.
         */
        static SmartPtr<IMesh> mergeSubMeshesByMaterial( SmartPtr<IMesh> mesh );

        /**
         * @brief Validate a mesh for basic correctness.
         * @param mesh Mesh to validate.
         * @return bool True if the mesh is valid (has consistent vertex and index data), false
         * otherwise.
         */
        static bool isMeshValid( SmartPtr<IMesh> mesh );

        /**
         * @brief Generate a mesh from a height buffer.
         * @param heightData Flattened or row-major height array.
         * @param worldScale Horizontal scale applied to x/z coordinates.
         * @param heightScale Vertical scale applied to height values.
         * @param tileSize Number of tiles/vertices per row (width).
         * @return SmartPtr<IMesh> Generated terrain mesh using height data.
         */
        static SmartPtr<IMesh> getMesh( const Array<f32> &heightData, f32 worldScale, f32 heightScale,
                                        u32 tileSize );

        /**
         * @brief Compute a 1D index into a height map or tile grid.
         * @param tileSize Width of the tile grid.
         * @param x X coordinate (column).
         * @param z Z coordinate (row).
         * @return u32 Index into a row-major array (z * tileSize + x).
         */
        static u32 getIndex( u32 tileSize, u32 x, u32 z );

        /**
         * @brief Ensure that each triangle in the mesh uses unique (unshared) vertices.
         * @param mesh Mesh to operate on. After call the mesh will have no shared vertices between
         * faces.
         */
        static void unshareVertices( SmartPtr<IMesh> mesh );

        /**
         * @brief Return the number of values stored by a given vertex element type.
         * @param etype VertexElementType to query.
         * @return u16 Count of components for the element type (e.g. float3 -> 3).
         *
         * Note: For packed/colour types the result may be implementation-specific.
         */
        static u16 getTypeCount( VertexElementType etype );

        /**
         * @brief Create a multi-component vertex element type from a base type and a component count.
         * @param baseType Base vertex element type (e.g. float1/byte1).
         * @param count Number of components to produce.
         * @return VertexElementType Multi-value vertex element type representing baseType * count.
         */
        static VertexElementType multiplyTypeCount( VertexElementType baseType, u16 count );

        /**
         * @brief Convert a multi-value vertex element type to its base/single-value equivalent.
         * @param multiType VertexElementType that may represent multiple values.
         * @return VertexElementType Single-value base type suitable for switch/case handling.
         *
         * This helper simplifies logic that needs to branch on the underlying storage type.
         */
        static VertexElementType getBaseType( VertexElementType multiType );

        /**
         * @brief Convert a packed 32-bit colour value from one vertex element format to another.
         * @param srcType Source packed colour type.
         * @param dstType Destination packed colour type.
         * @param ptr Pointer to the 32-bit value to read/modify in place.
         *
         * The function reads the packed value pointed to by ptr and writes the converted
         * packed value back to the same location.
         */
        static void convertColourValue( VertexElementType srcType, VertexElementType dstType, u32 *ptr );

        /**
         * @brief Query the most appropriate packed colour vertex element type supported by the engine.
         * @return VertexElementType Preferred packed colour format for vertex elements on this
         * platform/engine build.
         */
        static VertexElementType getBestColourVertexElementType();

        static SmartPtr<IMesh> cloneMesh( SmartPtr<IMesh> mesh );
    };
}  // namespace workphone

#endif
