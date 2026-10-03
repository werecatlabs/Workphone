// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#ifndef WP__COMPONENTS_OVERLAY_INCLUDE_OGREIMGUIOVERLAY_H_
#define WP__COMPONENTS_OVERLAY_INCLUDE_OGREIMGUIOVERLAY_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include "Workphone/WorkphoneEnums.hpp"
#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"
#include <OgreResourceGroupManager.h>
#include <OgreSimpleRenderable.h>
#include <OgreRenderQueueListener.h>
#include <imgui.h>

namespace workphone
{

    class _OgreOverlayExport ImGuiOverlayOgre : public Ogre::Overlay
    {
    public:
        class RenderQueueListener : public Ogre::RenderQueueListener
        {
        public:
            RenderQueueListener();
            ~RenderQueueListener() override;

            void renderQueueStarted( Ogre::uint8 queueGroupId, const Ogre::String &invocation,
                                     bool &skipThisInvocation ) override;

            ImGuiOverlayOgre *getOwner() const;
            void setOwner( ImGuiOverlayOgre *owner );

            SmartPtr<render::IGraphicsCamera> getCamera() const;
            void setCamera( SmartPtr<render::IGraphicsCamera> camera );

        private:
            ImGuiOverlayOgre *m_owner = nullptr;
            WeakPtr<render::IGraphicsCamera> m_camera;
        };

        class ImGUIRenderable : public Ogre::SimpleRenderable
        {
        public:
            ImGUIRenderable();
            ~ImGUIRenderable() override;

            void initialise();

            void updateVertexData( ImDrawData *draw_data );

            bool preRender( Ogre::SceneManager *sm, Ogre::RenderSystem *rsys ) override;

            void getWorldTransforms( Ogre::Matrix4 *xform ) const override;
            void getRenderOperation( Ogre::RenderOperation &op ) override;

            const Ogre::LightList &getLights( void ) const override;

            void createMaterial();
            void createFontTexture();

            const Ogre::MaterialPtr &getMaterial() const override;

            /// Implementation of Ogre::SimpleRenderable
            Ogre::Real getBoundingRadius( void ) const override;

            Ogre::Real getSquaredViewDepth( const Ogre::Camera * ) const override;

            void _update();

            Ogre::Matrix4 mXform;
            Ogre::TexturePtr mFontTex;
            Ogre::MaterialPtr mMaterial;
        };

        ImGuiOverlayOgre();
        ~ImGuiOverlayOgre() override;

        void load();
        void unload();

        /// add font from ogre .fontdef file
        /// must be called before first show()
        ImFont *addFont( const String &name, const String &group );

        static bool NewFrame();

        void _findVisibleObjects( Ogre::Camera *cam, Ogre::RenderQueue *queue,
                                  Ogre::Viewport *vp ) override;

        LoadingState getLoadingState() const;

        void setLoadingState( LoadingState loadingState );

        static ImGuiOverlayOgre *getOverlay();
        static void setOverlay( ImGuiOverlayOgre *overlay );

    private:
        void initialise() override;

        uint64_t m_lastTime = 0;

        using CodePointRange = Array<ImWchar>;
        Array<CodePointRange> mCodePointRanges;

        LoadingState m_loadingState = LoadingState::Allocated;
        Ogre::RenderQueueListener *m_renderQueueListener = nullptr;
        ImGUIRenderable *mRenderable = nullptr;
        Ogre::SceneNode *m_sceneNode = nullptr;
        static u32 m_frameCount;

        static ImGuiOverlayOgre *m_overlay;
    };

}  // namespace workphone

#endif /* COMPONENTS_OVERLAY_INCLUDE_OGREIMGUIOVERLAY_H_ */
