#ifndef ComponentHelper_h__
#define ComponentHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    class ComponentHelper
    {
    public:
        static SmartPtr<ISharedObject> getOwner( SmartPtr<scene::IComponent> component );
        static void setOwner( SmartPtr<scene::IComponent> component, SmartPtr<ISharedObject> owner );
    };

}  // namespace fb

#endif  // ComponentHelper_h__
