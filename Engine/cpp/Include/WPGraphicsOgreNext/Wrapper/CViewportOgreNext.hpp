#ifndef __CViewportOgreNext_H
#define __CViewportOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/Viewport.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class CViewportOgreNext
         * @brief OgreNext-specific implementation of the Viewport class.
         * @details This class provides an OgreNext rendering engine specific implementation of the Viewport interface.
         *          It wraps an Ogre::Viewport object and manages its lifecycle, state changes, and
         * integration with the workphone graphics system. The class handles initialization,
         * loading/unloading, and provides access to the underlying Ogre viewport object.
         *
         * @note This class is part of the OgreNext graphics rendering pipeline and requires OgreNext to be available.
         * @see Viewport
         * @see Ogre::Viewport
         *
         * @author Workphone Framework
         * @version 1.0
         */
        class CViewportOgreNext : public Viewport
        {
        public:
            /**
             * @class ViewportStateListener
             * @brief Internal state listener for handling viewport state changes.
             * @details This nested class implements the IStateListener interface to handle state messages
             *          and state changes for the viewport. It maintains a weak reference to its owning
             *          CViewportOgreNext instance to avoid circular dependencies and provides
             * thread-safe access through atomic operations.
             *
             * The listener is responsible for:
             * - Processing state messages related to viewport changes
             * - Handling viewport state transitions
             * - Managing the relationship with the parent viewport instance
             *
             * @note This class uses atomic weak pointers for thread-safe access to the owner.
             * @see IStateListener
             */
            class ViewportStateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 * @details Initializes the state listener with default values.
                 */
                ViewportStateListener();

                /**
                 * @brief Virtual destructor.
                 * @details Ensures proper cleanup of resources when the listener is destroyed.
                 */
                ~ViewportStateListener() override;

                /**
                 * @brief Handles incoming state messages.
                 * @param message The state message to process.
                 * @return true if the message was handled successfully, false otherwise.
                 * @details Currently returns false as message handling is not implemented.
                 *          This method can be extended to handle specific viewport-related messages.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state change notifications.
                 * @param state The state object that has changed.
                 * @return true if the state change was handled successfully, false otherwise.
                 * @details Marks the state as not dirty and returns false. This method manages
                 *          the viewport's internal state transitions and can be extended to
                 *          perform specific actions when the viewport state changes.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the owner viewport instance.
                 * @return Smart pointer to the owning CViewportOgreNext instance, or nullptr if the owner has been destroyed.
                 * @details Uses atomic operations to safely access the weak pointer and convert it to a shared pointer.
                 *          This ensures thread-safe access to the owner even if it's being destroyed on
                 * another thread.
                 */
                SmartPtr<CViewportOgreNext> getOwner() const;

                /**
                 * @brief Sets the owner viewport instance.
                 * @param owner Smart pointer to the CViewportOgreNext instance that owns this listener.
                 * @details Stores a weak reference to the owner to avoid circular dependencies.
                 *          The weak pointer allows the owner to be destroyed without affecting this
                 * listener.
                 */
                void setOwner( SmartPtr<CViewportOgreNext> owner );

            protected:
                /**
                 * @brief Atomic weak pointer to the owning viewport instance.
                 * @details Uses a weak pointer to avoid circular references and atomic operations
                 *          for thread-safe access. This allows the viewport to be destroyed
                 *          independently while maintaining safe access from the listener.
                 */
                AtomicWeakPtr<CViewportOgreNext> m_owner;
            };

            /**
             * @brief Default constructor.
             * @details Initializes the viewport with default settings, creates the state management
             *          infrastructure, and sets up the viewport state listener. The constructor
             *          also configures the visibility mask with reserved OgreNext flags.
             *
             * The initialization process includes:
             * - Setting up the state manager and state context
             * - Creating and configuring the viewport state data
             * - Establishing the state listener relationship
             * - Configuring default visibility settings
             */
            CViewportOgreNext();

            /**
             * @brief Virtual destructor.
             * @details Ensures proper cleanup of resources by calling unload(). This guarantees
             *          that all OgreNext-specific resources are properly released before the
             *          object is destroyed.
             */
            ~CViewportOgreNext() override;

            /**
             * @brief Loads the viewport with the specified data.
             * @param data Shared object containing initialization data (can be nullptr).
             * @details Sets the loading state to Loading, calls the base class load method,
             *          and then sets the loading state to Loaded. This ensures proper
             *          state management during the loading process.
             *
             * @note The data parameter is currently not used in the implementation but
             *       is preserved for interface compatibility and future extensions.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the viewport and releases resources.
             * @param data Shared object containing unload data (can be nullptr).
             * @details Sets the loading state to Unloading, calls the base class unload method,
             *          and then sets the loading state to Unloaded. This ensures proper
             *          state management during the unloading process.
             *
             * @note The data parameter is currently not used in the implementation but
             *       is preserved for interface compatibility and future extensions.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Initializes the viewport with an OgreNext viewport object.
             * @param viewport Pointer to the OgreNext viewport object to wrap.
             * @details This method establishes the connection between this wrapper and the
             *          underlying OgreNext viewport. The viewport pointer is stored directly
             *          and must remain valid for the lifetime of this object.
             *
             * @warning The caller is responsible for ensuring the viewport pointer remains valid.
             * @note This method should be called after construction and before using the viewport.
             */
            void initialise( Ogre::Viewport *viewport );

            /**
             * @brief Gets the underlying OgreNext viewport object.
             * @param[out] ppObject Pointer to receive the Ogre::Viewport pointer.
             * @details Provides direct access to the wrapped OgreNext viewport object.
             *          This method is used by the graphics system to access OgreNext-specific
             *          functionality that is not exposed through the abstract interface.
             *
             * @note The returned object should not be deleted by the caller.
             * @warning The returned pointer may be nullptr if initialise() has not been called.
             */
            void _getObject( void **ppObject ) const override;

#ifdef _DEBUG
            s32 addWeakReference();
            bool removeWeakReference();
#endif

            /**
             * @brief Class registration declaration for reflection/serialization.
             * @details This macro declares the necessary infrastructure for the class to be
             *          registered with the workphone framework's reflection and serialization systems.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Pointer to the underlying OgreNext viewport object.
             * @details This pointer provides direct access to the OgreNext viewport functionality.
             *          It is initialized to nullptr and set through the initialise() method.
             *          The lifetime of this object is managed externally by the OgreNext system.
             *
             * @note This pointer is not owned by this class and should not be deleted.
             */
            Ogre::Viewport *m_viewport = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif
