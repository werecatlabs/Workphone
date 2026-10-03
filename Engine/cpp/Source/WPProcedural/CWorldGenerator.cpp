#include "WPProcedural/WPProceduralPCH.hpp"
#include <WPProcedural/CWorldGenerator.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CTerrainGenerator.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/Math.hpp>

#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr int s_minDimension = 1;
            constexpr int s_minChunkSize = 1;
            constexpr int s_minOctaves = 0;
            constexpr float s_minScale = 0.0001f;
            constexpr float s_minLacunarity = 1.0f;
            constexpr double s_randomSeedMax = 9999.0;
        }  // namespace

        CWorldGenerator::CWorldGenerator() = default;

        CWorldGenerator::~CWorldGenerator() = default;

        void CWorldGenerator::unload( SmartPtr<ISharedObject> data )
        {
            WP_UNUSED( data );
            clear();
        }

        SmartPtr<IProceduralWorld> CWorldGenerator::getProceduralWorld() const
        {
            return m_world;
        }

        void CWorldGenerator::setProceduralWorld( SmartPtr<IProceduralWorld> proceduralWorld )
        {
            m_world = proceduralWorld;
        }

        const Array<SmartPtr<IProceduralScene>> CWorldGenerator::getScenes() const
        {
            return m_scenes;
        }

        void CWorldGenerator::addScene( SmartPtr<IProceduralScene> scene )
        {
            if( !scene )
            {
                return;
            }

            m_scenes.push_back( scene );

            if( m_world )
            {
                m_world->addScene( scene );
            }

            onOptionChanged();
        }

        void CWorldGenerator::removeScene( SmartPtr<IProceduralScene> scene )
        {
            if( !scene )
            {
                return;
            }

            auto it = std::find( m_scenes.begin(), m_scenes.end(), scene );
            if( it != m_scenes.end() )
            {
                m_scenes.erase( it );
            }

            if( m_world )
            {
                m_world->removeScene( scene );
            }
        }

        void CWorldGenerator::setScenes( Array<SmartPtr<IProceduralScene>> scenes )
        {
            m_scenes = std::move( scenes );

            if( m_world )
            {
                // Re-synchronise world scene list.
                for( auto &scene : m_scenes )
                {
                    m_world->addScene( scene );
                }
            }
        }

        void CWorldGenerator::generate()
        {
            m_finished = false;

            validate();

            if( m_options.randomizeSeed )
            {
                m_options.seed =
                    static_cast<int>( Math<real_Num>::RangedRandom( 0.0, s_randomSeedMax ) );
            }

            m_isGenerating = true;
            try
            {
                generateWorld();
            }
            catch( ... )
            {
                m_isGenerating = false;
                throw;
            }
            m_isGenerating = false;
            m_finished = true;
        }

        void CWorldGenerator::validate()
        {
            m_options.worldWidth = std::max( s_minDimension, m_options.worldWidth );
            m_options.worldHeight = std::max( s_minDimension, m_options.worldHeight );
            m_options.worldDepth = std::max( s_minDimension, m_options.worldDepth );
            m_options.chunkSize = std::max( s_minChunkSize, m_options.chunkSize );

            m_options.scale = std::max( s_minScale, m_options.scale );
            m_options.lacunarity = std::max( s_minLacunarity, m_options.lacunarity );
            m_options.persistence =
                Math<real_Num>::max( 0.0f, Math<real_Num>::min( 1.0f, m_options.persistence ) );

            m_options.seaLevel =
                Math<real_Num>::max( 0.0f, Math<real_Num>::min( 1.0f, m_options.seaLevel ) );
            m_options.mountainLevel = Math<real_Num>::max(
                m_options.seaLevel, Math<real_Num>::min( 1.0f, m_options.mountainLevel ) );
        }

        void CWorldGenerator::clear()
        {
            resetToDefaults();
            m_world = nullptr;
            m_scenes.clear();
            m_finished = false;
            m_isGenerating = false;
        }

        bool CWorldGenerator::isFinished() const
        {
            return m_finished;
        }

        const SWorldGeneratorOptions &CWorldGenerator::getOptions() const
        {
            return m_options;
        }

        void CWorldGenerator::setOptions( const SWorldGeneratorOptions &options )
        {
            m_options = options;
            validate();
            onOptionChanged();
        }

        void CWorldGenerator::resetToDefaults()
        {
            m_options = SWorldGeneratorOptions();
        }

        int CWorldGenerator::getWorldWidth() const
        {
            return m_options.worldWidth;
        }

        void CWorldGenerator::setWorldWidth( int width )
        {
            m_options.worldWidth = width;
            onOptionChanged();
        }

        int CWorldGenerator::getWorldHeight() const
        {
            return m_options.worldHeight;
        }

        void CWorldGenerator::setWorldHeight( int height )
        {
            m_options.worldHeight = height;
            onOptionChanged();
        }

        int CWorldGenerator::getWorldDepth() const
        {
            return m_options.worldDepth;
        }

        void CWorldGenerator::setWorldDepth( int depth )
        {
            m_options.worldDepth = depth;
            onOptionChanged();
        }

        int CWorldGenerator::getChunkSize() const
        {
            return m_options.chunkSize;
        }

        void CWorldGenerator::setChunkSize( int size )
        {
            m_options.chunkSize = size;
            onOptionChanged();
        }

        int CWorldGenerator::getSeed() const
        {
            return m_options.seed;
        }

        void CWorldGenerator::setSeed( int seed )
        {
            m_options.seed = seed;
            onOptionChanged();
        }

        float CWorldGenerator::getScale() const
        {
            return m_options.scale;
        }

        void CWorldGenerator::setScale( float scale )
        {
            m_options.scale = scale;
            onOptionChanged();
        }

        float CWorldGenerator::getLacunarity() const
        {
            return m_options.lacunarity;
        }

        void CWorldGenerator::setLacunarity( float lacunarity )
        {
            m_options.lacunarity = lacunarity;
            onOptionChanged();
        }

        float CWorldGenerator::getPersistence() const
        {
            return m_options.persistence;
        }

        void CWorldGenerator::setPersistence( float persistence )
        {
            m_options.persistence = persistence;
            onOptionChanged();
        }

        float CWorldGenerator::getSeaLevel() const
        {
            return m_options.seaLevel;
        }

        void CWorldGenerator::setSeaLevel( float seaLevel )
        {
            m_options.seaLevel = seaLevel;
            validate();
            onOptionChanged();
        }

        float CWorldGenerator::getMountainLevel() const
        {
            return m_options.mountainLevel;
        }

        void CWorldGenerator::setMountainLevel( float mountainLevel )
        {
            m_options.mountainLevel = mountainLevel;
            validate();
            onOptionChanged();
        }
        int CWorldGenerator::getOctaves() const
        {
            return m_options.octaves;
        }

        void CWorldGenerator::setOctaves( int octaves )
        {
            m_options.octaves = octaves;
            onOptionChanged();
        }

        bool CWorldGenerator::getRandomizeSeed() const
        {
            return m_options.randomizeSeed;
        }

        void CWorldGenerator::setRandomizeSeed( bool randomize )
        {
            m_options.randomizeSeed = randomize;
        }

        bool CWorldGenerator::getAutoUpdate() const
        {
            return m_options.autoUpdate;
        }

        void CWorldGenerator::setAutoUpdate( bool autoUpdate )
        {
            m_options.autoUpdate = autoUpdate;
        }

        bool CWorldGenerator::getGenerateTerrain() const
        {
            return m_options.generateTerrain;
        }

        void CWorldGenerator::setGenerateTerrain( bool generate )
        {
            m_options.generateTerrain = generate;
            onOptionChanged();
        }

        bool CWorldGenerator::getGenerateCities() const
        {
            return m_options.generateCities;
        }

        void CWorldGenerator::setGenerateCities( bool generate )
        {
            m_options.generateCities = generate;
            onOptionChanged();
        }

        bool CWorldGenerator::getGenerateRoads() const
        {
            return m_options.generateRoads;
        }

        void CWorldGenerator::setGenerateRoads( bool generate )
        {
            m_options.generateRoads = generate;
            onOptionChanged();
        }

        bool CWorldGenerator::getGenerateVegetation() const
        {
            return m_options.generateVegetation;
        }

        void CWorldGenerator::setGenerateVegetation( bool generate )
        {
            m_options.generateVegetation = generate;
            onOptionChanged();
        }

        void CWorldGenerator::loadOptions( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }

            SWorldGeneratorOptions options;

            options.worldWidth = properties->getPropertyAsInt( "WorldWidth", options.worldWidth );
            options.worldHeight = properties->getPropertyAsInt( "WorldHeight", options.worldHeight );
            options.worldDepth = properties->getPropertyAsInt( "WorldDepth", options.worldDepth );
            options.chunkSize = properties->getPropertyAsInt( "ChunkSize", options.chunkSize );
            options.seed = properties->getPropertyAsInt( "Seed", options.seed );

            options.scale = properties->getPropertyAsFloat( "Scale", options.scale );
            options.lacunarity = properties->getPropertyAsFloat( "Lacunarity", options.lacunarity );
            options.persistence = properties->getPropertyAsFloat( "Persistence", options.persistence );
            options.seaLevel = properties->getPropertyAsFloat( "SeaLevel", options.seaLevel );
            options.mountainLevel =
                properties->getPropertyAsFloat( "MountainLevel", options.mountainLevel );
            options.octaves = properties->getPropertyAsInt( "Octaves", options.octaves );

            options.randomizeSeed =
                properties->getPropertyAsBool( "RandomizeSeed", options.randomizeSeed );
            options.autoUpdate = properties->getPropertyAsBool( "AutoUpdate", options.autoUpdate );
            options.generateTerrain =
                properties->getPropertyAsBool( "GenerateTerrain", options.generateTerrain );
            options.generateCities =
                properties->getPropertyAsBool( "GenerateCities", options.generateCities );
            options.generateRoads =
                properties->getPropertyAsBool( "GenerateRoads", options.generateRoads );
            options.generateVegetation =
                properties->getPropertyAsBool( "GenerateVegetation", options.generateVegetation );

            setOptions( options );
        }

        void CWorldGenerator::saveOptions( SmartPtr<Properties> properties ) const
        {
            if( !properties )
            {
                return;
            }

            properties->setProperty( "WorldWidth", m_options.worldWidth );
            properties->setProperty( "WorldHeight", m_options.worldHeight );
            properties->setProperty( "WorldDepth", m_options.worldDepth );
            properties->setProperty( "ChunkSize", m_options.chunkSize );
            properties->setProperty( "Seed", m_options.seed );

            properties->setProperty( "Scale", m_options.scale );
            properties->setProperty( "Lacunarity", m_options.lacunarity );
            properties->setProperty( "Persistence", m_options.persistence );
            properties->setProperty( "SeaLevel", m_options.seaLevel );
            properties->setProperty( "MountainLevel", m_options.mountainLevel );
            properties->setProperty( "Octaves", m_options.octaves );

            properties->setProperty( "RandomizeSeed", m_options.randomizeSeed );
            properties->setProperty( "AutoUpdate", m_options.autoUpdate );
            properties->setProperty( "GenerateTerrain", m_options.generateTerrain );
            properties->setProperty( "GenerateCities", m_options.generateCities );
            properties->setProperty( "GenerateRoads", m_options.generateRoads );
            properties->setProperty( "GenerateVegetation", m_options.generateVegetation );
        }

        void CWorldGenerator::onOptionChanged()
        {
            if( m_options.autoUpdate && !m_isGenerating )
            {
                generate();
            }
        }

        void CWorldGenerator::generateWorld()
        {
            if( !m_world )
            {
                m_world = workphone::make_ptr<CProceduralWorld>();
            }

            if( m_scenes.empty() )
            {
                addScene( createDefaultScene() );
            }

            generateScenes();
        }

        void CWorldGenerator::generateScenes()
        {
            if( !m_options.generateTerrain )
            {
                return;
            }

            const auto chunkWidth = m_options.chunkSize;
            const auto chunkHeight = m_options.chunkSize;
            const auto chunksX = std::max( 1, m_options.worldWidth / chunkWidth );
            const auto chunksY = std::max( 1, m_options.worldHeight / chunkHeight );

            for( auto scene : m_scenes )
            {
                if( !scene )
                {
                    continue;
                }

                for( int cy = 0; cy < chunksY; ++cy )
                {
                    for( int cx = 0; cx < chunksX; ++cx )
                    {
                        auto terrainGenerator = workphone::make_ptr<CTerrainGenerator>();
                        terrainGenerator->setProceduralScene( scene );
                        terrainGenerator->setWidth( chunkWidth );
                        terrainGenerator->setHeight( chunkHeight );
                        terrainGenerator->setDepth( m_options.worldDepth );
                        terrainGenerator->setOctaves( m_options.octaves );
                        terrainGenerator->setScale( m_options.scale );
                        terrainGenerator->setLacunarity( m_options.lacunarity );
                        terrainGenerator->setPersistence( m_options.persistence );
                        terrainGenerator->setOffset( static_cast<f32>( m_options.seed ) );
                        terrainGenerator->setSeed( m_options.seed + cx + cy * chunksX );
                        terrainGenerator->setRandomize( false );
                        terrainGenerator->generate();
                    }
                }
            }

            // Placeholders for future feature toggles. Right now they just
            // express intent in the data-driven options struct.
            if( m_options.generateRoads )
            {
                // TODO: invoke a road generator once one is wired in.
            }

            if( m_options.generateCities )
            {
                // TODO: invoke a city generator once one is wired in.
            }

            if( m_options.generateVegetation )
            {
                // TODO: invoke a vegetation generator once one is wired in.
            }
        }

        SmartPtr<IProceduralScene> CWorldGenerator::createDefaultScene()
        {
            return workphone::make_ptr<CProceduralScene>();
        }
    }  // end namespace procedural
}  // namespace workphone
