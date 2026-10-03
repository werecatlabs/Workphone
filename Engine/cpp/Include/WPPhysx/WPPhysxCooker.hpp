#ifndef WPPhysxCooker_h__
#define WPPhysxCooker_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/Map.hpp>
#include "foundation/PxVec3.h"
#include "PxPhysics.h"
#include "common/PxCoreUtilityTypes.h"

namespace workphone
{
    namespace physics
    {
        using namespace physx;

        /**
         * @brief Helper that converts engine mesh data into PhysX cooked meshes.
         *
         * This class provides utilities to extract vertex/index data from engine
         * meshes, perform preprocessing (merge vertices, inset geometry) and
         * cook PhysX triangle and convex meshes. It also supports writing the
         * cooked data to a PxOutputStream or directly to a file.
         */
        class PhysxCooker
        {
        public:
            /**
             * @brief Collection of parameters that control how a mesh is cooked.
             *
             * - mScale: uniform or non-uniform scale applied to mesh vertices.
             * - mMaterialBindings: mapping from material name to PhysX material
             *   pointer used for per-triangle material assignments.
             * - mAddBackfaces: whether to add reversed triangles for two-sided
             *   (backface) collisions.
             */
            class Params
            {
            public:
                Vector3F                       mScale;
                std::map<String, PxMaterial *> mMaterialBindings;
                bool                           mAddBackfaces;

                Params();
                ~Params();

                /**
                 * @brief Set scale applied to vertices during cooking.
                 * @param scale Scale vector to apply.
                 * @return Reference to this Params object for chaining.
                 */
                Params &scale( const Vector3F &scale );

                /**
                 * @brief Set material bindings used for per-triangle materials.
                 * @param bindings Map from material name to PhysX material pointer.
                 * @return Reference to this Params object for chaining.
                 */
                Params &materials( std::map<String, PxMaterial *> &bindings );

                /**
                 * @brief Enable or disable adding backfaces (reversed triangles).
                 * @param addBackFaces True to add backfaces, false otherwise.
                 * @return Reference to this Params object for chaining.
                 */
                Params &backfaces( bool addBackFaces );
            };

            class AddedMaterials
            {
            public:
                PxMaterial **materials;
                PxU32        materialCount;
                /**
                 * @brief Holds the array of PhysX materials created/used during
                 * cooking and the number of materials.
                 *
                 * The array is owned by the caller when returned from cooking
                 * functions and should be freed according to the project's
                 * memory ownership rules.
                 */
                AddedMaterials();
                ~AddedMaterials();
            };

            struct MeshInfo
            {
                // vertex buffer
                Array<Vector3F> vertices;

                // index buffer
                Array<int> indices;

                // assigns a material to each triangle.
                /**
                 * @brief Per-triangle material names. One entry per triangle.
                 * The string is used to look up the corresponding PxMaterial in
                 * Params::mMaterialBindings when cooking with per-triangle
                 * materials.
                 */
                Array<String> materials;
            };

            PhysxCooker( void );
            ~PhysxCooker( void );

            /**
             * @brief Extracts vertex/index/material information from an engine mesh.
             *
             * The extracted data is placed in @p outInfo and is affected by the
             * supplied @p params (for example scaling). This is the primary
             * method used before cooking a PhysX mesh.
             *
             * @param mesh Source engine mesh.
             * @param params Cooking parameters (scale, materials, backfaces).
             * @param outInfo Output mesh data filled by the function.
             */
            void getMeshInfo( SmartPtr<IMesh> mesh, Params params, MeshInfo &outInfo );

            /**
             * @brief Merge nearby vertices to reduce duplicate positions.
             *
             * Vertices within @p fMergeDist are considered identical and are
             * welded together; indices are updated accordingly.
             *
             * @param outInfo Mesh data to process (modified in place).
             * @param fMergeDist Distance threshold used to merge vertices.
             */
            void mergeVertices( MeshInfo &outInfo, float fMergeDist = 1e-3f );

            /**
             * @brief Offset triangle faces inward by a small amount.
             *
             * Useful to avoid zero-thickness geometry or to create a small
             * inset when cooking collision geometry.
             *
             * @param outInfo Mesh data to modify.
             * @param fAmount Amount to inset each vertex along its normal.
             */
            void insetMesh( MeshInfo &outInfo, float fAmount );

