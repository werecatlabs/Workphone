#ifndef _CTextureOgre_H
#define _CTextureOgre_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <OgreTexture.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Ogre-backed implementation of the engine Texture abstraction.
         *
         * This class wraps an Ogre::TexturePtr and exposes the engine's
         * ITexture / Texture interface. It provides methods for loading,
         * saving, copying and querying the underlying GPU texture.
         *
         * @note Lifetime of the internal Ogre::TexturePtr is managed by this
         *       object. The underlying Ogre texture may be tied to a specific
         *       rendering device/context depending on the active graphics API.
         */
        class CTextureOgre : public Texture
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes internal members to safe defaults. Does not create
             * or load any GPU resources until `initialise` or `load` is called.
             */
            CTextureOgre();

            /**
             * @brief Destructor.
             *
             * Releases any held references to GPU resources and performs
             * necessary cleanup.
             */
            ~CTextureOgre() override;

            /**
             * @brief Update texture state.
             *
             * Called by the engine to allow the texture to perform per-frame
             * or deferred updates (for example, streaming or GPU-side
             * synchronization).
             */
            void update() override;

            /**
             * @brief Save the texture to its configured resource/location.
             *
             * Implementation-specific: may write to disk, asset store or
             * perform a GPU readback depending on how the texture was created.
             */
            void save() override;

            /**
             * @brief Save the texture contents to a file.
             *
             * @param filePath Path to the output file (absolute or relative).
             *                 The file format is implementation-defined (e.g. PNG).
             */
            void saveToFile( const String &filePath ) override;

            /**
             * @brief Load texture data from a file.
             *
             * @param filePath Path to the source file to load (absolute or relative).
             *                 Supported formats depend on the underlying loader.
             */
            void loadFromFile( const String &filePath ) override;

            /**
             * @copydoc ISharedObject::load
             *
             * Loads the texture using the provided data object. `data` typically
             * contains resource metadata or serialized texture contents.
             *
             * @param data Shared object containing load parameters or payload.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::reload
             *
             * Reinitializes or refreshes the texture from `data`, preserving any
             * existing external references where possible.
             *
             * @param data Shared object containing reload parameters or payload.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Releases GPU resources and clears internal state. After unload the
             * texture can be reloaded via `load` or `initialise`.
             *
             * @param data Optional shared object with unload instructions.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Initialize wrapper from an existing Ogre texture.
             *
             * Attaches this wrapper to an externally created Ogre::TexturePtr.
             * The wrapper will take a reference to the provided Ogre texture but
             * will not transfer ownership beyond that reference counting.
             *
             * @param texture Reference to an Ogre::TexturePtr to wrap.
             */
            void initialise( Ogre::TexturePtr &texture );

            /**
             * @brief Copy this texture's contents to another texture.
             *
             * Performs a GPU-side or CPU-side copy depending on implementation.
             *
             * @param target Reference to destination ITexture. Destination must
             *               be compatible (size/format) or the copy may fail.
             */
            void copyToTexture( SmartPtr<ITexture> &target ) override;

            /**
             * @brief Copy raw pixel data into this texture.
             *
             * Performs an upload of `data` into the texture storage. The layout
             * and pixel format are implementation-defined and must match the
             * texture's expected format.
             *
             * @param data Pointer to source pixel data.
             * @param size Dimensions (width, height) of the input data in pixels.
             */
            void copyData( void *data, const Vector2I &size ) override;

            /**
             * @brief Get the logical size set for this texture.
             *
             * This returns the size that was set via `setSize` or specified at
             * creation time, not necessarily the actual GPU texture size (which
             * may be padded to power-of-two, etc.).
             *
             * @return Texture size as a Vector2I (width, height).
             */
            Vector2I getSize() const override;

            /**
             * @brief Set the logical size of the texture.
             *
             * This may trigger reallocation of GPU resources depending on the
             * current state and implementation.
             *
             * @param size Desired texture size (width, height) in pixels.
             */
            void setSize( const Vector2I &size ) override;

            /**
             * @brief Retrieve a raw pointer to the underlying object.
             *
             * This method returns a pointer to the internal engine object for
             * interop with systems that need the raw handle. Typically used by
             * scripting or platform-specific code.
             *
             * @param ppObject Out parameter that will receive the pointer to the
             *                 underlying object. The value written is API-specific
             *                 (for example, an Ogre texture pointer).
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get actual GPU texture size.
             *
             * Returns the actual dimensions allocated on the GPU. This may
             * differ from getSize() if the GPU uses padding, alignment or power
             * of two resizing.
             *
             * @return Actual texture size as a Vector2I (width, height).
             */
            Vector2I getActualSize() const override;

            /**
             * @brief Retrieve the GPU texture handle.
             *
             * Writes an API-specific GPU texture pointer/handle into `ppTexture`.
             * For Direct3D this may be an ID3D11Texture2D*, for OpenGL a texture
             * name/ID or GL texture object pointer.
             *
             * @param ppTexture Out parameter that will receive the GPU texture handle.
             */
            void getTextureGPU( void **ppTexture ) const override;

            /**
             * @brief Retrieve the final texture object used for rendering.
             *
             * The "final" texture may differ from the raw GPU texture (for
             * example, when the engine wraps or layers resources). The object
             * returned is dependent on the graphics API in use.
             *
             * @param ppTexture Out parameter that will receive the final texture object.
             */
            void getTextureFinal( void **ppTexture ) const override;

            /**
             * @copydoc ITexture::getTextureHandle
             *
             * @return A numeric handle identifying the texture resource.
             */
            size_t getTextureHandle() const override;

            /**
             * @copydoc ITexture::getUsageFlags
             *
             * @return Bitfield representing how the texture is used (read/write/target/etc).
             */
            u32 getUsageFlags() const override;

            /**
             * @copydoc ITexture::setUsageFlags
             *
             * @param usageFlags Bitfield indicating intended usage of the texture.
             */
            void setUsageFlags( u32 usageFlags ) override;

            /**
             * @copydoc IResource::getProperties
             *
             * @return Properties object describing this resource (metadata).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IResource::setProperties
             *
             * Set resource properties/metadata for this texture.
             *
             * @param properties Properties container to associate with this texture.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc IObject::getChildObjects
             *
             * @return Array of child shared objects that belong to this texture.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the internal Ogre texture pointer.
             *
             * @return Ogre::TexturePtr - the managed pointer to the underlying Ogre texture.
             */
            Ogre::TexturePtr getTexture() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Create or initialise the object's state-tracking object.
             *
             * Called internally to ensure the object has the necessary state
             * object created for serialization, eventing or editor integration.
             */
            void createStateObject() override;

            /**
             * @brief The wrapped Ogre texture.
             *
             * Holds a reference to the underlying Ogre texture resource.
             */
            Ogre::TexturePtr m_texture;

            /**
             * @brief Extension flags / auxiliary data for this texture class.
             *
             * Implementation-specific static extension value used by the engine.
             */
            static u32 m_ext;
        };

    }  // end namespace render
}  // namespace workphone

#endif
