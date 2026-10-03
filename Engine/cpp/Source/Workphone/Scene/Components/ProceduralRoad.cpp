#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralProperties.hpp"
#include <Workphone/Scene/Components/ProceduralRoad.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        template <class T>
        SmartPtr<T> service( const String &name )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( !app || !app->getFactoryManager() )
                return nullptr;

            return app->getFactoryManager()->make_object<T>();
        }
    }  // namespace

    ProceduralRoad::ProceduralRoad() = default;
    ProceduralRoad::~ProceduralRoad() = default;

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralRoad, ProceduralMeshComponent );
    SmartPtr<IMesh> ProceduralRoad::buildMesh()
    {
        if( !m_service )
            m_service = service<procedural::IRoadSystem>( "IRoadSystem" );
        if( !m_service )
            throw std::runtime_error( "Load the WPProcedural plugin to generate roads" );

        m_spec.end = m_end;
        const auto length = ( m_spec.end - m_spec.start ).length();
        if( !std::isfinite( length ) || length < 0.01 || length > 10000 )
            throw std::runtime_error( "Road length must be between 0.01 and 10000 metres" );
        m_service->setSeed( m_spec.seed );
        auto result = m_service->generateSegment( m_spec );
        auto mesh = workphone::make_ptr<workphone::Mesh>();
        auto append = [&]( const procedural::RoadMesh &layer, const String &material ) {
            if( layer.indices.empty() )
                return;
            Array<Vector3<real_Num>> positions, normals;
            Array<Vector2<real_Num>> uvs;
            for( const auto &v : layer.vertices )
            {
                positions.push_back( v.position );
                normals.push_back( v.normal );
                uvs.push_back( v.uv );
            }
            auto part = MeshUtil::createMesh( positions, normals, uvs, layer.indices );
            for( auto sub : part->getSubMeshes() )
            {
                sub->setMaterialName( material );
                mesh->addSubMesh( sub );
            }
        };
        append( result.roadSurface, "Procedural/Road" );
        for( auto &layer : result.sidewalks )
            append( layer, "Procedural/Sidewalk" );
        for( auto &layer : result.kerbs )
            append( layer, "Procedural/Kerb" );
        append( result.markings, "Procedural/Markings" );
        append( result.dressing, "Procedural/Dressing" );
        append( result.oldTarmac, "Procedural/Road" );
        append( result.potholes, "Procedural/Road" );
        append( result.manholes, "Procedural/Metal" );
        append( result.gullyGrates, "Procedural/Metal" );
        append( result.arrows, "Procedural/Markings" );
        append( result.pedXing, "Procedural/Markings" );
        m_result = std::move( result );
        return mesh;
    }
    SmartPtr<Properties> ProceduralRoad::getProperties() const
    {
        auto p = ProceduralMeshComponent::getProperties();
        p->setProperty( "Seed", m_spec.seed );
        p->setProperty( "Start", m_spec.start );
        p->setProperty( "End", m_end );
        procedural_properties::setEnum( p, "Road Class", static_cast<s32>( m_spec.roadClass ),
                                        { "Highway", "Arterial", "Residential", "Alley", "Footway" } );
        procedural_properties::setEnum( p, "Surface", static_cast<s32>( m_spec.surface ),
                                        { "Asphalt", "Concrete", "Dirt" } );
        p->setProperty( "Sidewalks", m_spec.generateSidewalks );
        p->setProperty( "Kerbs", m_spec.generateKerbs );
        p->setProperty( "Markings", m_spec.generateMarkings );
        p->setProperty( "Dressing", m_spec.generateDressing );
        return p;
    }
    void ProceduralRoad::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
            return;
        ProceduralMeshComponent::setProperties( p );
        p->getPropertyValue( "Seed", m_spec.seed );
        p->getPropertyValue( "Start", m_spec.start );
        p->getPropertyValue( "End", m_end );
        s32 cls = static_cast<s32>( m_spec.roadClass ), surface = static_cast<s32>( m_spec.surface );
        cls = procedural_properties::getEnum(
            p, "Road Class", cls, { "Highway", "Arterial", "Residential", "Alley", "Footway" } );
        surface =
            procedural_properties::getEnum( p, "Surface", surface, { "Asphalt", "Concrete", "Dirt" } );
        m_spec.roadClass = static_cast<procedural::RoadClass>( std::clamp( cls, 0, 4 ) );
        m_spec.surface = static_cast<procedural::RoadSurface>( std::clamp( surface, 0, 2 ) );
        p->getPropertyValue( "Sidewalks", m_spec.generateSidewalks );
        p->getPropertyValue( "Kerbs", m_spec.generateKerbs );
        p->getPropertyValue( "Markings", m_spec.generateMarkings );
        p->getPropertyValue( "Dressing", m_spec.generateDressing );
    }
    void ProceduralRoad::unload( SmartPtr<ISharedObject> data )
    {
        ProceduralMeshComponent::unload( data );
        m_service = nullptr;
        m_result = {};
    }
}  // namespace workphone::scene
