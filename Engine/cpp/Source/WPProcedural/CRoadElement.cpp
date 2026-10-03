#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadElement,
                                   CProceduralObject<IRoadElement> );

        // ---------------------------------------------------------------
        // Property key constants
        // ---------------------------------------------------------------

        static const char *const kPropName = "name";
        static const char *const kPropRoadType = "roadType";
        static const char *const kPropLaneType = "laneType";
        static const char *const kPropRoadWidth = "roadWidth";
        static const char *const kPropSpeedLimit = "speedLimit";
        static const char *const kPropReference = "reference";
        static const char *const kPropNumConnections = "numConnections";
        static const char *const kPropIsLit = "isLit";
        static const char *const kPropOneWay = "oneWay";
        static const char *const kPropIsBicycle = "isBicycle";
        static const char *const kPropIsFootway = "isFootway";
        static const char *const kPropNodeCount = "nodeCount";
        static const char *const kPropSidewalkCount = "sidewalkCount";

        // ---------------------------------------------------------------
        // Enum <-> string helpers (anonymous namespace, file-local)
        // ---------------------------------------------------------------

        namespace
        {
            String roadTypeToString( IRoad::RoadType type )
            {
                switch( type )
                {
                case IRoad::RoadType::Residential:
                    return "Residential";
                case IRoad::RoadType::Trunk:
                    return "Trunk";
                case IRoad::RoadType::Footway:
                    return "Footway";
                case IRoad::RoadType::Steps:
                    return "Steps";
                case IRoad::RoadType::None:
                default:
                    return "None";
                }
            }

            IRoad::RoadType roadTypeFromString( const String &str )
            {
                if( str == "Residential" )
                    return IRoad::RoadType::Residential;
                if( str == "Trunk" )
                    return IRoad::RoadType::Trunk;
                if( str == "Footway" )
                    return IRoad::RoadType::Footway;
                if( str == "Steps" )
                    return IRoad::RoadType::Steps;
                return IRoad::RoadType::None;
            }

            String laneTypeToString( IRoad::LaneType type )
            {
                switch( type )
                {
                case IRoad::LaneType::OneLane:
                    return "OneLane";
                case IRoad::LaneType::TwoLane:
                    return "TwoLane";
                case IRoad::LaneType::ThreeLane:
                    return "ThreeLane";
                case IRoad::LaneType::FourLane:
                    return "FourLane";
                case IRoad::LaneType::FiveLane:
                    return "FiveLane";
                case IRoad::LaneType::SixLane:
                    return "SixLane";
                default:
                    return "TwoLane";
                }
            }

            IRoad::LaneType laneTypeFromString( const String &str )
            {
                if( str == "OneLane" )
                    return IRoad::LaneType::OneLane;
                if( str == "ThreeLane" )
                    return IRoad::LaneType::ThreeLane;
                if( str == "FourLane" )
                    return IRoad::LaneType::FourLane;
                if( str == "FiveLane" )
                    return IRoad::LaneType::FiveLane;
                if( str == "SixLane" )
                    return IRoad::LaneType::SixLane;
                return IRoad::LaneType::TwoLane;
            }
        }  // anonymous namespace

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadElement::CRoadElement()
        {
        }

        CRoadElement::~CRoadElement()
        {
            unload( nullptr );
        }

        void CRoadElement::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                for( auto &node : m_sidewalks )
                {
                    if( node )
                    {
                        node->unload( nullptr );
                    }
                }

                m_sidewalks.clear();
                m_parentSection = nullptr;

                CProceduralObject<IRoadElement>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Build / bounds
        // ---------------------------------------------------------------

        void CRoadElement::build()
        {
            try
            {
                updateBounds();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadElement::updateBounds()
        {
            try
            {
                auto bounds = AABB3<real_Num>();
                bool hasMerge = false;

                auto roadNodes = getRoadNodes();
                for( auto &node : roadNodes )
                {
                    if( !node )
                    {
                        continue;
                    }

                    bounds.merge( node->getPosition() );
                    hasMerge = true;
                }

                for( auto &node : m_sidewalks )
                {
                    if( !node )
                    {
                        continue;
                    }

                    bounds.merge( node->getPosition() );
                    hasMerge = true;
                }

                if( hasMerge )
                {
                    m_Bounds = bounds;
                    auto center = m_Bounds.getCenter();
                    auto halfExtent = m_Bounds.getExtent().length();
                    m_BoundingSphere = Sphere3<real_Num>( center, halfExtent );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Parent section
        // ---------------------------------------------------------------

        SmartPtr<IRoadSection> CRoadElement::getParentSection() const
        {
            return m_parentSection;
        }

        void CRoadElement::setParentSection( SmartPtr<IRoadSection> value )
        {
            m_parentSection = value;
        }

        // ---------------------------------------------------------------
        // Road nodes
        // ---------------------------------------------------------------

        Array<SmartPtr<IRoadNode>> CRoadElement::getRoadNodes() const
        {
            auto nodes = getNodes();

            auto roadNodes = Array<SmartPtr<IRoadNode>>();
            roadNodes.reserve( nodes.size() );

            for( auto &node : nodes )
            {
                if( !node )
                {
                    continue;
                }

                auto roadNode = workphone::static_pointer_cast<IRoadNode>( node );
                if( roadNode )
                {
                    roadNodes.push_back( roadNode );
                }
            }

            return roadNodes;
        }

        // ---------------------------------------------------------------
        // Sidewalks
        // ---------------------------------------------------------------

        Array<SmartPtr<IRoadNode>> CRoadElement::getSidewalks() const
        {
            return m_sidewalks;
        }

        void CRoadElement::setSidewalks( Array<SmartPtr<IRoadNode>> value )
        {
            m_sidewalks.clear();
            m_sidewalks.reserve( value.size() );

            for( auto &node : value )
            {
                if( !node )
                {
                    WP_LOG_ERROR( "CRoadElement::setSidewalks - null node skipped." );
                    continue;
                }

                m_sidewalks.push_back( node );
            }
        }

        // ---------------------------------------------------------------
        // Road / lane type
        // ---------------------------------------------------------------

        IRoad::RoadType CRoadElement::getRoadType() const
        {
            return m_roadType;
        }

        void CRoadElement::setRoadType( IRoad::RoadType value )
        {
            if( value < IRoad::RoadType::None || value >= IRoad::RoadType::Count )
            {
                WP_LOG_ERROR( "CRoadElement::setRoadType - invalid value, defaulting to None." );
                m_roadType = IRoad::RoadType::None;
                return;
            }

            m_roadType = value;
        }

        IRoad::LaneType CRoadElement::getLaneType() const
        {
            return m_laneType;
        }

        void CRoadElement::setLaneType( IRoad::LaneType value )
        {
            if( value < IRoad::LaneType::OneLane || value > IRoad::LaneType::SixLane )
            {
                WP_LOG_ERROR( "CRoadElement::setLaneType - invalid value, defaulting to TwoLane." );
                m_laneType = IRoad::LaneType::TwoLane;
                return;
            }

            m_laneType = value;
        }

        // ---------------------------------------------------------------
        // Lighting
        // ---------------------------------------------------------------

        bool CRoadElement::isLit() const
        {
            return m_IsLit;
        }

        void CRoadElement::setIsLit( bool value )
        {
            m_IsLit = value;
        }

        // ---------------------------------------------------------------
        // Extended metadata
        // ---------------------------------------------------------------

        bool CRoadElement::isOneWay() const
        {
            return m_OneWay;
        }

        void CRoadElement::setOneWay( bool value )
        {
            m_OneWay = value;
        }

        bool CRoadElement::isBicycle() const
        {
            return m_IsBicycle;
        }

        void CRoadElement::setIsBicycle( bool value )
        {
            m_IsBicycle = value;
        }

        bool CRoadElement::isFootway() const
        {
            return m_IsFootway;
        }

        void CRoadElement::setIsFootway( bool value )
        {
            m_IsFootway = value;
        }

        f32 CRoadElement::getRoadWidth() const
        {
            return m_RoadWidth;
        }

        void CRoadElement::setRoadWidth( f32 value )
        {
            if( value < 0.0f )
            {
                WP_LOG_ERROR(
                    "CRoadElement::setRoadWidth - negative width not allowed, clamping to 0." );
                m_RoadWidth = 0.0f;
                return;
            }

            m_RoadWidth = value;
        }

        s32 CRoadElement::getSpeedLimit() const
        {
            return m_SpeedLimit;
        }

        void CRoadElement::setSpeedLimit( s32 value )
        {
            if( value < 0 )
            {
                WP_LOG_ERROR(
                    "CRoadElement::setSpeedLimit - negative speed limit not allowed, clamping to 0." );
                m_SpeedLimit = 0;
                return;
            }

            m_SpeedLimit = value;
        }

        String CRoadElement::getReference() const
        {
            return m_Reference;
        }

        void CRoadElement::setReference( const String &value )
        {
            m_Reference = value;
        }

        s32 CRoadElement::getNumConnections() const
        {
            return m_NumConnections;
        }

        void CRoadElement::setNumConnections( s32 value )
        {
            if( value < 0 )
            {
                WP_LOG_ERROR(
                    "CRoadElement::setNumConnections - negative count not allowed, clamping to 0." );
                m_NumConnections = 0;
                return;
            }

            m_NumConnections = value;
        }

        // ---------------------------------------------------------------
        // Property system
        // ---------------------------------------------------------------

        SmartPtr<Properties> CRoadElement::getProperties() const
        {
            try
            {
                auto properties = CProceduralObject<IRoadElement>::getProperties();
                if( !properties )
                {
                    WP_LOG_ERROR( "CRoadElement::getProperties - base properties is null." );
                    return nullptr;
                }

                properties->setProperty( kPropName, m_name );
                properties->setProperty( kPropRoadType, roadTypeToString( m_roadType ) );
                properties->setProperty( kPropLaneType, laneTypeToString( m_laneType ) );
                properties->setProperty( kPropRoadWidth, m_RoadWidth );
                properties->setProperty( kPropSpeedLimit, m_SpeedLimit );
                properties->setProperty( kPropReference, m_Reference );
                properties->setProperty( kPropNumConnections, m_NumConnections );
                properties->setProperty( kPropIsLit, m_IsLit );
                properties->setProperty( kPropOneWay, m_OneWay );
                properties->setProperty( kPropIsBicycle, m_IsBicycle );
                properties->setProperty( kPropIsFootway, m_IsFootway );
                properties->setProperty( kPropNodeCount, static_cast<s32>( getRoadNodes().size() ) );
                properties->setProperty( kPropSidewalkCount, static_cast<s32>( m_sidewalks.size() ) );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoadElement::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoadElement::setProperties - null properties provided." );
                return;
            }

            try
            {
                CProceduralObject<IRoadElement>::setProperties( properties );

                auto name = m_name;
                auto roadTypeStr = roadTypeToString( m_roadType );
                auto laneTypeStr = laneTypeToString( m_laneType );
                auto roadWidth = m_RoadWidth;
                auto speedLimit = m_SpeedLimit;
                auto reference = m_Reference;
                auto numConnections = m_NumConnections;
                auto isLit = m_IsLit;
                auto oneWay = m_OneWay;
                auto isBicycle = m_IsBicycle;
                auto isFootway = m_IsFootway;

                properties->getPropertyValue( kPropName, name );
                properties->getPropertyValue( kPropRoadType, roadTypeStr );
                properties->getPropertyValue( kPropLaneType, laneTypeStr );
                properties->getPropertyValue( kPropRoadWidth, roadWidth );
                properties->getPropertyValue( kPropSpeedLimit, speedLimit );
                properties->getPropertyValue( kPropReference, reference );
                properties->getPropertyValue( kPropNumConnections, numConnections );
                properties->getPropertyValue( kPropIsLit, isLit );
                properties->getPropertyValue( kPropOneWay, oneWay );
                properties->getPropertyValue( kPropIsBicycle, isBicycle );
                properties->getPropertyValue( kPropIsFootway, isFootway );

                m_name = name;

                // Route all values through validated setters.
                setRoadType( roadTypeFromString( roadTypeStr ) );
                setLaneType( laneTypeFromString( laneTypeStr ) );
                setRoadWidth( roadWidth );
                setSpeedLimit( speedLimit );
                setNumConnections( numConnections );

                m_Reference = reference;
                m_IsLit = isLit;
                m_OneWay = oneWay;
                m_IsBicycle = isBicycle;
                m_IsFootway = isFootway;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // end namespace procedural
}  // namespace workphone
