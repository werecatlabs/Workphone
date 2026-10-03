#ifndef __Debug_h__
#define __Debug_h__

#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Runtime debug drawing and overlay helper.
         *
         * The Debug class implements the IDebug interface and provides utilities
         * for drawing simple debug primitives (points, lines, circles, text)
         * and for managing overlay elements used by the renderer for debugging.
         *
         * It also hosts a nested StateListener used to observe engine or
         * render-state changes and forward relevant messages to the debug system.
         */
        class WPCore_API Debug : public SharedGraphicsObject<IDebug>
        {
        public:
            /**
             * @brief Listener that receives state messages and state changes.
             *
             * This helper is typically registered with the engine's state manager.
             * It holds a weak reference to its owning Debug instance and forwards
             * messages or change notifications to that owner when appropriate.
             */
            class StateListener : public IStateListener
            {
            public:
                /** @brief Construct a StateListener without an owner. */
                StateListener();

                /** @brief Virtual destructor. */
                ~StateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 *
                 * Called when a state message is emitted by the engine. Implementations
                 * should inspect the message and return true if the message was handled.
                 *
                 * @param message Smart pointer to the state message.
                 * @return true if the message was processed; otherwise false.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle notification that a state object changed.
                 *
                 * Called when an observed state object reports that its internal data
                 * changed. Implementations should update any dependent debug overlay
                 * or primitives and return true if the change was handled.
                 *
                 * @param state Smart pointer to the state that changed.
                 * @return true if the change was processed; otherwise false.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the current owner Debug instance.
                 * @return Smart pointer to the owner Debug, or null if none.
                 */
                SmartPtr<Debug> getOwner() const;

                /**
                 * @brief Set the owner Debug instance.
                 *
                 * The listener stores a weak reference to avoid ownership cycles.
                 *
                 * @param owner Smart pointer to the Debug instance that owns this listener.
                 */
                void setOwner( SmartPtr<Debug> owner );

            protected:
                /** @brief Weak pointer back to the owning Debug instance. */
                WeakPtr<Debug> m_owner;
            };

            /** @brief Construct a Debug helper. */
            Debug();

            /** @brief Virtual destructor. */
            ~Debug() override;

            /**
             * @brief Load resources or state required by the debug system.
             *
             * Typical use includes creating materials, overlay containers and
             * registering state listeners.
             *
             * @copydoc ISharedObject::load
             * @param data Optional data used during load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload resources allocated by the debug system.
             *
             * Should release GPU resources, overlay elements and unregister listeners.
             *
             * @copydoc ISharedObject::unload
             * @param data Optional data passed to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Prepare debug primitives for the upcoming frame.
             *
             * Called before the main update loop. Use this to flush pending
             * removals or to prepare overlay elements for update.
             *
             * @copydoc ISharedObject::preUpdate
             */
            void preUpdate() override;

            /**
             * @brief Update debug primitives for the current frame.
             *
             * This is where animated or time-dependent debug visuals should be updated.
             *
             * @copydoc ISharedObject::update
             */
            void update() override;

            /**
             * @brief Finalize debug rendering for the current frame.
             *
             * Called after the main update. Use this to submit overlay updates or
             * to clear transient data for the next frame.
             *
             * @copydoc ISharedObject::postUpdate
             */
            void postUpdate() override;

            /**
             * @brief Remove all debug primitives and overlay elements.
             *
             * Clears internal arrays and queues so that no debug visuals remain.
             *
             * @copydoc IDebug::clear
             */
            void clear() override;

            /**
             * @brief Draw a single debug point.
             *
             * The point is identified by an id so callers can update or remove it
             * across frames.
             *
             * @copydoc IDebug::drawPoint
             * @param id Unique identifier for the point primitive.
             * @param positon World-space position of the point (note: parameter name preserved).
             * @param color Packed 32-bit color value (format depends on renderer).
             */
            void drawPoint( hash_type id, const Vector3<real_Num> &positon, u32 color ) override;

            /**
             * @brief Draw or retrieve a debug line primitive.
             *
             * If a line with the given id exists it will be updated; otherwise a new
             * line is created and returned.
             *
             * @copydoc IDebug::drawLine
             * @param id Unique identifier for the line primitive.
             * @param start World-space start position.
             * @param end World-space end position.
             * @param colour Packed 32-bit color value for the line.
             * @return Smart pointer to the debug line object.
             */
            SmartPtr<IDebugLine> drawLine( hash_type id, const Vector3<real_Num> &start,
                                           const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Draw or retrieve a debug circle primitive.
             *
             * Circles are defined by a center position, orientation, and radius.
             *
             * @param id Unique identifier for the circle primitive.
             * @param position World-space center position.
             * @param orientation Rotation applied to the circle plane.
             * @param radius Radius of the circle in world units.
             * @param color Packed 32-bit color value for the circle.
             * @return Smart pointer to the debug circle object.
             */
            SmartPtr<IDebugCircle> drawCircle( hash_type id, const Vector3<real_Num> &position,
                                               const Quaternion<real_Num> &orientation, real_Num radius,
                                               u32 color ) override;

            /**
             * @brief Draw or update a debug text overlay.
             *
             * Text is placed in 2D overlay space (screen or normalized coordinates
             * depending on the overlay implementation).
             *
             * @param id Unique identifier for the text element.
             * @param position 2D position for the text element.
             * @param text String to render.
             * @param color Packed 32-bit color value for the text.
             */
            void drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                           u32 color ) override;

        private:
            /**
             * @brief Create or configure the material used for drawing lines.
             *
             * This is an internal helper invoked during load or when the render
             * system requires the material to be (re)created.
             */
            void createLineMaterial();

            /**
             * @brief Internal: create and register a new debug line with id.
             * @param id Unique identifier for the line.
             * @return Smart pointer to the created debug line.
             */
            SmartPtr<IDebugLine> addLine( hash_type id );

            /** @brief Remove a debug line by id. */
            void removeLine( hash_type id );

            /** @brief Remove a debug line by pointer. */
            void removeLine( SmartPtr<IDebugLine> debugLine );

            /**
             * @brief Retrieve an existing debug line by id.
             * @param id Unique identifier for the line.
             * @return Smart pointer to the line, or null if not found.
             */
            SmartPtr<IDebugLine> getLine( hash_type id ) const;

            /**
             * @brief Internal: create and register a new debug circle with id.
             * @param id Unique identifier for the circle.
             * @return Smart pointer to the created debug circle.
             */
            SmartPtr<IDebugCircle> addCircle( hash_type id );

            /** @brief Remove a debug circle by id. */
            void removeCircle( hash_type id );

            /** @brief Remove a debug circle by pointer. */
            void removeCircle( SmartPtr<IDebugCircle> debugCircle );

            /**
             * @brief Retrieve an existing debug circle by id.
             * @param id Unique identifier for the circle.
             * @return Smart pointer to the circle, or null if not found.
             */
            SmartPtr<IDebugCircle> getCircle( hash_type id ) const;

            /**
             * @brief Get the overlay used to present debug UI and text.
             * @return Smart pointer to the overlay object.
             */
            SmartPtr<IOverlay> getOverlay() const;

            /**
             * @brief Set or replace the overlay used for debug UI.
             * @param overlay Smart pointer to the overlay instance to use.
             */
            void setOverlay( SmartPtr<IOverlay> overlay );

            /**
             * @brief Add an overlay element to the internal collection.
             * @param element Element to add (e.g., text, panel).
             */
            void addOverlayElement( SmartPtr<IOverlayElement> element );

            /**
             * @brief Retrieve an overlay element by its id.
             * @param id Identifier of the overlay element.
             * @return Smart pointer to the overlay element, or null if none found.
             */
            SmartPtr<IOverlayElement> getElementById( hash_type id ) const;

            /** @brief Queue of lines pending removal on the next update. */
            ConcurrentQueue<SmartPtr<IDebugLine>> m_removeQueue;

            /** @brief Collection of active debug lines. */
            ConcurrentArray<SmartPtr<IDebugLine>> m_debugLines;

            /** @brief Collection of active debug circles. */
            ConcurrentArray<SmartPtr<IDebugCircle>> m_debugCircles;

            /** @brief Collection of overlay elements managed by this debug helper. */
            ConcurrentArray<SmartPtr<IOverlayElement>> m_overlayElements;

            /** @brief Overlay instance used for debug text and UI. */
            SmartPtr<IOverlay> m_overlay;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CDebug_h__
