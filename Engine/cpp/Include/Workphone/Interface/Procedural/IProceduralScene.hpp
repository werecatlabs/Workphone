#ifndef IProceduralScene_h__
#define IProceduralScene_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralScene : public ISharedObject
        {
        public:
            ~IProceduralScene() override;

            virtual void addCity( SmartPtr<IProceduralCity> city ) = 0;
            virtual void removeCity( SmartPtr<IProceduralCity> city ) = 0;
            virtual Array<SmartPtr<IProceduralCity>> getCities() const = 0;
            virtual void setCities( Array<SmartPtr<IProceduralCity>> cities ) = 0;

            virtual void addTerrain( SmartPtr<IProceduralTerrain> terrain ) = 0;
            virtual void removeTerrain( SmartPtr<IProceduralTerrain> terrain ) = 0;
            virtual Array<SmartPtr<IProceduralTerrain>> getTerrains() const = 0;
            virtual void setTerrains( Array<SmartPtr<IProceduralTerrain>> terrains ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralScene_h__
