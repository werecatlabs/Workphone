#ifndef __CFont_h__
#define __CFont_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/Font.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>
#include <OgreFont.h>
#include <OgreRenderTargetListener.h>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Ogre-backed font resource wrapper.
         *
         * CFont wraps a graphics font resource integrated with the engine's
         * resource and state systems and provides hooks for renderer-specific
         * material creation and state handling.
         *
         * Responsibilities:
         * - Load/unload font resources from state or external data.
         * - Maintain renderer-specific metadata (renderer type hash).
         * - Provide property and child-object enumeration for editor/serialization.
         *
         * @note This class inherits from ResourceGraphics<Font> and uses engine
         *       SmartPtr / state system conventions.
         */
        class CFont : public ResourceGraphics<Font>
        {
        public:
            /// Hash key used to set the texture parameter on the underlying material.
            static const hash_type SET_TEXTURE_HASH;
            /// Hash key for a float fragment parameter.
            static const hash_type FRAGMENT_FLOAT_HASH;
            /// Hash key for a vec2 fragment parameter.
            static const hash_type FRAGMENT_VECTOR2F_HASH;
            /// Hash key for a vec3 fragment parameter.
            static const hash_type FRAGMENT_Vector3f_HASH;
            /// Hash key for a vec4 fragment parameter.
            static const hash_type FRAGMENT_VECTOR4F_HASH;
            /// Hash key used for colour fragment parameters.
            static const hash_type FRAGMENT_COLOUR_HASH;

            /**
             * @brief Construct an empty font wrapper.
             *
             * Initializes default font settings. Heavy work is performed in load().
             */
            CFont();

            CFont(u32 poolTypeId);

            /**
             * @brief Destructor.
             *
             * Ensures any engine resources are released. Prefer explicit unload().
             */
            ~CFont() override;

            /**
             * @brief Load font data and create associated graphics resources.
             *
             * The @p data parameter may be a state object, properties block or
             * other shared object used by the engine to describe the font.
             *
             * @param data Optional shared object containing load-time information.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload graphics resources associated with this font.
             *
             * @param data Optional data passed by the state system when unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the renderer type hash used to customize material creation.
             *
             * Renderer type is an engine-specific identifier used to determine
             * which material/technique variant to create for this font.
             *
             * @return Hash identifier for the renderer type.
             */
            hash32 getRendererType() const;

            /**
             * @brief Set the renderer type hash for material selection.
             *
             * @param rendererType Hash identifier representing the renderer.
             */
            void setRendererType( hash32 rendererType );

            /**
             * @copydoc ResourceGraphics<IFont>::setDirty
             *
             * Mark the font resource as dirty so dependent systems can re-create
             * or refresh associated runtime artefacts (materials, textures, etc).
             *
             * @param dirty True to mark resource dirty; false otherwise.
             */
            void setDirty( bool dirty );

            /**
             * @copydoc ResourceGraphics<IFont>::isDirty
             *
             * @return True if the resource is currently flagged dirty.
             */
            bool isDirty() const;

            /**
             * @copydoc ResourceGraphics<IFont>::getProperties
             *
             * Returns a Properties object representing serializable font metadata
             * such as font file, size and resolution.
             *
             * @return SmartPtr to a Properties instance.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc ResourceGraphics<IFont>::setProperties
             *
             * Apply properties previously created or loaded from state/asset data.
             *
             * @param properties Properties to apply to this font resource.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Return child objects owned by the font for enumeration.
             *
             * Child objects may include material techniques or other resources
             * that should be iterated by editors or serialization systems.
             *
             * @return Array of SmartPtr<ISharedObject> representing children.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Listener that forwards state system messages to the owner font.
             *
             * This small helper class binds into the engine state system and calls
             * back into the owning CFont instance when relevant messages or state
             * changes occur. It stores a raw pointer to the owner and does not
             * manage lifetime by itself.
             */
            class MaterialStateListener : public IStateListener
            {
            public:
                MaterialStateListener();
                /**
                 * @brief Construct a listener bound to an owner.
                 * @param material Pointer to the owning CFont instance.
                 */
                MaterialStateListener( CFont *material );
                ~MaterialStateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message The incoming state message.
                 * @return True if the message was handled; false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                /**
                 * @brief Handle notification that a state object has changed.
                 * @param state The state object that changed.
                 * @return True if the change was handled; false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owning font pointer.
                 * @return Raw pointer to the owner (may be nullptr).
                 */
                CFont *getOwner() const;
                /**
                 * @brief Set/change the owner pointer.
                 * @param owner Raw pointer to the owning CFont instance.
                 */
                void setOwner( CFont *owner );

            protected:
                /// Raw pointer to the owning CFont instance; not owning.
                CFont *m_owner = nullptr;
            };

            /**
             * @brief Render target listener used to update materials when render targets change.
             *
             * This class implements Ogre::RenderTargetListener callbacks to receive
             * notifications about render target and viewport updates. Typical use
             * is to update dynamic textures or material state right before/after
             * render updates.
             */
            class MaterialEvents : public Ogre::RenderTargetListener
            {
            public:
                /**
                 * @brief Construct and bind to owning font.
                 * @param material Pointer to the owning CFont instance.
                 */
                MaterialEvents( CFont *material );
                ~MaterialEvents() override;

                void preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;
                void postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;
                void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;
                void postViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;
                void viewportAdded( const Ogre::RenderTargetViewportEvent &evt ) override;
                void viewportRemoved( const Ogre::RenderTargetViewportEvent &evt ) override;

            private:
                /// Non-owning pointer to associated material/font.
                CFont *m_material = nullptr;
            };

            /**
             * @brief Create or update the material used by this font according to the renderer type.
             *
             * This method chooses an appropriate material/technique variant based on
             * the value of @c m_rendererType and the font metadata (type, source, size).
             * Implementations typically create or fetch engine materials and populate
             * m_root/m_technique/m_techniques.
             */
            void createMaterialByType();

            /// Root material node (engine-specific representation).
            SmartPtr<IMaterialNode> m_root;

            /// Bound state context for this font.
            SmartPtr<IStateContext> m_stateContext;

            /// Listener used to receive state system messages.
            SmartPtr<IStateListener> m_stateListener;

            /// Primary material technique used for rendering.
            SmartPtr<IMaterialTechnique> m_technique;

            /// All available techniques for this font (renderer variants).
            Array<SmartPtr<IMaterialTechnique>> m_techniques;

            /// Renderer-specific type identifier used to select materials.
            hash32 m_rendererType = 0;

            /// Font metadata: type name (e.g. "truetype").
            String font_type;

            /// Font source path or identifier.
            String font_source;

            /// Font size in points.
            u32 font_size = 12;

            /// Font rendering resolution (DPI).
            u32 font_resolution = 96;

            /// Static extension used to generate unique internal names.
            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // __CFont_h__
