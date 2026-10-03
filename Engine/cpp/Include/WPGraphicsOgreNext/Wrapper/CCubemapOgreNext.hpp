#ifndef __CCubemapOgreNext_h__
#define __CCubemapOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <OgreColourValue.h>
#include <OgrePixelFormatGpu.h>
#include <OgreFrameListener.h>

namespace workphone
{
    namespace render
    {
        class CGraphicsSceneOgreNext;

        class CCubemapOgreNext : public GraphicsCubemap
        {
        public:
            class CubemapFrameListener : public Ogre::FrameListener
            {
            public:
                CubemapFrameListener( CCubemapOgreNext *cubemap );
                ~CubemapFrameListener() override;

                bool frameEnded( const Ogre::FrameEvent &evt ) override;
                bool frameStarted( const Ogre::FrameEvent &evt ) override;
                bool frameRenderingQueued( const Ogre::FrameEvent &evt ) override;

            protected:
                CCubemapOgreNext *m_cubemap = nullptr;
                f32 m_accumulatedTime = 0.0f;
            };

            class CubemapRTListener
            {
            public:
                CubemapRTListener( CCubemapOgreNext *cubemap );
                ~CubemapRTListener();

            protected:
                CCubemapOgreNext *m_cubemap = nullptr;
            };

            CCubemapOgreNext();
            ~CCubemapOgreNext() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            String getTextureName() const override;
            void setTextureName( const String &textureName ) override;

            void render();
            void renderFace( u32 faceIndex );

            void setVisibilityMask( u32 visibilityMask ) override;

            u32 getExclusionMask() const override;
            void setExclusionMask( u32 exclusionMask ) override;

            SmartPtr<IGraphicsScene> getSceneManager() const override;

            void setSceneManager( SmartPtr<IGraphicsScene> smgr ) override;

            u32 getVisibilityMask() const override;

            Vector3F getPosition() const override;
            void setPosition( const Vector3F &position ) override;

            bool getEnable() const override;
            void setEnable( bool enable ) override;

            u32 getUpdateInterval() const override;
            void setUpdateInterval( u32 milliseconds ) override;

            UpdateMode getUpdateMode() const override;
            void setUpdateMode( UpdateMode updateMode ) override;

            TimeSlicingMode getTimeSlicingMode() const override;
            void setTimeSlicingMode( TimeSlicingMode timeSlicingMode ) override;

            u32 getFaceMask() const override;
            void setFaceMask( u32 faceMask ) override;

            void requestUpdate() override;
            bool isUpdatePending() const override;

            void addExcludedObject( SmartPtr<IGraphicsObject> object ) override;
            Array<SmartPtr<IGraphicsObject>> getExcludedObjects() const override;

            void generateMaterial( const String &materialName );

            SmartPtr<ITexture> getTexture() const override;

            bool getAutoApplyToMaterials() const override;
            void setAutoApplyToMaterials( bool autoApply ) override;

            ProjectionMode getProjectionMode() const override;
            void setProjectionMode( ProjectionMode projectionMode ) override;

            InfluenceShape getInfluenceShape() const override;
            void setInfluenceShape( InfluenceShape influenceShape ) override;

            f32 getSphereRadius() const override;
            void setSphereRadius( f32 sphereRadius ) override;

            Vector3F getBoxExtents() const override;
            void setBoxExtents( const Vector3F &boxExtents ) override;

            f32 getBlendDistance() const override;
            void setBlendDistance( f32 blendDistance ) override;

            f32 getImportance() const override;
            void setImportance( f32 importance ) override;

            f32 getIntensity() const override;
            void setIntensity( f32 intensity ) override;

            bool getAutoEnableByDistance() const override;
            void setAutoEnableByDistance( bool autoEnableByDistance ) override;

            f32 getEnableDistanceThreshold() const override;
            void setEnableDistanceThreshold( f32 distanceThreshold ) override;

            bool isAutomaticMaterialCandidate() const;
            bool canAffectPosition( const Vector3F &position ) const;
            f32 getInfluenceScore( const Vector3F &position ) const;

            /** Gets the Ogre-next cubemap texture GPU object. */
            Ogre::TextureGpu *getCubemapTexture() const;

            /** Sets the resolution of the cubemap faces. Default is 256. */
            void setResolution( u32 resolution );
            u32 getResolution() const;

