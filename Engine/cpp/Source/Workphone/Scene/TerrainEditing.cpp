#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/TerrainEditing.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Interface/System/ICommandManager.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#ifdef _WIN32
#    include <Windows.h>
#endif

namespace workphone::scene
{
    class TerrainHeightEdit final : public ICommand
    {
    public:
        AtomicWeakPtr<TerrainSystem> terrain;
        Vector2I dimensions;
        Vector2F spacing, origin;
        f32 heightScale = 1;
        Array<u32> indices;
        Array<f32> before, after;
        bool succeeded = false;
        String error;
        State state = State::Allocated;

        void apply( bool forward )
        {
            succeeded = false;
            error.clear();
            auto owner = terrain.load().lock();
            if( !owner )
            {
                error = "Terrain edit owner is no longer available.";
                return;
            }
            auto snapshot = owner->getTerrainSnapshot();
            if( !snapshot || snapshot->dimensions != dimensions || snapshot->spacing != spacing ||
                snapshot->origin != origin || snapshot->heightScale != heightScale )
            {
                error = "Terrain grid changed since this edit; undo/redo was rejected.";
                return;
            }
            const auto &expected = forward ? before : after;
            const auto &replacement = forward ? after : before;
            for( size_t i = 0; i < indices.size(); ++i )
                if( indices[i] >= snapshot->heights.size() ||
                    snapshot->heights[indices[i]] != expected[i] )
                {
                    error = "Terrain edit conflicts with newer height values.";
                    return;
                }
            auto candidate = *snapshot;
            for( size_t i = 0; i < indices.size(); ++i )
                candidate.heights[indices[i]] = replacement[i];
            succeeded = owner->applyTerrainData( candidate, error, snapshot->revision );
        }
        void execute() override
        {
            if( state == State::Finished )
                return;
            state = State::Executing;
            apply( true );
            state = State::Finished;
        }
        void undo() override
        {
            apply( false );
            if( !succeeded )
                WP_LOG_WARNING( error );
        }
        void redo() override
        {
            apply( true );
            if( !succeeded )
                WP_LOG_WARNING( error );
        }
        State getState() const override
        {
            return state;
        }
        void setState( State value ) override
        {
            state = value;
        }
        bool isPrimary() const override
        {
            return true;
        }
        void setPrimary( bool ) override
        {
        }
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainHeightEdit, ICommand );

