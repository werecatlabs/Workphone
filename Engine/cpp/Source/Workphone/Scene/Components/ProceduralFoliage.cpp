#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralFoliage.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include "ProceduralProperties.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        bool isGrass( ProceduralFoliage::Preset preset )
        {
            return preset == ProceduralFoliage::Preset::Grass ||
                   preset == ProceduralFoliage::Preset::TallGrass;
        }

        SmartPtr<render::ITexture> createCutout( ProceduralFoliage::Preset preset )
        {
            auto manager =
                core::IApplicationManager::instance()->getGraphicsSystem()->getTextureManager();
            if( !manager )
            {
                throw std::runtime_error( "Texture manager is unavailable" );
            }
            auto texture = dynamic_pointer_cast<render::ITexture>(
                manager->create( "ProceduralFoliage/" + StringUtil::getUUID() ) );
            if( !texture )
            {
                throw std::runtime_error( "Cannot create foliage cutout texture" );
            }
            try
            {
                auto properties = texture->getProperties();
                properties->setProperty( "mipmapFilter", s32( render::TextureMipFilter::Cutout ) );
                texture->setProperties( properties );
                String error;
                if( !texture->copyByteData( foliagePresetCutout( preset ),
                                            Vector2I( foliageCutoutSize, foliageCutoutSize ), error ) )
                {
                    throw std::runtime_error( error );
                }
                texture->load( nullptr );
                return texture;
            }
            catch( ... )
            {
                manager->destroyResource( texture );
                throw;
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralFoliage, ProceduralMesh );

    ProceduralFoliage::ProceduralFoliage()
    {
        setPreset( Preset::Oak );
    }

    ProceduralFoliage::~ProceduralFoliage() = default;

    void ProceduralFoliage::setPreset( Preset preset )
    {
        m_preset = static_cast<Preset>( std::clamp( s32( preset ), 0, s32( Preset::Palm ) ) );
        m_recipe = foliagePresetRecipe( m_preset );
        requestRegeneration();
    }

    ProceduralFoliage::Preset ProceduralFoliage::getPreset() const
    {
        return m_preset;
    }

    SmartPtr<IMesh> ProceduralFoliage::buildMesh()
    {
        render::FoliageSpecies species;
        String error;
        if( !generateFoliagePreset( m_preset, m_recipe, species, error ) )
        {
            throw std::runtime_error( error );
        }
        auto &source = species.lods[std::min( m_lod, 2u )];
        auto mesh = make_ptr<workphone::Mesh>();
        for( const auto &section : source.sections )
        {
            if( !section.indexCount )
            {
                continue;
            }
            auto positions = makeSourceBudgetSnapshot<Vector3<real_Num>>();
            auto normals = makeSourceBudgetSnapshot<Vector3<real_Num>>();
            auto uvs = makeSourceBudgetSnapshot<Vector2<real_Num>>();
            auto indices = makeSourceBudgetSnapshot<u32>();
            // Each generated section has its own vertex buffer, preserving bark
            // and cutout slots without retaining unused vertices from the other part.
            auto remap = makeSourceBudgetSnapshot<u32>();
            remap.resize( source.positions.size(), std::numeric_limits<u32>::max() );
            for( u32 i = section.firstIndex; i < section.firstIndex + section.indexCount; ++i )
            {
                const auto original = source.indices[i];
                if( remap[original] == std::numeric_limits<u32>::max() )
                {
                    remap[original] = u32( positions.size() );
                    const auto p = source.positions[original], n = source.normals[original];
                    const auto uv = source.uvs[original];
                    positions.emplace_back( p.x, p.y, p.z );
                    normals.emplace_back( n.x, n.y, n.z );
                    uvs.emplace_back( uv.x, uv.y );
                }
                indices.push_back( remap[original] );
            }
            auto part = MeshUtil::createMesh( positions, normals, uvs, indices );
            if( !part )
            {
                throw std::runtime_error( "Cannot create foliage mesh" );
            }
            for( auto sub : part->getSubMeshes() )
            {
                sub->setMaterialName( section.material ? "Procedural/FoliageLeaves"
                                                       : "Procedural/FoliageBark" );
                mesh->addSubMesh( sub );
            }
        }
        return mesh;
    }

    void ProceduralFoliage::applyGeneratedMaterials()
    {
        auto actor = getActor();
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        auto manager = graphics ? graphics->getMaterialManager() : nullptr;
        if( !actor || !manager )
        {
            return;
        }

        const u32 count = isGrass( m_preset ) ? 1 : 2;
        const auto existing = actor->getComponentsByType<Material>();
        if( m_generatedMaterials.empty() )
        {
            m_generatedMaterials.resize( count );
            for( auto component : existing )
            {
                const auto id = ";" + component->getHandle()->getUUIDAsString() + ";";
                if( component->getIndex() < count && m_generatedMaterialIds.find( id ) != String::npos )
                {
                    m_generatedMaterials[component->getIndex()] = component;
                }
            }
        }

        auto previousTexture = m_leafTexture;
        if( !m_leafTexture || m_texturePreset != m_preset )
        {
            m_leafTexture = createCutout( m_preset );
            m_texturePreset = m_preset;
        }
        if( m_generatedMaterials.size() < count )
        {
            m_generatedMaterials.resize( count );
        }
        if( m_materialResources.size() < count )
        {
            m_materialResources.resize( count );
        }
        for( u32 index = 0; index < count; ++index )
        {
            auto component = m_generatedMaterials[index];
            if( !component )
            {
                // A user-authored material may occupy just one slot. Leave that
                // slot alone and still supply defaults for the other plant parts.
                if( std::any_of( existing.begin(), existing.end(), [index]( const auto &candidate ) {
                        return candidate->getIndex() == index;
                    } ) )
                {
                    continue;
                }
                component = actor->addComponent<Material>();
                if( !component )
                {
                    throw std::runtime_error( "Cannot create foliage material component" );
                }
                m_generatedMaterials[index] = component;
                component->getHandle()->setUUID( StringUtil::getUUID() );
                component->setIndex( index );
                component->setState( getState() );
            }
            auto material = m_materialResources[index];
            if( material )
            {
                // An authored resource replacing this slot is no longer generated.
                if( component->getMaterial() != material )
                {
                    continue;
                }
            }
            else
            {
                material = dynamic_pointer_cast<render::IMaterial>(
                    manager->create( "ProceduralFoliage/" + StringUtil::getUUID() ) );
                if( !material )
                {
                    throw std::runtime_error( "Cannot create foliage material" );
                }
                // Serialized/cloned Material components can still reference the
                // source plant's resource. Only mutate resources created by this
                // component, keeping duplicated plants independently editable.
                m_materialResources[index] = material;
                // Material setters write technique/pass state. Load before the
                // first edit: setMaterial otherwise only queues an asynchronous
                // load, so tint, alpha cutout and texture edits would be lost.
                material->load( nullptr );
                component->setMaterial( material );
            }
            const bool leaves = isGrass( m_preset ) || index == 1;
            material->setDiffuse( leaves ? m_recipe.leaves.tint : m_recipe.bark.tint );
            // Match the procedural scene's dielectric material defaults. White
            // specular would suppress coloured diffuse in the PBR lighting model.
            material->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
            material->setRoughness( 1 );
            material->setMetalness( 0 );
            material->setTransparent( false );
            material->setDepthWrite( true );
            material->setCutout( leaves );
            material->setAlphaClip( .5f );
            material->setCullMode( 1 );
            material->setTexture( leaves ? m_leafTexture : SmartPtr<render::ITexture>(), 0 );
        }
        while( m_generatedMaterials.size() > count )
        {
            const auto index = m_generatedMaterials.size() - 1;
            auto component = m_generatedMaterials.back();
            m_generatedMaterials.pop_back();
            if( component && index < m_materialResources.size() &&
                component->getMaterial() == m_materialResources[index] )
            {
                component->unload( nullptr );
                actor->removeComponentInstance( component );
            }
        }
        while( m_materialResources.size() > count )
        {
            auto material = m_materialResources.back();
            m_materialResources.pop_back();
            if( material )
            {
                manager->destroyResource( material );
            }
        }
        if( previousTexture && previousTexture != m_leafTexture )
        {
            graphics->getTextureManager()->destroyResource( previousTexture );
        }
    }

    SmartPtr<Properties> ProceduralFoliage::getProperties() const
    {
        auto p = ProceduralMesh::getProperties();
        procedural_properties::setEnum(
            p, "Preset", s32( m_preset ),
            { "Oak", "Birch", "Pine", "Bush", "Grass", "Tall Grass", "Palm" } );
        p->getPropertyObject( "Preset" ).setReadOnly( true );
        p->setProperty( "Seed", m_recipe.seed );
        p->setProperty( "Height", m_recipe.height );
        p->setProperty( "Radius", m_recipe.radius );
        p->setProperty( "Trunk Radius", m_recipe.trunkRadius );
        p->setProperty( "Branches", m_recipe.branches );
        p->setProperty( "Bark Colour", m_recipe.bark.tint );
        p->setProperty( "Leaf Colour", m_recipe.leaves.tint );
        p->setProperty( "LOD", m_lod );
        auto ids = m_generatedMaterialIds;
        if( !m_generatedMaterials.empty() )
        {
            ids.clear();
            for( size_t index = 0; index < m_generatedMaterials.size(); ++index )
            {
                auto component = m_generatedMaterials[index];
                if( component && index < m_materialResources.size() &&
                    component->getMaterial() == m_materialResources[index] )
                {
                    ids += ";" + component->getHandle()->getUUIDAsString() + ";";
                }
            }
        }
        p->setProperty( "Generated Material IDs", ids );
        p->getPropertyObject( "Generated Material IDs" ).setReadOnly( true );
        return p;
    }

    void ProceduralFoliage::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
        {
            return;
        }
        ProceduralMesh::setProperties( p );
        const auto preset = procedural_properties::getEnum(
            p, "Preset", s32( m_preset ),
            { "Oak", "Birch", "Pine", "Bush", "Grass", "Tall Grass", "Palm" } );
        if( preset != s32( m_preset ) )
        {
            setPreset( static_cast<Preset>( preset ) );
        }
        p->getPropertyValue( "Seed", m_recipe.seed );
        p->getPropertyValue( "Height", m_recipe.height );
        p->getPropertyValue( "Radius", m_recipe.radius );
        p->getPropertyValue( "Trunk Radius", m_recipe.trunkRadius );
        p->getPropertyValue( "Branches", m_recipe.branches );
        p->getPropertyValue( "Bark Colour", m_recipe.bark.tint );
        p->getPropertyValue( "Leaf Colour", m_recipe.leaves.tint );
        p->getPropertyValue( "LOD", m_lod );
        m_lod = std::min( m_lod, 2u );
        p->getPropertyValue( "Generated Material IDs", m_generatedMaterialIds );
    }

    void ProceduralFoliage::unload( SmartPtr<ISharedObject> data )
    {
        auto actor = getActor();
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        auto manager = graphics ? graphics->getMaterialManager() : nullptr;
        for( size_t index = 0; index < m_generatedMaterials.size(); ++index )
        {
            auto component = m_generatedMaterials[index];
            if( component && index < m_materialResources.size() &&
                component->getMaterial() == m_materialResources[index] )
            {
                component->unload( nullptr );
                if( actor )
                {
                    actor->removeComponentInstance( component );
                }
            }
        }
        m_generatedMaterials.clear();
        m_generatedMaterialIds.clear();
        for( auto material : m_materialResources )
        {
            if( manager && material )
            {
                manager->destroyResource( material );
            }
        }
        m_materialResources.clear();
        if( graphics && m_leafTexture )
        {
            graphics->getTextureManager()->destroyResource( m_leafTexture );
        }
        m_leafTexture = nullptr;
        ProceduralMesh::unload( data );
    }
}  // namespace workphone::scene
