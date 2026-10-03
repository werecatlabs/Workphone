#ifndef IVehicleAiManager_h__
#define IVehicleAiManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    class WPCore_API IVehicleAiManager : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IVehicleAiManager() override;

        /**  */
        virtual void addVehicle( SmartPtr<vehicle::IVehicle> vehicle ) = 0;

        /**  */
        virtual void removeVehicle( SmartPtr<vehicle::IVehicle> vehicle ) = 0;

        /**  */
        virtual Array<SmartPtr<vehicle::IVehicle>> getVehicles() const = 0;

        /**  */
        virtual void setVehicles( const Array<SmartPtr<vehicle::IVehicle>> &vehicles ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IVehicleAiManager_h__
