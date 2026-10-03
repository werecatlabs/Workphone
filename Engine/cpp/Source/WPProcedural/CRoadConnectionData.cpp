#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoadConnectionData.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadConnectionData, IRoadConnectionData );

        // Property key constants
        static const char *const kPropMarker = "marker";
        static const char *const kPropConnection = "connection";

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadConnectionData::CRoadConnectionData()
        {
        }

        CRoadConnectionData::~CRoadConnectionData()
        {
            unload( nullptr );
        }

        void CRoadConnectionData::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                m_road = nullptr;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Road reference
        // ---------------------------------------------------------------

        void CRoadConnectionData::setRoad( SmartPtr<IRoad> road )
        {
            m_road = road;
        }

        SmartPtr<IRoad> CRoadConnectionData::getRoad() const
        {
            return m_road;
        }

        // ---------------------------------------------------------------
        // Marker / connection indices
        // ---------------------------------------------------------------

        void CRoadConnectionData::setMarker( s32 marker )
        {
            if( marker < -1 )
            {
                WP_LOG_ERROR( "CRoadConnectionData::setMarker - invalid marker index, clamping to -1." );
                m_marker = -1;
                return;
            }

            m_marker = marker;
        }

        int CRoadConnectionData::getMarker() const
        {
            return m_marker;
        }

        void CRoadConnectionData::setConnection( s32 connection )
        {
            if( connection < -1 )
            {
                WP_LOG_ERROR(
                    "CRoadConnectionData::setConnection - invalid connection index, clamping to -1." );
                m_connection = -1;
                return;
            }

            m_connection = connection;
        }

        s32 CRoadConnectionData::getConnection() const
        {
            return m_connection;
        }

        // ---------------------------------------------------------------
        // Transform
        // ---------------------------------------------------------------

        void CRoadConnectionData::setTransform( Transform3<real_Num> transform )
        {
            m_transform = transform;
        }

        Transform3<real_Num> CRoadConnectionData::getTransform() const
        {
            return m_transform;
        }

        // ---------------------------------------------------------------
        // Property system
        // ---------------------------------------------------------------

        SmartPtr<Properties> CRoadConnectionData::getProperties() const
        {
            try
            {
                auto properties = workphone::make_ptr<Properties>();
                if( !properties )
                {
                    WP_LOG_ERROR(
                        "CRoadConnectionData::getProperties - failed to allocate Properties." );
                    return nullptr;
                }

                properties->setProperty( kPropMarker, m_marker );
                properties->setProperty( kPropConnection, m_connection );

                // Expose the attachment position and orientation as readable data.
                auto pos = m_transform.getPosition();
                properties->setProperty( "positionX", pos.x );
                properties->setProperty( "positionY", pos.y );
                properties->setProperty( "positionZ", pos.z );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoadConnectionData::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoadConnectionData::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto marker = m_marker;
                auto connection = m_connection;

                properties->getPropertyValue( kPropMarker, marker );
                properties->getPropertyValue( kPropConnection, connection );

                setMarker( marker );
                setConnection( connection );

                auto pos = m_transform.getPosition();
                properties->getPropertyValue( "positionX", pos.x );
                properties->getPropertyValue( "positionY", pos.y );
                properties->getPropertyValue( "positionZ", pos.z );
                m_transform.setPosition( pos );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // namespace procedural
}  // namespace workphone
