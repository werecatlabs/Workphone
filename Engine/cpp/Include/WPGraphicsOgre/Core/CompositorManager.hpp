#ifndef _CCompositorManager_H_
#define _CCompositorManager_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>

namespace workphone
{
    namespace render
    {
        class CompositorManager : public ISharedObject
        {
        public:
            CompositorManager();
            ~CompositorManager() override;

            void registerCompositors( SmartPtr<IViewport> viewport );
            void registerCompositor( SmartPtr<IViewport> viewport, const String &compositorName );
            void removeCompositor( SmartPtr<IViewport> viewport, const String &compositorName );

            void setCompositorEnabled( const String &name, SmartPtr<IViewport> viewport, bool enabled );
            bool isCompositorEnabled( const String &name, SmartPtr<IViewport> viewport ) const;

            void setCompositorProperties( const String &name, SmartPtr<IViewport> viewport,
                                          const Properties &properties );
            void getCompositorProperties( const String &name, SmartPtr<IViewport> viewport,
                                          Properties &properties ) const;

            SmartPtr<IGraphicsScene> getSceneManager() const;

            void setSceneManager( SmartPtr<IGraphicsScene> sceneManager );

            SmartPtr<IGraphicsWindow> getWindow() const;

            void setWindow( SmartPtr<IGraphicsWindow> window );

            SmartPtr<IGraphicsCamera> getCamera() const;

            void setCamera( SmartPtr<IGraphicsCamera> camera );

            String getWorkspaceName() const;

            void setWorkspaceName( const String &workspaceName );

            bool isEnabled() const;

            void setEnabled( bool enabled );

        protected:
            /** Create the hard coded postfilter effects. */
            void createEffects();

            HDRListener *hdrListener;

            using DOFMap = std::map<Ogre::Viewport *, DepthOfFieldEffect *>;
            DOFMap m_dofEffects;
        };
    }  // end namespace render
}  // namespace workphone

#endif
