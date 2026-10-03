#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CTerrainGenerator.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>>
#include <Workphone/Math/FalloffGenerator.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

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

        CTerrainGenerator::CTerrainGenerator() = default;

        CTerrainGenerator::~CTerrainGenerator() = default;

        void CTerrainGenerator::unload( SmartPtr<ISharedObject> data )
        {
            clear();
        }

        SmartPtr<IProceduralScene> CTerrainGenerator::getProceduralScene() const
        {
            return m_scene;
        }

        void CTerrainGenerator::setProceduralScene( SmartPtr<IProceduralScene> scene )
        {
            m_scene = scene;
        }

        void CTerrainGenerator::generate()
        {
            m_finished = false;
            m_terrain = nullptr;

            validate();

            if( m_options.randomize )
            {
                m_options.offset =
                    static_cast<float>( Math<real_Num>::RangedRandom( 0.0, s_randomOffsetMax ) );
                m_options.seed =
                    static_cast<int>( Math<real_Num>::RangedRandom( 0.0, s_randomOffsetMax ) );
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

        int CTerrainGenerator::getWidth() const
        {
            return m_options.width;
        }

        void CTerrainGenerator::setWidth( int width )
        {
            m_options.width = width;
            onOptionChanged();
        }

        int CTerrainGenerator::getHeight() const
        {
            return m_options.height;
        }

        void CTerrainGenerator::setHeight( int height )
        {
            m_options.height = height;
            onOptionChanged();
        }

        int CTerrainGenerator::getDepth() const
        {
            return m_options.depth;
        }

        void CTerrainGenerator::setDepth( int depth )
        {
            m_options.depth = depth;
            onOptionChanged();
        }

        int CTerrainGenerator::getOctaves() const
        {
            return m_options.octaves;
        }

        void CTerrainGenerator::setOctaves( int octaves )
        {
            m_options.octaves = octaves;
            onOptionChanged();
        }

        float CTerrainGenerator::getScale() const
        {
            return m_options.scale;
        }

        void CTerrainGenerator::setScale( float scale )
        {
            m_options.scale = scale;
            onOptionChanged();
        }

        float CTerrainGenerator::getLacunarity() const
        {
            return m_options.lacunarity;
        }

        void CTerrainGenerator::setLacunarity( float lacunarity )
        {
            m_options.lacunarity = lacunarity;
            onOptionChanged();
        }

        float CTerrainGenerator::getPersistence() const
        {
            return m_options.persistence;
        }

        void CTerrainGenerator::setPersistence( float persistence )
        {
            m_options.persistence = persistence;
            onOptionChanged();
        }

        float CTerrainGenerator::getOffset() const
        {
            return m_options.offset;
        }

        void CTerrainGenerator::setOffset( float offset )
        {
            m_options.offset = offset;
            onOptionChanged();
        }

        float CTerrainGenerator::getFalloffDirection() const
        {
            return m_options.falloffDirection;
        }

        void CTerrainGenerator::setFalloffDirection( float direction )
        {
            m_options.falloffDirection = direction;
            onOptionChanged();
        }

        float CTerrainGenerator::getFalloffRange() const
        {
            return m_options.falloffRange;
        }

        void CTerrainGenerator::setFalloffRange( float range )
        {
            m_options.falloffRange = range;
            onOptionChanged();
        }

        bool CTerrainGenerator::getUseFalloffMap() const
        {
            return m_options.useFalloffMap;
        }

        void CTerrainGenerator::setUseFalloffMap( bool useFalloff )
        {
            m_options.useFalloffMap = useFalloff;
            onOptionChanged();
        }

        bool CTerrainGenerator::getRandomize() const
        {
            return m_options.randomize;
        }

        void CTerrainGenerator::setRandomize( bool randomize )
        {
            m_options.randomize = randomize;
            onOptionChanged();
        }

        bool CTerrainGenerator::getAutoUpdate() const
        {
            return m_options.autoUpdate;
        }

        void CTerrainGenerator::setAutoUpdate( bool autoUpdate )
        {
            m_options.autoUpdate = autoUpdate;
        }

        void CTerrainGenerator::validate()
        {
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

        void CTerrainGenerator::clear()
        {
            resetToDefaults();
            m_scene = nullptr;
            m_terrain = nullptr;
            m_finished = false;
            m_isGenerating = false;
        }

        SmartPtr<IProceduralTerrain> CTerrainGenerator::getTerrain() const
        {
            return m_terrain;
        }

        void CTerrainGenerator::setTerrain( SmartPtr<IProceduralTerrain> terrain )
        {
            m_terrain = terrain;
        }

        bool CTerrainGenerator::isFinished() const
        {
            return m_finished;
        }

        const STerrainGeneratorOptions &CTerrainGenerator::getOptions() const
        {
            return m_options;
        }

        void CTerrainGenerator::setOptions( const STerrainGeneratorOptions &options )
        {
            m_options = options;
            validate();
            onOptionChanged();
        }

        void CTerrainGenerator::resetToDefaults()
        {
            m_options = STerrainGeneratorOptions();
        }

        int CTerrainGenerator::getSeed() const
        {
            return m_options.seed;
        }

        void CTerrainGenerator::setSeed( int seed )
        {
            m_options.seed = seed;
            onOptionChanged();
        }

        bool CTerrainGenerator::getNormalize() const
        {
            return m_options.normalize;
        }

        void CTerrainGenerator::setNormalize( bool normalize )
        {
            m_options.normalize = normalize;
            onOptionChanged();
        }

        SmartPtr<LinearSpline1<real_Num>> CTerrainGenerator::getHeightCurve() const
        {
            return m_options.heightCurve;
        }

        void CTerrainGenerator::setHeightCurve( SmartPtr<LinearSpline1<real_Num>> curve )
        {
            m_options.heightCurve = curve;
            onOptionChanged();
        }

        void CTerrainGenerator::loadOptions( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }

            STerrainGeneratorOptions options;

            options.width = properties->getPropertyAsInt( "Width", options.width );
            options.height = properties->getPropertyAsInt( "Height", options.height );
            options.depth = properties->getPropertyAsInt( "Depth", options.depth );
            options.octaves = properties->getPropertyAsInt( "Octaves", options.octaves );
            options.seed = properties->getPropertyAsInt( "Seed", options.seed );

            options.scale = properties->getPropertyAsFloat( "Scale", options.scale );
            options.lacunarity = properties->getPropertyAsFloat( "Lacunarity", options.lacunarity );
            options.persistence = properties->getPropertyAsFloat( "Persistence", options.persistence );
            options.offset = properties->getPropertyAsFloat( "Offset", options.offset );
            options.falloffDirection =
                properties->getPropertyAsFloat( "FalloffDirection", options.falloffDirection );
            options.falloffRange =
                properties->getPropertyAsFloat( "FalloffRange", options.falloffRange );

            options.useFalloffMap =
                properties->getPropertyAsBool( "UseFalloffMap", options.useFalloffMap );
            options.randomize = properties->getPropertyAsBool( "Randomize", options.randomize );
            options.autoUpdate = properties->getPropertyAsBool( "AutoUpdate", options.autoUpdate );
            options.normalize = properties->getPropertyAsBool( "Normalize", options.normalize );

            try
            {
                String curveString;
                if( properties->getPropertyValue( "HeightCurve", curveString ) )
                {
                    Array<f32> curveValues;
                    StringUtil::parseArray( curveString, curveValues );
                    if( !curveValues.empty() )
                    {
                        Array<real_Num> points;
                        points.reserve( curveValues.size() );
                        for( auto value : curveValues )
                        {
                            points.push_back( static_cast<real_Num>( value ) );
                        }
                        options.heightCurve = workphone::make_ptr<LinearSpline1<real_Num>>();
                        options.heightCurve->setPoints( points );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            setOptions( options );
        }

        void CTerrainGenerator::saveOptions( SmartPtr<Properties> properties ) const
        {
            if( !properties )
            {
                return;
            }

            properties->setProperty( "Width", m_options.width );
            properties->setProperty( "Height", m_options.height );
            properties->setProperty( "Depth", m_options.depth );
            properties->setProperty( "Octaves", m_options.octaves );
            properties->setProperty( "Seed", m_options.seed );

            properties->setProperty( "Scale", m_options.scale );
            properties->setProperty( "Lacunarity", m_options.lacunarity );
            properties->setProperty( "Persistence", m_options.persistence );
            properties->setProperty( "Offset", m_options.offset );
            properties->setProperty( "FalloffDirection", m_options.falloffDirection );
            properties->setProperty( "FalloffRange", m_options.falloffRange );

            properties->setProperty( "UseFalloffMap", m_options.useFalloffMap );
            properties->setProperty( "Randomize", m_options.randomize );
            properties->setProperty( "AutoUpdate", m_options.autoUpdate );
            properties->setProperty( "Normalize", m_options.normalize );

            const auto pointCount = m_options.heightCurve ? m_options.heightCurve->getNumPoints() : 0;
            if( pointCount > 0 )
            {
                auto curve = m_options.heightCurve;
                Array<f32> curveValues;
                curveValues.reserve( pointCount );

                for( u32 i = 0; i < pointCount; ++i )
                {
                    const auto t = static_cast<real_Num>( i ) / static_cast<real_Num>( pointCount - 1 );
                    curveValues.push_back( static_cast<f32>( curve->interpolate( t ) ) );
                }

                properties->setProperty( "HeightCurve", StringUtil::toString( curveValues ) );
            }
        }

        void CTerrainGenerator::generateRandom()
        {
            const bool wasRandomize = m_options.randomize;
            m_options.randomize = true;
            generate();
            m_options.randomize = wasRandomize;
        }

        void CTerrainGenerator::generateTerrain()
        {
            auto scene = getProceduralScene();
            if( !scene )
            {
                return;
            }

            auto terrain = workphone::make_ptr<CProceduralTerrain>();
            m_terrain = terrain;

            const auto width = m_options.width;
            const auto height = m_options.height;
            const auto depth = m_options.depth;

            terrain->setHeightmapResolution( Vector2I( width + 1, height + 1 ) );
            terrain->setAlphamapResolution( Vector2I( width, height ) );
            terrain->setDetailResolution( Vector2I( width, height ) );
            terrain->setDetailResolutionPerPatch( 8 );
            terrain->setSize( Vector3F( static_cast<f32>( width ), static_cast<f32>( depth ),
                                        static_cast<f32>( height ) ) );

            Array<Array<real_Num>> falloff;
            if( m_options.useFalloffMap )
            {
                const auto noiseSize = std::max( width, height );
                FalloffGenerator<real_Num> falloffGenerator;
                falloffGenerator.setFalloffDirection(
                    static_cast<real_Num>( m_options.falloffDirection ) );
                falloffGenerator.setFalloffRange( static_cast<real_Num>( m_options.falloffRange ) );
                falloffGenerator.setSize( noiseSize );
                falloff = falloffGenerator.generate();
            }

            auto noiseMap = generateNoise( falloff );

            Array<f32> heightData;
            heightData.resize( width * height );

            for( int y = 0; y < height; ++y )
            {
                for( int x = 0; x < width; ++x )
                {
                    heightData[x + y * width] = static_cast<f32>( noiseMap[x][y] );
                }
            }

            terrain->setHeightData( heightData );
            scene->addTerrain( terrain );
        }

        Array<Array<real_Num>> CTerrainGenerator::generateNoise(
            const Array<Array<real_Num>> &falloffMap )
        {
            const auto width = m_options.width;
            const auto height = m_options.height;
            const auto size = std::max( width, height );

            PerlinNoiseGenerator<real_Num> noiseGenerator;
            noiseGenerator.setSize( size );
            noiseGenerator.setOctaves( m_options.octaves );
            noiseGenerator.setScale( static_cast<real_Num>( m_options.scale ) );
            noiseGenerator.setOffset( static_cast<real_Num>( m_options.offset + m_options.seed ) );
            noiseGenerator.setPersistance( static_cast<real_Num>( m_options.persistence ) );
            noiseGenerator.setLacunarity( static_cast<real_Num>( m_options.lacunarity ) );

            real_Num maxLocalNoiseHeight = 0.0;
            real_Num minLocalNoiseHeight = 0.0;
            auto noiseMap = noiseGenerator.generate( maxLocalNoiseHeight, minLocalNoiseHeight );

            if( noiseMap.empty() )
            {
                return noiseMap;
            }

            const bool applyFalloff = !falloffMap.empty();
            const bool applyCurve = m_options.heightCurve && m_options.heightCurve->getNumPoints() > 1;

            for( int y = 0; y < height; ++y )
            {
                for( int x = 0; x < width; ++x )
                {
                    auto value = noiseMap[x][y];

                    if( applyFalloff )
                    {
                        value -= falloffMap[x][y];
                    }

                    if( m_options.normalize && maxLocalNoiseHeight != minLocalNoiseHeight )
                    {
                        value = Math<real_Num>::inverseLerp( minLocalNoiseHeight, maxLocalNoiseHeight,
                                                             value );
                    }

                    if( applyCurve )
                    {
                        value = m_options.heightCurve->interpolate( value );
                    }

                    value = Math<real_Num>::max( 0.0, Math<real_Num>::min( 1.0, value ) );

                    noiseMap[x][y] = value;
                }
            }

            return noiseMap;
        }

        void CTerrainGenerator::onOptionChanged()
        {
            if( m_options.autoUpdate && !m_isGenerating )
            {
                generate();
            }
        }
    }  // end namespace procedural
}  // namespace workphone
