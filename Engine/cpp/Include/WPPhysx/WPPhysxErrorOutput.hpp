#ifndef WPPhysxErrorOutput_h__
#define WPPhysxErrorOutput_h__

#include "foundation/PxErrorCallback.h"

/**
 * @file WPPhysxErrorOutput.hpp
 * @brief PhysX error callback implementation used by the engine.
 *
 * This file contains a small wrapper that implements PhysX's
 * `physx::PxErrorCallback` interface so the engine can receive and
 * forward PhysX error, warning and informational messages to its
 * logging/debug systems.
 */

namespace workphone
{
    namespace physics
    {
        /**
         * @class PhysxErrorOutput
         * @brief Receives error callbacks from the PhysX SDK.
         *
         * Derived from `physx::PxErrorCallback` and overrides
         * `reportError` to handle messages produced by the PhysX
         * runtime. An instance of this class should be supplied to
         * PhysX so the SDK's messages can be routed into the engine's
         * logging and debug infrastructure.
         */
        class PhysxErrorOutput : public physx::PxErrorCallback
        {
        public:
            /**
             * @brief Construct a new PhysxErrorOutput.
             *
             * Perform any initialization required by the engine's
             * error reporting (if needed). Lightweight and non-blocking
             * operations are recommended because PhysX may call the
             * callback from internal threads.
             */
            PhysxErrorOutput();

            /**
             * @brief Destructor.
             */
            ~PhysxErrorOutput() override;

            /**
             * @brief PhysX error callback.
             *
             * This method is invoked by the PhysX SDK whenever an error,
             * warning or informational message is generated.
             *
             * @param e The PhysX error code indicating severity/type.
             * @param message A null-terminated message string provided by PhysX.
             * @param file The source file in the PhysX code where the event originated (may be nullptr).
             * @param line The source line number in @p file where the event originated.
             *
             * Note: PhysX may call this callback from its internal threads;
             * avoid expensive or blocking operations here. Prefer dispatching
             * the message to the engine logger or a thread-safe queue.
             */
            void reportError( physx::PxErrorCode::Enum e, const char *message, const char *file,
                              int line ) override;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxErrorOutput_h__
