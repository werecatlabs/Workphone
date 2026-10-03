#ifndef WPCorePlugin_h__
#define WPCorePlugin_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

/**
 * @namespace workphone
 * @brief Main namespace for all Workphone game engine components
 *
 * This namespace contains all classes, functions, and constants that are part
 * of the Workphone game engine framework.
 */
namespace workphone
{
    /**
     * @class WPCore
     * @brief Core initialization and management class for the Workphone game engine
     *
     * The WPCore class serves as the primary entry point for initializing and managing
     * the Workphone game engine. It handles the setup and teardown of core engine
     * subsystems, resource allocation, and provides the foundation for all other
     * engine components.
     *
     * This class implements the ISharedObject interface to provide consistent
     * object lifecycle management throughout the engine.
     *
     * @par Thread Safety
     * This class is not thread-safe. All operations should be performed on the
     * main thread unless explicitly documented otherwise.
     *
     * @par Usage Example
     * @code{.cpp}
     * // Create and initialize the core engine
     * auto core = std::make_shared<workphone::WPCore>();
     * core->load(nullptr);
     *
     * // ... use engine functionality ...
     *
     * // Clean up when done
     * core->unload(nullptr);
     * @endcode
     *
     * @see ISharedObject
     * @since Engine Version 1.0
     * @author Workphone Development Team
     */
    class WPCore_API WorkphonePlugin : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor
         *
         * Constructs a new WPCore instance. The constructor performs minimal
         * initialization - actual engine setup occurs in the load() method.
         *
         * @note The engine is not usable until load() has been called successfully.
         *
         * @see load()
         */
        WorkphonePlugin();

        /**
         * @brief Virtual destructor
         *
         * Destroys the WPCore instance and ensures proper cleanup of all
         * allocated resources. If unload() has not been called explicitly,
         * the destructor will attempt to perform cleanup automatically.
         *
         * @note It is recommended to call unload() explicitly before destruction
         *       to ensure graceful shutdown of engine subsystems.
         *
         * @see unload()
         */
        ~WorkphonePlugin() override;

        /**
         * @brief Initializes the engine core and allocates necessary resources
         *
         * This method performs the primary initialization of the Workphone game engine,
         * including setup of core subsystems, memory allocation, and preparation of
         * engine components for use.
         *
         * @param data Optional shared data object that can be used to pass
         *             initialization parameters or configuration data to the engine.
         *             Can be nullptr if no additional data is required.
         *
         * @throws std::runtime_error If initialization fails due to insufficient
         *                           resources or invalid configuration
         *
         * @pre The WPCore instance must be in an unloaded state
         * @post On success, the engine is fully initialized and ready for use
         *
         * @note This method should only be called once per WPCore instance.
         *       Subsequent calls may result in undefined behavior.
         *
         * @see unload(), ISharedObject::load()
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Shuts down the engine core and releases allocated resources
         *
         * This method performs a graceful shutdown of all engine subsystems,
         * releases allocated memory, and prepares the engine for destruction.
         * All pending operations are completed or safely terminated.
         *
         * @param data Optional shared data object that can be used to pass
         *             shutdown parameters or receive cleanup status information.
         *             Can be nullptr if no additional data exchange is required.
         *
         * @pre The engine must be in a loaded state (load() must have been called)
         * @post The engine is in an unloaded state and no longer functional
         *
         * @note After calling this method, the WPCore instance should not be used
         *       for engine operations. A new load() call would be required to
         *       reinitialize the engine.
         *
         * @warning Calling this method while engine operations are in progress
         *          may result in data loss or corruption. Ensure all operations
         *          are completed before unloading.
         *
         * @see load(), ISharedObject::unload()
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Class registration declaration macro
         *
         * This macro declares the necessary methods and data members for
         * integrating this class with the engine's reflection and serialization
         * systems. It enables runtime type information and factory creation
         * capabilities.
         *
         * @note This is an engine-specific macro that provides RTTI functionality
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // end namespace workphone

#endif  // WPCorePlugin_h__
