#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CRoadHitPoint.hpp"
#include <Workphone/WorkphoneInterface.hpp>

namespace workphone
{
    namespace procedural
    {
        CRoadHitPoint::CRoadHitPoint() : m_distance( 0.0 )
        {
        }

        CRoadHitPoint::~CRoadHitPoint() = default;

        workphone::SmartPtr<workphone::procedural::IRoad> CRoadHitPoint::getRoad() const
        {
            return m_road;
        }

        void CRoadHitPoint::setRoad( SmartPtr<IRoad> road )
        {
            m_road = road;
        }

        workphone::SmartPtr<workphone::procedural::IRoad> CRoadHitPoint::getOtherRoad() const
        {
            return m_otherRoad;
        }

        void CRoadHitPoint::setOtherRoad( SmartPtr<IRoad> road )
        {
            m_otherRoad = road;
        }

        workphone::Vector3<workphone::real_Num> CRoadHitPoint::getPosition() const
        {
            return m_position;
        }

        void CRoadHitPoint::setPosition( const Vector3<real_Num> &position )
        {
            m_position = position;
        }

        workphone::Vector3<workphone::real_Num> CRoadHitPoint::getNormal() const
        {
            return m_normal;
        }

        void CRoadHitPoint::setNormal( const Vector3<real_Num> &normal )
        {
            m_normal = normal;
        }

        workphone::real_Num CRoadHitPoint::getDistance() const
        {
            return m_distance;
        }

        void CRoadHitPoint::setDistance( real_Num distance )
        {
            m_distance = distance;
        }

    }  // end namespace procedural
}  // namespace workphone
