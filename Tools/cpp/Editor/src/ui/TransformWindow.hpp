#ifndef TransformWindow_h__
#define TransformWindow_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @class TransformWindow
         * @brief A window component that displays and allows editing of transform properties for selected objects.
         * 
         * This window provides UI controls for viewing and modifying the transform properties (position, rotation, scale)
         * of selected objects in both local and world space. It includes toggles for enabling/disabling objects
         * and separate controls for local and world transform values.
         */
        class TransformWindow : public EditorWindow
        {
        public:
            /**
             * @class VectorListener
             * @brief Event listener class for handling vector value changes in the transform window.
             * 
             * This class monitors changes to vector values (position, rotation, scale) and updates
             * the corresponding transform properties accordingly.
             */
            class VectorListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor for VectorListener
                 */
                VectorListener();

                /**
                 * @brief Destructor
                 */
                ~VectorListener() override;

                /**
                 * @brief Unloads the listener and cleans up resources
                 * @param data Shared object data to be unloaded
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles events for vector value changes
                 * @param eventType Type of the event
                 * @param eventValue Hash value of the event
                 * @param arguments Array of event parameters
                 * @param sender The object that sent the event
                 * @param object The target object
                 * @param event The event object
                 * @return Parameter containing the event result
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Handles value changes in the vector UI
                 */
                void handleValueChanged();

                /**
                 * @brief Gets the owner TransformWindow
                 * @return Smart pointer to the owner TransformWindow
                 */
                SmartPtr<TransformWindow> getOwner() const;

                /**
                 * @brief Sets the owner TransformWindow
                 * @param owner Smart pointer to the new owner
                 */
                void setOwner( SmartPtr<TransformWindow> owner );

                /**
                 * @brief Gets the vector UI component
                 * @return Smart pointer to the vector UI
                 */
                SmartPtr<ui::IUIVector3> getVectorUI() const;

                /**
                 * @brief Sets the vector UI component
                 * @param vectorUI Smart pointer to the new vector UI
                 */
                void setVectorUI( SmartPtr<ui::IUIVector3> vectorUI );

                /**
                 * @brief Gets the transform type
                 * @return The type of transform being monitored
                 */
                scene::ITransform::Type getType() const;

                /**
                 * @brief Sets the transform type
                 * @param type The new transform type
                 */
                void setType( scene::ITransform::Type type );

                WP_CLASS_REGISTER_DECL;

            private:
                scene::ITransform::Type m_type = scene::ITransform::Type::None;  ///< Type of transform being monitored
                WeakPtr<TransformWindow> m_owner;  ///< Reference to the owner window
                WeakPtr<ui::IUIVector3> m_vectorUI;  ///< Reference to the vector UI component
            };

            /**
             * @brief Default constructor for TransformWindow
             */
            TransformWindow();

            /**
             * @brief Destructor
             */
            ~TransformWindow() override;

            /**
             * @brief Loads the window and initializes its components
             * @param data Shared object data for initialization
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the window and cleans up resources
             * @param data Shared object data to be unloaded
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the window based on the current selection
             */
            void updateSelection() override;

            /**
             * @brief Gets the current transform being edited
             * @return Smart pointer to the transform
             */
            SmartPtr<scene::ITransform> getTransform() const;

            /**
             * @brief Sets the transform to be edited
             * @param transform Smart pointer to the new transform
             */
            void setTransform( SmartPtr<scene::ITransform> transform );

            /**
             * @brief Gets whether world transform is being shown
             * @return True if world transform is visible, false otherwise
             */
            bool getShowWorldTransform() const;

            /**
             * @brief Sets whether to show world transform
             * @param showWorldTransform True to show world transform, false otherwise
             */
            void setShowWorldTransform( bool showWorldTransform );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Refreshes all transform UI elements from the current transform.
             *
             * Reads local and world transform values and pushes them to the
             * corresponding UI vector fields. Safe to call when the window is
             * not fully loaded or when no transform is set.
             */
            void refreshTransformUI();

            /**
             * @brief Creates a labeled IUIVector3 field and adds it to the parent window.
             * @param ui The UI manager used to create the element.
             * @param parentWindow The parent window to which the field will be added.
             * @param label The label text for the field.
             * @return SmartPtr to the created vector field, or nullptr on failure.
             */
            SmartPtr<ui::IUIVector3> createVectorField( SmartPtr<ui::IUIManager> ui,
                                                       SmartPtr<ui::IUIWindow> parentWindow,
                                                       const String &label );

            /**
             * @brief Creates and configures a VectorListener for a vector UI field.
             * @param vectorUI The vector UI element the listener monitors.
             * @param type The transform type the listener handles.
             * @return SmartPtr to the configured listener, or nullptr on failure.
             */
            SmartPtr<VectorListener> setupListener( SmartPtr<ui::IUIVector3> vectorUI,
                                                    scene::ITransform::Type type );

            /**
             * @brief Detaches and unloads a listener, then nulls the owning member.
             * @param listener The listener member to clean up (passed by reference).
             * @param vectorUI The vector UI element the listener was attached to.
             */
            void cleanupListener( SmartPtr<VectorListener> &listener,
                                  SmartPtr<ui::IUIVector3> vectorUI );

            SmartPtr<ui::IUILabelTogglePair> m_actorEnabled;  ///< Toggle for enabling/disabling the actor

            SmartPtr<ui::IUIVector3> m_localPosition;  ///< UI for local position
            SmartPtr<ui::IUIVector3> m_localRotation;  ///< UI for local rotation
            SmartPtr<ui::IUIVector3> m_localScale;     ///< UI for local scale

            SmartPtr<ui::IUIVector3> m_position;  ///< UI for world position
            SmartPtr<ui::IUIVector3> m_rotation;  ///< UI for world rotation
            SmartPtr<ui::IUIVector3> m_scale;     ///< UI for world scale

            SmartPtr<VectorListener> m_localPositionListener;  ///< Listener for local position changes
            SmartPtr<VectorListener> m_localRotationListener;  ///< Listener for local rotation changes
            SmartPtr<VectorListener> m_localScaleListener;     ///< Listener for local scale changes

            SmartPtr<VectorListener> m_positionListener;  ///< Listener for world position changes
            SmartPtr<VectorListener> m_rotationListener;  ///< Listener for world rotation changes
            SmartPtr<VectorListener> m_scaleListener;     ///< Listener for world scale changes

            SmartPtr<scene::ITransform> m_transform;  ///< Current transform being edited

            bool m_showWorldTransform = false;  ///< Flag indicating whether world transform is visible
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // TransformWindow_h__
