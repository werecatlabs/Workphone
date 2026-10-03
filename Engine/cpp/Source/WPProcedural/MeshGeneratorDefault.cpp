#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        SmartPtr<IMesh> MeshGeneratorDefault::generate( SmartPtr<IRoadNetwork> roadNetwork )
        {
            SmartPtr<IMesh> mesh;

            return mesh;
        }

        SmartPtr<IMesh> MeshGeneratorDefault::generateFromTerrain( SmartPtr<IProceduralTerrain> terrain )
        {
            return {};
        }

        SmartPtr<IMesh> MeshGeneratorDefault::generateFromObjects(
            const Array<SmartPtr<IProceduralObject>> &objects )
        {
            return {};
        }

        real_Num MeshGeneratorDefault::getMeshQuality() const
        {
            return {};
        }

        void MeshGeneratorDefault::setMeshQuality( real_Num quality )
        {
        }

        u32 MeshGeneratorDefault::getNumLodLevels() const
        {
            return {};
        }

        void MeshGeneratorDefault::setNumLodLevels( u32 numLods )
        {
        }

        real_Num MeshGeneratorDefault::getLodDistance() const
        {
            return {};
        }

        void MeshGeneratorDefault::setLodDistance( real_Num distance )
        {
        }

        u32 MeshGeneratorDefault::getMaxVertexCount() const
        {
            return {};
        }

        void MeshGeneratorDefault::setMaxVertexCount( u32 maxVertices )
        {
        }

        u32 MeshGeneratorDefault::getMaxTriangleCount() const
        {
            return {};
        }

        void MeshGeneratorDefault::setMaxTriangleCount( u32 maxTriangles )
        {
        }

        real_Num MeshGeneratorDefault::getSimplificationThreshold() const
        {
            return 0;
        }

        void MeshGeneratorDefault::setSimplificationThreshold( real_Num threshold )
        {
        }

        bool MeshGeneratorDefault::getGenerateUVs() const
        {
            return {};
        }

        void MeshGeneratorDefault::setGenerateUVs( bool generateUVs )
        {
        }

        real_Num MeshGeneratorDefault::getUVScale() const
        {
            return {};
        }

        void MeshGeneratorDefault::setUVScale( real_Num scale )
        {
        }

        bool MeshGeneratorDefault::getGenerateNormals() const
        {
            return {};
        }

        void MeshGeneratorDefault::setGenerateNormals( bool generateNormals )
        {
        }

        bool MeshGeneratorDefault::getGenerateTangents() const
        {
            return {};
        }

        void MeshGeneratorDefault::setGenerateTangents( bool generateTangents )
        {
        }

        bool MeshGeneratorDefault::getOptimizeMesh() const
        {
            return false;
        }

        void MeshGeneratorDefault::setOptimizeMesh( bool optimize )
        {
        }

        bool MeshGeneratorDefault::getMergeDuplicateVertices() const
        {
            return {};
        }

        void MeshGeneratorDefault::setMergeDuplicateVertices( bool merge )
        {
        }

        real_Num MeshGeneratorDefault::getVertexMergeTolerance() const
        {
            return {};
        }

        void MeshGeneratorDefault::setVertexMergeTolerance( real_Num tolerance )
        {
        }

        bool MeshGeneratorDefault::getGenerateCollision() const
        {
            return {};
        }

        void MeshGeneratorDefault::setGenerateCollision( bool generateCollision )
        {
        }

        real_Num MeshGeneratorDefault::getCollisionSimplification() const
        {
            return {};
        }

        void MeshGeneratorDefault::setCollisionSimplification( real_Num simplification )
        {
        }

        void MeshGeneratorDefault::validateParameters()
        {
        }

        bool MeshGeneratorDefault::isReady() const
        {
            return {};
        }

        void MeshGeneratorDefault::clear()
        {
        }

        SmartPtr<IMesh> MeshGeneratorDefault::getLastGeneratedMesh() const
        {
            return {};
        }

        Array<SmartPtr<IMesh>> MeshGeneratorDefault::getAllGeneratedMeshes() const
        {
            return {};
        }

        real_Num MeshGeneratorDefault::getGenerationProgress() const
        {
            return {};
        }

        String MeshGeneratorDefault::getGenerationStatus() const
        {
            return {};
        }

        MeshGeneratorDefault::~MeshGeneratorDefault()
        {
        }

        MeshGeneratorDefault::MeshGeneratorDefault()
        {
        }
    }  // namespace procedural
}  // namespace workphone