            /** Gets the near clip distance for the cubemap camera. */
            f32 getNearClipDistance() const;

            /** Sets the near clip distance for the cubemap camera. */
            void setNearClipDistance( f32 nearClip );

            /** Gets the far clip distance for the cubemap camera. */
            f32 getFarClipDistance() const;

            /** Sets the far clip distance for the cubemap camera. */
            void setFarClipDistance( f32 farClip );

            /** Gets whether all six cubemap faces are rendered in a single frame. */
            bool getUpdateAllFaces() const;

            /** Sets whether all six cubemap faces are rendered in a single frame.
             *  When false (default), one face is rendered per frame for better performance. */
            void setUpdateAllFaces( bool updateAllFaces );

            /** Gets the pixel format used for the cubemap texture. */
            Ogre::PixelFormatGpu getPixelFormat() const;

            /** Sets the pixel format used for the cubemap texture.
             *  Must be called before load(). Default is PFG_RGBA8_UNORM_SRGB. */
            void setPixelFormat( Ogre::PixelFormatGpu format );

            /** Gets the background clear colour used when rendering each cubemap face. */
            Ogre::ColourValue getBackgroundColour() const;

            /** Sets the background clear colour used when rendering each cubemap face.
             *  Must be called before load(). Default is opaque black. */
            void setBackgroundColour( const Ogre::ColourValue &colour );

        protected:
            void createCubemapTexture( bool renderToTexture );
            void createCubemapCamera();
            void createCubemapWorkspaces();
            void destroyCubemapWorkspaces();
            void destroyCubemapTexture();
            void updateTextureWrapper();
            void registerWithScene();
            void unregisterFromScene();
            void notifyAutomaticMaterialStateChanged();
            CGraphicsSceneOgreNext *getOgreNextScene() const;
            void renderFaces( u32 faceMask );
            u32 findNextFace( u32 faceMask ) const;
            void completeRequestedUpdate();

            Array<Ogre::Viewport *> m_viewports;
            Array<Ogre::RenderTarget *> m_renderTargets;
            Array<Ogre::CompositorWorkspace *> m_workspaces;

            Ogre::Camera *m_camera = nullptr;
            Ogre::TextureGpu *m_cubemapTexture = nullptr;
            SmartPtr<ITexture> m_texture;

            CubemapFrameListener *m_frameListener = nullptr;
            Ogre::SceneManager *m_sceneMgr = nullptr;
            SmartPtr<IGraphicsScene> m_graphicsScene;

            Vector3F m_position;

            u32 m_visibilityMask = 0xFFFFFFFF;
            u32 m_exclusionMask = 0;
            u32 m_currentIndex = 0;
            u32 m_resolution = 256;
            u32 m_updateInterval = 0;
            u32 m_faceMask = FaceAll;
            u32 m_pendingFaceMask = 0;

            Ogre::PixelFormatGpu m_pixelFormat = Ogre::PFG_RGBA8_UNORM_SRGB;
            Ogre::ColourValue m_backgroundColor{ 0.0f, 0.0f, 0.0f, 1.0f };

            f32 m_nearClip = 0.01f;
            f32 m_farClip = 10000.0f;
            f32 m_sphereRadius = 5.0f;
            f32 m_blendDistance = 1.0f;
            f32 m_importance = 1.0f;
            f32 m_intensity = 1.0f;
            f32 m_enableDistanceThreshold = 50.0f;

            bool m_enable = false;
            bool m_renderToTexture = false;
            bool m_updateQueued = false;
            bool m_autoApplyToMaterials = true;
            bool m_autoEnableByDistance = true;
            bool m_registeredWithScene = false;

            ProjectionMode m_projectionMode = ProjectionMode::Box;
            InfluenceShape m_influenceShape = InfluenceShape::Sphere;
            UpdateMode m_updateMode = UpdateMode::Automatic;
            TimeSlicingMode m_timeSlicingMode = TimeSlicingMode::OneFacePerFrame;
            Vector3F m_boxExtents = Vector3F( 5.0f, 5.0f, 5.0f );

            String m_textureName;

            Array<SmartPtr<IGraphicsObject>> m_objects;
            Array<u32> m_oldMasks;

            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // Cubemap_h__
