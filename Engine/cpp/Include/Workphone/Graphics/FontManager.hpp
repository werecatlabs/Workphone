#ifndef FontManager_h__
#define FontManager_h__

#include <Workphone/Interface/Graphics/IFontManager.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class FontManager
         * @brief Manages font resources, providing creation, retrieval, loading, saving, and destruction
         * of fonts.
         *
         * The FontManager is responsible for handling all font-related resources in the rendering
         * system. It provides methods to create, load, retrieve, clone, and destroy font resources, as
         * well as manage their lifecycle. This class implements the IFontManager interface.
         */
        class WPCore_API FontManager : public IFontManager
        {
        public:
            /**
             * @brief Constructs a new FontManager instance.
             */
            FontManager();

            /**
             * @brief Destroys the FontManager instance and releases all managed resources.
             */
            ~FontManager() override;

            /**
             * @brief Loads the font manager with the provided shared object data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the font manager and releases associated resources.
             * @param data Shared object containing data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Creates a new font resource with the given name.
             * @param name The name of the font resource to create.
             * @return A smart pointer to the created font resource.
             */
            SmartPtr<IResource> create( const String &name ) override;

            /**
             * @brief Creates a new font resource with the given UUID and name.
             * @param uuid The unique identifier for the font resource.
             * @param name The name of the font resource.
             * @return A smart pointer to the created font resource.
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @brief Creates or retrieves a font resource by UUID, path, and type.
             * @param uuid The unique identifier for the font resource.
             * @param path The file path to the font resource.
             * @param type The type of the font resource.
             * @return A pair containing the smart pointer to the resource and a boolean indicating if it
             * was created (true) or retrieved (false).
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @brief Creates or retrieves a font resource by file path.
             * @param path The file path to the font resource.
             * @return A pair containing the smart pointer to the resource and a boolean indicating if it
             * was created (true) or retrieved (false).
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /**
             * @brief Saves a font resource to a file.
             * @param filePath The file path where the resource will be saved.
             * @param resource The font resource to save.
             */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /**
             * @brief Destroys a specific font resource.
             * @param resource The font resource to destroy.
             */
            void destroyResource( SmartPtr<IResource> resource ) override;

            /**
             * @brief Destroys all managed font resources.
             */
            void destroyAll() override;

            /**
             * @brief Loads a font resource from a file.
             * @param filePath The file path to load the resource from.
             * @return A smart pointer to the loaded font resource.
             */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /**
             * @brief Loads a font resource by name.
             * @param name The name of the font resource to load.
             * @return A smart pointer to the loaded font resource.
             */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /**
             * @brief Retrieves a font resource by name.
             * @param name The name of the font resource to retrieve.
             * @return A smart pointer to the font resource, or nullptr if not found.
             */
            SmartPtr<IResource> getByName( const String &name ) override;

            /**
             * @brief Retrieves a font resource by UUID.
             * @param uuid The unique identifier of the font resource to retrieve.
             * @return A smart pointer to the font resource, or nullptr if not found.
             */
            SmartPtr<IResource> getById( const String &uuid ) override;

            /**
             * @brief Clones a font resource, giving the clone a new name.
             * @param resource The font resource to clone.
             * @param clonedResourceName The name for the cloned resource.
             * @return A smart pointer to the cloned font resource.
             */
            SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Clones a font resource by name, giving the clone a new name.
             * @param name The name of the font resource to clone.
             * @param clonedResourceName The name for the cloned resource.
             * @return A smart pointer to the cloned font resource.
             */
            SmartPtr<IResource> cloneResource( const String &name,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Retrieves the underlying object pointer for internal use.
             * @param ppObject Double pointer to receive the object address.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the raw pointer to the attached IStateContext.
             * Useful when C-style pointer access is required. The returned pointer is not
             * accompanied by ownership guarantees; prefer getStateContext() for ownership.
             * @return Raw IStateContext pointer or nullptr if none set.
             */
            virtual IStateContext *getStateContextPtr() const;

            /**
             * @brief Get the SmartPtr to the attached IStateContext.
             * Returns the internal smart pointer that owns the state context.
             * @return SmartPtr<IStateContext> reference (may be nullptr).
             */
            virtual SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Attach a state context to this object.
             * The provided SmartPtr will be stored and later unloaded/removed by
             * destroyStateContext() during object unload.
             * @param stateContext Smart pointer to the state context to attach.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext );

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Array of managed font resources.
             */
            ConcurrentArray<SmartPtr<IFont>> m_fonts;
        };
    }  // namespace render
}  // namespace workphone

#endif  // FontManager_h__
