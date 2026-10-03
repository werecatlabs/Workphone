#ifndef _IUIMANAGER_H
#define _IUIMANAGER_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

/**
 * @file IUIManager.hpp
 * @brief Interface declarations for the UI manager used by the Workphone UI
 *        subsystem.
 */

namespace workphone
{
    namespace ui
    {
        /**
         * @class IUIManager
         * @brief Abstract interface for a UI manager responsible for creating,
         *        tracking and coordinating UI applications, windows and
         *        elements.
         *
         * The UI manager serves as the primary entry point for the UI
         * subsystem. Implementations are responsible for element creation
         * and lifetime management, forwarding input and render requests to
         * the active application, and integrating with the underlying
         * graphics/input platform.
         */
        class WPCore_API IUIManager : public ISharedObject
        {
        public:
            IUIManager();

            IUIManager( u32 poolTypeId );

            /**
             * @brief Virtual destructor to allow proper cleanup through
             *        base-class pointers.
             */
            ~IUIManager() override;

            /**
             * @brief Load a font resource for use by the UI system.
             *
             * @param fontPath Filesystem path or resource identifier of the
             *                 font to load.
             * @param type     Optional subtype or face name. When omitted the
             *                 implementation may choose a sensible default.
             * @return True if the font was loaded (or scheduled for load)
             *         successfully; false on error.
             */
            virtual bool loadFont( const String &fontPath,
                                   const String &type = StringUtil::EmptyString ) = 0;

            /**
             * @brief Unload a previously loaded font resource.
             *
             * @param fontPath Filesystem path or resource identifier of the
             *                 font to unload. If empty, implementations may
             *                 unload all known fonts of the given @p type.
             * @param type     Optional subtype/face name to restrict unload
             *                 behavior.
             */
            virtual void unloadFont( const String &fontPath = StringUtil::EmptyString,
                                     const String &type = StringUtil::EmptyString ) = 0;

            /**
             * @brief Pump and process pending UI messages or events.
             *
             * This method should be called regularly (for example once per
             * frame) to allow the UI manager to handle queued messages,
             * input events or other asynchronous tasks. The optional @p data
             * pointer may carry platform-specific event/context information.
             *
             * @param data Optional implementation-defined context pointer;
             *             may be nullptr.
             * @return The number of messages/events processed by this call.
             */
            virtual size_Num messagePump( SmartPtr<ISharedObject> data ) = 0;

            /**
             * @brief Render the current UI state.
             */
            virtual void render() = 0;

            /**
             * @brief Create, register and return a new UI application instance.
             *
             * Ownership of the returned application is managed via the
             * returned smart pointer. Implementations may additionally keep
             * internal references to manage lifetime and ordering.
             *
             * @return Smart pointer to the newly created @c IUIApplication on
             *         success; nullptr on failure.
             */
            virtual SmartPtr<IUIApplication> addApplication() = 0;

            /**
             * @brief Remove and unregister an application instance.
             *
             * After calling this method the manager should no longer update
             * or render the provided application. The exact deletion timing
             * is implementation-defined (immediate or deferred).
             *
             * @param application Smart pointer identifying the application to
             *                    remove. Passing nullptr has no effect.
             */
            virtual void removeApplication( SmartPtr<IUIApplication> application ) = 0;

            /**
             * @brief Return the currently active UI application.
             *
             * @return Pointer to the active application, or nullptr if
             *         none is set.
             */
            virtual IUIApplication *getApplicationPtr() const = 0;

            /**
             * @brief Return the currently active UI application.
             *
             * @return Smart pointer to the active application, or nullptr if
             *         none is set.
             */
            virtual SmartPtr<IUIApplication> getApplication() const = 0;

            /**
             * @brief Set the currently active UI application.
             *
             * The active application receives input and render routing from
             * the UI manager. Passing nullptr clears the active application.
             *
             * @param application Smart pointer to the application to set as
             *                    active (may be nullptr).
             */
            virtual void setApplication( SmartPtr<IUIApplication> application ) = 0;

            /**
             * @brief Create and add a UI element by a hashed type identifier.
             *
             * The @p type parameter is a hashed identifier that the manager
             * uses to construct the appropriate concrete UI element class.
             *
             * @param type Hashed type identifier for the UI element to create.
             * @return Smart pointer to the created @c IUIElement on success;
             *         nullptr if the element could not be created.
             */
            virtual SmartPtr<IUIElement> addElement( hash64 type ) = 0;

            /**
             * @brief Remove a single UI element from the manager.
             *
             * The element will be unregistered and scheduled for cleanup
             * according to the manager's lifetime policy.
             *
             * @param element Smart pointer to the element to remove.
             */
            virtual void removeElement( SmartPtr<IUIElement> element ) = 0;

            /**
             * @brief Remove multiple UI elements in a single batch operation.
             *
             * Batching removals may be more efficient than removing elements
             * one-by-one because implementations can avoid repeated lookups
             * and re-layout operations.
             *
             * @param elementsToRemove Array of smart pointers to elements to
             *                         remove.
             */
            virtual void removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove ) = 0;

