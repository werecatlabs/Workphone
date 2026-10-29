#include "WPGraphics/WPClawHammerPCH.hpp"
#include "MaterialShaderEditorDX11.hpp"
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Memory/ObjectObserver.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <workphone_graphics_renderer.h>
#include <mutex>
#include <unordered_map>

namespace workphone::render
{
    namespace
    {
        struct Entry
        {
            ObjectObserver<ClawMaterial> material;
            SourceBudgetString vertex, pixel, error, shaderName;
            u64 version = 0, processed = 0, id = 0;
            bool watching = true;
            MaterialProgramReloadDX11::Ptr candidate;
        };

        std::recursive_mutex mutex;
        using Pair = std::pair<const ClawMaterial *const, std::shared_ptr<Entry>>;
        std::unordered_map<const ClawMaterial *, std::shared_ptr<Entry>, std::hash<const ClawMaterial *>,
                           std::equal_to<const ClawMaterial *>, SourceBudgetAllocator<Pair>>
            entries;

        std::shared_ptr<Entry> entry( ClawMaterial *material )
        {
            auto found = entries.find( material );
            if( found != entries.end() && !found->second->material.expired() )
            {
                return found->second;
            }
            auto value = std::allocate_shared<Entry>( SourceBudgetAllocator<Entry>{} );
            value->material = material;
            entries[material] = value;
            return value;
        }

        void fileSettings( const Entry &value, SourceBudgetString &vertex, SourceBudgetString &pixel,
                           bool &watching )
        {
            vertex = value.vertex;
            pixel = value.pixel;
            watching = value.watching;
            if( value.version != value.processed || !value.id )
            {
                return;
            }
            const auto status = MaterialProgramReloadDX11::EditorStatus( value.id );
            if( !status->getPropertyAsBool( "available" ) )
            {
                return;
            }
            vertex = status->getProperty( "vertexFile" );
            pixel = status->getProperty( "pixelFile" );
            watching = status->getPropertyAsBool( "watching" );
        }

        struct ParameterKind
        {
            MaterialShaderParameterType type;
            const char *name;
        };

        constexpr ParameterKind parameterKinds[] = {
            { MaterialShaderParameterType::Scalar, "float" },
            { MaterialShaderParameterType::Colour, "colour" },
            { MaterialShaderParameterType::Vector2, "float2" },
            { MaterialShaderParameterType::Vector4, "float4" },
            { MaterialShaderParameterType::Matrix, "matrix4" },
            { MaterialShaderParameterType::Int, "int" },
            { MaterialShaderParameterType::UInt, "uint" },
            { MaterialShaderParameterType::Bool, "bool" },
            { MaterialShaderParameterType::IntVector2, "int2" },
            { MaterialShaderParameterType::IntVector4, "int4" },
            { MaterialShaderParameterType::Texture, "texture" },
            { MaterialShaderParameterType::Buffer, "buffer" },
            { MaterialShaderParameterType::Sampler, "sampler" }
        };
    }  // namespace

    namespace detail
    {
        void prepareShaderReloadStatus( ClawMaterial *material )
        {
            SourceBudgetString name = material->getShaderID().c_str();
            std::lock_guard<std::recursive_mutex> lock( mutex );
            auto value = entry( material );
            value->shaderName = std::move( name );
        }

        void shaderReloadAttached( ClawMaterial *material, u64 id )
        {
            std::lock_guard<std::recursive_mutex> lock( mutex );
            if( !id && entries.find( material ) == entries.end() )
            {
                return;
            }
            auto value = entry( material );
            value->id = id;
            value->processed = value->version;
        }

        void forgetShaderReload( ClawMaterial *material ) noexcept
        {
            try
            {
                std::lock_guard<std::recursive_mutex> lock( mutex );
                entries.erase( material );
            }
            catch( ... )
            {
            }
        }

        void copyShaderFiles( const ClawMaterial *source, ClawMaterial *target )
        {
            std::lock_guard<std::recursive_mutex> lock( mutex );
            const auto found = entries.find( source );
            if( found == entries.end() )
            {
                return;
            }
            auto value = std::allocate_shared<Entry>( SourceBudgetAllocator<Entry>{} );
            fileSettings( *found->second, value->vertex, value->pixel, value->watching );
            if( value->pixel.empty() )
            {
                return;
            }
            value->material = target;
            value->shaderName = found->second->shaderName;
            entries[target] = std::move( value );
        }

