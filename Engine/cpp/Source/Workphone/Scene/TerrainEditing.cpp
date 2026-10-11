#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/TerrainEditing.hpp>
#include <Workphone/Scene/TerrainRecipes.hpp>
#include <Workphone/Scene/TerrainMasks.hpp>
#include <cJSON.h>
#include <future>
#include <chrono>
#include <cctype>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
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
#include <map>
#include <optional>
#include <sstream>
#include <iomanip>
#include <cstring>
#ifdef _WIN32
#    include <Windows.h>
#endif

namespace workphone::scene
{
    namespace
    {
        Vector4F weightsAt( const render::TerrainData &data, u32 index )
        {
            return data.layerWeights.empty() ? Vector4F( 1, 0, 0, 0 ) : data.layerWeights[index];
        }
        Vector2F masksAt( const render::TerrainData &data, u32 index )
        {
            return data.surfaceMasks.empty() ? Vector2F( 1, 1 ) : data.surfaceMasks[index];
        }
        u8 holeAt( const render::TerrainData &data, u32 index )
        {
            return data.holes.empty() ? 0 : data.holes[index];
        }
        bool sameGrid( const render::TerrainData &a, const render::TerrainData &b )
        {
            return a.dimensions == b.dimensions && a.spacing == b.spacing && a.origin == b.origin &&
                   a.heightScale == b.heightScale;
        }
        double brushWeight( const render::TerrainData &data, const TerrainBrush &brush, s32 x, s32 z,
                            bool cell = false )
        {
            const double dx =
                data.origin.x + ( double( x ) + ( cell ? 0.5 : 0 ) ) * data.spacing.x - brush.centre.x;
            const double dz =
                data.origin.y + ( double( z ) + ( cell ? 0.5 : 0 ) ) * data.spacing.y - brush.centre.y;
            const double t = std::max( 0.0, 1.0 - std::hypot( dx, dz ) / brush.radius );
            return t * t * ( 3 - 2 * t );
        }
        u32 noiseHash( u32 x )
        {
            x ^= x >> 16;
            x *= 0x7feb352du;
            x ^= x >> 15;
            x *= 0x846ca68bu;
            return x ^ ( x >> 16 );
        }
        bool validBrush( const TerrainBrush &brush, String &error )
        {
            const bool blended =
                brush.mode == TerrainBrushMode::Smooth || brush.mode == TerrainBrushMode::Flatten ||
                brush.mode == TerrainBrushMode::Paint || brush.mode == TerrainBrushMode::ClearPaint ||
                brush.mode == TerrainBrushMode::Terrace || brush.mode >= TerrainBrushMode::PaintWetness;
            if( !std::isfinite( brush.centre.x ) || !std::isfinite( brush.centre.y ) ||
                !std::isfinite( brush.radius ) || brush.radius <= 0 ||
                !std::isfinite( brush.strength ) || brush.strength < 0 ||
                ( blended && brush.strength > 1 ) || !std::isfinite( brush.targetHeight ) ||
                !std::isfinite( brush.terraceStep ) || brush.terraceStep <= 0 || brush.layer >= 4 ||
                !std::isfinite( brush.minHeight ) || !std::isfinite( brush.maxHeight ) ||
                brush.minHeight > brush.maxHeight || !std::isfinite( brush.minSlope ) ||
                !std::isfinite( brush.maxSlope ) || brush.minSlope < 0 || brush.maxSlope > 90 ||
                brush.minSlope > brush.maxSlope ||
                unsigned( brush.mode ) > unsigned( TerrainBrushMode::ClearSnow ) )
            {
                error =
                    "Brush requires finite supported parameters, positive radius, valid layer and "
                    "bounded strength.";
                return false;
            }
            if( !brush.stamp.empty() )
            {
                if( brush.stampDimensions.x < 2 || brush.stampDimensions.y < 2 ||
                    u64( brush.stampDimensions.x ) * brush.stampDimensions.y != brush.stamp.size() ||
                    brush.stamp.size() > terrainMaximumEditSamples )
                {
                    error = "Stamp requires a matching bounded rectangular image.";
                    return false;
                }
                for( const auto value : brush.stamp )
                    if( !std::isfinite( value ) || value < 0 || value > 1 )
                    {
                        error = "Stamp samples must be normalized finite values.";
                        return false;
                    }
            }
            return true;
        }
        bool brushCandidate( const render::TerrainData &source, const TerrainBrush &brush,
                             render::TerrainData &candidate, String &error )
        {
            if( !validBrush( brush, error ) || !render::validateTerrainData( source, error ) )
                return false;
            const bool holes =
                brush.mode == TerrainBrushMode::Hole || brush.mode == TerrainBrushMode::FillHole;
            const bool paint = brush.mode == TerrainBrushMode::Paint ||
                               brush.mode == TerrainBrushMode::ClearPaint ||
                               brush.mode == TerrainBrushMode::Normalize;
            const bool masks = brush.mode >= TerrainBrushMode::PaintWetness;
            if( !holes && !paint && !masks && source.heightScale == 0 )
            {
                error = "Height editing requires a nonzero height scale.";
                return false;
            }
            const s32 width = source.dimensions.x, depth = source.dimensions.y;
            const s32 endX = width - ( holes ? 1 : 0 ), endZ = depth - ( holes ? 1 : 0 );
            const auto bound = []( double value, s32 high ) {
                return s32( std::clamp( value, 0.0, double( high ) ) );
            };
            const s32 x0 =
                bound( std::floor( ( double( brush.centre.x ) - brush.radius - source.origin.x ) /
                                   source.spacing.x ),
                       endX );
            const s32 x1 =
                bound( std::ceil( ( double( brush.centre.x ) + brush.radius - source.origin.x ) /
                                  source.spacing.x ) +
                           1,
                       endX );
            const s32 z0 =
                bound( std::floor( ( double( brush.centre.y ) - brush.radius - source.origin.y ) /
                                   source.spacing.y ),
                       endZ );
            const s32 z1 =
                bound( std::ceil( ( double( brush.centre.y ) + brush.radius - source.origin.y ) /
                                  source.spacing.y ) +
                           1,
                       endZ );
            candidate = source;
            if( holes && candidate.holes.empty() )
                candidate.holes.assign( size_t( width - 1 ) * ( depth - 1 ), 0 );
            if( paint && candidate.layerWeights.empty() )
                candidate.layerWeights.assign( source.heights.size(), Vector4F( 1, 0, 0, 0 ) );
            if( masks && candidate.surfaceMasks.empty() )
                candidate.surfaceMasks.assign( source.heights.size(), Vector2F( 1, 1 ) );
            size_t changed = 0;
            for( s32 z = z0; z < z1; ++z )
                for( s32 x = x0; x < x1; ++x )
                {
                    const double weight = brushWeight( source, brush, x, z, holes );
                    if( weight <= 0 )
                        continue;
                    if( holes )
                    {
                        const auto index = u32( z * ( width - 1 ) + x );
                        const u8 replacement = brush.mode == TerrainBrushMode::Hole ? 1 : 0;
                        if( candidate.holes[index] == replacement )
                            continue;
                        candidate.holes[index] = replacement;
                    }
                    else
                    {
                        const auto index = u32( z * width + x );
                        const double old = source.heights[index], localHeight = old * source.heightScale;
                        if( localHeight < brush.minHeight || localHeight > brush.maxHeight )
                            continue;
                        const auto xLeft = std::max( 0, x - 1 ), xRight = std::min( width - 1, x + 1 );
                        const auto zDown = std::max( 0, z - 1 ), zUp = std::min( depth - 1, z + 1 );
                        const double dx = ( double( source.heights[size_t( z ) * width + xRight] ) -
                                            source.heights[size_t( z ) * width + xLeft] ) *
                                          source.heightScale /
                                          ( ( xRight - xLeft ) * double( source.spacing.x ) );
                        const double dz = ( double( source.heights[size_t( zUp ) * width + x] ) -
                                            source.heights[size_t( zDown ) * width + x] ) *
                                          source.heightScale /
                                          ( ( zUp - zDown ) * double( source.spacing.y ) );
                        const double slope = std::atan( std::hypot( dx, dz ) ) * 180 / 3.141592653589793;
                        if( slope < brush.minSlope || slope > brush.maxSlope )
                            continue;
                        if( masks )
                        {
                            auto value = masksAt( source, index );
                            const bool snow = brush.mode == TerrainBrushMode::PaintSnow ||
                                              brush.mode == TerrainBrushMode::ClearSnow;
                            const f32 target = brush.mode == TerrainBrushMode::PaintWetness ||
                                                       brush.mode == TerrainBrushMode::PaintSnow
                                                   ? 1.0f
                                                   : 0.0f;
                            auto &channel = snow ? value.y : value.x;
                            channel += ( target - channel ) * f32( brush.strength * weight );
                            if( value == masksAt( source, index ) )
                                continue;
                            candidate.surfaceMasks[index] = value;
                        }
                        else if( paint )
                        {
                            auto value = render::normalizeTerrainWeights( weightsAt( source, index ) );
                            if( brush.mode != TerrainBrushMode::Normalize )
                            {
                                const f32 target = brush.mode == TerrainBrushMode::Paint ? 1.0f : 0.0f;
                                const f32 selected =
                                    value[brush.layer] +
                                    ( target - value[brush.layer] ) * f32( brush.strength * weight );
                                const f32 remainder = 1 - value[brush.layer];
                                for( u32 i = 0; i < 4; ++i )
                                    if( i != brush.layer )
                                        value[i] = remainder > 1.e-7f
                                                       ? value[i] * ( 1 - selected ) / remainder
                                                       : ( 1 - selected ) / 3;
                                value[brush.layer] = selected;
                                value = render::normalizeTerrainWeights( value );
                            }
                            if( value == weightsAt( source, index ) )
                                continue;
                            candidate.layerWeights[index] = value;
                        }
                        else
                        {
                            double value = old;
                            switch( brush.mode )
                            {
                            case TerrainBrushMode::Raise:
                            case TerrainBrushMode::Lower:
                                value += ( brush.mode == TerrainBrushMode::Raise ? 1 : -1 ) *
                                         brush.strength * weight / source.heightScale;
                                break;
                            case TerrainBrushMode::Flatten:
                                value += ( brush.targetHeight / double( source.heightScale ) - old ) *
                                         brush.strength * weight;
                                break;
                            case TerrainBrushMode::Terrace:
                                value += ( std::round( localHeight / brush.terraceStep ) *
                                               brush.terraceStep / source.heightScale -
                                           old ) *
                                         brush.strength * weight;
                                break;
                            case TerrainBrushMode::Noise:
                                value += ( double( noiseHash( index ^ brush.seed ) ) / 4294967295.0 * 2 -
                                           1 ) *
                                         brush.strength * weight / source.heightScale;
                                break;
                            case TerrainBrushMode::Stamp:
                            {
                                double stamp = 1;
                                if( !brush.stamp.empty() )
                                {
                                    const double sx =
                                        std::clamp( ( source.origin.x + double( x ) * source.spacing.x -
                                                      brush.centre.x ) /
                                                            ( 2 * double( brush.radius ) ) +
                                                        0.5,
                                                    0.0, 1.0 ) *
                                        ( brush.stampDimensions.x - 1 );
                                    const double sz =
                                        std::clamp( ( source.origin.y + double( z ) * source.spacing.y -
                                                      brush.centre.y ) /
                                                            ( 2 * double( brush.radius ) ) +
                                                        0.5,
                                                    0.0, 1.0 ) *
                                        ( brush.stampDimensions.y - 1 );
                                    const auto ix = std::min( s32( sx ), brush.stampDimensions.x - 2 ),
                                               iz = std::min( s32( sz ), brush.stampDimensions.y - 2 );
                                    const auto at = [&]( s32 a, s32 b ) {
                                        return brush.stamp[size_t( b ) * brush.stampDimensions.x + a];
                                    };
                                    const double fx = sx - ix, fz = sz - iz;
                                    stamp =
                                        ( at( ix, iz ) * ( 1 - fx ) + at( ix + 1, iz ) * fx ) *
                                            ( 1 - fz ) +
                                        ( at( ix, iz + 1 ) * ( 1 - fx ) + at( ix + 1, iz + 1 ) * fx ) *
                                            fz;
                                }
                                value += stamp * brush.strength * weight / source.heightScale;
                                break;
                            }
                            default:
                            {
                                double total = 0;
                                u32 count = 0;
                                for( s32 nz = zDown; nz <= zUp; ++nz )
                                    for( s32 nx = xLeft; nx <= xRight; ++nx )
                                    {
                                        total += source.heights[size_t( nz ) * width + nx];
                                        ++count;
                                    }
                                value += ( total / count - old ) * brush.strength * weight;
                            }
                            }
                            const f32 replacement = f32( value );
                            if( !std::isfinite( replacement ) ||
                                !std::isfinite( replacement * source.heightScale ) )
                            {
                                error = "Brush output exceeds finite height limits.";
                                return false;
                            }
                            if( replacement == old )
                                continue;
                            candidate.heights[index] = replacement;
                        }
                    }
                    if( ++changed > terrainMaximumEditSamples )
                    {
                        error = "Brush affects more than 65536 values; reduce its radius.";
                        return false;
                    }
                }
            if( changed == 0 )
                candidate = source;
            return true;
        }
        bool validErosion( const TerrainErosionRecipe &recipe, String &error )
        {
            if( !validBrush( recipe.region, error ) )
                return false;
            if( recipe.iterations == 0 || recipe.iterations > 1024 ||
                unsigned( recipe.mode ) > unsigned( TerrainErosionMode::Hydraulic ) ||
                !std::isfinite( recipe.talusAngle ) || recipe.talusAngle < 0 ||
                recipe.talusAngle >= 89 || !std::isfinite( recipe.rain ) || recipe.rain < 0 ||
                recipe.rain > 100 || !std::isfinite( recipe.sedimentCapacity ) ||
                recipe.sedimentCapacity < 0 || recipe.sedimentCapacity > 1 )
            {
                error = "Erosion parameters exceed supported ranges.";
                return false;
            }
            return true;
        }
        bool validGrade( const TerrainGradeRecipe &recipe, String &error )
        {
            if( recipe.points.size() < 2 || recipe.points.size() > 1024 ||
                !std::isfinite( recipe.width ) || recipe.width <= 0 ||
                !std::isfinite( recipe.shoulder ) || recipe.shoulder < 0 ||
                !std::isfinite( recipe.strength ) || recipe.strength < 0 || recipe.strength > 1 ||
                !std::isfinite( recipe.depth ) || recipe.depth < 0 )
            {
                error = "Grading requires a bounded finite polyline, width and blend strength.";
                return false;
            }
            for( const auto &point : recipe.points )
                if( !std::isfinite( point.x ) || !std::isfinite( point.y ) || !std::isfinite( point.z ) )
                {
                    error = "Grading polyline contains nonfinite coordinates.";
                    return false;
                }
            return true;
        }
    }  // namespace

