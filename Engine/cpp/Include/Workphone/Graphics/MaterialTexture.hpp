#ifndef __WP_MaterialTexture_h__
#define __WP_MaterialTexture_h__

#include <Workphone/Interface/Graphics/IMaterialTexture.hpp>
#include <Workphone/Graphics/MaterialNode.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

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
            /**
             * @brief Create renderer-specific texture unit state.
             *
             * Called during load/reload to create or update the underlying renderer's
             * texture unit/state objects. Override in platform/renderer specific
             * implementations to perform the creation.
             */
            virtual void createTextureUnitState();

            /** @brief UV scale applied when sampling the texture. Defaults to unit scale. */
            Vector3<real_Num> m_scale = Vector3<real_Num>::unit();

            /** @brief Colour tint applied to sampled texture (multiplied). Defaults to white (no tint).
             */
            ColourF m_tint = ColourF::White;

            /** @brief Optional animator used for animated textures (frame changes, transforms, etc.). */
            SmartPtr<IAnimator> m_animator;

            /** @brief The texture resource assigned to this material texture node. */
            AtomicSmartPtr<ITexture> m_texture;

            /**
             * @brief Renderer-specific texture type identifier.
             * Use to distinguish between 2D, cube map, volume textures, or other custom types.
             */
            u32 m_textureType = 0;

            /** @brief Cube face textures when the texture type represents a cube (six textures). */
            ConcurrentArray<SmartPtr<ITexture>> m_cubeTextures;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __WP_MaterialTexture_h__
