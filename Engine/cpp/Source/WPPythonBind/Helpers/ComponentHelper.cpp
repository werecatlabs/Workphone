#include "WPPythonBind/WPPythonBindPCH.hpp"
#include "WPPythonBind/Helpers/ComponentHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{

    SmartPtr<ISharedObject> ComponentHelper::getOwner( SmartPtr<scene::IComponent> component )
    {
       // return component->getOwner();
        return nullptr;
    }

    void ComponentHelper::setOwner( SmartPtr<scene::IComponent> component,
                                    SmartPtr<ISharedObject> owner )
    {
     //   component->setOwner( owner.getPtr() );
    }

}  // end namespace fb