            /**
             * @brief Remove and clear all registered UI elements.
             *
             * After this call the manager should not contain any active
             * elements. Implementations must ensure proper resource
             * deallocation where necessary.
             */
            virtual void clear() = 0;

            /**
             * @brief Retrieve the UI cursor object.
             *
             * The cursor encapsulates current pointer state and appearance
             * used by the UI system (for example cursor image and hotspot).
             *
             * @return Smart pointer to the @c IUICursor or nullptr if unset.
             */
            virtual SmartPtr<IUICursor> getCursor() const = 0;

            /**
             * @brief Find a UI element by its string identifier.
             *
             * @param id Unique string identifier of the element to search for.
             * @return Smart pointer to the element if found, otherwise
             *         nullptr.
             */
            virtual SmartPtr<IUIElement> findElement( const String &id ) const = 0;

            /**
             * @brief Query whether the UI manager is currently handling a drag
             *        and-drop operation.
             *
             * @return True if an element is being dragged; false otherwise.
             */
            virtual bool isDragging() const = 0;

            /**
             * @brief Set the dragging state for the UI manager.
             *
             * Typically used to notify the manager that a drag operation has
             * started or finished so that the manager can update visual state
             * and input handling accordingly.
             *
             * @param dragging True to indicate a drag operation is active.
             */
            virtual void setDragging( bool dragging ) = 0;

            /**
             * @brief Get the primary (main) UI window used by the manager.
             *
             * The main window usually serves as the top-level container for
             * layout and input routing.
             *
             * @return Smart pointer to the main @c IUIWindow or nullptr if
             *         none is set.
             */
            virtual SmartPtr<IUIWindow> getMainWindow() const = 0;

            /**
             * @brief Set the primary (main) UI window.
             *
             * @param uiWindow Smart pointer to the window to treat as main.
             */
            virtual void setMainWindow( SmartPtr<IUIWindow> uiWindow ) = 0;

            /**
             * @brief Invalidate cached UI layout/state and request an update.
             *
             * Calling this signals that cached element state or layout is
             * dirty and should be recomputed on the next update/render pass.
             */
            virtual void invalidate() = 0;

            /**
             * @brief Retrieve the underlying concrete implementation pointer.
             *
             * This method exposes the raw implementation pointer for rare
             * situations where direct access to platform-specific features is
             * required. Use with care as it breaks encapsulation.
             *
             * @param ppObject Output pointer that will receive the raw
             *                 implementation pointer (may be set to nullptr).
             */
            virtual void _getObject( void **ppObject ) = 0;

            /**
             * @brief Load a graphics-related object used by the UI via the
             *        graphics subsystem.
             *
             * Some UI resources must be uploaded to GPU memory or otherwise
             * registered with the graphics system. Implementations may load
             * immediately or enqueue the operation for deferred processing.
             *
             * @param graphicsObject Smart pointer to the object to load.
             * @param forceQueue      If true, force the load to be queued for
             *                        deferred processing even if immediate
             *                        loading would be possible.
             */
            virtual void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

            /**
             * @brief Unload a graphics-related object from the graphics system.
             *
             * This should release any GPU or graphics-system resources
             * associated with the object. The unload may be immediate or
             * deferred depending on @p forceQueue and implementation policy.
             *
             * @param graphicsObject Smart pointer to the object to unload.
             * @param forceQueue      If true, force the unload to be queued
             *                        for deferred processing.
             */
            virtual void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

            /**
             * @brief Get the current overlay object used by the UI manager.
             *
             * Overlays are typically rendered on top of UI elements and can
             * be used for debugging, diagnostics or auxiliary rendering.
             *
             * @return Smart pointer to the overlay object or nullptr if none
             *         is set.
             */
            virtual SmartPtr<ISharedObject> getOverlay() const = 0;

            /**
             * @brief Set the overlay object for the UI manager.
             *
             * @param overlay Smart pointer to the overlay object to set.
             */
            virtual void setOverlay( SmartPtr<ISharedObject> overlay ) = 0;

            /**
             * @brief Convenience template for creating and adding a UI element
             *        using its static type information.
             *
             * This template calls the static @c typeInfo() method on the
             * element class to obtain the hashed identifier and then calls
             * @c addElement(hash64) to create the instance. It asserts that
             * the created element can be cast to the requested type.
             *
             * @tparam T Concrete UI element type to create.
             * @return Smart pointer of type T to the created element or
             *         nullptr on failure.
             */
            template <class T>
            SmartPtr<T> addElementByType();

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        SmartPtr<T> IUIManager::addElementByType()
        {
            auto typeInfo = T::typeInfo();
            WP_ASSERT( typeInfo != 0 );

            if( auto element = addElement( typeInfo ) )
            {
                WP_ASSERT( workphone::dynamic_pointer_cast<T>( element ) );
                return workphone::static_pointer_cast<T>( element );
            }

            return nullptr;
        }

    }  // end namespace ui
}  // namespace workphone

#endif
