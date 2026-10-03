#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuaBind/Helpers/SceneManagerHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    using namespace render;

    SmartPtr<IGraphicsObject> SceneManagerHelper::_addGraphicsObject( IGraphicsScene *smgr,
                                                                      const char     *type )
    {
        return smgr->addGraphicsObject( type );
    }

    SmartPtr<IGraphicsObject> SceneManagerHelper::_addGraphicsObjectNamed( IGraphicsScene *smgr,
                                                                           const char     *name,
                                                                           const char     *type )
    {
        return smgr->addGraphicsObject( name, type );
    }

    SmartPtr<IGraphicsMesh> SceneManagerHelper::_addMesh( IGraphicsScene *smgr, const char *meshFile )
    {
        return nullptr;
        // return smgr->addMesh( meshFile );
    }

    SmartPtr<IGraphicsMesh> SceneManagerHelper::_addMeshNamed( IGraphicsScene *smgr, const char *name,
                                                               const char *meshFile )
    {
        return nullptr;
        // return smgr->addMesh( name, meshFile );
    }

    IGraphicsSceneNode *SceneManagerHelper::_getRootSceneNode( IGraphicsScene *smgr )
    {
        return smgr->getRootSceneNode().get();
    }

    SmartPtr<IParticleSystem> SceneManagerHelper::_addParticleSystem( IGraphicsScene *smgr,
                                                                      const char     *name,
                                                                      const char     *templateName )
    {
        if( !smgr )
        {
            return nullptr;
        }

        auto particleSystem = smgr->addGraphicsObjectByType<IParticleSystem>();
        if( particleSystem )
        {
            if( name )
            {
                particleSystem->setName( name );
            }

            if( templateName )
            {
                particleSystem->setTemplateName( templateName );
                particleSystem->reload( nullptr );
            }
        }

        return particleSystem;
    }

    SmartPtr<IGraphicsObject> SceneManagerHelper::_getParticleSystem( IGraphicsScene *smgr,
                                                                      const char     *name )
    {
        if( !smgr || !name )
        {
            return nullptr;
        }

        auto particleSystems = smgr->getGraphicsObjectsByType<IParticleSystem>();
        for( auto &particleSystem : particleSystems )
        {
            if( particleSystem && particleSystem->getName() == name )
            {
                return particleSystem;
            }
        }

        return nullptr;
    }

    SmartPtr<IGraphicsSceneNode> SceneManagerHelper::_addSceneNode( IGraphicsScene *smgr )
    {
        return smgr->addSceneNode();
    }

    void SceneManagerHelper::_loadSceneFile( IGraphicsScene *smgr, const char *filePath,
                                             SmartPtr<IGraphicsSceneNode> parent )
    {
        // smgr->loadSceneFile( filePath, parent );
    }
} // namespace workphone
