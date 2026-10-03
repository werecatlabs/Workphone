#ifndef FactoryHelper_h__
#define FactoryHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    class FactoryHelper
    {
    public:
        static SmartPtr<ISharedObject> create( SmartPtr<IFactory> factory, const char *factoryName );

        static void createFromScript( SmartPtr<IFactory> factory, const char *factoryName );

        static SmartPtr<ISharedObject> createById( SmartPtr<IFactory> factory, u32 factoryId );
    };

}  // end namespace fb

#endif  // FactoryHelper_h__
