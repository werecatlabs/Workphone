#ifndef MeshGeneratorDefault_h__
#define MeshGeneratorDefault_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IMeshGenerator.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API MeshGeneratorDefault : public CProceduralGenerator<IMeshGenerator>
        {
        public:
            MeshGeneratorDefault();
            ~MeshGeneratorDefault() override;

            SmartPtr<IMesh> generate( SmartPtr<IRoadNetwork> roadNetwork ) override;

            SmartPtr<IMesh> generateFromTerrain( SmartPtr<IProceduralTerrain> terrain ) override;

            SmartPtr<IMesh> generateFromObjects(
                const Array<SmartPtr<IProceduralObject>> &objects ) override;

            real_Num getMeshQuality() const override;

            void setMeshQuality( real_Num quality ) override;

            u32 getNumLodLevels() const override;

            void setNumLodLevels( u32 numLods ) override;

            real_Num getLodDistance() const override;

            void setLodDistance( real_Num distance ) override;

            u32 getMaxVertexCount() const override;

            void setMaxVertexCount( u32 maxVertices ) override;

            u32 getMaxTriangleCount() const override;

            void setMaxTriangleCount( u32 maxTriangles ) override;

            real_Num getSimplificationThreshold() const override;

            void setSimplificationThreshold( real_Num threshold ) override;

            bool getGenerateUVs() const override;

            void setGenerateUVs( bool generateUVs ) override;

            real_Num getUVScale() const override;

            void setUVScale( real_Num scale ) override;

            bool getGenerateNormals() const override;

            void setGenerateNormals( bool generateNormals ) override;

            bool getGenerateTangents() const override;

            void setGenerateTangents( bool generateTangents ) override;

            bool getOptimizeMesh() const override;

            void setOptimizeMesh( bool optimize ) override;

            bool getMergeDuplicateVertices() const override;

            void setMergeDuplicateVertices( bool merge ) override;

            real_Num getVertexMergeTolerance() const override;

            void setVertexMergeTolerance( real_Num tolerance ) override;

            bool getGenerateCollision() const override;

            void setGenerateCollision( bool generateCollision ) override;

            real_Num getCollisionSimplification() const override;

            void setCollisionSimplification( real_Num simplification ) override;

            void validateParameters() override;

            bool isReady() const override;

            void clear() override;

            SmartPtr<IMesh> getLastGeneratedMesh() const override;

            Array<SmartPtr<IMesh>> getAllGeneratedMeshes() const override;

            real_Num getGenerationProgress() const override;

            String getGenerationStatus() const override;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // MeshGeneratorDefault_h__
