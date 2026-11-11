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

        // Compact cutout sprites need no project-specific textures. White shades
        // retain the inspector's leaf tint; transparent texels preserve silhouette gaps.
        Array<u8> cutoutPixels( ProceduralFoliage::Preset preset )
        {
            constexpr u32 size = 128;
            auto pixels = makeSourceBudgetSnapshot<u8>();
            pixels.resize( size * size * 4, 0 );
            const bool grass = isGrass( preset );
            const bool pine = preset == ProceduralFoliage::Preset::Pine;
            const bool palm = preset == ProceduralFoliage::Preset::Palm;
            for( u32 y = 0; y < size; ++y )
            {
                for( u32 x = 0; x < size; ++x )
                {
                    const float u = ( x + .5f ) / size, v = ( y + .5f ) / size;
                    bool covered = false;
                    if( grass )
                    {
                        for( int blade = 0; blade < 7; ++blade )
                        {
                            const float top = .04f + .055f * ( ( blade * 3 ) % 5 );
                            const float t = std::clamp( ( 1 - v ) / ( 1 - top ), 0.f, 1.f );
                            const float base = .20f + .10f * blade;
                            const float lean = ( blade - 3 ) * .045f;
                            const float centre = base + lean * t * t;
                            covered |= v >= top && v < .99f &&
                                       std::abs( u - centre ) < .028f * ( 1 - t ) + .0015f;
                        }
                    }
                    else if( palm )
                    {
                        // One feathered frond, mapped continuously over its curved
                        // strip: base at v=1, tip at v=0. Gaps separate the leaflets.
                        covered = v > .025f && v < .99f && std::abs( u - .5f ) < .012f;
                        for( int leaf = 0; leaf < 14; ++leaf )
                        {
                            const float stem = .10f + leaf * .058f;
                            const float reach = .12f + .35f * std::sin( 3.14159265f * stem );
                            const float dx = std::abs( u - .5f );
                            const float dy = v - ( stem - dx * .22f );
                            covered |= dx < reach &&
                                       std::abs( dy ) < .016f * ( 1 - dx / reach ) + .002f;
                        }
                    }
                    else
                    {
                        covered = v > .10f && v < .96f && std::abs( u - .5f ) < .009f;
                        const int pairs = pine ? 12 : 5;
                        for( int leaf = 0; leaf < pairs; ++leaf )
                        {
                            const float stem = .20f + leaf * ( .65f / pairs );
                            for( int side : { -1, 1 } )
                            {
                                const float dx = ( u - .5f ) * side;
                                const float reach = pine ? .31f : .24f;
                                const float dy = v - ( stem - dx * .6f );
                                if( pine )
                                {
                                    covered |= dx > .015f && dx < reach &&
                                               std::abs( dy ) < .007f * ( 1 - dx / reach ) + .002f;
                                }
                                else
                                {
                                    const float a = ( dx - .14f ) / .15f;
                                    const float b = dy / .050f;
                                    covered |= a * a + b * b < 1;
                                }
                            }
                        }
                        // A terminal leaf closes the tip of the broadleaf spray.
                        if( !pine )
                        {
                            const float dx = ( u - .5f ) / .06f;
                            const float dy = ( v - .12f ) / .10f;
                            covered |= dx * dx + dy * dy < 1;
                        }
                    }
                    const auto index = ( size_t( y ) * size + x ) * 4;
                    const auto shade = u8( 205 + 45 * ( 1 - v ) );
                    pixels[index] = pixels[index + 1] = pixels[index + 2] = shade;
                    pixels[index + 3] = covered ? 255 : 0;
                }
            }
            return pixels;
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
                if( !texture->copyByteData( cutoutPixels( preset ), Vector2I( 128, 128 ), error ) )
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
        m_recipe = {};
        m_recipe.seed = 7919;
        m_recipe.bark.tint = { .27f, .15f, .065f, 1 };
        m_recipe.leaves.tint = { .25f, .48f, .12f, 1 };
        switch( m_preset )
        {
        case Preset::Oak:
            m_recipe.height = 6;
            m_recipe.radius = 2.3f;
            m_recipe.trunkRadius = .20f;
            m_recipe.branches = 28;
            break;
        case Preset::Birch:
            m_recipe.height = 7;
            m_recipe.radius = 1.35f;
            m_recipe.trunkRadius = .10f;
            m_recipe.branches = 20;
            m_recipe.bark.tint = { .80f, .78f, .70f, 1 };
            m_recipe.leaves.tint = { .40f, .58f, .16f, 1 };
            break;
        case Preset::Pine:
            m_recipe.height = 8;
            m_recipe.radius = 2.3f;
            m_recipe.trunkRadius = .16f;
            m_recipe.branches = 32;
            m_recipe.leaves.tint = { .12f, .31f, .16f, 1 };
            break;
        case Preset::Bush:
            m_recipe.kind = render::FoliageKind::Bush;
            m_recipe.height = 1.3f;
            m_recipe.radius = 1.0f;
            m_recipe.trunkRadius = .035f;
            m_recipe.branches = 16;
            break;
        case Preset::Palm:
            m_recipe.height = 7;
            m_recipe.radius = 3;
            m_recipe.trunkRadius = .18f;
            m_recipe.branches = 12;
            m_recipe.bark.tint = { .46f, .34f, .20f, 1 };
            m_recipe.leaves.tint = { .22f, .43f, .10f, 1 };
            break;
        case Preset::Grass:
        case Preset::TallGrass:
            m_recipe.kind = render::FoliageKind::Grass;
            m_recipe.height = m_preset == Preset::Grass ? .45f : 1.1f;
            m_recipe.radius = m_preset == Preset::Grass ? .40f : .55f;
            m_recipe.trunkRadius = .01f;
            m_recipe.branches = 16;
            m_recipe.leaves.tint = { .36f, .52f, .16f, 1 };
            break;
        }
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
        const auto shape = m_preset == Preset::Palm ? FoliagePlantShape::Palm :
                                                    FoliagePlantShape::Broadleaf;
        if( !generateFoliagePlant( m_recipe, shape, species, error ) )
        {
            throw std::runtime_error( error );
        }
        auto &source = species.lods[std::min( m_lod, 2u )];
        if( m_preset == Preset::Pine )
        {
            // Same conifer taper used by the procedural racing scene. Transform
            // normals by the inverse transpose so crown lighting follows the shape.
            for( size_t i = 0; i < source.positions.size(); ++i )
            {
                auto &p = source.positions[i];
                auto &n = source.normals[i];
                const float k = .68f / m_recipe.height;
                const float width = .96f - .68f * std::clamp( p.y / m_recipe.height, 0.f, 1.f );
                n = { n.x / width, n.y + k * ( p.x * n.x + p.z * n.z ) / width, n.z / width };
                n.normalise();
                p.x *= width;
                p.z *= width;
            }
        }
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
        procedural_properties::setEnum( p, "Preset", s32( m_preset ),
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
            p, "Preset", s32( m_preset ), { "Oak", "Birch", "Pine", "Bush", "Grass", "Tall Grass", "Palm" } );
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