    class TerrainHeightEdit final : public ICommand
    {
    public:
        template <class T>
        struct Change
        {
            T before, after;
        };
        AtomicWeakPtr<TerrainSystem> terrain;
        render::TerrainData grid;  // metadata only, never retains complete height/weight arrays
        std::map<u32, Change<f32>> heights;
        std::map<u32, Change<Vector4F>> weights;
        std::map<u32, Change<u8>> holes;
        std::map<u32, Change<Vector2F>> masks;
        std::map<u32, Change<render::TerrainLayer>> layers;
        std::optional<Change<render::TerrainAppearance>> appearance;
        bool implicitWeightsBefore = true, implicitHolesBefore = true, implicitMasksBefore = true;
        bool succeeded = false;
        String error;
        State state = State::Allocated;
        bool empty() const
        {
            return heights.empty() && weights.empty() && holes.empty() && masks.empty() &&
                   layers.empty() && !appearance;
        }
        bool capture( const render::TerrainData &before, const render::TerrainData &after )
        {
            if( !( before.appearance == after.appearance ) )
            {
                if( !appearance )
                    appearance =
                        Change<render::TerrainAppearance>{ before.appearance, after.appearance };
                else
                    appearance->after = after.appearance;
            }
            for( u32 i = 0; i < 4; ++i )
                if( !( before.layers[i] == after.layers[i] ) )
                {
                    auto found = layers.find( i );
                    if( found == layers.end() )
                        layers.emplace(
                            i, Change<render::TerrainLayer>{ before.layers[i], after.layers[i] } );
                    else
                        found->second.after = after.layers[i];
                }
            for( u32 i = 0; i < before.heights.size(); ++i )
            {
                if( before.heights[i] != after.heights[i] )
                {
                    auto found = heights.find( i );
                    if( found == heights.end() )
                        heights.emplace( i, Change<f32>{ before.heights[i], after.heights[i] } );
                    else
                        found->second.after = after.heights[i];
                }
                if( weightsAt( before, i ) != weightsAt( after, i ) )
                {
                    auto found = weights.find( i );
                    if( found == weights.end() )
                        weights.emplace(
                            i, Change<Vector4F>{ weightsAt( before, i ), weightsAt( after, i ) } );
                    else
                        found->second.after = weightsAt( after, i );
                }
                if( masksAt( before, i ) != masksAt( after, i ) )
                {
                    auto found = masks.find( i );
                    if( found == masks.end() )
                        masks.emplace( i,
                                       Change<Vector2F>{ masksAt( before, i ), masksAt( after, i ) } );
                    else
                        found->second.after = masksAt( after, i );
                }
                if( heights.size() + weights.size() + holes.size() + masks.size() >
                    terrainMaximumEditSamples )
                    return false;
            }
            for( u32 i = 0; i < u32( before.dimensions.x - 1 ) * ( before.dimensions.y - 1 ); ++i )
            {
                if( holeAt( before, i ) != holeAt( after, i ) )
                {
                    auto found = holes.find( i );
                    if( found == holes.end() )
                        holes.emplace( i, Change<u8>{ holeAt( before, i ), holeAt( after, i ) } );
                    else
                        found->second.after = holeAt( after, i );
                }
                if( heights.size() + weights.size() + holes.size() + masks.size() >
                    terrainMaximumEditSamples )
                    return false;
            }
            return true;
        }
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
            auto source = owner->getTerrainSnapshot();
            if( !source || !sameGrid( *source, grid ) )
            {
                error = "Terrain grid changed; undo/redo rejected.";
                return;
            }
            const auto expected = [forward]( const auto &change ) {
                return forward ? change.before : change.after;
            };
            const auto replacement = [forward]( const auto &change ) {
                return forward ? change.after : change.before;
            };
            for( const auto &[i, change] : heights )
                if( source->heights[i] != expected( change ) )
                {
                    error = "Terrain height conflicts with a newer edit.";
                    return;
                }
            for( const auto &[i, change] : weights )
                if( weightsAt( *source, i ) != expected( change ) )
                {
                    error = "Terrain weights conflict with a newer edit.";
                    return;
                }
            for( const auto &[i, change] : holes )
                if( holeAt( *source, i ) != expected( change ) )
                {
                    error = "Terrain holes conflict with a newer edit.";
                    return;
                }
            for( const auto &[i, change] : masks )
                if( masksAt( *source, i ) != expected( change ) )
                {
                    error = "Terrain surface masks conflict with a newer edit.";
                    return;
                }
            for( const auto &[i, change] : layers )
                if( !( source->layers[i] == expected( change ) ) )
                {
                    error = "Terrain layer material conflicts with a newer edit.";
                    return;
                }
            if( appearance && !( source->appearance == expected( *appearance ) ) )
            {
                error = "Terrain appearance conflicts with a newer edit.";
                return;
            }
            auto candidate = *source;
            if( appearance )
                candidate.appearance = replacement( *appearance );
            for( const auto &[i, change] : layers )
                candidate.layers[i] = replacement( change );
            if( !masks.empty() && candidate.surfaceMasks.empty() )
                candidate.surfaceMasks.assign( candidate.heights.size(), Vector2F( 1, 1 ) );
            for( const auto &[i, change] : masks )
                candidate.surfaceMasks[i] = replacement( change );
            if( !forward && implicitMasksBefore &&
                std::all_of( candidate.surfaceMasks.begin(), candidate.surfaceMasks.end(),
                             []( const Vector2F &value ) { return value == Vector2F( 1, 1 ); } ) )
                candidate.surfaceMasks.clear();
            for( const auto &[i, change] : heights )
                candidate.heights[i] = replacement( change );
            if( !weights.empty() && candidate.layerWeights.empty() )
                candidate.layerWeights.assign( candidate.heights.size(), Vector4F( 1, 0, 0, 0 ) );
            for( const auto &[i, change] : weights )
                candidate.layerWeights[i] = replacement( change );
            if( !holes.empty() && candidate.holes.empty() )
                candidate.holes.assign(
                    size_t( candidate.dimensions.x - 1 ) * ( candidate.dimensions.y - 1 ), 0 );
            for( const auto &[i, change] : holes )
                candidate.holes[i] = replacement( change );
            if( !forward && implicitWeightsBefore &&
                std::all_of( candidate.layerWeights.begin(), candidate.layerWeights.end(),
                             []( const Vector4F &value ) { return value == Vector4F( 1, 0, 0, 0 ); } ) )
                candidate.layerWeights.clear();
            if( !forward && implicitHolesBefore &&
                std::all_of( candidate.holes.begin(), candidate.holes.end(),
                             []( u8 value ) { return value == 0; } ) )
                candidate.holes.clear();
            succeeded = owner->applyTerrainData( candidate, error, source->revision );
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

