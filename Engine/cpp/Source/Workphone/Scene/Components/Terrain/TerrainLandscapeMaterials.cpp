#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLandscapeMaterials.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/GeneratedTextureSource.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::scene
{
    bool createLandscapeMaterials( TerrainSystem *terrain, SmartPtr<ICommandManager> commands,
                                     String &error )
    {
        try
        {
            auto app = core::IApplicationManager::instancePtr();
            auto factories = app ? app->getFactoryManager() : nullptr;
            auto forge = factories ? factories->createObjectFromType<procedural::ITextureForge>(
                                         "ITextureForge" ) : nullptr;
            if( !terrain || !forge )
            {
                error = "Select a terrain and load the WPProcedural plugin to create landscape textures.";
                return false;
            }
            const auto source = terrain->getTerrainSnapshot();
            if( !source )
            {
                error = "Terrain samples are unavailable.";
                return false;
            }
            auto layers = source->layers;
            const procedural::SurfaceTag surfaces[] = { procedural::SurfaceTag::Sand,
                procedural::SurfaceTag::Foliage, procedural::SurfaceTag::Stone,
                procedural::SurfaceTag::Dirt };
            const char *names[] = { "sand", "grass", "rock", "soil" };
            procedural::SurfaceBakeParams params;
            params.size = 256;
            for( u32 index = 0; index < 4; ++index )
            {
                forge->setSeed( 4200 + index );
                const auto surface = forge->bakeSurface( surfaces[index], params );
                auto &layer = layers[index];
                const String directory = "Assets/Landscape/Materials";
                const String name = names[index];
                if( !render::saveGeneratedTextureSource( surface.albedo, app->getProjectPath(),
                        directory, name + "-albedo", render::TextureMipFilter::Colour,
                        layer.albedo, error ) ||
                    !render::saveGeneratedTextureSource( surface.normal, app->getProjectPath(),
                        directory, name + "-normal", render::TextureMipFilter::Normal,
                        layer.normal, error ) ||
                    !render::saveGeneratedTextureSource( surface.orm, app->getProjectPath(),
                        directory, name + "-orm", render::TextureMipFilter::Data,
                        layer.orm, error ) )
                    return false;
                layer.tint = { 1, 1, 1, 1 };
                layer.uvScale = { .25f, .25f };
                layer.roughness = 1;
                layer.metallic = 0;
                layer.normalStrength = .65f;
            }
            return applyTerrainLayers( terrain, layers, commands, error );
        }
        catch( const std::exception &exception )
        {
            error = String( "Cannot create landscape materials: " ) + exception.what();
            return false;
        }
    }
}
