#ifndef _COverlayManager_H
#define _COverlayManager_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayManager.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class COverlayManagerOgreNext
         * @brief OgreNext-based implementation of IOverlayManager.
         *
         * This class manages overlays and overlay elements for the OgreNext
         * renderer. It provides creation, lookup and removal operations for
         * overlays and elements and is responsible for updating overlay state
         * each frame when update() is called.
         */
        class COverlayManagerOgreNext : public IOverlayManager
        {
        public:
            /**
             * @brief Construct a new overlay manager.
             *
             * Initializes internal containers used to track overlays and
             * overlay elements.
             */
            COverlayManagerOgreNext();

            /**
             * @brief Destroy the overlay manager and release resources.
             */
            ~COverlayManagerOgreNext() override;

            /**
             * @brief Per-frame update called by the renderer.
             *
             * Must be invoked once per frame to allow any internal overlay
             * state or animations to progress.
             */
            void update() override;

            /**
             * @brief Load shared object data required by the manager.
             * @param data Shared object passed to the manager (implementation-specific).
             *
             * See ISharedObject::load for the semantics of the data parameter.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload shared object data previously provided to load().
             * @param data Shared object passed to the manager (implementation-specific).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Create or register a new overlay with the given name.
             * @param instanceName Human-readable name or identifier for the overlay.
             * @return SmartPtr<IOverlay> The created overlay instance (or null on failure).
             */
            SmartPtr<IOverlay> addOverlay( const String &instanceName ) override;

            /**
             * @brief Remove an overlay by name.
             * @param instanceName Name of the overlay to remove.
             * @return true if an overlay was found and removed, false otherwise.
             */
            bool removeOverlay( const String &instanceName ) override;

            /**
             * @brief Remove the provided overlay instance.
             * @param overlay Overlay instance to remove.
             * @return true if the overlay was removed, false otherwise.
             */
            bool removeOverlay( SmartPtr<IOverlay> overlay ) override;

            /**
             * @brief Find an overlay by its instance name.
             * @param instanceName Name of the overlay to find.
             * @return SmartPtr<IOverlay> The overlay if found, otherwise null.
             */
            SmartPtr<IOverlay> findOverlay( const String &instanceName ) override;

            /**
             * @brief Check whether an overlay with the given name exists.
             * @param instanceName Name to query for.
             * @return true if the overlay exists, false otherwise.
             */
            bool hasOverlay( const String &instanceName ) override;

            /**
             * @brief Create an overlay element of the specified type and name.
             * @param typeName The type identifier for the overlay element (e.g. "Panel", "Text").
             * @param instanceName The name/identifier for the new element.
             * @return SmartPtr<IOverlayElement> The created element, or null on failure.
             */
            SmartPtr<IOverlayElement> addElement( const String &typeName,
                                                  const String &instanceName ) override;

            /**
             * @brief Locate an overlay element by name.
             * @param instanceName Name of the element to find.
             * @return SmartPtr<IOverlayElement> The element if found, otherwise null.
             */
            SmartPtr<IOverlayElement> findElement( const String &instanceName ) override;

            /**
             * @brief Remove an overlay element instance.
             * @param element Element to remove.
             * @return true if the element was removed, false otherwise.
             */
            bool removeElement( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Retrieve the underlying native renderer object.
             * @param ppObject Pointer to a void* that will receive the native object pointer.
             *
             * The method stores a pointer to the underlying renderer-specific
             * object (if any) in *ppObject. The exact object type is
             * renderer-dependent and callers should cast appropriately.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Return a snapshot of all known overlays.
             * @return Array<SmartPtr<IOverlay>> Array of overlay smart pointers.
             */
            Array<SmartPtr<IOverlay>> getOverlays() const override;

            /**
             * @brief Acquire the internal lock for thread-safe modifications.
             */
            void lock() override;

            /**
             * @brief Release the internal lock previously acquired by lock().
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Container holding all overlay elements managed by this manager.
             *
             * This ConcurrentArray allows safe concurrent access from multiple
             * threads for operations such as creation, lookup and removal of
             * overlay elements.
             */
            ConcurrentArray<SmartPtr<IOverlayElement>> m_overlayElements;

            /**
             * @brief Container holding all overlays managed by this manager.
             */
            ConcurrentArray<SmartPtr<IOverlay>> m_overlays;

            /**
             * @brief Static counter used to generate unique names when necessary.
             *
             * Each time a new overlay/element is created without an explicit
             * unique name this counter may be used to append a numeric suffix.
             */
            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
