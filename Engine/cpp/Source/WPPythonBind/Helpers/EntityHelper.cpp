#include "WPPythonBind/WPPythonBindPCH.hpp"
#include "WPPythonBind/Helpers/EntityHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{
    python_Integer EntityHelper::getEntityId( SmartPtr<scene::IActor> entity )
    {
        auto handle = entity->getHandle();
        hash32 hash = handle->getId();
        return *reinterpret_cast<python_Integer *>(&hash);
    }

    void EntityHelper::setEntityId( SmartPtr<scene::IActor> entity, python_Integer id )
    {
        auto handle = entity->getHandle();
        hash32 hash = *reinterpret_cast<u32 *>(&id);
        handle->setId( hash );
    }

    python_Integer EntityHelper::getEntityTypeId( SmartPtr<scene::IActor> entity )
    {
        auto handle = entity->getHandle();
        hash32 hash = 0; // handle->getType();
        return *reinterpret_cast<python_Integer *>(&hash);
    }

    void EntityHelper::setEntityTypeId( SmartPtr<scene::IActor> entity, python_Integer id )
    {
        hash32 hash = *reinterpret_cast<u32 *>(&id);
        //entity->setType( hash );
    }

    python_Integer EntityHelper::getFactoryType( SmartPtr<scene::IActor> entity )
    {
        auto handle = entity->getHandle();
        hash32 hash = 0; //handle->getFactoryType();
        return *reinterpret_cast<python_Integer *>(&hash);
    }

    void EntityHelper::setFactoryType( SmartPtr<scene::IActor> entity, python_Integer type )
    {
        hash32 hash = *reinterpret_cast<u32 *>(&type);
        //entity->setFactoryType( hash );
    }

    //fb::FSMContainerPtr EntityHelper::getFSMs( SmartPtr<scene::IActor> entity )
    //{
    //    return entity->getFSMs();
    //}

    //fb::ComponentContainerPtr EntityHelper::getComponents( SmartPtr<scene::IActor> entity )
    //{
    //    return entity->getComponents();
    //}
} // namespace fb
