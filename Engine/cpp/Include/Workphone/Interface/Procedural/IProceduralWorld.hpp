#ifndef IProceduralWorld_h__
#define IProceduralWorld_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralWorld : public ISharedObject
        {
        public:
            ~IProceduralWorld() override = default;

            virtual void addScene( SmartPtr<IProceduralScene> scene ) = 0;
            virtual void removeScene( SmartPtr<IProceduralScene> scene ) = 0;
            virtual Array<SmartPtr<IProceduralScene>> getScenes() const = 0;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralWorld_h__
