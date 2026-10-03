#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Helpers/SceneManagerHelper.hpp>
#include <Workphone/Workphone.hpp>

namespace fb
{

    SmartPtr<ISharedObject> SceneManagerHelper::_addGraphicsObject( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                                    const char *type )
    {
        return smgr->addGraphicsObject( type );
    }

    SmartPtr<ISharedObject> SceneManagerHelper::_addGraphicsObjectNamed(
        SmartPtr<render::IGraphicsSceneManager> smgr, const char *name, const char *type )
    {
        return smgr->addGraphicsObject( name, type );
    }

    SmartPtr<render::IGraphicsMesh> SceneManagerHelper::_addMesh( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                                  const char *meshFile )
    {
        return smgr->addMesh( meshFile );
    }

    SmartPtr<render::IGraphicsMesh> SceneManagerHelper::_addMeshNamed(
        SmartPtr<render::IGraphicsSceneManager> smgr, const char *name, const char *meshFile )
    {
        return smgr->addMesh( name, meshFile );
    }

    SmartPtr<render::ISceneNode> SceneManagerHelper::_getRootSceneNode(
        SmartPtr<render::IGraphicsSceneManager> smgr )
    {
        return smgr->getRootSceneNode();
    }

    SmartPtr<ISharedObject> SceneManagerHelper::_addParticleSystem( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                                    const char *name,
                                                                    const char *templateName )
    {
        return smgr->addParticleSystem( name, templateName );
    }

    SmartPtr<ISharedObject> SceneManagerHelper::_getParticleSystem( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                                    const char *name )
    {
        return smgr->getParticleSystem( name );
    }

    SmartPtr<render::ISceneNode> SceneManagerHelper::_addSceneNode(
        SmartPtr<render::IGraphicsSceneManager> smgr )
    {
        return smgr->addSceneNode();
    }

}  // end namespace fb
