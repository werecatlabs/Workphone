#ifndef SceneManagerHelper_h__
#define SceneManagerHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneManager.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    /* SceneManager functions */
    class SceneManagerHelper
    {
    public:
        static SmartPtr<ISharedObject> _addGraphicsObject( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                           const char *type );
        static SmartPtr<ISharedObject> _addGraphicsObjectNamed( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                                const char *name, const char *type );
        static SmartPtr<render::IGraphicsMesh> _addMesh( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                         const char *meshFile );
        static SmartPtr<render::IGraphicsMesh> _addMeshNamed( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                              const char *name, const char *meshFile );
        static SmartPtr<render::ISceneNode> _getRootSceneNode( SmartPtr<render::IGraphicsSceneManager> smgr );
        static SmartPtr<ISharedObject> _addParticleSystem( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                           const char *name, const char *templateName );
        static SmartPtr<ISharedObject> _getParticleSystem( SmartPtr<render::IGraphicsSceneManager> smgr,
                                                           const char *name );
        static SmartPtr<render::ISceneNode> _addSceneNode( SmartPtr<render::IGraphicsSceneManager> smgr );
    };

}  // end namespace fb

#endif  // SceneManagerHelper_h__