    namespace
    {
        SmartPtr<TerrainHeightEdit> newEdit( SmartPtr<TerrainSystem> terrain,
                                             const render::TerrainData &source )
        {
            auto edit = make_ptr<TerrainHeightEdit>();
            edit->terrain = terrain;
            edit->grid.dimensions = source.dimensions;
            edit->grid.spacing = source.spacing;
            edit->grid.origin = source.origin;
            edit->grid.heightScale = source.heightScale;
            edit->grid.heights.clear();
            edit->implicitWeightsBefore = source.layerWeights.empty();
            edit->implicitHolesBefore = source.holes.empty();
            edit->implicitMasksBefore = source.surfaceMasks.empty();
            return edit;
        }
        bool commitCandidate( SmartPtr<TerrainSystem> terrain, const render::TerrainData &source,
                              const render::TerrainData &candidate, SmartPtr<ICommandManager> commands,
                              String &error )
        {
            if( !terrain || !commands )
            {
                error = "Terrain edit requires terrain and command history.";
                return false;
            }
            auto edit = newEdit( terrain, source );
            if( !sameGrid( source, candidate ) || !edit->capture( source, candidate ) )
            {
                error = "Terrain operation exceeds bounded undo patch capacity.";
                return false;
            }
            if( edit->empty() )
            {
                error = "Operation does not change terrain samples.";
                return false;
            }
            if( !terrain->applyTerrainData( candidate, error, source.revision ) )
                return false;
            edit->state = ICommand::State::Finished;
            commands->addCommand( edit );
            return true;
        }
    }  // namespace

