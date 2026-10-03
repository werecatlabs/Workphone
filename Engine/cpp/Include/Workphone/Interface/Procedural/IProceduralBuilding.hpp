#ifndef IProceduralBuilding_h__
#define IProceduralBuilding_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralBuilding : public ISharedObject
        {
        public:
            ~IProceduralBuilding() override = default;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralBuilding_h__
