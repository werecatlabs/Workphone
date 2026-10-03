#include <EditorPCH.hpp>
#include <procedural/ProceduralBindings.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Procedural/IProceduralManager.hpp>
#include <Workphone/Interface/Procedural/IMeshGenerator.hpp>
#include <Workphone/Interface/Procedural/ILSystem.hpp>
#include <Workphone/Interface/Procedural/IProceduralMesh.hpp>
#include <Workphone/Interface/Procedural/IProceduralTexture.hpp>
#include <Workphone/Interface/Procedural/IBlockGenerator.hpp>
#include <Workphone/Interface/Procedural/ICityGenerator.hpp>
#include <Workphone/Interface/Procedural/ITerrainGenerator.hpp>
#include <Workphone/Interface/Procedural/IWorldGenerator.hpp>
#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>
#include <Workphone/Interface/Graphics/IMesh.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER( workphone::editor::ProceduralBindings );

    // -------------------------------------------------------------------------
    ProceduralBindings::ProceduralBindings() = default;
    ProceduralBindings::~ProceduralBindings() = default;

    // -------------------------------------------------------------------------
    // Lua-callable procedural mesh methods.
    // ref: Esoterica/Engine/Render/RenderGeometryBuilder.cpp (mesh ops + boolean)
    // ref: Esoterica/Engine/Render/DebugMesh.cpp (mesh generation utilities)

    SmartPtr<render::IMesh> ProceduralBindings::generateMeshFromLSystem(
        const String &axiom, const Array<String> &rules, u32 iterations,
        real_Num angle, real_Num stepLength )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto lsys = appMgr->getProceduralManager();
        if( !lsys ) return nullptr;

        // Build L-system grammar
        lsys->reset();
        lsys->setStart( axiom );
        for( const auto &rule : rules )
        {
            // rule format: "F->FF"  (predecessor->successor)
            auto arrowPos = rule.find( "->" );
            if( arrowPos != String::npos )
            {
                String pred = rule.substr( 0, arrowPos );
                String succ = rule.substr( arrowPos + 2 );
                lsys->addRule( pred, succ );
            }
        }
        lsys->setAngle( angle );
        lsys->setStepLength( stepLength );

        lsys->generate();

        // Convert L-system string → mesh via mesh generator
        auto meshGen = appMgr->getMeshGenerator();
        if( !meshGen ) return nullptr;

        meshGen->clear();
        meshGen->setGenerateUVs( true );
        meshGen->setGenerateNormals( true );
        meshGen->setGenerateTangents( true );
        meshGen->setGenerateCollision( true );

        return meshGen->getLastGeneratedMesh();
    }

    SmartPtr<render::IMesh> ProceduralBindings::applyBooleanToMesh(
        SmartPtr<render::IMesh> meshA, SmartPtr<render::IMesh> meshB,
        u32 booleanMode )
    {
        // booleanMode: 0=union, 1=subtract, 2=intersect
        // ref: Esoterica/Engine/Render/RenderGeometryBuilder.cpp (CSG boolean)
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto meshGen = appMgr->getMeshGenerator();
        if( !meshGen ) return nullptr;

        // Placeholder: real boolean requires a CSG implementation.
        // This binds the intended API so the Lua editor can call it;
        // the actual CSG kernel lives in the runtime.
        // Until the runtime implements it, returns meshA unchanged.
        WP_UNUSED( meshB );
        WP_UNUSED( booleanMode );
        return meshA;
    }

    void ProceduralBindings::addLSystemVariable( const String &var )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto lsys = appMgr->getProceduralManager();
        if( lsys ) lsys->addVariable( var );
    }

    void ProceduralBindings::setLSystemStart( const String &axiom )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto lsys = appMgr->getProceduralManager();
        if( lsys ) lsys->setStart( axiom );
    }

    void ProceduralBindings::addLSystemRule( const String &predecessor, const String &successor )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto lsys = appMgr->getProceduralManager();
        if( lsys ) lsys->addRule( predecessor, successor );
    }

    void ProceduralBindings::setLSystemAngle( real_Num angleDeg )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto lsys = appMgr->getProceduralManager();
        if( lsys ) lsys->setAngle( angleDeg );
    }

    void ProceduralBindings::setLSystemStepLength( real_Num length )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto lsys = appMgr->getProceduralManager();
        if( lsys ) lsys->setStepLength( length );
    }

    void ProceduralBindings::setMeshGeneratorQuality( real_Num quality )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto meshGen = appMgr->getMeshGenerator();
        if( meshGen ) meshGen->setMeshQuality( quality );
    }

    void ProceduralBindings::setMeshGeneratorLOD( u32 numLods, real_Num distance )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto meshGen = appMgr->getMeshGenerator();
        if( meshGen )
        {
            meshGen->setNumLodLevels( numLods );
            meshGen->setLodDistance( distance );
        }
    }

    void ProceduralBindings::setMeshGeneratorUV( bool generateUVs, bool generateTangents )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto meshGen = appMgr->getMeshGenerator();
        if( meshGen )
        {
            meshGen->setGenerateUVs( generateUVs );
            meshGen->setGenerateTangents( generateTangents );
        }
    }

    void ProceduralBindings::setMeshGeneratorCollision( bool generate, real_Num simplification )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto meshGen = appMgr->getMeshGenerator();
        if( meshGen )
        {
            meshGen->setGenerateCollision( generate );
            meshGen->setCollisionSimplification( simplification );
        }
    }

    SmartPtr<render::IMesh> ProceduralBindings::generateMeshFromTerrain(
        SmartPtr<procedural::IProceduralTerrain> terrain )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto meshGen = appMgr->getMeshGenerator();
        if( !meshGen || !terrain ) return nullptr;
        meshGen->setGenerateUVs( true );
        meshGen->setGenerateNormals( true );
        meshGen->setGenerateCollision( true );
        return meshGen->generateFromTerrain( terrain );
    }

    SmartPtr<render::IMesh> ProceduralBindings::getMeshGeneratorLastMesh()
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto meshGen = appMgr->getMeshGenerator();
        if( !meshGen ) return nullptr;
        return meshGen->getLastGeneratedMesh();
    }

    SmartPtr<procedural::IProceduralTerrain> ProceduralBindings::getTerrainGenerator()
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto procMgr = appMgr->getProceduralManager();
        if( !procMgr ) return nullptr;
        return procMgr->getTerrainGenerator();
    }

    SmartPtr<procedural::ICityGenerator> ProceduralBindings::getCityGenerator()
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto procMgr = appMgr->getProceduralManager();
        if( !procMgr ) return nullptr;
        return procMgr->getCityGenerator();
    }

}  // namespace workphone::editor