    bool applyTerrainBrush( SmartPtr<TerrainSystem> terrain, const TerrainBrush &brush,
                            SmartPtr<ICommandManager> commands, String &error )
    {
        error.clear();
        try
        {
            if( !terrain || !commands )
            {
                error = "Terrain editing requires a selected terrain and command manager.";
                return false;
            }
            const bool blended =
                brush.mode == TerrainBrushMode::Smooth || brush.mode == TerrainBrushMode::Flatten;
            if( !std::isfinite( brush.centre.x ) || !std::isfinite( brush.centre.y ) ||
                !std::isfinite( brush.radius ) || brush.radius <= 0 ||
                !std::isfinite( brush.strength ) || brush.strength < 0 ||
                ( blended && brush.strength > 1 ) || !std::isfinite( brush.targetHeight ) ||
                static_cast<unsigned>( brush.mode ) >
                    static_cast<unsigned>( TerrainBrushMode::Flatten ) )
            {
                error =
                    "Brush parameters must be finite, radius positive and strength within its supported "
                    "range.";
                return false;
            }
            auto snapshot = terrain->getTerrainSnapshot();
            if( !snapshot || !render::validateTerrainData( *snapshot, error ) )
                return false;
            if( snapshot->heightScale == 0 )
            {
                error = "Height editing requires a nonzero height scale.";
                return false;
            }
            auto command = make_ptr<TerrainHeightEdit>();
            command->terrain = terrain;
            command->dimensions = snapshot->dimensions;
            command->spacing = snapshot->spacing;
            command->origin = snapshot->origin;
            command->heightScale = snapshot->heightScale;
            const auto width = snapshot->dimensions.x;
            const auto depth = snapshot->dimensions.y;
            for( s32 z = 0; z < depth; ++z )
                for( s32 x = 0; x < width; ++x )
                {
                    const double dx = double( snapshot->origin.x ) + double( x ) * snapshot->spacing.x -
                                      brush.centre.x;
                    const double dz = double( snapshot->origin.y ) + double( z ) * snapshot->spacing.y -
                                      brush.centre.y;
                    const double distance = std::sqrt( dx * dx + dz * dz );
                    if( distance >= brush.radius )
                        continue;
                    const auto index = static_cast<u32>( z * width + x );
                    const double old = snapshot->heights[index];
                    const double t = 1 - distance / brush.radius;
                    const double weight = t * t * ( 3 - 2 * t );
                    double value = old;
                    if( brush.mode == TerrainBrushMode::Raise || brush.mode == TerrainBrushMode::Lower )
                        value += ( brush.mode == TerrainBrushMode::Raise ? 1 : -1 ) *
                                 double( brush.strength ) * weight / snapshot->heightScale;
                    else if( brush.mode == TerrainBrushMode::Flatten )
                        value += ( double( brush.targetHeight ) / snapshot->heightScale - old ) *
                                 brush.strength * weight;
                    else
                    {
                        double total = 0;
                        u32 count = 0;
                        for( s32 nz = std::max( 0, z - 1 ); nz <= std::min( depth - 1, z + 1 ); ++nz )
                            for( s32 nx = std::max( 0, x - 1 ); nx <= std::min( width - 1, x + 1 );
                                 ++nx )
                            {
                                total += snapshot->heights[static_cast<size_t>( nz ) * width + nx];
                                ++count;
                            }
                        value += ( total / count - old ) * brush.strength * weight;
                    }
                    const auto replacement = static_cast<f32>( value );
                    if( !std::isfinite( replacement ) ||
                        !std::isfinite( replacement * snapshot->heightScale ) )
                    {
                        error = "Brush output exceeds finite terrain height limits.";
                        return false;
                    }
                    if( replacement == old )
                        continue;
                    if( command->indices.size() >= terrainMaximumEditSamples )
                    {
                        error = "Brush affects more than 65536 samples; reduce its radius.";
                        return false;
                    }
                    command->indices.push_back( index );
                    command->before.push_back( static_cast<f32>( old ) );
                    command->after.push_back( replacement );
                }
            if( command->indices.empty() )
            {
                error = "Brush does not change any terrain samples.";
                return false;
            }
            // The editor's MT manager queues execution. Complete the owner-thread edit
            // first, then register an idempotent command for the existing history UI.
            command->execute();
            if( !command->succeeded )
            {
                error = command->error.empty() ? "Terrain command was not executed synchronously."
                                               : command->error;
                return false;
            }
            commands->addCommand( command );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool saveTerrainDataFile( SmartPtr<TerrainSystem> terrain, const String &path, String &error )
    {
        error.clear();
        std::filesystem::path temporary;
        try
        {
            if( !terrain || path.empty() || path.find( '\0' ) != String::npos )
            {
                error = "Saving terrain requires a selected terrain and valid file path.";
                return false;
            }
            const auto bytes = terrain->exportTerrainData();
            if( bytes.empty() )
            {
                error = "Terrain has no serializable height data.";
                return false;
            }
            const auto destination = std::filesystem::u8path( path.c_str() );
            temporary = destination;
            temporary += ".tmp-" + std::string( StringUtil::getUUID().c_str() );
            {
                std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                stream.write( bytes.data(), static_cast<std::streamsize>( bytes.size() ) );
                stream.flush();
                if( !stream )
                    throw std::runtime_error( "Failed to write terrain data." );
                stream.close();
                if( !stream )
                    throw std::runtime_error( "Failed to close terrain data." );
            }
#ifdef _WIN32
            if( !MoveFileExW( temporary.c_str(), destination.c_str(),
                              MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                throw std::runtime_error( "Failed to replace terrain data file." );
#else
            std::filesystem::rename( temporary, destination );
#endif
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            if( !temporary.empty() )
            {
                std::error_code ignored;
                std::filesystem::remove( temporary, ignored );
            }
            return false;
        }
    }

    bool loadTerrainDataFile( SmartPtr<TerrainSystem> terrain, const String &path, String &error )
    {
        error.clear();
        try
        {
            if( !terrain || path.empty() || path.find( '\0' ) != String::npos )
            {
                error = "Loading terrain requires a selected terrain and valid file path.";
                return false;
            }
            std::ifstream stream( std::filesystem::u8path( path.c_str() ),
                                  std::ios::binary | std::ios::ate );
            const auto size = stream ? stream.tellg() : std::streampos( -1 );
            if( size <= 0 || size > std::streamoff( 128 * 1024 * 1024 ) )
            {
                error = "Terrain file is missing, empty or exceeds 128 MiB.";
                return false;
            }
            String bytes( static_cast<size_t>( size ), '\0' );
            stream.seekg( 0 );
            stream.read( bytes.data(), static_cast<std::streamsize>( bytes.size() ) );
            if( !stream )
            {
                error = "Failed to read complete terrain data.";
                return false;
            }
            return terrain->importTerrainData( bytes, error );
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
}  // namespace workphone::scene
