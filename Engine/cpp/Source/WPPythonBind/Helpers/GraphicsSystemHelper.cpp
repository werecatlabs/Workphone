#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Helpers/GraphicsSystemHelper.hpp>
#include <Workphone/Workphone.hpp>

namespace fb
{
    SmartPtr<render::IGraphicsSceneManager> GraphicsSystemHelper::addSceneManager(
        SmartPtr<render::IGraphicsSystem> graphicsSystem, const char *type, const char *name )
    {
        return graphicsSystem->addSceneManager( type, name );
    }

    SmartPtr<render::IGraphicsSceneManager> GraphicsSystemHelper::getSceneManager(
        SmartPtr<render::IGraphicsSystem> gfxSystem, const char *sceneManagerName )
    {
        return gfxSystem->getSceneManager( sceneManagerName );
    }

    SmartPtr<render::IWindow> GraphicsSystemHelper::getRenderWindow(
        SmartPtr<render::IGraphicsSystem> sys )
    {
        return sys->getRenderWindow();
    }

    SmartPtr<render::IWindow> GraphicsSystemHelper::getRenderWindowNamed(
        SmartPtr<render::IGraphicsSystem> sys, const char *name )
    {
        return sys->getRenderWindow( name );
    }

    fb::SmartPtr<fb::render::IDebug> GraphicsSystemHelper::getDebug(
        SmartPtr<render::IGraphicsSystem> sys )
    {
        return sys->getDebug();
    }

} // namespace fb
