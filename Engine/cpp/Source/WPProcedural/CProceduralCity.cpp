#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/Area.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Workphone.hpp>

#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr real_Num s_minSize = 1.0;
            constexpr real_Num s_minCenterRadius = 1.0;
            constexpr u32 s_minMaxCenters = 0;
            constexpr u32 s_minMaxBlocks = 0;
        }  // namespace

        CProceduralCity::CProceduralCity()
        {
            m_options.size = Vector2<real_Num>( 500, 500 );
        }

        CProceduralCity::~CProceduralCity() = default;

        void CProceduralCity::load( SmartPtr<ISharedObject> data )
        {
            WP_UNUSED( data );

            // Original OSM loading path is currently disabled. When OSM data is
            // reintroduced, this is the hook to parse bounds and derive size/latLong.
        }

        void CProceduralCity::unload( SmartPtr<ISharedObject> data )
        {
            WP_UNUSED( data );

            try
            {
                if( m_roadNetwork )
                {
                    m_roadNetwork->unload( nullptr );
                    m_roadNetwork = nullptr;
                }

                m_area = nullptr;
                m_cityCenters.clear();
                m_blocks.clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CProceduralCity::validate()
        {
            m_options.size.X() = Math<real_Num>::max( s_minSize, m_options.size.X() );
            m_options.size.Y() = Math<real_Num>::max( s_minSize, m_options.size.Y() );
            m_options.defaultCenterRadius =
                Math<real_Num>::max( s_minCenterRadius, m_options.defaultCenterRadius );
            m_options.maxCenters = std::max( s_minMaxCenters, m_options.maxCenters );
            m_options.maxBlocks = std::max( s_minMaxBlocks, m_options.maxBlocks );
        }

        SmartPtr<IRoadNetwork> CProceduralCity::getRoadNetwork() const
        {
            return m_roadNetwork;
        }

        void CProceduralCity::setRoadNetwork( SmartPtr<IRoadNetwork> value )
        {
            m_roadNetwork = value;
        }

        SmartPtr<ICityMap> CProceduralCity::getArea() const
        {
            return m_area;
        }

        void CProceduralCity::setArea( SmartPtr<ICityMap> value )
        {
            m_area = value;
        }

        bool CProceduralCity::isWithin( const Sphere3F &sphere ) const
        {
            const auto halfSize = Vector3F( static_cast<f32>( m_options.size.X() ) * 0.5f, 0.0f,
                                            static_cast<f32>( m_options.size.Y() ) * 0.5f );
            const auto center = Vector3F( halfSize.X(), 0.0f, halfSize.Z() );
            const auto radius = halfSize.length();

            Sphere3F bounds;
            bounds.setCenter( center );
            bounds.setRadius( radius );

            return bounds.intersects( sphere );
        }

        Array<SmartPtr<IProceduralCityCenter>> CProceduralCity::getCityCenters() const
        {
            return m_cityCenters;
        }

        void CProceduralCity::setCityCenters( Array<SmartPtr<IProceduralCityCenter>> cityCenters )
        {
            m_cityCenters = std::move( cityCenters );
        }

        void CProceduralCity::addCenter( SmartPtr<IProceduralCityCenter> center )
        {
            if( !center || m_cityCenters.size() >= m_options.maxCenters )
            {
                return;
            }

            m_cityCenters.push_back( center );
        }

        void CProceduralCity::removeCenter( SmartPtr<IProceduralCityCenter> center )
        {
            if( !center )
            {
                return;
            }

            auto it = std::find( m_cityCenters.begin(), m_cityCenters.end(), center );
            if( it != m_cityCenters.end() )
            {
                m_cityCenters.erase( it );
            }
        }

        void CProceduralCity::addBlock( SmartPtr<ICityBlock> block )
        {
            if( !block || m_blocks.size() >= m_options.maxBlocks )
            {
                return;
            }

            m_blocks.push_back( block );
        }

        void CProceduralCity::removeBlock( SmartPtr<ICityBlock> block )
        {
            if( !block )
            {
                return;
            }

            auto it = std::find( m_blocks.begin(), m_blocks.end(), block );
            if( it != m_blocks.end() )
            {
                m_blocks.erase( it );
            }
        }

        Array<SmartPtr<ICityBlock>> CProceduralCity::getBlocks() const
        {
            return m_blocks;
        }

        Vector2<real_Num> CProceduralCity::getSize() const
        {
            return m_options.size;
        }

        void CProceduralCity::setSize( const Vector2<real_Num> &value )
        {
            m_options.size = value;
            validate();
        }

        Vector2<real_Num> CProceduralCity::getRelativeCoordinates( const Vector2<real_Num> &lat_long )
        {
            const auto latRange = m_options.maxLatLong[0] - m_options.minLatLong[0];
            const auto lonRange = m_options.maxLatLong[1] - m_options.minLatLong[1];

            if( Math<real_Num>::equals( latRange, 0.0 ) || Math<real_Num>::equals( lonRange, 0.0 ) )
            {
                return Vector2<real_Num>::zero();
            }

            const auto rel_lat =
                ( lat_long[0] - m_options.minLatLong[0] ) / latRange * m_options.size[0];
            const auto rel_lon =
                ( lat_long[1] - m_options.minLatLong[1] ) / lonRange * m_options.size[1];

            return Vector2<real_Num>( rel_lat, rel_lon );
        }

        SmartPtr<IRoad> CProceduralCity::getRoadByName( const String &name )
        {
            WP_ASSERT( m_roadNetwork );

            if( m_roadNetwork )
            {
                auto roads = m_roadNetwork->getRoads();
                for( auto road : roads )
                {
                    if( road )
                    {
                        auto roadName = road->getName();
                        if( roadName == name )
                        {
                            return road;
                        }
                    }
                }
            }

            return nullptr;
        }

        Array<SmartPtr<IRoad>> CProceduralCity::getRoads() const
        {
            WP_ASSERT( m_roadNetwork );

            if( m_roadNetwork )
            {
                return m_roadNetwork->getRoads();
            }

            return Array<SmartPtr<IRoad>>();
        }

        Vector2<real_Num> CProceduralCity::getMinLatLong() const
        {
            return m_options.minLatLong;
        }

        void CProceduralCity::setMinLatLong( const Vector2<real_Num> &minLatLong )
        {
            m_options.minLatLong = minLatLong;
        }

        Vector2<real_Num> CProceduralCity::getMaxLatLong() const
        {
            return m_options.maxLatLong;
        }

        void CProceduralCity::setMaxLatLong( const Vector2<real_Num> &maxLatLong )
        {
            m_options.maxLatLong = maxLatLong;
        }

        const SCityOptions &CProceduralCity::getOptions() const
        {
            return m_options;
        }

        void CProceduralCity::setOptions( const SCityOptions &options )
        {
            m_options = options;
            validate();
        }

        void CProceduralCity::resetToDefaults()
        {
            m_options = SCityOptions();
            m_options.size = Vector2<real_Num>( 500, 500 );
        }

        String CProceduralCity::getName() const
        {
            return m_options.name;
        }

        void CProceduralCity::setName( const String &name )
        {
            m_options.name = name;
        }

        real_Num CProceduralCity::getDefaultCenterRadius() const
        {
            return m_options.defaultCenterRadius;
        }

        void CProceduralCity::setDefaultCenterRadius( real_Num radius )
        {
            m_options.defaultCenterRadius = radius;
            validate();
        }

        u32 CProceduralCity::getMaxCenters() const
        {
            return m_options.maxCenters;
        }

        void CProceduralCity::setMaxCenters( u32 maxCenters )
        {
            m_options.maxCenters = maxCenters;
            validate();
        }

        u32 CProceduralCity::getMaxBlocks() const
        {
            return m_options.maxBlocks;
        }

        void CProceduralCity::setMaxBlocks( u32 maxBlocks )
        {
            m_options.maxBlocks = maxBlocks;
            validate();
        }

        bool CProceduralCity::getAutoBuildNetwork() const
        {
            return m_options.autoBuildNetwork;
        }

        void CProceduralCity::setAutoBuildNetwork( bool autoBuild )
        {
            m_options.autoBuildNetwork = autoBuild;
        }

        void CProceduralCity::loadOptions( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }

            SCityOptions options;

            String name;
            if( properties->getPropertyValue( "Name", name ) )
            {
                options.name = name;
            }

            Vector2F size;
            if( properties->getPropertyValue( "Size", size ) )
            {
                options.size = Vector2<real_Num>( size.X(), size.Y() );
            }

            Vector2F minLatLong;
            if( properties->getPropertyValue( "MinLatLong", minLatLong ) )
            {
                options.minLatLong = Vector2<real_Num>( minLatLong.X(), minLatLong.Y() );
            }

            Vector2F maxLatLong;
            if( properties->getPropertyValue( "MaxLatLong", maxLatLong ) )
            {
                options.maxLatLong = Vector2<real_Num>( maxLatLong.X(), maxLatLong.Y() );
            }

            options.defaultCenterRadius = properties->getPropertyAsFloat(
                "DefaultCenterRadius", static_cast<f32>( options.defaultCenterRadius ) );
            options.maxCenters = static_cast<u32>(
                properties->getPropertyAsInt( "MaxCenters", static_cast<s32>( options.maxCenters ) ) );
            options.maxBlocks = static_cast<u32>(
                properties->getPropertyAsInt( "MaxBlocks", static_cast<s32>( options.maxBlocks ) ) );
            options.autoBuildNetwork =
                properties->getPropertyAsBool( "AutoBuildNetwork", options.autoBuildNetwork );

            setOptions( options );
        }

        void CProceduralCity::saveOptions( SmartPtr<Properties> properties ) const
        {
            if( !properties )
            {
                return;
            }

            properties->setProperty( "Name", m_options.name );
            properties->setProperty( "Size", Vector2F( static_cast<f32>( m_options.size.X() ),
                                                       static_cast<f32>( m_options.size.Y() ) ) );
            properties->setProperty( "MinLatLong",
                                     Vector2F( static_cast<f32>( m_options.minLatLong.X() ),
                                               static_cast<f32>( m_options.minLatLong.Y() ) ) );
            properties->setProperty( "MaxLatLong",
                                     Vector2F( static_cast<f32>( m_options.maxLatLong.X() ),
                                               static_cast<f32>( m_options.maxLatLong.Y() ) ) );
            properties->setProperty( "DefaultCenterRadius",
                                     static_cast<f32>( m_options.defaultCenterRadius ) );
            properties->setProperty( "MaxCenters", static_cast<s32>( m_options.maxCenters ) );
            properties->setProperty( "MaxBlocks", static_cast<s32>( m_options.maxBlocks ) );
            properties->setProperty( "AutoBuildNetwork", m_options.autoBuildNetwork );
        }
    }  // end namespace procedural
}  // namespace workphone
