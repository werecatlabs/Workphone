#ifndef IRoadConnectionData_h__
#define IRoadConnectionData_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IRoadConnectionData : public ISharedObject
        {
        public:
            ~IRoadConnectionData() override;

            virtual void setRoad( SmartPtr<IRoad> road ) = 0;
            virtual SmartPtr<IRoad> getRoad() const = 0;

            virtual void setMarker( s32 marker ) = 0;
            virtual int getMarker() const = 0;

            virtual void setConnection( s32 connection ) = 0;
            virtual int getConnection() const = 0;

            virtual void setTransform( Transform3<real_Num> transform ) = 0;
            virtual Transform3<real_Num> getTransform() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadConnection_h__
