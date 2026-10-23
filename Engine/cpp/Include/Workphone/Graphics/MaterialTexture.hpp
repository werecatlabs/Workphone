#ifndef __WP_MaterialTexture_h__
#define __WP_MaterialTexture_h__

#include <Workphone/Interface/Graphics/IMaterialTexture.hpp>
#include <Workphone/Graphics/MaterialNode.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <memory>

namespace workphone
{
    namespace render
    {
        /**
         * @file MaterialTexture.hpp
         * @brief Concrete implementation of an IMaterialTexture material node.
         *
         * The `MaterialTexture` class represents a texture slot / unit within a material
         * node graph. It manages texture resource references, UV scale, tint color,
         * animation binding and serialization to/from data objects.
         *
         * This header contains the class declaration only. Platform or renderer-specific
         * behaviour is implemented in derived or wrapper classes (for example, an Ogre
         * implementation).
         */
        /**
         * @class MaterialTexture
         * @brief Material node that holds texture-related state.
         *
         * MaterialTexture stores a texture reference plus auxiliary state used by the
         * rendering pipeline: UV scale, tint colour, animator and texture type.
         *
         * The class derives from MaterialNode<IMaterialTexture> so it inherits the
         * material node lifecycle (load, reload, unload) and interfaces used by the
         * engine to reflect and serialize material state.
         */
        class WPCore_API MaterialTexture : public MaterialNode<IMaterialTexture>
        {
        public:
            static const String textureStr;
            static const String generateStr;

            /**
             * @brief State listener specific to MaterialTexture nodes.
             *
             * This listener handles state messages and state-changed callbacks coming
             * from the material system and propagates them to the underlying renderer
             * or resource manager as needed.
             */
            class WPCore_API MaterialTextureStateListener : public MaterialNodeStateListener
            {
            public:
                MaterialTextureStateListener();
                ~MaterialTextureStateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message The state message to handle.
                 * @return True if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a state object that has changed.
                 * @param state The changed state to handle.
                 * @return True if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            /** @brief Default constructor. Initializes members to default values. */
            MaterialTexture();
            /** Shared resource descriptor writer for implicit and authored slots. */
            static SmartPtr<Properties> textureToData(SmartPtr<ITexture> texture);

            /** @brief Virtual destructor. Cleans up resources. */
            ~MaterialTexture() override;

            /**
             * @copydoc MaterialNode<IMaterialTexture>::load
             *
             * Loads texture-specific state from the provided shared data object.
             * Implementations typically create renderer-specific texture unit state here.
             *
             * @param data Shared object containing serialized material texture data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc MaterialNode<IMaterialTexture>::reload
             *
             * Reload resource references and rebind renderer state using the provided data.
             *
             * @param data Shared object containing serialized material texture data.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc MaterialNode<IMaterialTexture>::unload
             *
             * Release renderer resources or detach texture units associated with this node.
             *
             * @param data Optional shared object passed during unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc MaterialNode<IMaterialTexture>::getTextureName
             *
             * @return The texture asset name or identifier associated with this texture unit.
             */
            String getTextureName() const override;

            /**
             * @copydoc IMaterialTexture::setTextureName
             *
             * Sets the texture asset identifier for this material texture. This may update
             * internal path state and cause the texture to be (re)loaded by the resource system.
             *
             * @param name Texture asset name or path.
             */
            void setTextureName( const String &name ) override;

            /** Cold unresolved-name object bytes; excludes string capacity and allocator overhead. */
            size_t getUnresolvedTextureNameObjectBytes() const;
            /** Optional texture-unit metadata, excluding allocator overhead. */
            size_t getTextureAuthoringStorageBytes() const;
            Vector3<real_Num> getScale() const;

            /**
             * @copydoc IMaterialTexture::getTexture
             *
             * @return Smart pointer to the currently assigned texture resource, or null if none.
             */
            SmartPtr<ITexture> getTexture() const override;

            /**
             * @copydoc IMaterialTexture::setTexture
             *
             * Assigns a texture resource to this material texture node. Ownership is
             * managed via SmartPtr.
             *
             * @param texture Smart pointer to the texture to assign.
             */
            void setTexture( SmartPtr<ITexture> texture ) override;
            void setParent( SmartPtr<IMaterialNode> parent ) override;

