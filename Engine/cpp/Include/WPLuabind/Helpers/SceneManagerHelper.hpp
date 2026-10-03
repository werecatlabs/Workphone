#ifndef SceneManagerHelper_h__
#define SceneManagerHelper_h__

#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>

namespace workphone
{
    /* SceneManager functions */
    class SceneManagerHelper
    {
    public:
        static SmartPtr<render::IGraphicsObject> _addGraphicsObject( render::IGraphicsScene *smgr,
                                                                     const char             *type );
        static SmartPtr<render::IGraphicsObject> _addGraphicsObjectNamed( render::IGraphicsScene *smgr,
                                                                          const char             *name,
                                                                          const char             *type );
        static SmartPtr<render::IGraphicsMesh>   _addMesh( render::IGraphicsScene *smgr,
                                                           const char             *meshFile );
        static SmartPtr<render::IGraphicsMesh>   _addMeshNamed( render::IGraphicsScene *smgr,
                                                                const char *name, const char *meshFile );
        static render::IGraphicsSceneNode       *_getRootSceneNode( render::IGraphicsScene *smgr );
        static SmartPtr<render::IParticleSystem> _addParticleSystem( render::IGraphicsScene *smgr,
                                                                     const char             *name,
                                                                     const char *templateName );
        static SmartPtr<render::IGraphicsObject> _getParticleSystem( render::IGraphicsScene *smgr,
                                                                     const char             *name );
        static SmartPtr<render::IGraphicsSceneNode> _addSceneNode( render::IGraphicsScene *smgr );

        static void _loadSceneFile( render::IGraphicsScene *smgr, const char *filePath,
                                    SmartPtr<render::IGraphicsSceneNode> parent );
    };
} // namespace workphone

#endif // SceneManagerHelper_h__
