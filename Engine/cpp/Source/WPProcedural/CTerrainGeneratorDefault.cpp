#include "WPProcedural/WPProceduralPCH.hpp"
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <Workphone/Interface/Procedural/IProceduralScene.hpp>
#include <Workphone/Math/PerlinNoiseGenerator.hpp>
#include <Workphone/Math/LinearSpline1.hpp>
#include <Workphone/Math/FalloffGenerator.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Math.hpp>

#include <algorithm>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr float s_minScale = 0.0001f;
            constexpr float s_minLacunarity = 1.0f;
            constexpr float s_minFalloff = 0.0f;
            constexpr int s_minDimension = 1;
            constexpr int s_minOctaves = 0;
            constexpr double s_randomOffsetMax = 9999.0;
        }  // namespace

        CTerrainGeneratorDefault::CTerrainGeneratorDefault()
        {
            // Default-legacy values: the original implementation used a 10-unit depth
            // and squared dimensions driven by width. Preserve that behaviour.
            m_options.depth = 10;
            m_options.normalize = false;
            m_options.randomize = false;
        }

        CTerrainGeneratorDefault::~CTerrainGeneratorDefault() = default;

        void CTerrainGeneratorDefault::generate()
        {
            m_finished = false;
            m_terrain = nullptr;

            validate();

            if( m_options.randomize )
            {
                m_options.offset =
                    static_cast<float>( Math<real_Num>::RangedRandom( 0.0, s_randomOffsetMax ) );
            }

            auto scene = getProceduralScene();
            if( !scene )
            {
                m_finished = true;
                return;
            }

            m_isGenerating = true;
            try
            {
                generateTerrain();
            }
            catch( ... )
            {
                m_isGenerating = false;
                throw;
            }
            m_isGenerating = false;
            m_finished = true;
        }

        void CTerrainGeneratorDefault::validate()
        {
            // Maintain legacy defaults: width/height/depth/octaves/scale/lacunarity.
            m_options.width = std::max( s_minDimension, m_options.width );
            m_options.height = std::max( s_minDimension, m_options.height );
            m_options.depth = std::max( s_minDimension, m_options.depth );
            m_options.octaves = std::max( s_minOctaves, m_options.octaves );
            m_options.scale = std::max( s_minScale, m_options.scale );
            m_options.lacunarity = std::max( s_minLacunarity, m_options.lacunarity );
            m_options.persistence =
                Math<real_Num>::max( 0.0, Math<real_Num>::min( 1.0, m_options.persistence ) );
            m_options.falloffDirection = Math<real_Num>::max( s_minFalloff, m_options.falloffDirection );
            m_options.falloffRange = Math<real_Num>::max( s_minFalloff, m_options.falloffRange );
        }

        void CTerrainGeneratorDefault::clear()
        {
            resetToDefaults();

            // Restore the legacy default overrides after the base reset.
            m_options.depth = 10;
            m_options.normalize = false;
            m_options.randomize = false;

            m_scene = nullptr;
            m_terrain = nullptr;
            m_finished = false;
            m_isGenerating = false;
        }

        void CTerrainGeneratorDefault::loadOptions( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }

            CTerrainGenerator::loadOptions( properties );

            // Default override: in this implementation width always drives height.
            m_options.height = m_options.width;
        }

        void CTerrainGeneratorDefault::saveOptions( SmartPtr<Properties> properties ) const
        {
            if( !properties )
            {
                return;
            }

            CTerrainGenerator::saveOptions( properties );
        }

        void CTerrainGeneratorDefault::generateRandom()
        {
            const bool wasRandomize = m_options.randomize;
            m_options.randomize = true;
            generate();
            m_options.randomize = wasRandomize;
        }

        void CTerrainGeneratorDefault::generateTerrain()
        {
            auto scene = getProceduralScene();
            if( !scene )
            {
                return;
            }

            // Legacy behaviour: width drives both dimensions and the height/depth axes are
            // (Depth, Height) as in the original implementation.
            const auto width = m_options.width;
            const auto height = m_options.height;
            const auto depth = m_options.depth;

            auto terrainData = workphone::make_ptr<CProceduralTerrain>();
            m_terrain = terrainData;

            terrainData->setHeightmapResolution( Vector2I( width + 1, height + 1 ) );
            terrainData->setAlphamapResolution( Vector2I( width, height ) );
            terrainData->setDetailResolution( Vector2I( width, height ) );
            terrainData->setDetailResolutionPerPatch( 8 );
            terrainData->setSize( Vector3F( static_cast<f32>( width ), static_cast<f32>( depth ),
                                            static_cast<f32>( height ) ) );

            Array<Array<real_Num>> falloff;
            if( m_options.useFalloffMap )
            {
                FalloffGenerator<real_Num> falloffGenerator;
                falloffGenerator.setFalloffDirection(
                    static_cast<real_Num>( m_options.falloffDirection ) );
                falloffGenerator.setFalloffRange( static_cast<real_Num>( m_options.falloffRange ) );
                falloffGenerator.setSize( width );
                falloff = falloffGenerator.generate();
            }

            auto noiseMap = generateNoise( falloff );

            Array<f32> heightData;
            heightData.reserve( width * height );

            for( auto x = 0; x < noiseMap.size(); ++x )
            {
                for( auto y = 0; y < noiseMap[x].size(); ++y )
                {
                    heightData.push_back( static_cast<f32>( noiseMap[x][y] ) );
                }
            }

            terrainData->setHeightData( heightData );
            scene->addTerrain( terrainData );
        }

        Array<Array<real_Num>> CTerrainGeneratorDefault::generateNoise(
            const Array<Array<real_Num>> &falloffMap )
        {
            const auto width = m_options.width;
            const auto height = m_options.height;
            const auto size = std::max( width, height );

            LinearSpline1<real_Num> heightCurve;
            auto points = Array<real_Num>( { 0.0, 1.0 } );
            heightCurve.setPoints( points );

            real_Num maxLocalNoiseHeight = 1000.0f;
            real_Num minLocalNoiseHeight = 0.0f;

            PerlinNoiseGenerator<real_Num> perlinNoiseGenerator;
            perlinNoiseGenerator.setSize( size );
            perlinNoiseGenerator.setOctaves( m_options.octaves );
            perlinNoiseGenerator.setScale( static_cast<real_Num>( m_options.scale ) );
            perlinNoiseGenerator.setOffset( static_cast<real_Num>( m_options.offset ) );
            perlinNoiseGenerator.setPersistance( static_cast<real_Num>( m_options.persistence ) );
            perlinNoiseGenerator.setLacunarity( static_cast<real_Num>( m_options.lacunarity ) );

            Array<Array<real_Num>> noiseMap =
                perlinNoiseGenerator.generate( maxLocalNoiseHeight, minLocalNoiseHeight );

            if( noiseMap.empty() )
            {
                return noiseMap;
            }

            for( int y = 0; y < height; ++y )
            {
                for( int x = 0; x < width; ++x )
                {
                    auto value = noiseMap[x][y];

                    auto lerp =
                        Math<real_Num>::inverseLerp( minLocalNoiseHeight, maxLocalNoiseHeight, value );

                    if( !falloffMap.empty() )
                    {
                        lerp -= falloffMap[x][y];
                    }

                    noiseMap[x][y] = lerp;
                }
            }

            return noiseMap;
        }
    }  // namespace procedural
}  // namespace workphone