            /**
             * @copydoc IMaterialTexture::setScale
             *
             * Sets the UV scale applied to the texture coordinates when sampling this texture.
             *
             * @param scale UV scale as a 3-component vector (X, Y, Z). Typically Z is unused.
             */
            void setScale( const Vector3<real_Num> &scale ) override;

            /**
             * @copydoc IMaterialTexture::getAnimator
             *
             * @return Animator associated with this texture (for animated textures), or null.
             */
            SmartPtr<IAnimator> getAnimator() const override;

            /**
             * @copydoc IMaterialTexture::setAnimator
             *
             * Bind an animator to the texture. The animator may drive frame changes,
             * texture transforms, or other per-frame updates.
             *
             * @param animator Animator to bind to this texture node.
             */
            void setAnimator( SmartPtr<IAnimator> animator ) override;

            /**
             * @copydoc IMaterialTexture::getTint
             *
             * @return The tint colour multiplied with sampled texture colour when rendering.
             */
            ColourF getTint() const override;

            /**
             * @copydoc IMaterialTexture::setTint
             *
             * Sets a colour tint that is applied to the sampled texture colour.
             *
             * @param tint RGBA tint colour.
             */
            void setTint( const ColourF &tint ) override;

            /**
             * @copydoc IMaterialTexture::_getObject
             *
             * Provides the underlying native object pointer, if one exists for the
             * renderer-specific implementation.
             *
             * @param ppObject Pointer to receive the native object pointer.
             */
            void _getObject( void **ppObject ) override;

            /**
             * @copydoc IMaterialTexture::toData
             * Serializes the material texture state to a generic shared data object.
             * @return SmartPtr<ISharedObject> containing serialized state.
             */
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @copydoc IMaterialTexture::fromData
             * Restores material texture state from a shared data object created by toData().
             * @param data Serialized state to load.
             */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IResource::getProperties
             * Returns a property bag representing this resource's editable properties.
             * @return SmartPtr<Properties> Properties object describing the resource.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IResource::setProperties
             * Applies properties from a Properties bag to this material texture.
             * @param properties Properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc IResource::getChildObjects
             * @return List of child shared objects (for example linked textures).
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc IResource::getTextureType
             * @return Texture type mask or enum value used by the renderer (e.g. 2D, Cube).
             */
            u32 getTextureType() const override;

            /**
             * @copydoc IResource::setTextureType
             * Set the texture type used by the renderer (e.g. 2D, Cube).
             * @param textureType Integer representing the texture type.
             */
            void setTextureType( u32 textureType ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            friend class MaterialPass;
            // The attached pass owns resolved bindings. Effective state changes
            // retire detached fallback resources and obsolete pending paths.
            void onCanonicalBindingChanged();
            /** Resolve before publishing; Claw also requests an immediate resource load. */
            void setTextureNameImpl( const String &name, bool loadResource );

            /**
             * @brief Create renderer-specific texture unit state.
             *
             * Called during load/reload to create or update the underlying renderer's
             * texture unit/state objects. Override in platform/renderer specific
             * implementations to perform the creation.
             */
            virtual void createTextureUnitState();

            // Only edited compatibility facades need per-unit metadata. Rendered
            // bindings live in pass state; shared defaults need no allocation.
            struct TextureAuthoringData
            {
                Vector3<real_Num> scale = Vector3<real_Num>::unit();
                ColourF tint = ColourF::White;
                SmartPtr<IAnimator> animator;
            };
            void releaseDefaultAuthoringData();
            std::unique_ptr<TextureAuthoringData> m_textureAuthoring;

            /** @brief The texture resource assigned to this material texture node. */
            AtomicSmartPtr<ITexture> m_texture;

            /** Bound names belong to the resource. Only unresolved authoring names need storage. */
            std::unique_ptr<String> m_unresolvedTextureName;

            // Pass creation assigns this ordinal for ordinary texture slots.
            // Keep it inline so a bound/default slot needs no metadata block.
            u32 m_textureType = 0;

        };

    }  // end namespace render
}  // namespace workphone

#endif  // __WP_MaterialTexture_h__
