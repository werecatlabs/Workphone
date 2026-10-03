#ifndef WaterStandard_h__
#define WaterStandard_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWater.hpp>

namespace workphone
{
    namespace render
    {

        class CWater : public IGraphicsWater
        {
        public:
            CWater();
            ~CWater() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate );

            void update() override;

            SmartPtr<IGraphicsScene> getSceneManager() const override;
            void setSceneManager( SmartPtr<IGraphicsScene> sceneMgr ) override;

            SmartPtr<IGraphicsCamera> getCamera() const override;
            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;

            SmartPtr<IViewport> getViewport() const override;
            void setViewport( SmartPtr<IViewport> viewport ) override;

            WaterMesh *getWaterMesh() const;
            void setWaterMesh( WaterMesh *waterMesh );

            Vector3F getPosition() const override;
            void setPosition( const Vector3F &position ) override;

        protected:
            SmartPtr<IGraphicsSceneNode> m_sceneNode;
            SmartPtr<IGraphicsMesh> m_mesh;

            SmartPtr<IGraphicsScene> m_sceneMgr;
            SmartPtr<IGraphicsCamera> m_camera;
            SmartPtr<IViewport> m_viewport;
            Ogre::FrameListener *m_frameListener = nullptr;
            WaterMesh *m_waterMesh = nullptr;

            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // WaterStandard_h__
