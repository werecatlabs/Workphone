#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CRoadConnectionData.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/IntersectionResult.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadConnection,
                                   CProceduralObject<IRoadConnection> );

        // Property key constants
        static const char *const kPropName = "name";
        static const char *const kPropResourceName = "resourceName";
        static const char *const kPropConnectionType = "connectionType";
        static const char *const kPropType = "type";
        static const char *const kPropNodeCount = "nodeCount";
        static const char *const kPropDataCount = "connectionDataCount";

        // ---------------------------------------------------------------
        // EType <-> string helpers
        // ---------------------------------------------------------------

        namespace
        {
            /// Convert an EType enum value to a human-readable string.
            String eTypeToString( IRoadConnection::EType type )
            {
                using EType = IRoadConnection::EType;
                switch( type )
                {
                case EType::T_Crossing:
                    return "T_Crossing";
                case EType::X_Crossing:
                    return "X_Crossing";
                case EType::L_Connection:
                    return "L_Connection";
                case EType::RoundingAbout:
                    return "RoundingAbout";
                case EType::None:
                default:
                    return "None";
                }
            }

            /// Convert a string back to an EType enum value.
            IRoadConnection::EType eTypeFromString( const String &str )
            {
                using EType = IRoadConnection::EType;
                if( str == "T_Crossing" )
                {
                    return EType::T_Crossing;
                }
                if( str == "X_Crossing" )
                {
                    return EType::X_Crossing;
                }
                if( str == "L_Connection" )
                {
                    return EType::L_Connection;
                }
                if( str == "RoundingAbout" )
                {
                    return EType::RoundingAbout;
                }
                return EType::None;
            }
        }  // anonymous namespace

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadConnection::CRoadConnection()
        {
        }

        CRoadConnection::~CRoadConnection()
        {
            unload( nullptr );
        }

        void CRoadConnection::setUserData( hash_type id, void *userData )
        {
            if( id == StringUtil::getHash( "WPIntersectionResult" ) )
            {
                auto previous = getUserData( id );
                if( previous == userData )
                {
                    return;
                }
                // Transfer ownership only after metadata publication succeeds.
                // On failure the caller retains the incoming payload.
                IObject::setUserData( id, userData );
                delete static_cast<IntersectionResult *>( previous );
                return;
            }
            IObject::setUserData( id, userData );
        }

        void CRoadConnection::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto retired_connectionData = std::move( m_connectionData );
                auto retired_roadNodes = std::move( m_roadNodes );
                auto owner = std::exchange( m_node, nullptr );

                const auto resultId = StringUtil::getHash( "WPIntersectionResult" );
                if( getUserData( resultId ) )
                {
                    setUserData( resultId, nullptr );
                }
                CProceduralObject<IRoadConnection>::unload( data );
                for( auto &connData : retired_connectionData )
                {
                    if( connData )
                    {
                        try { connData->unload( nullptr ); }
                        catch( const std::exception &e ) { WP_LOG_EXCEPTION( e ); }
                    }
                }


            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Resource / type metadata
        // ---------------------------------------------------------------

        String CRoadConnection::getResourceName() const
        {
            return m_resourceName;
        }

        void CRoadConnection::setResourceName( const String &name )
        {
            m_resourceName = name;
        }

        String CRoadConnection::getConnectionType() const
        {
            return m_connectionType;
        }

        void CRoadConnection::setConnectionType( const String &connectionType )
        {
            m_connectionType = connectionType;
        }

        IRoadConnection::EType CRoadConnection::getType() const
        {
            return m_type;
        }

        void CRoadConnection::setType( EType type )
        {
            if( type < EType::None || type >= EType::Count )
            {
                WP_LOG_ERROR( "CRoadConnection::setType - invalid type value, defaulting to None." );
                m_type = EType::None;
                return;
            }

            m_type = type;
        }

        // ---------------------------------------------------------------
        // Connection data management
        // ---------------------------------------------------------------

        void CRoadConnection::addConnection( SmartPtr<IRoadConnectionData> connection )
        {
            if( !connection )
            {
                WP_LOG_ERROR( "CRoadConnection::addConnection - null connection data provided." );
                return;
            }

            m_connectionData.push_back( connection );
        }

        void CRoadConnection::removeConnection( SmartPtr<IRoadConnectionData> connection )
        {
            if( !connection )
            {
                return;
            }

            auto it = std::find( m_connectionData.begin(), m_connectionData.end(), connection );
            if( it != m_connectionData.end() )
            {
                m_connectionData.erase( it );
            }
        }

        Array<SmartPtr<IRoadConnectionData>> CRoadConnection::getConnectionData() const
        {
            return m_connectionData;
        }

        void CRoadConnection::setConnectionData( Array<SmartPtr<IRoadConnectionData>> connectionData )
        {
            // Filter out any null entries defensively.
            Array<SmartPtr<IRoadConnectionData>> candidate{ Allocator<SmartPtr<IRoadConnectionData>>( sourceBudget ) };
            candidate.reserve( connectionData.capacity() );

            for( auto &entry : connectionData )
            {
                if( !entry )
                {
                    WP_LOG_ERROR( "CRoadConnection::setConnectionData - null entry skipped." );
                    continue;
                }

                candidate.push_back( entry );
            }
            m_connectionData.swap( candidate );
        }

        // ---------------------------------------------------------------
        // Node access
        // ---------------------------------------------------------------

        SmartPtr<IRoadNode> CRoadConnection::getNode() const
        {
            return m_node;
        }

        Array<SmartPtr<IRoadNode>> CRoadConnection::getRoadNodes() const
        {
            return m_roadNodes;
        }

        void CRoadConnection::setRoadNodes( const Array<SmartPtr<IRoadNode>> &roadNodes )
        {
            // Filter out null entries defensively.
            Array<SmartPtr<IRoadNode>> candidate{ Allocator<SmartPtr<IRoadNode>>( sourceBudget ) };
            candidate.reserve( roadNodes.capacity() );

            for( auto &node : roadNodes )
            {
                if( !node )
                {
                    WP_LOG_ERROR( "CRoadConnection::setRoadNodes - null node entry skipped." );
                    continue;
                }

                candidate.push_back( node );
            }
            m_roadNodes.swap( candidate );
        }

        // ---------------------------------------------------------------
        // Property system
        // ---------------------------------------------------------------

        SmartPtr<Properties> CRoadConnection::getProperties() const
        {
            try
            {
                auto properties = CProceduralObject<IRoadConnection>::getProperties();
                if( !properties )
                {
                    WP_LOG_ERROR( "CRoadConnection::getProperties - base properties is null." );
                    return nullptr;
                }

                properties->setProperty( kPropName, getName() );
                properties->setProperty( kPropResourceName, m_resourceName );
                properties->setProperty( kPropConnectionType, m_connectionType );
                properties->setProperty( kPropType, eTypeToString( m_type ) );
                properties->setProperty( kPropNodeCount, static_cast<s32>( m_roadNodes.size() ) );
                properties->setProperty( kPropDataCount, static_cast<s32>( m_connectionData.size() ) );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoadConnection::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoadConnection::setProperties - null properties provided." );
                return;
            }

            try
            {
                CProceduralObject<IRoadConnection>::setProperties( properties );

                auto name = getName();
                auto resourceName = m_resourceName;
                auto connectionType = m_connectionType;
                auto typeStr = eTypeToString( m_type );

                properties->getPropertyValue( kPropName, name );
                properties->getPropertyValue( kPropResourceName, resourceName );
                properties->getPropertyValue( kPropConnectionType, connectionType );
                properties->getPropertyValue( kPropType, typeStr );

                setName( name );
                m_resourceName = resourceName;
                m_connectionType = connectionType;
                m_type = eTypeFromString( typeStr );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // namespace procedural
}  // namespace workphone