    struct TerrainEditStroke::State
    {
        AtomicWeakPtr<TerrainSystem> terrain;
        SmartPtr<ICommandManager> commands;
        SmartPtr<TerrainHeightEdit> edit;
        u64 revision = 0;
        bool closed = false;
    };
    TerrainEditStroke::TerrainEditStroke( SmartPtr<TerrainSystem> terrain,
                                          SmartPtr<ICommandManager> commands ) :
        m_state( std::make_unique<State>() )
    {
        m_state->terrain = terrain;
        m_state->commands = commands;
        if( terrain )
        {
            auto source = terrain->getTerrainSnapshot();
            m_state->revision = source->revision;
            m_state->edit = newEdit( terrain, *source );
        }
    }
    TerrainEditStroke::~TerrainEditStroke()
    {
        if( !m_state->closed )
        {
            try
            {
                String error;
                cancel( error );
            }
            catch( ... )
            {
            }
        }
    }
    bool TerrainEditStroke::update( const TerrainBrush &brush, String &error )
    {
        error.clear();
        try
        {
            auto owner = m_state->terrain.load().lock();
            if( m_state->closed || !owner || !m_state->commands || !m_state->edit )
            {
                error = "Stroke is closed or its owner/history is unavailable.";
                return false;
            }
            auto source = owner->getTerrainSnapshot();
            if( source->revision != m_state->revision )
            {
                error = "Terrain changed during this stroke; cancel before starting another.";
                return false;
            }
            render::TerrainData candidate;
            if( !brushCandidate( *source, brush, candidate, error ) )
                return false;
            auto staged = newEdit( owner, *source );
            staged->heights = m_state->edit->heights;
            staged->weights = m_state->edit->weights;
            staged->holes = m_state->edit->holes;
            staged->masks = m_state->edit->masks;
            staged->implicitMasksBefore = m_state->edit->implicitMasksBefore;
            staged->implicitWeightsBefore = m_state->edit->implicitWeightsBefore;
            staged->implicitHolesBefore = m_state->edit->implicitHolesBefore;
            if( !staged->capture( *source, candidate ) )
            {
                error = "Stroke exceeds 65536 edited values; commit before continuing.";
                return false;
            }
            if( !owner->applyTerrainData( candidate, error, source->revision ) )
                return false;
            m_state->edit = staged;
            m_state->revision = owner->getTerrainRevision();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool TerrainEditStroke::commit( String &error )
    {
        error.clear();
        auto owner = m_state->terrain.load().lock();
        if( m_state->closed || !owner || !m_state->commands || !m_state->edit )
        {
            error = "Stroke is closed or unavailable.";
            return false;
        }
        if( owner->getTerrainRevision() != m_state->revision )
        {
            error = "Terrain changed before stroke commit; cancel the stroke.";
            return false;
        }
        if( m_state->edit->empty() )
        {
            m_state->closed = true;
            return true;
        }
        m_state->edit->state = ICommand::State::Finished;
        m_state->commands->addCommand( m_state->edit );
        m_state->closed = true;
        return true;
    }
    bool TerrainEditStroke::cancel( String &error )
    {
        error.clear();
        if( m_state->closed )
            return true;
        if( m_state->edit && !m_state->edit->empty() )
        {
            m_state->edit->apply( false );
            if( !m_state->edit->succeeded )
            {
                error = m_state->edit->error;
                return false;
            }
        }
        m_state->closed = true;
        return true;
    }
    TerrainDirtyRect TerrainEditStroke::getDirtyRect() const
    {
        TerrainDirtyRect rect;
        if( !m_state->edit )
            return rect;
        const auto add = [&]( u32 i, s32 width ) {
            s32 x = s32( i % width ), z = s32( i / width );
            if( rect.maxX < 0 )
            {
                rect.minX = rect.maxX = x;
                rect.minZ = rect.maxZ = z;
            }
            else
            {
                rect.minX = std::min( rect.minX, x );
                rect.maxX = std::max( rect.maxX, x );
                rect.minZ = std::min( rect.minZ, z );
                rect.maxZ = std::max( rect.maxZ, z );
            }
        };
        for( const auto &item : m_state->edit->heights )
            add( item.first, m_state->edit->grid.dimensions.x );
        for( const auto &item : m_state->edit->weights )
            add( item.first, m_state->edit->grid.dimensions.x );
        for( const auto &item : m_state->edit->masks )
            add( item.first, m_state->edit->grid.dimensions.x );
        for( const auto &item : m_state->edit->holes )
            add( item.first, m_state->edit->grid.dimensions.x - 1 );
        return rect;
    }
    bool applyTerrainBrush( SmartPtr<TerrainSystem> terrain, const TerrainBrush &brush,
                            SmartPtr<ICommandManager> commands, String &error )
    {
        TerrainEditStroke stroke( terrain, commands );
        return stroke.update( brush, error ) && stroke.commit( error );
    }

    bool applyTerrainAppearance( SmartPtr<TerrainSystem> terrain,
                                 const render::TerrainAppearance &appearance,
                                 SmartPtr<ICommandManager> commands, String &error )
    {
        error.clear();
        if( !terrain )
        {
            error = "No terrain selected.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        auto candidate = *source;
        candidate.appearance = appearance;
        if( !render::validateTerrainData( candidate, error ) )
            return false;
        if( candidate.appearance == source->appearance )
            return true;
        return commitCandidate( terrain, *source, candidate, commands, error );
    }
    bool applyTerrainLayer( SmartPtr<TerrainSystem> terrain, u32 index,
                            const render::TerrainLayer &layer, SmartPtr<ICommandManager> commands,
                            String &error )
    {
        error.clear();
        if( !terrain || index >= 4 )
        {
            error = "Select a valid terrain layer.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        auto candidate = *source;
        candidate.layers[index] = layer;
        if( !render::validateTerrainData( candidate, error ) )
            return false;
        if( candidate.layers[index] == source->layers[index] )
            return true;
        return commitCandidate( terrain, *source, candidate, commands, error );
    }

    namespace
    {
        bool writeTerrainFile( const String &path, const String &bytes, String &error )
        {
            std::filesystem::path temporary;
            try
            {
                if( path.empty() || path.find( '\0' ) != String::npos || bytes.empty() ||
                    bytes.size() > 256ull * 1024 * 1024 )
                    throw std::runtime_error(
                        "Terrain output requires a valid path and at most 256 MiB." );
                auto destination = std::filesystem::u8path( path.c_str() );
                temporary = destination;
                temporary += ".tmp-" + std::string( StringUtil::getUUID().c_str() );
                std::ofstream file( temporary, std::ios::binary | std::ios::trunc );
                file.write( bytes.data(), std::streamsize( bytes.size() ) );
                file.flush();
                if( !file )
                    throw std::runtime_error( "Cannot write terrain output." );
                file.close();
                if( !file )
                    throw std::runtime_error( "Cannot close terrain output." );
#ifdef _WIN32
                if( !MoveFileExW( temporary.c_str(), destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                    throw std::runtime_error( "Cannot atomically replace terrain output." );
#else
                std::filesystem::rename( temporary, destination );
#endif
                error.clear();
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
        bool readHeightFile( const String &path, const TerrainHeightFileOptions &options,
                             Vector2I &dimensions, Array<f32> &samples, String &error )
        {
            try
            {
                if( path.empty() || path.find( '\0' ) != String::npos )
                    throw std::runtime_error( "Height file path is invalid." );
                const bool floating = options.encoding == TerrainHeightEncoding::RawF32LE;
                if( unsigned( options.encoding ) > unsigned( TerrainHeightEncoding::RawF32LE ) ||
                    ( !floating && ( !std::isfinite( options.minimumHeight ) ||
                                     !std::isfinite( options.maximumHeight ) ||
                                     options.maximumHeight <= options.minimumHeight ) ) )
                    throw std::runtime_error( "Height encoding/range is invalid." );
                std::ifstream file( std::filesystem::u8path( path.c_str() ),
                                    std::ios::binary | std::ios::ate );
                const auto length = file ? file.tellg() : std::streampos( -1 );
                if( length <= 0 || length > std::streamoff( 32 * 1024 * 1024 ) )
                    throw std::runtime_error( "Height file is missing, empty or exceeds 32 MiB." );
                std::string bytes( size_t( length ), '\0' );
                file.seekg( 0 );
                file.read( bytes.data(), std::streamsize( bytes.size() ) );
                if( !file )
                    throw std::runtime_error( "Height file read failed." );
                size_t offset = 0;
                dimensions = options.dimensions;
                u32 maxValue = 65535;
                if( options.encoding == TerrainHeightEncoding::Pgm16 )
                {
                    const auto token = [&]() {
                        while( offset < bytes.size() )
                        {
                            if( bytes[offset] == '#' )
                            {
                                while( offset < bytes.size() && bytes[offset] != '\n' )
                                    ++offset;
                            }
                            else if( std::isspace( static_cast<unsigned char>( bytes[offset] ) ) )
                                ++offset;
                            else
                                break;
                        }
                        const auto start = offset;
                        while( offset < bytes.size() &&
                               !std::isspace( static_cast<unsigned char>( bytes[offset] ) ) &&
                               bytes[offset] != '#' )
                            ++offset;
                        if( offset - start > 16 )
                            throw std::runtime_error( "PGM header token is too long." );
                        return bytes.substr( start, offset - start );
                    };
                    if( token() != "P5" )
                        throw std::runtime_error(
                            "Height image must be a binary PGM (P5), not a renamed file." );
                    const auto integer = [&]( const std::string &value ) {
                        if( value.empty() ||
                            value.find_first_not_of( "0123456789" ) != std::string::npos )
                            throw std::runtime_error( "Invalid PGM integer." );
                        return std::stoul( value );
                    };
                    const auto width = integer( token() ), depth = integer( token() );
                    maxValue = u32( integer( token() ) );
                    if( width > render::terrainMaximumDimension ||
                        depth > render::terrainMaximumDimension || maxValue < 1 || maxValue > 65535 )
                        throw std::runtime_error( "Unsupported PGM dimensions or precision." );
                    dimensions = Vector2I( s32( width ), s32( depth ) );
                    if( offset >= bytes.size() ||
                        !std::isspace( static_cast<unsigned char>( bytes[offset] ) ) )
                        throw std::runtime_error( "PGM raster separator missing." );
                    if( bytes[offset++] == '\r' && offset < bytes.size() && bytes[offset] == '\n' )
                        ++offset;
                }
                if( dimensions.x < 2 || dimensions.y < 2 ||
                    dimensions.x > render::terrainMaximumDimension ||
                    dimensions.y > render::terrainMaximumDimension ||
                    u64( dimensions.x ) * dimensions.y > render::terrainMaximumSamples )
                    throw std::runtime_error( "Height dimensions are outside supported limits." );
                const size_t count = size_t( dimensions.x ) * dimensions.y;
                const size_t stride =
                    floating
                        ? 4
                        : ( options.encoding == TerrainHeightEncoding::Pgm16 && maxValue < 256 ? 1 : 2 );
                if( bytes.size() - offset != count * stride )
                    throw std::runtime_error(
                        "Height payload length does not match dimensions/encoding." );
                Array<f32> decoded;
                decoded.reserve( count );
                for( size_t i = 0; i < count; ++i )
                {
                    const auto *value =
                        reinterpret_cast<const unsigned char *>( bytes.data() + offset + i * stride );
                    f32 height;
                    if( floating )
                    {
                        const u32 bits = u32( value[0] ) | ( u32( value[1] ) << 8 ) |
                                         ( u32( value[2] ) << 16 ) | ( u32( value[3] ) << 24 );
                        std::memcpy( &height, &bits, sizeof( height ) );
                    }
                    else
                    {
                        const u32 encoded = stride == 1 ? value[0]
                                            : options.encoding == TerrainHeightEncoding::Pgm16
                                                ? ( u32( value[0] ) << 8 ) | value[1]
                                                : value[0] | ( u32( value[1] ) << 8 );
                        if( encoded > maxValue )
                            throw std::runtime_error( "Height image sample exceeds declared maximum." );
                        height = f32( options.minimumHeight +
                                      ( double( options.maximumHeight ) - options.minimumHeight ) *
                                          encoded / maxValue );
                    }
                    if( !std::isfinite( height ) )
                        throw std::runtime_error( "Height file contains nonfinite samples." );
                    decoded.push_back( height );
                }
                samples = std::move( decoded );
                error.clear();
                return true;
            }
            catch( const std::exception &e )
            {
                error = e.what();
                return false;
            }
        }
    }  // namespace
    bool importTerrainHeightFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                  const TerrainHeightFileOptions &options, String &error )
    {
        if( !terrain )
        {
            error = "Select a terrain before importing height data.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        auto candidate = *source;
        if( !readHeightFile( path, options, candidate.dimensions, candidate.heights, error ) )
            return false;
        if( candidate.dimensions != source->dimensions )
        {
            candidate.layerWeights.clear();
            candidate.surfaceMasks.clear();
            candidate.holes.clear();
            candidate.origin = Vector2F( -( candidate.dimensions.x - 1 ) * candidate.spacing.x * 0.5f,
                                         -( candidate.dimensions.y - 1 ) * candidate.spacing.y * 0.5f );
        }
        return terrain->applyTerrainData( candidate, error, source->revision );
    }
    bool exportTerrainHeightFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                  const TerrainHeightFileOptions &options, String &error )
    {
        try
        {
            if( !terrain )
                throw std::runtime_error( "Select a terrain before exporting height data." );
            auto source = terrain->getTerrainSnapshot();
            const bool floating = options.encoding == TerrainHeightEncoding::RawF32LE;
            if( unsigned( options.encoding ) > unsigned( TerrainHeightEncoding::RawF32LE ) ||
                ( !floating &&
                  ( !std::isfinite( options.minimumHeight ) || !std::isfinite( options.maximumHeight ) ||
                    options.maximumHeight <= options.minimumHeight ) ) )
                throw std::runtime_error( "Height encoding/range is invalid." );
            const auto extension = std::filesystem::u8path( path.c_str() ).extension().u8string();
            if( ( options.encoding == TerrainHeightEncoding::Pgm16 && extension != ".pgm" ) ||
                ( options.encoding != TerrainHeightEncoding::Pgm16 && extension != ".raw" &&
                  extension != ".r32" && extension != ".r16" ) )
                throw std::runtime_error(
                    "Use .pgm for PGM images or .raw/.r16/.r32 for explicit raw encoding." );
            String bytes;
            if( options.encoding == TerrainHeightEncoding::Pgm16 )
                bytes = "P5\n" + String( std::to_string( source->dimensions.x ).c_str() ) + " " +
                        String( std::to_string( source->dimensions.y ).c_str() ) + "\n65535\n";
            bytes.reserve( bytes.size() + source->heights.size() * ( floating ? 4 : 2 ) );
            for( const auto height : source->heights )
            {
                if( floating )
                {
                    u32 bits;
                    std::memcpy( &bits, &height, sizeof( bits ) );
                    for( u32 b = 0; b < 4; ++b )
                        bytes.push_back( char( ( bits >> ( b * 8 ) ) & 255 ) );
                }
                else
                {
                    if( height < options.minimumHeight || height > options.maximumHeight )
                        throw std::runtime_error(
                            "Height is outside the requested U16 range; adjust bounds or export raw "
                            "float." );
                    const auto encoded = u32( std::llround(
                        ( double( height ) - options.minimumHeight ) /
                        ( double( options.maximumHeight ) - options.minimumHeight ) * 65535 ) );
                    const bool big = options.encoding == TerrainHeightEncoding::Pgm16;
                    bytes.push_back( char( big ? encoded >> 8 : encoded & 255 ) );
                    bytes.push_back( char( big ? encoded & 255 : encoded >> 8 ) );
                }
            }
            return writeTerrainFile( path, bytes, error );
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool exportTerrainMeshFile( SmartPtr<TerrainSystem> terrain, const String &path, String &error )
    {
        try
        {
            if( !terrain || std::filesystem::u8path( path.c_str() ).extension() != ".obj" )
                throw std::runtime_error(
                    "Terrain mesh export requires a selected terrain and .obj output." );
            if( terrain->getTerrainSnapshot()->heights.size() > 1024ull * 1024 )
                throw std::runtime_error(
                    "Terrain OBJ export is limited to one million vertices; export a smaller tile." );
            render::TerrainMeshData mesh;
            if( !render::buildTerrainMeshData( *terrain->getTerrainSnapshot(), mesh, error ) )
                return false;
            std::ostringstream out;
            out.imbue( std::locale::classic() );
            out << std::setprecision( std::numeric_limits<f32>::max_digits10 )
                << "# Workphone terrain; local metres; holes omitted\n";
            for( const auto &p : mesh.positions )
                out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
            for( const auto &p : mesh.uvs )
                out << "vt " << p.x << ' ' << p.y << '\n';
            for( const auto &p : mesh.normals )
                out << "vn " << p.x << ' ' << p.y << ' ' << p.z << '\n';
            for( size_t i = 0; i < mesh.indices.size(); i += 3 )
            {
                out << 'f';
                for( size_t j = 0; j < 3; ++j )
                {
                    const auto index = mesh.indices[i + j] + 1;
                    out << ' ' << index << '/' << index << '/' << index;
                }
                out << '\n';
                if( out.tellp() > std::streamoff( 256 * 1024 * 1024 ) )
                    throw std::runtime_error( "Terrain OBJ exceeds 256 MiB; export a smaller terrain." );
            }
            return writeTerrainFile( path, String( out.str().c_str() ), error );
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool prepareTerrainErosion( const render::TerrainData &source, const TerrainErosionRecipe &recipe,
                                render::TerrainData &output, const std::atomic_bool *cancelled,
                                String &error )
    {
        try
        {
            error.clear();
            if( !render::validateTerrainData( source, error ) || !validErosion( recipe, error ) )
                return false;
            if( source.heightScale == 0 || recipe.iterations == 0 || recipe.iterations > 1024 ||
                unsigned( recipe.mode ) > unsigned( TerrainErosionMode::Hydraulic ) ||
                !std::isfinite( recipe.talusAngle ) || recipe.talusAngle < 0 ||
                recipe.talusAngle >= 89 || !std::isfinite( recipe.rain ) || recipe.rain < 0 ||
                recipe.rain > 100 || !std::isfinite( recipe.sedimentCapacity ) ||
                recipe.sedimentCapacity < 0 || recipe.sedimentCapacity > 1 )
                throw std::runtime_error( "Erosion parameters exceed supported ranges." );
            const auto width = source.dimensions.x, depth = source.dimensions.y;
            Array<u32> writeIndices;
            s32 firstX = width, firstZ = depth, lastX = -1, lastZ = -1;
            for( s32 z = 0; z < depth; ++z )
            {
                if( cancelled && cancelled->load() )
                    throw std::runtime_error( "Terrain erosion cancelled before publication." );
                for( s32 x = 0; x < width; ++x )
                    if( brushWeight( source, recipe.region, x, z ) > 0 )
                    {
                        if( writeIndices.size() >= terrainMaximumEditSamples )
                            throw std::runtime_error( "Erosion region exceeds 65536 samples." );
                        writeIndices.push_back( u32( z * width + x ) );
                        firstX = std::min( firstX, x );
                        firstZ = std::min( firstZ, z );
                        lastX = std::max( lastX, x );
                        lastZ = std::max( lastZ, z );
                    }
            }
            if( writeIndices.empty() )
            {
                output = source;
                return true;
            }
            // Each thermal step has a one-cell dependency stencil. Hydraulic
            // incoming flow also depends on a neighbour's choice among its own
            // neighbours, so its stencil grows by two cells per iteration.
            // Read the halo from the same authoritative snapshot and publish only
            // the selection: adjacent independently prepared edits reproduce the
            // same full-domain simulation, including their shared border.
            const s32 halo =
                s32( recipe.iterations ) * ( recipe.mode == TerrainErosionMode::Hydraulic ? 2 : 1 );
            firstX = std::max( 0, firstX - halo );
            firstZ = std::max( 0, firstZ - halo );
            lastX = std::min( width - 1, lastX + halo );
            lastZ = std::min( depth - 1, lastZ + halo );
            const s32 simulationWidth = lastX - firstX + 1;
            const auto simulationCount = u64( simulationWidth ) * u64( lastZ - firstZ + 1 );
            if( simulationCount > 262144 || simulationCount * recipe.iterations > 16ull * 1024 * 1024 )
                throw std::runtime_error(
                    "Erosion halo exceeds 262144 samples or 16 million sample iterations; reduce the "
                    "region or iteration count." );
            Array<u32> indices;
            indices.reserve( size_t( simulationCount ) );
            for( s32 z = firstZ; z <= lastZ; ++z )
            {
                if( cancelled && cancelled->load() )
                    throw std::runtime_error( "Terrain erosion cancelled before publication." );
                for( s32 x = firstX; x <= lastX; ++x )
                    indices.push_back( u32( z * width + x ) );
            }
            Array<double> height( indices.size() ), water( indices.size(), 0 ),
                sediment( indices.size(), 0 ), delta( indices.size(), 0 ),
                waterDelta( indices.size(), 0 ), sedimentDelta( indices.size(), 0 );
            for( size_t i = 0; i < indices.size(); ++i )
                height[i] = double( source.heights[indices[i]] ) * source.heightScale;
            const auto slot = [&]( s32 x, s32 z ) {
                return size_t( z - firstZ ) * simulationWidth + ( x - firstX );
            };
            const auto neighbours = [&]( size_t i, const auto &visit ) {
                const u32 index = indices[i];
                const s32 x = s32( index % width ), z = s32( index / width );
                for( const auto &offset :
                     { Vector2I( -1, 0 ), Vector2I( 1, 0 ), Vector2I( 0, -1 ), Vector2I( 0, 1 ) } )
                {
                    const s32 nx = x + offset.x, nz = z + offset.y;
                    if( nx < firstX || nz < firstZ || nx > lastX || nz > lastZ )
                        continue;
                    visit( slot( nx, nz ), offset.x ? source.spacing.x : source.spacing.y );
                }
            };
            for( u32 iteration = 0; iteration < recipe.iterations; ++iteration )
            {
                if( cancelled && cancelled->load() )
                {
                    error = "Terrain erosion cancelled before publication.";
                    return false;
                }
                std::fill( delta.begin(), delta.end(), 0 );
                std::fill( waterDelta.begin(), waterDelta.end(), 0 );
                std::fill( sedimentDelta.begin(), sedimentDelta.end(), 0 );
                if( recipe.mode == TerrainErosionMode::Hydraulic )
                    for( size_t i = 0; i < indices.size(); ++i )
                        water[i] +=
                            recipe.rain *
                            ( 0.75 + double( noiseHash( indices[i] ^ recipe.seed ^ iteration ) ) /
                                         4294967295.0 * 0.5 );
                for( size_t i = 0; i < indices.size(); ++i )
                {
                    if( ( i & 255 ) == 0 && cancelled && cancelled->load() )
                    {
                        error = "Terrain erosion cancelled before publication.";
                        return false;
                    }
                    if( recipe.mode == TerrainErosionMode::Thermal )
                    {
                        neighbours( i, [&]( size_t j, double spacing ) {
                            const double excess =
                                height[i] - height[j] -
                                std::tan( recipe.talusAngle * 3.141592653589793 / 180 ) * spacing;
                            if( excess > 0 )
                            {
                                const double moved = excess * 0.125;
                                delta[i] -= moved;
                                delta[j] += moved;
                            }
                        } );
                    }
                    else
                    {
                        size_t target = i;
                        double lowest = height[i] + water[i], distance = 1;
                        neighbours( i, [&]( size_t j, double spacing ) {
                            const double level = height[j] + water[j];
                            if( level < lowest )
                            {
                                lowest = level;
                                target = j;
                                distance = spacing;
                            }
                        } );
                        if( target == i )
                        {
                            const double deposit = sediment[i] * 0.25;
                            delta[i] += deposit;
                            sedimentDelta[i] -= deposit;
                            continue;
                        }
                        const double flow =
                            std::min( water[i] * 0.5, ( height[i] + water[i] - lowest ) * 0.25 );
                        const double capacity =
                            flow * std::max( 0.0, ( height[i] - height[target] ) / distance ) *
                            recipe.sedimentCapacity;
                        const double exchange = ( capacity - sediment[i] ) * 0.25;
                        // Positive exchange erodes, negative deposits; soil+sediment stays conserved.
                        delta[i] -= exchange;
                        sedimentDelta[i] += exchange;
                        const double moved =
                            water[i] > 0 ? std::max( 0.0, sediment[i] + exchange ) * flow / water[i] : 0;
                        waterDelta[i] -= flow;
                        waterDelta[target] += flow;
                        sedimentDelta[i] -= moved;
                        sedimentDelta[target] += moved;
                    }
                }
                for( size_t i = 0; i < indices.size(); ++i )
                {
                    height[i] += delta[i];
                    water[i] = std::max( 0.0, water[i] + waterDelta[i] ) * 0.9;
                    sediment[i] = std::max( 0.0, sediment[i] + sedimentDelta[i] );
                }
            }
            auto candidate = source;
            for( const auto index : writeIndices )
            {
                const auto i = slot( s32( index % width ), s32( index / width ) );
                candidate.heights[index] = f32( ( height[i] + sediment[i] ) / source.heightScale );
            }
            if( cancelled && cancelled->load() )
            {
                error = "Terrain erosion cancelled before publication.";
                return false;
            }
            if( !render::validateTerrainData( candidate, error ) )
                return false;
            output = std::move( candidate );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool applyTerrainErosion( SmartPtr<TerrainSystem> terrain, const TerrainErosionRecipe &recipe,
                              SmartPtr<ICommandManager> commands, String &error )
    {
        if( !terrain )
        {
            error = "Select a terrain before erosion.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        render::TerrainData candidate;
        return prepareTerrainErosion( *source, recipe, candidate, nullptr, error ) &&
               commitCandidate( terrain, *source, candidate, commands, error );
    }
    static bool prepareTerrainGrade( render::TerrainSnapshot source, const TerrainGradeRecipe &recipe,
                                     render::TerrainData &output, String &error,
                                     const std::atomic_bool *cancelled = nullptr )
    {
        try
        {
            if( !source || recipe.points.size() < 2 || recipe.points.size() > 1024 ||
                !std::isfinite( recipe.width ) || recipe.width <= 0 ||
                !std::isfinite( recipe.shoulder ) || recipe.shoulder < 0 ||
                !std::isfinite( recipe.strength ) || recipe.strength < 0 || recipe.strength > 1 ||
                !std::isfinite( recipe.depth ) || recipe.depth < 0 )
                throw std::runtime_error(
                    "Grading requires a bounded finite polyline, width and blend strength." );
            for( const auto &p : recipe.points )
                if( !std::isfinite( p.x ) || !std::isfinite( p.y ) || !std::isfinite( p.z ) )
                    throw std::runtime_error( "Grading polyline contains nonfinite coordinates." );
            auto candidate = *source;
            if( source->heights.size() * u64( recipe.points.size() - 1 ) > 16ull * 1024 * 1024 )
                throw std::runtime_error(
                    "Grading work exceeds 16 million sample segments; split the centreline." );
            if( source->heightScale == 0 )
                throw std::runtime_error( "Grading requires nonzero height scale." );
            size_t changed = 0;
            const double halfWidth = recipe.width * 0.5;
            for( s32 z = 0; z < source->dimensions.y; ++z )
                for( s32 x = 0; x < source->dimensions.x; ++x )
                {
                    if( ( x & 31 ) == 0 && cancelled && cancelled->load() )
                        throw std::runtime_error( "Terrain grading cancelled before publication." );
                    const double px = source->origin.x + double( x ) * source->spacing.x,
                                 pz = source->origin.y + double( z ) * source->spacing.y;
                    double distance = std::numeric_limits<double>::infinity(), target = 0;
                    for( size_t j = 1; j < recipe.points.size(); ++j )
                    {
                        const auto &a = recipe.points[j - 1], &b = recipe.points[j];
                        const double dx = double( b.x ) - a.x, dz = double( b.z ) - a.z,
                                     length2 = dx * dx + dz * dz;
                        if( length2 <= 0 )
                            continue;
                        const double t =
                            std::clamp( ( ( px - a.x ) * dx + ( pz - a.z ) * dz ) / length2, 0.0, 1.0 );
                        const double d = std::hypot( px - a.x - t * dx, pz - a.z - t * dz );
                        if( d < distance )
                        {
                            distance = d;
                            target = a.y + t * ( double( b.y ) - a.y );
                        }
                    }
                    if( distance > halfWidth + recipe.shoulder )
                        continue;
                    double blend = 1;
                    if( distance > halfWidth )
                    {
                        const double t = ( distance - halfWidth ) / recipe.shoulder;
                        blend = 1 - t * t * ( 3 - 2 * t );
                    }
                    if( recipe.river )
                        target -= recipe.depth *
                                  std::max( 0.0, 1 - distance * distance / ( halfWidth * halfWidth ) );
                    const size_t index = size_t( z ) * source->dimensions.x + x;
                    candidate.heights[index] =
                        f32( source->heights[index] +
                             ( target / source->heightScale - source->heights[index] ) *
                                 recipe.strength * blend );
                    if( candidate.heights[index] != source->heights[index] &&
                        ++changed > terrainMaximumEditSamples )
                        throw std::runtime_error(
                            "Grading affects more than 65536 samples; split the polyline." );
                }
            if( !render::validateTerrainData( candidate, error ) )
                return false;
            output = std::move( candidate );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool applyTerrainGrade( SmartPtr<TerrainSystem> terrain, const TerrainGradeRecipe &recipe,
                            SmartPtr<ICommandManager> commands, String &error )
    {
        if( !terrain )
        {
            error = "Select a terrain before grading.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        render::TerrainData output;
        return prepareTerrainGrade( source, recipe, output, error ) &&
               commitCandidate( terrain, *source, output, commands, error );
    }

    bool exportTerrainRuleMask( SmartPtr<TerrainSystem> terrain, const String &path,
                                const TerrainBrush &rule, String &error )
    {
        if( !terrain )
        {
            error = "Select terrain before exporting a rule mask.";
            return false;
        }
        auto source = *terrain->getTerrainSnapshot();
        source.layerWeights.clear();
        auto paint = rule;
        paint.mode = TerrainBrushMode::Paint;
        paint.layer = 1;
        paint.strength = 1;
        render::TerrainData candidate;
        if( !brushCandidate( source, paint, candidate, error ) )
            return false;
        for( u32 i = 0; i < candidate.heights.size(); ++i )
            candidate.heights[i] = weightsAt( candidate, i ).y;
        candidate.heightScale = 1;
        candidate.layerWeights.clear();
        candidate.holes.clear();
        auto mask = make_ptr<TerrainSystem>();
        if( !mask->applyTerrainData( candidate, error ) )
            return false;
        TerrainHeightFileOptions encoding;
        return exportTerrainHeightFile( mask, path, encoding, error );
    }
    bool exportTerrainErosionMask( SmartPtr<TerrainSystem> terrain, const String &path,
                                   const TerrainErosionRecipe &recipe, String &error )
    {
        if( !terrain )
        {
            error = "Select terrain before exporting an erosion mask.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        render::TerrainData candidate;
        if( !prepareTerrainErosion( *source, recipe, candidate, nullptr, error ) )
            return false;
        double maximum = 0;
        for( size_t i = 0; i < candidate.heights.size(); ++i )
        {
            const double delta = std::abs( ( double( candidate.heights[i] ) - source->heights[i] ) *
                                           source->heightScale );
            if( !std::isfinite( delta ) || delta > std::numeric_limits<f32>::max() )
            {
                error = "Erosion displacement mask exceeds supported range.";
                return false;
            }
            candidate.heights[i] = f32( delta );
            maximum = std::max( maximum, delta );
        }
        if( maximum > 0 )
            for( auto &height : candidate.heights )
                height = f32( height / maximum );
        candidate.heightScale = 1;
        candidate.layerWeights.clear();
        candidate.holes.clear();
        auto mask = make_ptr<TerrainSystem>();
        if( !mask->applyTerrainData( candidate, error ) )
            return false;
        TerrainHeightFileOptions encoding;
        return exportTerrainHeightFile( mask, path, encoding, error );
    }

    struct TerrainRecipeStack::State
    {
        struct Result
        {
            render::TerrainSnapshot data;
            String error;
            u64 generation = 0;
        };
        AtomicWeakPtr<TerrainSystem> owner;
        render::TerrainSnapshot base, preview;
        Array<TerrainRecipeOperation> operations;
        u64 generation = 1, expectedRevision = 0;
        std::shared_ptr<std::atomic_bool> cancelled;
        std::future<Result> worker;
    };
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainRecipeStack, ISharedObject );
    TerrainRecipeStack::TerrainRecipeStack() : m_state( std::make_unique<State>() )
    {
    }
    TerrainRecipeStack::~TerrainRecipeStack()
    {
        cancel();
    }
    void TerrainRecipeStack::unload( SmartPtr<ISharedObject> )
    {
        cancel();
        m_state->base.reset();
        m_state->preview.reset();
        m_state->operations.clear();
    }
    void TerrainRecipeStack::cancel()
    {
        if( m_state->cancelled )
            m_state->cancelled->store( true );
        if( m_state->worker.valid() )
        {
            try
            {
                m_state->worker.get();
            }
            catch( ... )
            {
            }
        }
        m_state->cancelled.reset();
        m_state->preview.reset();
        ++m_state->generation;
    }
    bool TerrainRecipeStack::setBase( SmartPtr<TerrainSystem> terrain, String &error )
    {
        if( !terrain )
        {
            error = "Recipe requires a selected terrain.";
            return false;
        }
        auto base = terrain->getTerrainSnapshot();
        if( base->heights.size() > terrainMaximumEditSamples )
        {
            error = "Recipe base exceeds 65536 samples; use a bounded terrain tile.";
            return false;
        }
        cancel();
        m_state->base = base;
        m_state->owner = terrain;
        m_state->expectedRevision = base->revision;
        m_state->operations.clear();
        error.clear();
        return true;
    }
    bool TerrainRecipeStack::append( const TerrainRecipeOperation &operation, String &error )
    {
        if( !m_state->base || m_state->operations.size() >= 32 )
        {
            error = "Recipe requires a base and at most 32 ordered operations.";
            return false;
        }
        if( unsigned( operation.kind ) > unsigned( TerrainRecipeOperation::Kind::Grade ) )
        {
            error = "Unknown terrain recipe operation.";
            return false;
        }
        if( operation.kind == TerrainRecipeOperation::Kind::Brush &&
            !validBrush( operation.brush, error ) )
            return false;
        if( operation.kind == TerrainRecipeOperation::Kind::Erosion &&
            !validErosion( operation.erosion, error ) )
            return false;
        if( operation.kind == TerrainRecipeOperation::Kind::Grade &&
            !validGrade( operation.grade, error ) )
            return false;
        cancel();
        m_state->operations.push_back( operation );
        error.clear();
        return true;
    }
    bool TerrainRecipeStack::move( u32 from, u32 to, String &error )
    {
        if( from >= m_state->operations.size() || to >= m_state->operations.size() )
        {
            error = "Recipe reorder index is out of range.";
            return false;
        }
        cancel();
        auto value = m_state->operations[from];
        m_state->operations.erase( m_state->operations.begin() + from );
        m_state->operations.insert( m_state->operations.begin() + to, value );
        error.clear();
        return true;
    }
    bool TerrainRecipeStack::remove( u32 index, String &error )
    {
        if( index >= m_state->operations.size() )
        {
            error = "Recipe removal index is out of range.";
            return false;
        }
        cancel();
        m_state->operations.erase( m_state->operations.begin() + index );
        error.clear();
        return true;
    }
    bool TerrainRecipeStack::setEnabled( u32 index, bool enabled, String &error )
    {
        if( index >= m_state->operations.size() )
        {
            error = "Recipe enable index is out of range.";
            return false;
        }
        cancel();
        m_state->operations[index].enabled = enabled;
        error.clear();
        return true;
    }
    u32 TerrainRecipeStack::getOperationCount() const
    {
        return u32( m_state->operations.size() );
    }
    u64 TerrainRecipeStack::getGeneration() const
    {
        return m_state->generation;
    }
    bool TerrainRecipeStack::isBusy() const
    {
        return m_state->worker.valid();
    }
    render::TerrainSnapshot TerrainRecipeStack::getPreview() const
    {
        return m_state->preview;
    }
    render::TerrainSnapshot TerrainRecipeStack::getPreviewFor( SmartPtr<TerrainSystem> terrain ) const
    {
        if( !terrain || m_state->worker.valid() || m_state->owner.load().lock() != terrain ||
            terrain->getTerrainRevision() != m_state->expectedRevision )
            return {};
        return m_state->preview;
    }
    bool TerrainRecipeStack::requestPreview( String &error )
    {
        if( !m_state->base )
        {
            error = "Terrain recipe has no base.";
            return false;
        }
        cancel();
        const auto base = m_state->base;
        const auto operations = m_state->operations;
        const auto generation = m_state->generation;
        auto cancelled = std::make_shared<std::atomic_bool>( false );
        m_state->cancelled = cancelled;
        try
        {
            m_state->worker = std::async( std::launch::async, [base, operations, generation, cancelled] {
                State::Result result;
                result.generation = generation;
                try
                {
                    auto current = base;
                    for( const auto &operation : operations )
                    {
                        if( cancelled->load() )
                        {
                            result.error = "Recipe generation cancelled.";
                            return result;
                        }
                        if( !operation.enabled )
                            continue;
                        render::TerrainData candidate;
                        bool success = false;
                        switch( operation.kind )
                        {
                        case TerrainRecipeOperation::Kind::Brush:
                            success =
                                brushCandidate( *current, operation.brush, candidate, result.error );
                            break;
                        case TerrainRecipeOperation::Kind::Erosion:
                            success = prepareTerrainErosion( *current, operation.erosion, candidate,
                                                             cancelled.get(), result.error );
                            break;
                        case TerrainRecipeOperation::Kind::Grade:
                            success = prepareTerrainGrade( current, operation.grade, candidate,
                                                           result.error, cancelled.get() );
                            break;
                        }
                        if( !success )
                            return result;
                        current = std::make_shared<const render::TerrainData>( std::move( candidate ) );
                    }
                    if( cancelled->load() )
                        result.error = "Recipe generation cancelled.";
                    else
                        result.data = current;
                }
                catch( const std::exception &e )
                {
                    result.error = e.what();
                }
                return result;
            } );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool TerrainRecipeStack::poll( String &error )
    {
        error.clear();
        if( !m_state->worker.valid() )
            return bool( m_state->preview );
        if( m_state->worker.wait_for( std::chrono::milliseconds( 0 ) ) != std::future_status::ready )
            return false;
        auto result = m_state->worker.get();
        if( result.generation != m_state->generation ||
            ( m_state->cancelled && m_state->cancelled->load() ) )
        {
            error = "Stale terrain recipe generation discarded.";
            return false;
        }
        if( !result.data )
        {
            error = result.error.empty() ? "Terrain recipe preparation failed." : result.error;
            return false;
        }
        m_state->preview = result.data;
        return true;
    }
    bool TerrainRecipeStack::commit( SmartPtr<TerrainSystem> terrain, SmartPtr<ICommandManager> commands,
                                     String &error )
    {
        if( !terrain || m_state->owner.load().lock() != terrain ||
            terrain->getTerrainRevision() != m_state->expectedRevision )
        {
            error =
                "Terrain changed since the recipe base/last commit; reset the base to preserve manual "
                "edits.";
            return false;
        }
        if( !poll( error ) || !m_state->preview )
        {
            if( error.empty() )
                error = "Recipe preview is not ready.";
            return false;
        }
        auto source = terrain->getTerrainSnapshot();
        auto candidate = *m_state->preview;
        // Recipes own operation deltas, not current material descriptors or untouched
        // weather-mask channels. Loading an old base must preserve those author edits.
        candidate.layers = source->layers;
        candidate.appearance = source->appearance;
        candidate.surfaceMasks = source->surfaceMasks;
        for( u32 i = 0; i < candidate.heights.size(); ++i )
        {
            const auto before = masksAt( *m_state->base, i );
            const auto after = masksAt( *m_state->preview, i );
            if( before == after )
                continue;
            if( candidate.surfaceMasks.empty() )
                candidate.surfaceMasks.assign( candidate.heights.size(), Vector2F( 1, 1 ) );
            if( before.x != after.x )
                candidate.surfaceMasks[i].x = after.x;
            if( before.y != after.y )
                candidate.surfaceMasks[i].y = after.y;
        }
        if( !commitCandidate( terrain, *source, candidate, commands, error ) )
            return false;
        m_state->expectedRevision = terrain->getTerrainRevision();
        return true;
    }
    String TerrainRecipeStack::serialize() const
    {
        if( !m_state->base )
            return {};
        using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;
        Json root( cJSON_CreateObject(), cJSON_Delete );
        if( !root )
            return {};
        cJSON_AddStringToObject( root.get(), "format", "workphone.terrain.recipe" );
        cJSON_AddNumberToObject( root.get(), "version", 1 );
        const auto baseBytes = render::serializeTerrainData( *m_state->base );
        Json base( cJSON_Parse( baseBytes.c_str() ), cJSON_Delete );
        if( !base )
            return {};
        cJSON_AddItemToObject( root.get(), "base", base.release() );
        auto operations = cJSON_AddArrayToObject( root.get(), "operations" );
        const auto putBrush = []( cJSON *item, const TerrainBrush &brush ) {
            cJSON_AddNumberToObject( item, "mode", unsigned( brush.mode ) );
            cJSON_AddNumberToObject( item, "x", brush.centre.x );
            cJSON_AddNumberToObject( item, "z", brush.centre.y );
            cJSON_AddNumberToObject( item, "radius", brush.radius );
            cJSON_AddNumberToObject( item, "strength", brush.strength );
            cJSON_AddNumberToObject( item, "target", brush.targetHeight );
            cJSON_AddNumberToObject( item, "layer", brush.layer );
            cJSON_AddNumberToObject( item, "seed", brush.seed );
            cJSON_AddNumberToObject( item, "terrace", brush.terraceStep );
            cJSON_AddNumberToObject( item, "minHeight", brush.minHeight );
            cJSON_AddNumberToObject( item, "maxHeight", brush.maxHeight );
            cJSON_AddNumberToObject( item, "minSlope", brush.minSlope );
            cJSON_AddNumberToObject( item, "maxSlope", brush.maxSlope );
            cJSON_AddNumberToObject( item, "stampWidth", brush.stampDimensions.x );
            cJSON_AddNumberToObject( item, "stampDepth", brush.stampDimensions.y );
            cJSON_AddItemToObject(
                item, "stamp",
                brush.stamp.empty()
                    ? cJSON_CreateArray()
                    : cJSON_CreateFloatArray( brush.stamp.data(), int( brush.stamp.size() ) ) );
        };
        for( const auto &operation : m_state->operations )
        {
            auto item = cJSON_CreateObject();
            if( !item || !operations )
                return {};
            cJSON_AddItemToArray( operations, item );
            cJSON_AddNumberToObject( item, "kind", unsigned( operation.kind ) );
            cJSON_AddBoolToObject( item, "enabled", operation.enabled );
            if( operation.kind == TerrainRecipeOperation::Kind::Brush )
                putBrush( item, operation.brush );
            else if( operation.kind == TerrainRecipeOperation::Kind::Erosion )
            {
                putBrush( item, operation.erosion.region );
                const auto &recipe = operation.erosion;
                cJSON_AddNumberToObject( item, "erosionMode", unsigned( recipe.mode ) );
                cJSON_AddNumberToObject( item, "iterations", recipe.iterations );
                cJSON_AddNumberToObject( item, "talus", recipe.talusAngle );
                cJSON_AddNumberToObject( item, "rain", recipe.rain );
                cJSON_AddNumberToObject( item, "capacity", recipe.sedimentCapacity );
                cJSON_AddNumberToObject( item, "erosionSeed", recipe.seed );
            }
            else
            {
                const auto &recipe = operation.grade;
                cJSON_AddNumberToObject( item, "width", recipe.width );
                cJSON_AddNumberToObject( item, "shoulder", recipe.shoulder );
                cJSON_AddNumberToObject( item, "strength", recipe.strength );
                cJSON_AddNumberToObject( item, "depth", recipe.depth );
                cJSON_AddBoolToObject( item, "river", recipe.river );
                Array<f32> points;
                for( const auto &point : recipe.points )
                {
                    points.push_back( point.x );
                    points.push_back( point.y );
                    points.push_back( point.z );
                }
                cJSON_AddItemToObject( item, "points",
                                       cJSON_CreateFloatArray( points.data(), int( points.size() ) ) );
            }
        }
        std::unique_ptr<char, decltype( &cJSON_free )> bytes( cJSON_PrintUnformatted( root.get() ),
                                                              cJSON_free );
        if( !bytes )
            return {};
        const auto length = std::char_traits<char>::length( bytes.get() );
        return length <= 32ull * 1024 * 1024 ? String( bytes.get(), length ) : String();
    }
    bool TerrainRecipeStack::deserialize( const String &json, SmartPtr<TerrainSystem> target,
                                          String &error )
    {
        try
        {
            if( !target || json.empty() || json.size() > 32ull * 1024 * 1024 )
                throw std::runtime_error( "Terrain recipe requires a target and at most 32 MiB." );
            using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;
            const char *end = nullptr;
            Json root( cJSON_ParseWithLengthOpts( json.c_str(), json.size() + 1, &end, 1 ),
                       cJSON_Delete );
            if( !root || end != json.c_str() + json.size() )
                throw std::runtime_error( "Invalid recipe JSON." );
            const auto field = []( cJSON *item, const char *key ) {
                return cJSON_GetObjectItemCaseSensitive( item, key );
            };
            const auto num = [&]( cJSON *item, const char *key ) {
                auto value = field( item, key );
                if( !cJSON_IsNumber( value ) || !std::isfinite( value->valuedouble ) ||
                    std::abs( value->valuedouble ) > std::numeric_limits<f32>::max() )
                    throw std::runtime_error( "Invalid recipe number." );
                return value->valuedouble;
            };
            const auto integer = [&]( cJSON *item, const char *key, u32 maximum ) {
                auto value = num( item, key );
                if( value < 0 || value > maximum || std::floor( value ) != value )
                    throw std::runtime_error( "Invalid recipe integer." );
                return u32( value );
            };
            const auto boolean = [&]( cJSON *item, const char *key ) {
                auto value = field( item, key );
                if( !cJSON_IsBool( value ) )
                    throw std::runtime_error( "Invalid recipe boolean." );
                return cJSON_IsTrue( value ) != 0;
            };
            auto format = field( root.get(), "format" );
            if( !cJSON_IsString( format ) ||
                String( format->valuestring ) != "workphone.terrain.recipe" ||
                num( root.get(), "version" ) != 1 )
                throw std::runtime_error( "Unsupported terrain recipe format." );
            std::unique_ptr<char, decltype( &cJSON_free )> baseJson(
                cJSON_PrintUnformatted( field( root.get(), "base" ) ), cJSON_free );
            render::TerrainData base;
            if( !baseJson || !render::deserializeTerrainData( baseJson.get(), base, error ) )
                return false;
            if( base.heights.size() > terrainMaximumEditSamples )
                throw std::runtime_error( "Recipe base exceeds 65536 samples." );
            auto list = field( root.get(), "operations" );
            if( !cJSON_IsArray( list ) || cJSON_GetArraySize( list ) > 32 )
                throw std::runtime_error( "Recipe requires at most 32 operations." );
            Array<TerrainRecipeOperation> operations;
            const auto readBrush = [&]( cJSON *item, TerrainBrush &brush ) {
                brush.mode =
                    TerrainBrushMode( integer( item, "mode", unsigned( TerrainBrushMode::ClearSnow ) ) );
                brush.centre = Vector2F( f32( num( item, "x" ) ), f32( num( item, "z" ) ) );
                brush.radius = f32( num( item, "radius" ) );
                brush.strength = f32( num( item, "strength" ) );
                brush.targetHeight = f32( num( item, "target" ) );
                brush.layer = integer( item, "layer", 3 );
                brush.seed = integer( item, "seed", std::numeric_limits<u32>::max() );
                brush.terraceStep = f32( num( item, "terrace" ) );
                brush.minHeight = f32( num( item, "minHeight" ) );
                brush.maxHeight = f32( num( item, "maxHeight" ) );
                brush.minSlope = f32( num( item, "minSlope" ) );
                brush.maxSlope = f32( num( item, "maxSlope" ) );
                brush.stampDimensions =
                    Vector2I( integer( item, "stampWidth", terrainMaximumEditSamples ),
                              integer( item, "stampDepth", terrainMaximumEditSamples ) );
                auto stamp = field( item, "stamp" );
                if( !cJSON_IsArray( stamp ) || cJSON_GetArraySize( stamp ) > terrainMaximumEditSamples )
                    throw std::runtime_error( "Invalid recipe stamp." );
                for( auto value = stamp->child; value; value = value->next )
                {
                    if( !cJSON_IsNumber( value ) )
                        throw std::runtime_error( "Invalid stamp sample." );
                    brush.stamp.push_back( f32( value->valuedouble ) );
                }
                if( !validBrush( brush, error ) )
                    throw std::runtime_error( error.c_str() );
            };
            for( auto item = list->child; item; item = item->next )
            {
                TerrainRecipeOperation operation;
                operation.kind = TerrainRecipeOperation::Kind( integer( item, "kind", 2 ) );
                operation.enabled = boolean( item, "enabled" );
                if( operation.kind == TerrainRecipeOperation::Kind::Brush )
                    readBrush( item, operation.brush );
                else if( operation.kind == TerrainRecipeOperation::Kind::Erosion )
                {
                    auto &recipe = operation.erosion;
                    readBrush( item, recipe.region );
                    recipe.mode = TerrainErosionMode( integer( item, "erosionMode", 1 ) );
                    recipe.iterations = integer( item, "iterations", 1024 );
                    recipe.talusAngle = f32( num( item, "talus" ) );
                    recipe.rain = f32( num( item, "rain" ) );
                    recipe.sedimentCapacity = f32( num( item, "capacity" ) );
                    recipe.seed = integer( item, "erosionSeed", std::numeric_limits<u32>::max() );
                }
                else
                {
                    auto &recipe = operation.grade;
                    recipe.width = f32( num( item, "width" ) );
                    recipe.shoulder = f32( num( item, "shoulder" ) );
                    recipe.strength = f32( num( item, "strength" ) );
                    recipe.depth = f32( num( item, "depth" ) );
                    recipe.river = boolean( item, "river" );
                    auto points = field( item, "points" );
                    if( !cJSON_IsArray( points ) || cJSON_GetArraySize( points ) < 6 ||
                        cJSON_GetArraySize( points ) > 3072 || cJSON_GetArraySize( points ) % 3 )
                        throw std::runtime_error( "Invalid grading points." );
                    Vector3F point;
                    int channel = 0;
                    for( auto value = points->child; value; value = value->next )
                    {
                        if( !cJSON_IsNumber( value ) || !std::isfinite( value->valuedouble ) ||
                            std::abs( value->valuedouble ) > std::numeric_limits<f32>::max() )
                            throw std::runtime_error( "Invalid grading coordinate." );
                        point[channel++] = f32( value->valuedouble );
                        if( channel == 3 )
                        {
                            recipe.points.push_back( point );
                            channel = 0;
                        }
                    }
                }
                if( operation.kind == TerrainRecipeOperation::Kind::Erosion &&
                    !validErosion( operation.erosion, error ) )
                    return false;
                if( operation.kind == TerrainRecipeOperation::Kind::Grade &&
                    !validGrade( operation.grade, error ) )
                    return false;
                operations.push_back( std::move( operation ) );
            }
            // Imported recipes may not silently overwrite a different grid or manual terrain.
            auto current = target->getTerrainSnapshot();
            if( !sameGrid( base, *current ) )
                throw std::runtime_error(
                    "Recipe grid differs from target; import its terrain base first." );
            cancel();
            m_state->base = std::make_shared<const render::TerrainData>( std::move( base ) );
            m_state->operations = std::move( operations );
            m_state->owner = target;
            m_state->expectedRevision = current->revision;
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool TerrainRecipeStack::save( const String &path, String &error ) const
    {
        const auto bytes = serialize();
        if( bytes.empty() )
        {
            error = "Recipe has no serializable base or exceeds the 32 MiB file limit.";
            return false;
        }
        return writeTerrainFile( path, bytes, error );
    }
    bool TerrainRecipeStack::load( const String &path, SmartPtr<TerrainSystem> target, String &error )
    {
        try
        {
            if( path.empty() || path.find( '\0' ) != String::npos )
                throw std::runtime_error( "Recipe path is invalid." );
            std::ifstream file( std::filesystem::u8path( path.c_str() ),
                                std::ios::binary | std::ios::ate );
            const auto size = file ? file.tellg() : std::streampos( -1 );
            if( size <= 0 || size > std::streamoff( 32 * 1024 * 1024 ) )
                throw std::runtime_error( "Recipe file is missing, empty or too large." );
            String bytes( size_t( size ), '\0' );
            file.seekg( 0 );
            file.read( bytes.data(), std::streamsize( bytes.size() ) );
            if( !file )
                throw std::runtime_error( "Recipe file read failed." );
            return deserialize( bytes, target, error );
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
