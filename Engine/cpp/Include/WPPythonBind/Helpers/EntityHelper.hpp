#ifndef EntityHelper_h__
#define EntityHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    class EntityHelper
    {
    public:
        //static ComponentContainerPtr getComponents( SmartPtr<scene::IActor> entity );

        //static FSMContainerPtr getFSMs( SmartPtr<scene::IActor> entity );

        static python_Integer getEntityId( SmartPtr<scene::IActor> entity );
        static void setEntityId( SmartPtr<scene::IActor> entity, python_Integer id );

        static python_Integer getEntityTypeId( SmartPtr<scene::IActor> entity );
        static void setEntityTypeId( SmartPtr<scene::IActor> entity, python_Integer id );

        static python_Integer getFactoryType( SmartPtr<scene::IActor> entity );
        static void setFactoryType( SmartPtr<scene::IActor> entity, python_Integer type );
    };

}  // end namespace fb

#endif  // EntityHelper_h__
