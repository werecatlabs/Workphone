#ifndef __ActorWindow_h__
#define __ActorWindow_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <EditorTypes.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @brief A window for managing actors and their components in the editor.
         * 
         * The ActorWindow provides a user interface for viewing and manipulating actors in the scene.
         * It displays a hierarchical tree of actors and their components, allowing users to:
         * - View and edit actor properties (name, enabled state, visibility, etc.)
         * - Add and remove components
         * - Manage actor transformations
         * - Configure particle systems and materials
         */
        class ActorWindow : public EditorWindow
        {
        public:
            /**
             * @brief Event listener class for handling UI element interactions.
             * 
             * This class processes events from UI elements within the ActorWindow,
             * such as button clicks and property changes.
             */
            class UIElementListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                UIElementListener();

                /**
                 * @brief Destructor.
                 */
                ~UIElementListener() override;

                /**
                 * @brief Handles events from UI elements.
                 * @param eventType The type of event that occurred.
                 * @param eventValue The hash value identifying the event.
                 * @param arguments Additional parameters for the event.
                 * @param sender The object that sent the event.
                 * @param object The target object of the event.
                 * @param event The event object itself.
                 * @return Parameter containing the result of event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner ActorWindow instance.
                 * @return Smart pointer to the owner ActorWindow.
                 */
                SmartPtr<ActorWindow> getOwner() const;

                /**
                 * @brief Sets the owner ActorWindow instance.
                 * @param owner Smart pointer to the ActorWindow to set as owner.
                 */
                void setOwner( SmartPtr<ActorWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<ActorWindow> m_owner;  ///< Weak reference to the owner ActorWindow
            };

            /**
             * @brief Enumeration of widget identifiers used in the ActorWindow.
             */
            enum class WidgetId
            {
                Label,          ///< Label widget
                Enabled,        ///< Enable/disable toggle
                Visible,        ///< Visibility toggle
                Static,         ///< Static state toggle
                SmoothMotion,   ///< Smooth motion toggle
                CollisionMask,  ///< Physics collision mask input
                ManageCollisionMasks,  ///< Opens central collision mask dialog
                Layer,          ///< Actor layer input
                ManageLayers,   ///< Opens central layer management dialog
                Tags,           ///< Actor tags input
                ManageTags,     ///< Opens central tag management dialog
                ConfigureNetwork, ///< Adds or selects the actor's NetworkView
                ComponentSearch,///< Component hierarchy search input
                AddComponent,   ///< Add component button
                RemoveComponent,///< Remove component button
                Count           ///< Total number of widgets
            };

            /**
             * @brief Constructs an ActorWindow instance.
             * @param parent The parent UI window.
             */
            ActorWindow( SmartPtr<ui::IUIWindow> parent );

            /**
             * @brief Destructor.
             */
            ~ActorWindow() override;

            /**
             * @brief Loads data into the window.
             * @param data The data to load into the window.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads data from the window.
             * @param data The data to unload from the window.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Builds the actor hierarchy tree view.
             * 
             * This method populates the tree control with all actors and their components
             * in the current scene.
             */
            void buildTree();

            /**
             * @brief Adds a component to the actor tree.
             * @param component The component to add to the tree.
             * @param node The parent tree node where the component will be added.
             */
            void addComponentToTree( SmartPtr<scene::IComponent> component,
                                     SmartPtr<ui::IUITreeNode> node );

            /**
             * @brief Adds a generic object to the actor tree.
             * @param object The object to add to the tree.
             * @param node The parent tree node where the object will be added.
             */
            void addObjectToTree( SmartPtr<ISharedObject> object, SmartPtr<ui::IUITreeNode> node );

            /**
             * @brief Adds an actor to the actor tree.
             * @param actor The actor to add to the tree.
             * @param parentNode The parent tree node where the actor will be added.
             */
            void addActorToTree( SmartPtr<scene::IGameActor> actor,
                                 SmartPtr<ui::IUITreeNode> parentNode );

            /**
             * @brief Updates the current selection in the window.
             * 
             * This method is called when the user selects a different actor or component
             * in the tree view.
             */
            void updateSelection() override;

            void refreshLayerDropdown();

            void refreshCollisionMaskDropdown();

            void refreshTagDisplay();

            /** Applies a case-insensitive filter to the component hierarchy. */
            void applyComponentFilter( const String &filter );

            /**
             * @brief Gets the transform window associated with this actor window.
             * @return Smart pointer to the transform window.
             */
            SmartPtr<TransformWindow> getTransformWindow() const;

            /**
             * @brief Sets the transform window for this actor window.
             * @param transformWindow The transform window to associate.
             */
            void setTransformWindow( SmartPtr<TransformWindow> transformWindow );

            /**
             * @brief Gets the properties window associated with this actor window.
             * @return Smart pointer to the properties window.
             */
            SmartPtr<PropertiesWindow> getPropertiesWindow() const;

            /**
             * @brief Sets the properties window for this actor window.
             * @param propertiesWindow The properties window to associate.
             */
            void setPropertiesWindow( SmartPtr<PropertiesWindow> propertiesWindow );

            /**
             * @brief Gets the particle system window associated with this actor window.
             * @return Smart pointer to the particle system window.
             */
            SmartPtr<EditorWindow> getParticleSystemWindow() const;

            /**
             * @brief Sets the particle system window for this actor window.
             * @param particleWindow The particle system window to associate.
             */
            void setParticleSystemWindow( SmartPtr<EditorWindow> particleWindow );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Sets the name of the currently selected actor.
             * @param textStr The new name for the actor.
             */
            void setActorName( const String &textStr );

            /**
             * @brief Updates the selection state for a given object.
             * @param object The object to update selection for.
             */
            void updateObjectSelection( SmartPtr<ISharedObject> object );

            /** Updates the specialized detail panes without mutating editor selection. */
            void updateDetailsForObject( SmartPtr<ISharedObject> object );

            void selectLayer( const String &layer );

            void selectCollisionMask( u32 collisionMask );

            bool updateComponentNodeFilter( SmartPtr<ui::IUITreeNode> node,
                                            const String &filter );

            void setActorControlsEnabled( bool enabled );

            void clearInspector( const String &message );

            SmartPtr<ui::IUIWindow> m_actorWindow;      ///< Main actor window container
            SmartPtr<ui::IUIWindow> m_componentWindow;  ///< Component window container

            SmartPtr<ui::IUITreeCtrl> m_tree;          ///< Tree control for actor hierarchy
            SmartPtr<IEventListener> m_uiListener;      ///< UI event listener
            SmartPtr<ui::IUIText> m_selectionSummary;   ///< Current inspector selection summary
            SmartPtr<ui::IUITextEntry> m_componentSearchEntry; ///< Component filter input

            SmartPtr<ui::IUILabelTogglePair> m_actorEnabled;      ///< Actor enabled state toggle
            SmartPtr<ui::IUILabelTogglePair> m_actorVisible;      ///< Actor visibility toggle
            SmartPtr<ui::IUILabelTogglePair> m_actorStatic;       ///< Actor static state toggle
            SmartPtr<ui::IUILabelTogglePair> m_actorSmoothMotion; ///< Actor smooth motion toggle
            SmartPtr<ui::IUILabelDropdownPair> m_actorCollisionMask; ///< Actor physics collision mask
            SmartPtr<ui::IUIButton> m_manageCollisionMasksButton;    ///< Opens collision mask dialog
            SmartPtr<ui::IUILabelDropdownPair> m_actorLayerPair; ///< Actor layer dropdown
            SmartPtr<ui::IUIButton> m_manageLayersButton;        ///< Opens layer dialog
            SmartPtr<ui::IUILabelTextInputPair> m_actorTagsPair; ///< Actor tags display field
            SmartPtr<ui::IUIButton> m_manageTagsButton;          ///< Opens tag dialog
            SmartPtr<ui::IUIText> m_networkSummary;              ///< Actor networking status
            SmartPtr<ui::IUIButton> m_configureNetworkButton;    ///< Adds/selects NetworkView

            SmartPtr<ui::IUIButton> m_addComponentButton;    ///< Button to add components
            SmartPtr<ui::IUIButton> m_removeComponentButton; ///< Button to remove components

            SmartPtr<ui::IUILabelTextInputPair> m_actorNamePair; ///< Actor name input field

            SmartPtr<TransformWindow> m_transformWindow;     ///< Transform properties window
            SmartPtr<PropertiesWindow> m_propertiesWindow;   ///< General properties window
            SmartPtr<EventsWindow> m_eventsWindow;          ///< Events configuration window

            SmartPtr<EditorWindow> m_materialWindow;        ///< Material properties window
            SmartPtr<EditorWindow> m_terrainWindow;         ///< Terrain properties window
            SmartPtr<EditorWindow> m_particleSystemWindow;  ///< Particle system window

            SmartPtr<ISharedObject> m_selectedObject;  ///< Currently selected object
            SmartPtr<ISharedObject> m_selectedEntity;  ///< Currently selected entity

            ObjectType m_objectType = ObjectType::None;    ///< Type of the selected object
            ObjectType m_resourceType = ObjectType::None;  ///< Type of the selected resource
            String m_componentSearchFilter;                ///< Normalized component filter
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // EntityWindow_h__