        bool restoreShaderSchema( ClawMaterial *material, SmartPtr<Properties> properties )
        {
            const auto schema = properties->getChild( "runtimeShaderSchema" );
            if( !schema || properties->getProperty( "runtimeShaderPixelFile" ).empty() )
            {
                return false;
            }
            u32 version = 0;
            schema->getPropertyValue( "version", version );
            const auto fields = schema->getChildren();
            if( version != 1 || fields.size() > 64 )
            {
                throw std::invalid_argument( "Invalid saved runtime shader schema" );
            }
            Array<MaterialShaderParameterInfo> info;
            for( const auto &field : fields )
            {
                if( !field )
                {
                    throw std::invalid_argument( "Missing runtime shader parameter descriptor" );
                }
                const auto name = field->getProperty( "name" ), kind = field->getProperty( "type" );
                if( name.empty() || name.size() > 255 || name.find( '\0' ) != String::npos )
                {
                    throw std::invalid_argument( "Invalid saved runtime shader parameter name" );
                }
                bool found = false;
                for( const auto &known : parameterKinds )
                {
                    if( kind == known.name )
                    {
                        info.push_back( { known.type, name } );
                        found = true;
                        break;
                    }
                }
                if( !found )
                {
                    throw std::invalid_argument( "Unknown saved runtime shader parameter type" );
                }
            }
            if( !MaterialShaderParameterLayout::Create( info ) )
            {
                throw std::invalid_argument( "Invalid saved runtime shader layout" );
            }
            if( properties->hasProperty( Material::shaderIDStr ) )
            {
                material->setShaderID( properties->getProperty( Material::shaderIDStr ) );
            }
            material->setShaderParameters( info );
            return true;
        }

        void appendShaderFiles( const ClawMaterial *material, SmartPtr<Properties> properties )
        {
            std::lock_guard<std::recursive_mutex> lock( mutex );
            auto found = entries.find( material );
            if( found == entries.end() )
            {
                return;
            }
            if( found->second->shaderName != material->getShaderID().c_str() )
            {
                properties->setProperty( "runtimeShaderVertexFile", String() );
                properties->setProperty( "runtimeShaderPixelFile", String() );
                properties->setProperty( "runtimeShaderWatching", false );
                return;
            }
            SourceBudgetString vertex, pixel;
            bool watching = false;
            fileSettings( *found->second, vertex, pixel, watching );
            properties->setProperty( "runtimeShaderVertexFile", static_cast<String>( vertex ) );
            properties->setProperty( "runtimeShaderPixelFile", static_cast<String>( pixel ) );
            properties->setProperty( "runtimeShaderWatching", watching );
            if( !pixel.empty() && material->getShaderParameters().GetLayout() )
            {
                auto schema = make_ptr<Properties>();
                schema->setName( "runtimeShaderSchema" );
                schema->setProperty( "version", u32( 1 ) );
                for( const auto &parameter : material->getShaderParameters().GetParameterInfo() )
                {
                    auto field = make_ptr<Properties>();
                    field->setName( "parameter" );
                    field->setProperty( "name", parameter.m_name );
                    for( const auto &kind : parameterKinds )
                    {
                        if( kind.type == parameter.m_type )
                        {
                            field->setProperty( "type", String( kind.name ) );
                            break;
                        }
                    }
                    schema->addChild( field );
                }
                properties->addChild( schema );
            }
        }
    }  // namespace detail

    bool ClawMaterial::requestRuntimeShaderFiles( const String &vertex, const String &pixel )
    {
        if( pixel.empty() )
        {
            return false;
        }
        SourceBudgetString v( vertex ), p( pixel );
        std::lock_guard<std::recursive_mutex> lock( mutex );
        auto value = entry( this );
        value->shaderName = getShaderID().c_str();
        value->vertex = std::move( v );
        value->pixel = std::move( p );
        ++value->version;
        value->error.clear();
        return true;
    }

    bool ClawMaterial::requestRuntimeShaderReload()
    {
        std::lock_guard<std::recursive_mutex> lock( mutex );
        auto found = entries.find( this );
        if( found == entries.end() )
        {
            return false;
        }
        auto &value = *found->second;
        if( value.id && MaterialProgramReloadDX11::QueueReload( value.id ) )
        {
            return true;
        }
        if( value.pixel.empty() )
        {
            return false;
        }
        ++value.version;
        value.error.clear();
        return true;
    }

    bool ClawMaterial::setRuntimeShaderWatching( bool enabled )
    {
        std::lock_guard<std::recursive_mutex> lock( mutex );
        auto found = entries.find( this );
        if( found == entries.end() )
        {
            return false;
        }
        auto &value = *found->second;
        if( value.pixel.empty() && !value.id )
        {
            return false;
        }
        value.watching = enabled;
        if( value.id )
        {
            MaterialProgramReloadDX11::QueueWatching( value.id, enabled );
        }
        return true;
    }

