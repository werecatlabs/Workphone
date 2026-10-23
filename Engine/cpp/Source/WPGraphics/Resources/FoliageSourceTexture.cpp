#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/FoliageSourceTexture.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_renderer_dx11.h>

namespace workphone::render
{
    namespace
    {
        struct SourceTexture
        {
            StreamBudget budget;
            CookedTextureData data;
        };

        std::shared_ptr<SourceTexture> read( const String &root, const String &id, String &error )
        {
            resource::ResourceID resource;
            if( root.empty() || id.substr( 0, 7 ) != "data://" || !resource.set( id ) ||
                resource.isSubResource() || resource.type() != resource::ResourceTypeID( "texres" ) )
            {
                error =
                    "Foliage source textures require a project root and canonical data://*.texres ID.";
                return {};
            }
            auto result = std::make_shared<SourceTexture>();
            if( !decodeGraphicsTextureSource( root, resource, result->data, error, 32ull << 20,
                                              &result->budget ) )
            {
                return {};
            }
            return result;
        }

        struct TextureBudgets
        {
            StreamBudget gpu, cpu;

            TextureBudgets( StreamBudget g, StreamBudget c ) :
                gpu( std::move( g ) ),
                cpu( std::move( c ) )
            {
            }
        };

        // Base destruction order releases ClawTexture's GPU/CPU allocation first.
        class SourceClawTexture final : private TextureBudgets, public ClawTexture
        {
        public:
            SourceClawTexture( StreamBudget g, StreamBudget c ) :
                TextureBudgets( std::move( g ), std::move( c ) )
            {
            }
        };
    }  // namespace

    std::shared_ptr<const TextureMipLevel> readFoliageSourceTexture( const String &root,
                                                                     const String &id, String &error )
    {
        try
        {
            auto source = read( root, id, error );
            if( !source )
            {
                return {};
            }
            if( source->data.levels.empty() )
            {
                error = "Foliage source texture has no portable CPU base mip.";
                return {};
            }
            return { source, &source->data.levels.front() };
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return {};
        }
    }

    SmartPtr<ITexture> loadFoliageSourceTexture( const String &root, const String &id,
                                                 wp_renderer_dx11 *renderer, String &error )
    {
        try
        {
            if( !renderer )
            {
                error = "Foliage source texture upload requires a DX11 render owner.";
                return {};
            }
            auto source = read( root, id, error );
            if( !source )
            {
                return {};
            }
            // The decoder and ClawTexture separately own the source and retained
            // reupload copy. ClawTexture also budgets the physical GPU resource.
            auto texture = make_ptr<ClawTexture>();
            if( !texture->uploadCookedMips( source->data.levels, source->data.mipSettings, renderer,
                                            error ) )
            {
                return {};
            }
            texture->setName( id );
            return texture;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return {};
        }
    }
}  // namespace workphone::render
