#ifndef __PropertiesWindow_h__
#define __PropertiesWindow_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include "Workphone/System/Job.hpp"

namespace workphone
{
    namespace editor
    {

        /**
         * Window for displaying and editing the properties of a selected object.
         * This class extends the BaseWindow class and provides functionality to manage the properties of objects and display them in a property grid.
         */
        class PropertiesWindow : public EditorWindow
        {
        public:
            /** Enumeration for the different IDs that can be assigned to the properties window. */
            enum
            {
                PropertiesId,
            };

            class PropertiesDropTarget : public ui::IUIDropTarget
            {
            public:
                PropertiesDropTarget();

                ~PropertiesDropTarget() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner of the PropertiesListener.
                 * @return A pointer to the owning PropertiesWindow.
                 */
                SmartPtr<PropertiesWindow> getOwner() const;

                /**
                 * @brief Sets the owner of the PropertiesListener.
                 * @param owner A pointer to the PropertiesWindow to set as the owner.
                 */
                void setOwner( SmartPtr<PropertiesWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                /**
                 * @var PropertiesWindow *m_owner
                 * @brief A pointer to the owning PropertiesWindow.
                 */
                AtomicWeakPtr<PropertiesWindow> m_owner;
            };

            class UpdateSelectionJob : public Job
            {
            public:
                UpdateSelectionJob();

                ~UpdateSelectionJob() override;

                void execute() override;

                SmartPtr<PropertiesWindow> getOwner() const;

                void setOwner( SmartPtr<PropertiesWindow> owner );

                SmartPtr<ISharedObject> getObject() const;

                void setObject( SmartPtr<ISharedObject> object );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<ISharedObject> m_object;
                AtomicWeakPtr<PropertiesWindow> m_owner;
            };

            /**
             * @class PropertiesListener
             * @brief Class for listening to property changes.
             * @extends CSharedObject<IEventListener>
             */
            class PropertiesListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                PropertiesListener();

                /**
                 * @brief Default destructor.
                 */
                ~PropertiesListener() override;

                /**
                 * @brief Handles property change events.
                 * @param eventType The type of the event.
                 * @param eventValue The value of the event.
                 * @param arguments An array of event arguments.
                 * @param sender The sender of the event.
                 * @param object The object associated with the event.
                 * @param event A smart pointer to the event.
                 * @return The result of the event handling as a Parameter.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner of the PropertiesListener.
                 * @return A pointer to the owning PropertiesWindow.
                 */
                SmartPtr<PropertiesWindow> getOwner() const;

                /**
                 * @brief Sets the owner of the PropertiesListener.
                 * @param owner A pointer to the PropertiesWindow to set as the owner.
                 */
                void setOwner( SmartPtr<PropertiesWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                /**
                 * @var PropertiesWindow *m_owner
                 * @brief A pointer to the owning PropertiesWindow.
                 */
                AtomicWeakPtr<PropertiesWindow> m_owner;
            };

            /** Default constructor. */
            PropertiesWindow();

            /** Default destructor. */
            ~PropertiesWindow() override;

            /**
             * Loads the data for the properties window.
             * @param data The data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * Unloads the data for the properties window.
             * @param data The data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Updates the properties window. */
            void update() override;

            /** Updates the selection. */
            void updateSelection() override;

            /**
             * Updates the selection.
             * @param object The object to select.
             */
            void updateSelection( SmartPtr<ISharedObject> object );

            /** Returns whether the properties window is dirty. */
            bool isDirty() const;

            /**
             * Sets whether the properties window is dirty.
             * @param val True if the window is dirty.
             */
            void setDirty( bool val );

            /** Gets the property grid. */
            SmartPtr<ui::IUIPropertyGrid> getPropertyGrid() const;

            /**
             * Sets the property grid.
             * @param val The property grid to set.
             */
            void setPropertyGrid( SmartPtr<ui::IUIPropertyGrid> val );

            /** Gets the selected object. */
            SmartPtr<ISharedObject> getSelected() const;

            /**
             * Sets the selected object.
             * @param selected The selected object.
             */
            void setSelected( SmartPtr<ISharedObject> selected );

            /** Gets the window's selection */
            Array<SmartPtr<ISharedObject>> getSelection() const;

            /** Sets the window's selection */
            void setSelection( Array<SmartPtr<ISharedObject>> selection );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles property change events.
             * @param name The name of the property.
             * @param value The new value of the property.
             * @param isButton Flag indicating if the change is triggered by a button press (default: false).
             */
            void propertyChange( SmartPtr<Properties> properties, const String &name,
                                 const String &value, bool isButton = false );

            /** Updates the selection. */
            void updateSelectionMT( SmartPtr<ISharedObject> object );

            /**
             * @brief Handles property change events for a specific object.
             * @param object The shared object associated with the property change.
             * @param name The name of the property.
             * @param value The new value of the property.
             * @param isButton Flag indicating if the change is triggered by a button press (default: false).
             * @return The resource type of the changed property.
             */
            hash_type propertyChange( SmartPtr<Properties> properties, SmartPtr<ISharedObject> object,
                                      const String &name, const String &value, bool isButton );

            /**
             * @var SmartPtr<ui::IUIPropertyGrid> m_propertyGrid
             * @brief A smart pointer to the UI property grid.
             */
            SmartPtr<ui::IUIPropertyGrid> m_propertyGrid;

            /**
             * @var SmartPtr<ISharedObject> m_selected
             * @brief A smart pointer to the currently selected shared object.
             */
            SmartPtr<ISharedObject> m_selected;

            /** The selected objects. */
            Array<SmartPtr<ISharedObject>> m_selection;

            /** The window's drop target. */
            SmartPtr<ui::IUIDropTarget> m_dropTarget;

            /**
             * @var bool m_isDirty
             * @brief A flag indicating if the properties have been modified (default: false).
             */
            bool m_isDirty = false;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // PropertiesWindow_h__