    SmartPtr<Properties> ClawMaterial::getRuntimeShaderReloadStatus() const
    {
        u64 id = 0;
        bool pending = false, configured = false, watching = false;
        String vertex, pixel, error;
        {
            std::lock_guard<std::recursive_mutex> lock( mutex );
            auto found = entries.find( this );
            if( found != entries.end() )
            {
                const auto &value = *found->second;
                id = value.id;
                pending = value.version != value.processed;
                vertex = static_cast<String>( value.vertex );
                pixel = static_cast<String>( value.pixel );
                error = static_cast<String>( value.error );
                watching = value.watching;
                configured = !value.pixel.empty();
            }
        }
        auto status = MaterialProgramReloadDX11::EditorStatus( id );
        bool available = false;
        status->getPropertyValue( "available", available );
        if( configured || pending )
        {
            status->setProperty( "available", true );
            if( !available || pending )
            {
                status->setProperty( "vertexFile", vertex );
                status->setProperty( "pixelFile", pixel );
            }
            // File coordinators may be shared by several materials. Once an
            // owner exists, report its applied watch state to every subscriber.
            if( !available || pending )
            {
                status->setProperty( "watching", watching );
            }
        }
        if( pending )
        {
            status->setProperty( "pending", true );
            status->setProperty( "status", String( "queued" ) );
        }
        if( !error.empty() )
        {
            status->setProperty( "diagnostic", error );
            status->setProperty( "success", false );
            status->setProperty( "status", String( "failed" ) );
        }
        if( !available && !configured )
        {
            status->setProperty( "diagnostic",
                                 String( "No reloadable shader is configured for this material." ) );
        }
        return status;
    }

    void ClawMaterial::processRuntimeShaderReloads( ClawRendererDX11 &renderer )
    {
        auto native = renderer.getNativeRenderer();
        auto dx = native ? wp_renderer_get_dx11( native ) : nullptr;
        if( !dx )
        {
            return;
        }
        {
            std::lock_guard<std::recursive_mutex> lock( mutex );
            for( auto it = entries.begin(); it != entries.end(); )
            {
                auto current = it++;
                if( auto material = current->second->material.lock() )
                {
                    material->refreshRuntimeShaderSelection();
                }
                else
                {
                    entries.erase( current );
                }
            }
        }
        for( ;; )
        {
            std::shared_ptr<Entry> work;
            u64 version = 0;
            String vertex, pixel;
            bool build = false;
            {
                std::lock_guard<std::recursive_mutex> lock( mutex );
                for( auto it = entries.begin(); it != entries.end(); )
                {
                    if( it->second->material.expired() )
                    {
                        it = entries.erase( it );
                        continue;
                    }
                    auto &value = *it->second;
                    if( value.version != value.processed ||
                        ( value.candidate && value.candidate->HasProgram() ) )
                    {
                        work = it->second;
                        version = value.version;
                        build = value.version != value.processed;
                        vertex = static_cast<String>( value.vertex );
                        pixel = static_cast<String>( value.pixel );
                        break;
                    }
                    ++it;
                }
            }
            if( !work )
            {
                break;
            }
            auto material = work->material.lock();
            if( !material )
            {
                continue;
            }
            try
            {
                String error;
                auto candidate =
                    build ? MaterialProgramReloadDX11::CreateFromFiles(
                                wp_renderer_dx11_get_device( dx ),
                                material->getShaderParameters().GetLayout(), vertex, pixel, 1, error )
                          : work->candidate;
                std::lock_guard<std::recursive_mutex> lock( mutex );
                if( work->version != version )
                {
                    continue;
                }
                work->processed = version;
                if( !candidate )
                {
                    work->error = error;
                    work->candidate.reset();
                    continue;
                }
                work->id = candidate->GetId();
                work->candidate = candidate;
                MaterialProgramReloadDX11::QueueWatching( work->id, work->watching );
                if( candidate->HasProgram() )
                {
                    // Candidate preparation precedes this owner-thread installation.
                    material->setRuntimeReload( candidate, material->runtimeShaderResources() );
                    work->candidate.reset();
                    work->error.clear();
                }
            }
            catch( const std::exception &error )
            {
                std::lock_guard<std::recursive_mutex> lock( mutex );
                if( work->version == version )
                {
                    work->processed = version;
                    work->error = String( error.what() );
                    work->candidate.reset();
                }
            }
        }
    }
}  // namespace workphone::render
