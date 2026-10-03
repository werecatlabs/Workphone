#ifndef __WPPlugin_H__
#define __WPPlugin_H__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

#ifdef WP_PLATFORM_WIN32
#    include <libloaderapi.h>
#endif

namespace workphone
{
#if defined WP_PLATFORM_WIN32
    typedef HMODULE LibraryHandle;  ///< Handle to a dynamically loaded library (Windows).
    typedef FARPROC
        LibraryFunction;  ///< Pointer to a function in a dynamically loaded library (Windows).
#else
    typedef void *LibraryHandle;  ///< Handle to a dynamically loaded library (non-Windows).
    typedef void
        *LibraryFunction;  ///< Pointer to a function in a dynamically loaded library (non-Windows).
#endif

    /**
     * @brief Interface for a dynamically loadable plugin.
     *
     * This interface provides methods for managing the lifecycle and properties of a plugin,
     * including access to its library handle, exported functions, and file path.
     *
     * Implementations of this interface allow for dynamic loading, unloading, and interaction
     * with shared libraries (DLLs or shared objects) at runtime.
     */
    class IPlugin : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe cleanup of derived plugin objects.
         */
        ~IPlugin() override = default;

        /**
         * @brief Gets the handle to the loaded plugin library.
         *
         * @return LibraryHandle The handle to the loaded library (platform-specific).
         */
        virtual LibraryHandle getLibraryHandle() const = 0;

        /**
         * @brief Sets the handle to the loaded plugin library.
         *
         * @param handle The handle to the loaded library (platform-specific).
         */
        virtual void setLibraryHandle( LibraryHandle handle ) = 0;

        /**
         * @brief Retrieves a function pointer from the loaded plugin library by name.
         *
         * @param name The name of the function to retrieve.
         * @return LibraryFunction Pointer to the requested function, or nullptr if not found.
         */
        virtual LibraryFunction getFunction( const String &name ) const = 0;

        /**
         * @brief Gets the file path of the plugin library.
         *
         * @return StringW The wide string representing the file path of the plugin.
         */
        virtual StringW getFilePath() const = 0;

        /**
         * @brief Sets the file path of the plugin library.
         *
         * @param fileName The wide string representing the new file path of the plugin.
         */
        virtual void setFilePath( const StringW &fileName ) = 0;
    };
}  // namespace workphone

#endif
