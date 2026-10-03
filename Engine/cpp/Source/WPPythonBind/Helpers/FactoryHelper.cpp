#include "WPPythonBind/WPPythonBindPCH.hpp"
#include "WPPythonBind/Helpers\FactoryHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{

    SmartPtr<ISharedObject> FactoryHelper::create( SmartPtr<IFactory> factory, const char *factoryName )
    {
        SmartPtr<ISharedObject> scriptObject;//( factory->create( factoryName ) );
        if( !scriptObject )
        {
            //FB_LOG( "Factory", String( "Could not create instance: " ) + String( factoryName ) );
        }

        return scriptObject;
    }

    void FactoryHelper::createFromScript( SmartPtr<IFactory> factory, const char *factoryName )
    {
        //FB_LOG( "Script", "createFromScript not supported" );
    }

    SmartPtr<ISharedObject> FactoryHelper::createById( SmartPtr<IFactory> factory, u32 factoryId )
    {
        SmartPtr<ISharedObject> scriptObject;//( factory->createById( factoryId ) );
        if( !scriptObject )
        {
            //FB_LOG( "Factory",
            //             String( "Could not create instance: " ) + StringUtil::toString( factoryId ) );
        }

        return scriptObject;
    }

}  // namespace fb
