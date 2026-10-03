#ifndef __CCubemap_h__
#define __CCubemap_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Core/Array.hpp>
#include <OgreFrameListener.h>
#include <OgreRenderTargetListener.h>
#include <OgreTexture.h>

namespace workphone
{
    namespace render
    {
        /**
         * @class CCubemap
         * @brief Cubemap renderer wrapper using Ogre.
         *
         * CCubemap manages an Ogre cubemap texture, associated cameras/viewports,
         * and the update/render lifecycle for generating a dynamic cubemap.
         *
         * Responsibilities:
         * - Create and maintain the underlying Ogre cubemap texture and viewports.
         * - Provide an ICubemap implementation that allows scene association,
         *   position, visibility/exclusion masks and update scheduling.
         * - Handle frame & render-target events via nested listener classes to
         *   keep the cubemap updated at configured intervals.
         *
         * Threading / lifetime:
         * - Most methods expect to be called from the main render thread.
         * - The class stores raw Ogre pointers (camera, scene manager) and Ogre
         *   smart pointers (TexturePtr) � callers must respect Ogre lifetime rules.
         *
         * @see ICubemap
         */
        class CCubemap : public GraphicsCubemap
        {
        public:
            /**
             * @brief Helper that receives Ogre frame callbacks for the cubemap.
             *
             * The listener forwards Ogre frame events to the parent CCubemap so
             * it can drive per-frame logic such as update interval tracking and
             * starting render passes.
             */
            class CubemapFrameListener : public Ogre::FrameListener
            {
            public:
                /**
                 * @brief Construct the frame listener for the given cubemap.
                 * @param cubemap Parent CCubemap instance. Must outlive this listener.
                 */
                CubemapFrameListener( CCubemap *cubemap );
                ~CubemapFrameListener() override;

                /**
                 * @brief Called by Ogre after a frame has finished.
                 * @param evt Frame event information (timing).
                 * @return True to continue rendering, false to stop.
                 */
                bool frameEnded( const Ogre::FrameEvent &evt ) override;

                /**
                 * @brief Called by Ogre before a frame starts.
                 * @param evt Frame event information (timing).
                 * @return True to continue rendering, false to stop.
                 */
                bool frameStarted( const Ogre::FrameEvent &evt ) override;

                /**
                 * @brief Called by Ogre when the frame is queued for rendering.
                 * @param evt Frame event information (timing).
                 * @return True to continue rendering, false to stop.
                 */
                bool frameRenderingQueued( const Ogre::FrameEvent &evt ) override;

            protected:
                CCubemap *m_cubemap = nullptr; /**< Parent cubemap; not owned. */
            };

            /**
             * @brief Render target listener used to intercept viewport / render target events.
             *
             * This listener is attached to the cubemap render targets to perform
             * per-viewport configuration and to react to render target updates.
             */
            class CubemapRTListener : public Ogre::RenderTargetListener
            {
            public:
                /**
                 * @brief Create a render target listener bound to a cubemap.
                 * @param cubemap Parent CCubemap instance. Must outlive this listener.
                 */
                CubemapRTListener( CCubemap *cubemap );
                ~CubemapRTListener() override;

                /**
                 * @brief Called before a viewport is updated for this render target.
                 * @param evt Event containing viewport and render target info.
                 */
                void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;

                /**
                 * @brief Called when a viewport is added to the render target.
                 * @param evt Event containing viewport and render target info.
                 */
                void viewportAdded( const Ogre::RenderTargetViewportEvent &evt ) override;

                /**
                 * @brief Called after a viewport update completes.
                 * @param evt Event containing viewport and render target info.
                 */
                void postViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;

                /**
                 * @brief Called after the render target has been updated.
                 * @param evt Event containing render target info.
                 */
                void postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;

                /**
                 * @brief Called when a viewport is removed from the render target.
                 * @param evt Event containing viewport and render target info.
                 */
                void viewportRemoved( const Ogre::RenderTargetViewportEvent &evt ) override;

                /**
                 * @brief Called before the render target begins its update.
                 * @param evt Event containing render target info.
                 */
                void preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;

            protected:
                CCubemap *m_cubemap = nullptr; /**< Parent cubemap; not owned. */
            };

            /**
             * @brief Default constructor.
             *
             * Initializes internal state. Does not create Ogre resources � call
             * load() to initialize resources when ready.
             */
            CCubemap();

            /**
             * @brief Destructor.
             *
             * Releases resources and detaches listeners. Implementations should
             * ensure Ogre resources are released in the correct thread/context.
             */
            ~CCubemap() override;

            /**
             * @brief Initialize or configure the cubemap from a data object.
             *
             * The data object is an opaque ISharedObject used by the system to
             * pass configuration or resource handles required to construct the
             * cubemap.
             *
             * @param data Shared configuration or resource object.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release resources associated with the cubemap.
             *
             * @param data Optional shared object provided during load; may be used
             *             to reverse configuration or free external resources.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the name of the underlying cubemap texture.
             * @return Texture resource name (empty if none).
             */
            String getTextureName() const override;

