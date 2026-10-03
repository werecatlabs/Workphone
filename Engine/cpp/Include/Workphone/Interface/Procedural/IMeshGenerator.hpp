#ifndef IMeshGenerator_h__
#define IMeshGenerator_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for procedural mesh generation.
         *
         * This interface defines the contract for mesh generator implementations, providing
         * methods for generating meshes from various input sources including road networks,
         * terrain data, and procedural objects. It includes parameters for controlling
         * mesh quality, LOD levels, and generation options.
         */
        class WPCore_API IMeshGenerator : public IProceduralGenerator
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IMeshGenerator() override;

            // Primary generation methods

            /**
             * @brief Generates a mesh from a road network.
             * @param roadNetwork Smart pointer to the road network.
             * @return Smart pointer to the generated mesh.
             */
            virtual SmartPtr<IMesh> generate( SmartPtr<IRoadNetwork> roadNetwork ) = 0;

            /**
             * @brief Generates a mesh from terrain data.
             * @param terrain Smart pointer to the terrain data.
             * @return Smart pointer to the generated mesh.
             */
            virtual SmartPtr<IMesh> generateFromTerrain( SmartPtr<IProceduralTerrain> terrain ) = 0;

            /**
             * @brief Generates a mesh from procedural objects.
             * @param objects Array of procedural objects.
             * @return Smart pointer to the generated mesh.
             */
            virtual SmartPtr<IMesh> generateFromObjects(
                const Array<SmartPtr<IProceduralObject>> &objects ) = 0;

            // Mesh quality and LOD parameters

            /**
             * @brief Gets the mesh quality level.
             * @return Quality level (0-1, where 1 is highest quality).
             */
            virtual real_Num getMeshQuality() const = 0;

            /**
             * @brief Sets the mesh quality level.
             * @param quality Quality level (0-1, where 1 is highest quality).
             */
            virtual void setMeshQuality( real_Num quality ) = 0;

            /**
             * @brief Gets the number of LOD levels to generate.
             * @return Number of LOD levels.
             */
            virtual u32 getNumLodLevels() const = 0;

            /**
             * @brief Sets the number of LOD levels to generate.
             * @param numLods Number of LOD levels.
             */
            virtual void setNumLodLevels( u32 numLods ) = 0;

            /**
             * @brief Gets the LOD distance threshold.
             * @return Distance threshold for LOD switching.
             */
            virtual real_Num getLodDistance() const = 0;

            /**
             * @brief Sets the LOD distance threshold.
             * @param distance Distance threshold for LOD switching.
             */
            virtual void setLodDistance( real_Num distance ) = 0;

            // Geometry parameters

            /**
             * @brief Gets the maximum vertex count per mesh.
             * @return Maximum vertex count.
             */
            virtual u32 getMaxVertexCount() const = 0;

            /**
             * @brief Sets the maximum vertex count per mesh.
             * @param maxVertices Maximum vertex count.
             */
            virtual void setMaxVertexCount( u32 maxVertices ) = 0;

            /**
             * @brief Gets the maximum triangle count per mesh.
             * @return Maximum triangle count.
             */
            virtual u32 getMaxTriangleCount() const = 0;

            /**
             * @brief Sets the maximum triangle count per mesh.
             * @param maxTriangles Maximum triangle count.
             */
            virtual void setMaxTriangleCount( u32 maxTriangles ) = 0;

            /**
             * @brief Gets the mesh simplification threshold.
             * @return Simplification threshold.
             */
            virtual real_Num getSimplificationThreshold() const = 0;

            /**
             * @brief Sets the mesh simplification threshold.
             * @param threshold Simplification threshold.
             */
            virtual void setSimplificationThreshold( real_Num threshold ) = 0;

            // UV and texture parameters

            /**
             * @brief Gets whether to generate UV coordinates.
             * @return True if UV coordinates should be generated.
             */
            virtual bool getGenerateUVs() const = 0;

            /**
             * @brief Sets whether to generate UV coordinates.
             * @param generateUVs True to generate UV coordinates.
             */
            virtual void setGenerateUVs( bool generateUVs ) = 0;

            /**
             * @brief Gets the UV scale factor.
             * @return UV scale factor.
             */
            virtual real_Num getUVScale() const = 0;

            /**
             * @brief Sets the UV scale factor.
             * @param scale UV scale factor.
             */
            virtual void setUVScale( real_Num scale ) = 0;

            /**
             * @brief Gets whether to generate normal vectors.
             * @return True if normal vectors should be generated.
             */
            virtual bool getGenerateNormals() const = 0;

            /**
             * @brief Sets whether to generate normal vectors.
             * @param generateNormals True to generate normal vectors.
             */
            virtual void setGenerateNormals( bool generateNormals ) = 0;

            /**
             * @brief Gets whether to generate tangent vectors.
             * @return True if tangent vectors should be generated.
             */
            virtual bool getGenerateTangents() const = 0;

            /**
             * @brief Sets whether to generate tangent vectors.
             * @param generateTangents True to generate tangent vectors.
             */
            virtual void setGenerateTangents( bool generateTangents ) = 0;

            // Optimization parameters

            /**
             * @brief Gets whether to optimize the generated mesh.
             * @return True if mesh optimization is enabled.
             */
            virtual bool getOptimizeMesh() const = 0;

            /**
             * @brief Sets whether to optimize the generated mesh.
             * @param optimize True to enable mesh optimization.
             */
            virtual void setOptimizeMesh( bool optimize ) = 0;

            /**
             * @brief Gets whether to merge duplicate vertices.
             * @return True if duplicate vertex merging is enabled.
             */
            virtual bool getMergeDuplicateVertices() const = 0;

            /**
             * @brief Sets whether to merge duplicate vertices.
             * @param merge True to enable duplicate vertex merging.
             */
            virtual void setMergeDuplicateVertices( bool merge ) = 0;

            /**
             * @brief Gets the vertex merge tolerance.
             * @return Vertex merge tolerance.
             */
            virtual real_Num getVertexMergeTolerance() const = 0;

            /**
             * @brief Sets the vertex merge tolerance.
             * @param tolerance Vertex merge tolerance.
             */
            virtual void setVertexMergeTolerance( real_Num tolerance ) = 0;

            // Collision and physics parameters

            /**
             * @brief Gets whether to generate collision data.
             * @return True if collision data should be generated.
             */
            virtual bool getGenerateCollision() const = 0;

            /**
             * @brief Sets whether to generate collision data.
             * @param generateCollision True to generate collision data.
             */
            virtual void setGenerateCollision( bool generateCollision ) = 0;

            /**
             * @brief Gets the collision mesh simplification level.
             * @return Collision mesh simplification level.
             */
            virtual real_Num getCollisionSimplification() const = 0;

            /**
             * @brief Sets the collision mesh simplification level.
             * @param simplification Collision mesh simplification level.
             */
            virtual void setCollisionSimplification( real_Num simplification ) = 0;

            // Validation and utility methods

            /**
             * @brief Validates the current mesh generation parameters.
             */
            virtual void validateParameters() = 0;

            /**
             * @brief Gets whether the generator is ready to generate meshes.
             * @return True if the generator is ready.
             */
            virtual bool isReady() const = 0;

            /**
             * @brief Clears all generated meshes and resets the generator.
             */
            virtual void clear() = 0;

            /**
             * @brief Gets the last generated mesh.
             * @return Smart pointer to the last generated mesh.
             */
            virtual SmartPtr<IMesh> getLastGeneratedMesh() const = 0;

            /**
             * @brief Gets all generated meshes.
             * @return Array of smart pointers to generated meshes.
             */
            virtual Array<SmartPtr<IMesh>> getAllGeneratedMeshes() const = 0;

            /**
             * @brief Gets the generation progress (0-1).
             * @return Generation progress as a value between 0 and 1.
             */
            virtual real_Num getGenerationProgress() const = 0;

            /**
             * @brief Gets the generation status message.
             * @return Status message describing the current generation state.
             */
            virtual String getGenerationStatus() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IMeshGenerator_h__