            /*
            hasPxMesh
            Checks whether the resource System has a resource PxsFile.
            */
            /**
             * @brief Check whether a cooked Px mesh resource exists.
             * @param PxsFile Resource identifier / filename of the cooked mesh.
             * @return True if the resource exists in the engine resource system.
             */
            bool hasPxMesh( String PxsFile );

            /*
            loadPxMeshFromFile
            Loads a PhysX triangle mesh from a Pxs file. Throws an exception if the file is not found in
            the Ogre resource system.
            */
            /**
             * @brief Load a cooked PxTriangleMesh from a Pxs file in the resource system.
             * @param PxsFile Resource identifier / filename of the cooked mesh.
             * @return Pointer to the loaded PxTriangleMesh.
             * @throws exception if the file is not found or cannot be loaded.
             */
            PxTriangleMesh *loadPxTriangleMeshFromFile( String PxsFile );

            /*
            cookPxTriangleMesh
            cooks an PhysX triangle mesh from an Ogre mesh.
            out_addedMaterials can be passed to obtain information about the used materials (for per
            triangle materials).
            @see PxShape::setMaterials
            */
            /**
             * @brief Cook a PhysX triangle mesh from an engine mesh and write
             * the cooked data to an output stream.
             *
             * Optionally returns information about the materials actually
             * added to the cooked mesh via @p out_addedMaterials (useful for
             * setting per-shape materials later).
             *
             * @param mesh Source engine mesh.
             * @param outputStream Destination output stream for cooked data.
             * @param params Cooking parameters.
             * @param out_addedMaterials Optional out parameter receiving
             *        material array and count used by the cooked mesh.
             */
            void cookPxTriangleMesh( SmartPtr<IMesh> mesh, PxOutputStream &outputStream, Params params,
                                     AddedMaterials *out_addedMaterials = nullptr );

            /**
             * @brief Cook a PhysX triangle mesh and save the cooked result to a file
             * identified by @p PxsOutputFile in the engine's resource system.
             *
             * @param mesh Source engine mesh.
             * @param PxsOutputFile Target filename/resource for the cooked mesh.
             * @param params Cooking parameters.
             * @param out_addedMaterials Optional out parameter receiving
             *        material array and count used by the cooked mesh.
             */
            void cookPxTriangleMeshToFile( SmartPtr<IMesh> mesh, String PxsOutputFile, Params params,
                                           AddedMaterials *out_addedMaterials = nullptr );

            /**
             * @brief Cook a convex PhysX mesh from the provided engine mesh and
             * write it to a PxOutputStream.
             *
             * Convex cooking may perform vertex reduction and hull generation
             * appropriate for PhysX convex meshes.
             */
            void cookPxConvexMesh( SmartPtr<IMesh> mesh, PxOutputStream &outputStream, Params params );

            /**
             * @brief Cook a CCD (continuous collision detection) skeleton from
             * the provided mesh and write it to @p outputStream.
             *
             * The CCD skeleton is used for narrow-phase CCD queries in PhysX.
             */
            void cookPxCCDSkeleton( SmartPtr<IMesh> mesh, PxOutputStream &outputStream, Params params );

            /*
            createPxTriangleMesh
            Cooks an Px mesh from an ogre mesh and returns it, does not save to file.
            */
            /**
             * @brief Create and return an in-memory PxTriangleMesh by cooking
             * the supplied engine mesh. Does not write to disk.
             *
             * @param mesh Source engine mesh.
             * @param params Cooking parameters.
             * @param out_addedMaterials Optional out parameter receiving
             *        material array and count used by the cooked mesh.
             * @return Pointer to the created PxTriangleMesh (owned by caller
             *         according to PhysX memory rules).
             */
            PxTriangleMesh *createPxTriangleMesh( SmartPtr<IMesh> mesh, Params params,
                                                  AddedMaterials *out_addedMaterials = nullptr );

            /**
             * @brief Create and return a cooked PxConvexMesh for the provided mesh.
             * @param mesh Source engine mesh.
             * @param params Cooking parameters.
             * @return Pointer to the created PxConvexMesh.
             */
            PxConvexMesh *createPxConvexMesh( SmartPtr<IMesh> mesh, Params params );

            // Singleton
            /**
             * @brief Get the singleton instance of the PhysxCooker helper.
             * @return Reference to the global PhysxCooker instance.
             */
            static PhysxCooker &getSingleton();
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxCooker_h__
