#ifndef IRoadHitPoint_h__
#define IRoadHitPoint_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    namespace procedural
    {
        class IRoad;

        class WPCore_API IRoadHitPoint : public ISharedObject
        {
        public:
            ~IRoadHitPoint() override;

            virtual SmartPtr<IRoad> getRoad() const = 0;
            virtual void setRoad( SmartPtr<IRoad> road ) = 0;

            virtual SmartPtr<IRoad> getOtherRoad() const = 0;
            virtual void setOtherRoad( SmartPtr<IRoad> road ) = 0;

            virtual Vector3<real_Num> getPosition() const = 0;
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            virtual Vector3<real_Num> getNormal() const = 0;
            virtual void setNormal( const Vector3<real_Num> &normal ) = 0;

            virtual real_Num getDistance() const = 0;
            virtual void setDistance( real_Num distance ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadHitPoint_h__
