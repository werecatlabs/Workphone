#ifndef CRoadHitPoint_h__
#define CRoadHitPoint_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IRoadHitPoint.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CRoadHitPoint
         * @brief Represents a hit point on a road, storing position, normal, distance and related road references.
         */
        class WPProcedural_API CRoadHitPoint : public IRoadHitPoint
        {
        public:
            CRoadHitPoint();
            ~CRoadHitPoint() override;

            SmartPtr<IRoad> getRoad() const override;

            void setRoad( SmartPtr<IRoad> road ) override;

            SmartPtr<IRoad> getOtherRoad() const override;

            void setOtherRoad( SmartPtr<IRoad> road ) override;

            Vector3<real_Num> getPosition() const override;

            void setPosition( const Vector3<real_Num> &position ) override;

            Vector3<real_Num> getNormal() const override;

            void setNormal( const Vector3<real_Num> &normal ) override;

            real_Num getDistance() const override;

            void setDistance( real_Num distance ) override;

        private:
            SmartPtr<IRoad> m_road;        ///< The primary road associated with this hit point
            SmartPtr<IRoad> m_otherRoad;   ///< The secondary road associated with this hit point
            Vector3<real_Num> m_position;  ///< The 3D position of the hit point
            Vector3<real_Num> m_normal;    ///< The normal vector at the hit point
            real_Num m_distance;           ///< The distance value associated with the hit point
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadHitPoint_h__
