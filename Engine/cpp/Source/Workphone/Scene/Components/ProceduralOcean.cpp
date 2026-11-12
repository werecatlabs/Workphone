#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralOcean.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        void subscribeOceanUpdates( ProceduralOcean *ocean )
        {
            if( ocean->getActor() )
            {
                if( auto app = core::IApplicationManager::instancePtr() )
                {
                    if( auto manager = app->getGameManager(); manager && manager->isLoaded() )
                    {
                        // Child actors may not have an actor update slot. Defer generation
                        // through the component until actor and sibling loading has finished.
                        manager->registerComponentUpdate( TaskId::Application,
                                                          Thread::UpdateState::Update, ocean );
                    }
                }
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralOcean, ProceduralMesh );

    ProceduralOcean::ProceduralOcean() = default;
    ProceduralOcean::~ProceduralOcean() = default;

    void ProceduralOcean::setSize( f32 width, f32 depth )
    {
        if( m_width != width || m_depth != depth )
        {
            m_width = width;
            m_depth = depth;
            requestRegeneration();
        }
        subscribeOceanUpdates( this );
    }

    bool ProceduralOcean::isLandscapeOwned() const
    {
        return m_landscapeOwned;
    }

    void ProceduralOcean::setLandscapeOwned( bool owned )
    {
        m_landscapeOwned = owned;
    }

    SmartPtr<IMesh> ProceduralOcean::buildMesh()
    {
        if( !std::isfinite( m_width ) || !std::isfinite( m_depth ) || m_width < .01f || m_depth < .01f ||
            m_width > 1000000 || m_depth > 1000000 )
        {
            throw std::runtime_error( "Ocean width and depth must be between 0.01 and 1000000 metres." );
        }
        if( !std::isfinite( m_roughness ) || m_roughness < 0 || m_roughness > 1 ||
            !std::isfinite( m_colour.r ) || !std::isfinite( m_colour.g ) ||
            !std::isfinite( m_colour.b ) || m_colour.r < 0 || m_colour.r > 1 || m_colour.g < 0 ||
            m_colour.g > 1 || m_colour.b < 0 || m_colour.b > 1 )
        {
            throw std::runtime_error( "Ocean colour and roughness must be finite values in [0,1]." );
        }

        const auto x = real_Num( m_width ) * .5f, z = real_Num( m_depth ) * .5f;
        Array<Vector3<real_Num>> positions = { { -x, 0, -z }, { -x, 0, z }, { x, 0, z }, { x, 0, -z } };
        Array<Vector3<real_Num>> normals( 4, Vector3<real_Num>::unitY() );
        Array<Vector2<real_Num>> uvs = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
        Array<u32> indices = { 0, 1, 2, 0, 2, 3 };
        auto mesh = MeshUtil::createMesh( positions, normals, uvs, indices );
        if( !mesh )
        {
            throw std::runtime_error( "Cannot create ocean surface geometry." );
        }
        for( auto subMesh : mesh->getSubMeshes() )
        {
            subMesh->setMaterialName( "Procedural/Ocean" );
        }
        return mesh;
    }

    void ProceduralOcean::applyGeneratedMaterials()
    {
        auto actor = getActor();
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        auto manager = graphics ? graphics->getMaterialManager() : nullptr;
        if( !actor || !manager )
        {
            return;
        }
        if( !m_generatedMaterial )
        {
            for( auto component : actor->getComponentsByType<Material>() )
            {
                if( component->getIndex() == 0 )
                {
                    // A material explicitly supplied by the user takes precedence.
                    if( component->getHandle()->getUUIDAsString() != m_generatedMaterialId )
                    {
                        return;
                    }
                    m_generatedMaterial = component;
                    break;
                }
            }
            if( !m_generatedMaterial )
            {
                m_generatedMaterial = actor->addComponent<Material>();
                if( !m_generatedMaterial )
                {
                    throw std::runtime_error( "Cannot create ocean material component." );
                }
                m_generatedMaterial->getHandle()->setUUID( StringUtil::getUUID() );
                m_generatedMaterial->setIndex( 0 );
                m_generatedMaterial->setState( getState() );
            }
        }
        if( m_materialResource && m_generatedMaterial->getMaterial() != m_materialResource )
        {
            return;
        }
        if( !m_materialResource )
        {
            // Serialized components may refer to another instance's resource. Rebind
            // to an owned material so editing or removing this ocean cannot affect it.
            m_materialResource = dynamic_pointer_cast<render::IMaterial>(
                manager->create( "ProceduralOcean/" + StringUtil::getUUID() ) );
            if( !m_materialResource )
            {
                throw std::runtime_error( "Cannot create ocean surface material." );
            }
            m_materialResource->load( nullptr );
            m_generatedMaterial->setMaterial( m_materialResource );
        }
        m_materialResource->setDiffuse( ColourF( m_colour.r, m_colour.g, m_colour.b, 1 ) );
        m_materialResource->setSpecular( ColourF( .02f, .02f, .02f, 1 ) );
        m_materialResource->setRoughness( m_roughness );
        m_materialResource->setMetalness( 0 );
        m_materialResource->setTransparent( false );
        m_materialResource->setCutout( false );
        m_materialResource->setDepthWrite( true );
        m_materialResource->setCullMode( 1 );
    }

    SmartPtr<Properties> ProceduralOcean::getProperties() const
    {
        auto properties = ProceduralMesh::getProperties();
        properties->setProperty( "Width", m_width );
        properties->setProperty( "Depth", m_depth );
        properties->setProperty( "Colour", m_colour );
        properties->setProperty( "Roughness", m_roughness );
        properties->setProperty( "Landscape Owned", m_landscapeOwned );
        properties->getPropertyObject( "Landscape Owned" ).setReadOnly( true );
        auto materialId = m_generatedMaterialId;
        if( m_generatedMaterial )
        {
            materialId = m_generatedMaterial->getMaterial() == m_materialResource
                             ? m_generatedMaterial->getHandle()->getUUIDAsString()
                             : String();
        }
        properties->setProperty( "Generated Material ID", materialId );
        properties->getPropertyObject( "Generated Material ID" ).setReadOnly( true );
        return properties;
    }

    void ProceduralOcean::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }
        ProceduralMesh::setProperties( properties );
        properties->getPropertyValue( "Width", m_width );
        properties->getPropertyValue( "Depth", m_depth );
        properties->getPropertyValue( "Colour", m_colour );
        properties->getPropertyValue( "Roughness", m_roughness );
        properties->getPropertyValue( "Landscape Owned", m_landscapeOwned );
        properties->getPropertyValue( "Generated Material ID", m_generatedMaterialId );
        subscribeOceanUpdates( this );
    }

    void ProceduralOcean::unload( SmartPtr<ISharedObject> data )
    {
        if( auto app = core::IApplicationManager::instancePtr() )
        {
            if( auto manager = app->getGameManager() )
            {
                manager->unregisterComponentUpdate( TaskId::Application, Thread::UpdateState::Update,
                                                    this );
            }
        }
        auto actor = getActor();
        if( m_generatedMaterial && m_generatedMaterial->getMaterial() == m_materialResource )
        {
            m_generatedMaterial->unload( nullptr );
            if( actor )
            {
                actor->removeComponentInstance( m_generatedMaterial );
            }
        }
        m_generatedMaterial = nullptr;
        m_generatedMaterialId.clear();
        if( auto app = core::IApplicationManager::instancePtr() )
        {
            if( auto graphics = app->getGraphicsSystem() )
            {
                if( auto manager = graphics->getMaterialManager(); manager && m_materialResource )
                {
                    manager->destroyResource( m_materialResource );
                }
            }
        }
        m_materialResource = nullptr;
        ProceduralMesh::unload( data );
    }
}  // namespace workphone::scene