            /**
             * @brief Set the name for the generated cubemap texture.
             * @param textureName Name to assign to the Ogre texture resource.
             */
            void setTextureName( const String &textureName ) override;

            /**
             * @brief Trigger an immediate cubemap render pass.
             *
             * Renders all 6 faces into the cubemap texture. Typically called by
             * the frame listener when an update is due.
             */
            void render();

            /**
             * @brief Set the visibility mask used when rendering the cubemap.
             * @param visibilityMask Bitmask controlling which objects are visible.
             */
            void setVisibilityMask( u32 visibilityMask ) override;

            /**
             * @brief Get the object exclusion mask used when rendering.
             * @return Bitmask of excluded objects.
             */
            u32 getExclusionMask() const override;

            /**
             * @brief Set a bitmask used to exclude objects from cubemap renders.
             * @param exclusionMask Bitmask where bits correspond to object categories.
             */
            void setExclusionMask( u32 exclusionMask ) override;

            /**
             * @brief Get the Ogre scene manager associated with this cubemap.
             * @return SmartPtr to the scene manager (may be null).
             */
            SmartPtr<IGraphicsScene> getSceneManager() const override;

            /**
             * @brief Associate an Ogre scene manager with the cubemap.
             * @param smgr Scene manager used to create cameras, viewports and attach objects.
             */
            void setSceneManager( SmartPtr<IGraphicsScene> smgr ) override;

            /**
             * @brief Get the visibility mask currently used for cubemap rendering.
             * @return Current visibility bitmask.
             */
            u32 getVisibilityMask() const override;

            /**
             * @brief Get the world position used as the center of the cubemap.
             * @return Position in world-space where the cubemap camera is placed.
             */
            Vector3F getPosition() const override;

            /**
             * @brief Set the cubemap center position in world-space.
             * @param position New center position for cubemap rendering.
             */
            void setPosition( const Vector3F &position ) override;

            /**
             * @brief Check whether the cubemap is enabled.
             * @return True if automatic updates are enabled.
             */
            bool getEnable() const override;

            /**
             * @brief Enable or disable automatic cubemap updates.
             * @param enable True to enable updates, false to disable.
             */
            void setEnable( bool enable ) override;

            /**
             * @brief Get the update interval for automatic cubemap rendering.
             * @return Interval in milliseconds between automatic updates.
             */
            u32 getUpdateInterval() const override;

            /**
             * @brief Set how often the cubemap should be updated automatically.
             * @param milliseconds Interval in milliseconds; zero may indicate every-frame.
             */
            void setUpdateInterval( u32 milliseconds ) override;

            /**
             * @brief Add an object to be excluded from cubemap rendering.
             * @param object Graphics object to exclude. Stored as a SmartPtr.
             */
            void addExcludedObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @brief Retrieve the list of objects currently excluded from rendering.
             * @return Array of SmartPtr<IGraphicsObject> representing excluded objects.
             */
            Array<SmartPtr<IGraphicsObject>> getExcludedObjects() const override;

            /**
             * @brief Generate an Ogre material to sample the cubemap texture.
             * @param materialName Name to assign to the generated material.
             *
             * The function will create or update a material that uses the internal
             * cubemap texture so other objects can sample it (for reflections, etc).
             */
            void generateMaterial( const String &materialName );

            WP_CLASS_REGISTER_DECL;

        protected:
            Ogre::Camera *m_camera =
                nullptr; /**< Ogre camera used to render the cubemap faces; not owned here. */
            CubemapFrameListener *m_frameListener = nullptr; /**< Frame listener instance; not owned. */
            Ogre::SceneManager *m_sceneMgr =
                nullptr; /**< Raw pointer to Ogre scene manager; not owned. */

            Ogre::TexturePtr m_cubemapTexture; /**< Ogre-managed cubemap texture. */

            Vector3F m_position; /**< World-space center of cubemap rendering. */

            u32 m_visibilityMask = 0; /**< Visibility mask used during cubemap renders. */
            u32 m_exclusionMask = 0;  /**< Exclusion mask used to filter objects. */
            u32 m_currentIndex = 0;   /**< Internal index used when stepping through faces/updates. */

            bool m_enable = true; /**< Whether automatic updates are enabled. */

            String m_textureName; /**< Name of the cubemap texture resource. */

            Array<Ogre::Viewport *> m_viewports;         /**< Viewports used for each cubemap face. */
            Array<Ogre::RenderTarget *> m_renderTargets; /**< Render targets for the cubemap faces. */
            Array<SmartPtr<IGraphicsObject>>
                m_objects;         /**< Objects excluded from rendering (SmartPtr wrappers). */
            Array<u32> m_oldMasks; /**< Previous visibility masks stored for restore after render. */

            static u32 m_nameExt; /**< Static counter used to generate unique resource names. */
        };
    }  // end namespace render
}  // namespace workphone

#endif  // __CCubemap_h__