#ifndef __IGameActor_h__
#define __IGameActor_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/List.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/OBB3.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <algorithm>
#include <iterator>

namespace workphone
{
    namespace scene
    {

        /**
         * @class IGameActor
         * @brief Interface for an actor class. Actors are entities in a scene that can be manipulated,
         * moved, and organized hierarchically.
         *
         * Actors can have components, children, and various states. This interface provides methods for
         * managing transforms, hierarchy, components, tags, layers, and state, as well as event handling
         * and scene integration.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API IGameActor : public IResource
        {
        public:
            /**
             * @brief The possible states of an actor.
             */
            enum class State
            {
                None,       ///< No state assigned.
                Create,     ///< The actor is being created.
                Destroyed,  ///< The actor has been destroyed.
                Edit,       ///< The actor is being edited.
                Play,       ///< The actor is being played.
                Count       ///< The number of possible states.
            };

            /**
             * @name Actor Flags
             * Constants representing the state and behavior flags of an actor.
             * @{ */
            static const u32 ActorFlagReserved;   ///< Reserved actor flag.
            static const u32 ActorFlagStatic;     ///< Actor is static (non-moving).
            static const u32 ActorFlagVisible;    ///< Actor is visible.
            static const u32 ActorFlagEnabled;    ///< Actor is enabled.
            static const u32 ActorFlagMine;       ///< Actor belongs to the client in a network game.
            static const u32 ActorFlagPerpetual;  ///< Actor should persist when a new scene is loaded.
            static const u32 ActorFlagDirty;      ///< Actor is dirty (needs update).
            static const u32 ActorFlagAwake;      ///< Actor is awake.
            static const u32 ActorFlagStarted;    ///< Actor is started.
            static const u32 ActorFlagDummy;      ///< Actor is a dummy.
            static const u32 ActorFlagInScene;    ///< Actor is in the scene.
            static const u32 ActorFlagEnabledInScene;  ///< Actor is enabled in the scene.
            static const u32 ActorFlagIsEditor;        ///< Actor is visible in the editor.
            static const u32 ActorFlagSmoothMotion;    ///< Actor uses smooth interpolation.
            static const u32 ActorFlagHidden;          ///< Actor is hidden.
            static const u32 ActorFlagDontSave;        ///< Actor should not be saved.
            static const u32 ActorFlagPrefab;          ///< Actor is a prefab.
            /** @} */

            static const String actorName;

            IGameActor();

            IGameActor( u32 poolTypeId );

            /**
             * @brief Destructor.
             */
            ~IGameActor() override;

            /**
             * @brief Update the actor when it's marked as dirty.
             * @param flags Bitmask indicating which aspects of the actor have changed.
             * @param oldFlags Bitmask indicating the previous state of the actor.
             */
            virtual void updateDirty( u32 flags, u32 oldFlags ) = 0;

            /**
             * @brief Gets the local transform of the actor.
             * @return The local transform.
             */
            virtual Transform3<real_Num> getLocalTransform() const = 0;

            /**
             * @brief Gets the world transform of the actor.
             * @return The world transform.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Gets the local transform of the actor at a specific time.
             * @param t The time interval.
             * @return The local transform at the given time.
             */
            virtual Transform3<real_Num> getLocalTransform( time_interval t ) const = 0;

            /**
             * @brief Gets the world transform of the actor at a specific time.
             * @param t The time interval.
             * @return The world transform at the given time.
             */
            virtual Transform3<real_Num> getWorldTransform( time_interval t ) const = 0;

            /**
             * @brief Gets the local position of the actor.
             * @return The local position.
             */
            virtual Vector3<real_Num> getLocalPosition() const = 0;

            /**
             * @brief Sets the local position of the actor.
             * @param localPosition The new local position.
             */
            virtual void setLocalPosition( const Vector3<real_Num> &localPosition ) = 0;

            /**
             * @brief Gets the local scale of the actor.
             * @return The local scale.
             */
            virtual Vector3<real_Num> getLocalScale() const = 0;

            /**
             * @brief Sets the local scale of the actor.
             * @param localScale The new local scale.
             */
            virtual void setLocalScale( const Vector3<real_Num> &localScale ) = 0;

            /**
             * @brief Gets the local orientation of the actor.
             * @return The local orientation.
             */
            virtual Quaternion<real_Num> getLocalOrientation() const = 0;

            /**
             * @brief Sets the local orientation of the actor.
             * @param localOrientation The new local orientation.
             */
            virtual void setLocalOrientation( const Quaternion<real_Num> &localOrientation ) = 0;

            /**
             * @brief Gets the local rotation of the actor.
             * @return The local rotation.
             */
            virtual Vector3<real_Num> getLocalRotation() const = 0;

            /**
             * @brief Sets the local rotation of the actor.
             * @param localRotation The new local rotation.
             */
            virtual void setLocalRotation( const Vector3<real_Num> &localRotation ) = 0;

            /**
             * @brief Gets the world position of the actor.
             * @return The actor's current position in world space.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Rotates the actor to face another actor.
             * @param actor The target actor to look at.
             */
            virtual void lookAt( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Rotates the actor to face a specific world position.
             * @param position The target world position to look at.
             */
            virtual void lookAt( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Rotates the actor to face a world position with a specified yaw axis.
             * @param position The target world position.
             * @param yawAxis The axis to use for the yaw rotation.
             */
            virtual void lookAt( const Vector3<real_Num> &position,
                                 const Vector3<real_Num> &yawAxis ) = 0;

            /**
             * @brief Sets the world position of the actor.
             * @param position The new world position coordinates.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Gets the world scale of the actor.
             * @return The actor's current scale in world space.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Sets the world scale of the actor.
             * @param scale The new world scale vector.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /**
             * @brief Gets the world orientation of the actor.
             * @return The actor's current orientation as a quaternion in world space.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Sets the world orientation of the actor.
             * @param orientation The new orientation quaternion.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Gets the world rotation of the actor.
             * @return The actor's current rotation in world space (Euler angles).
             */
            virtual Vector3<real_Num> getRotation() const = 0;

            /**
             * @brief Sets the world rotation of the actor.
             * @param rotation The new world rotation vector.
             */
            virtual void setRotation( const Vector3<real_Num> &rotation ) = 0;

            /**
             * @brief Callback invoked when the level is loaded.
             * @param scene Smart pointer to the loaded scene.
             */
            virtual void levelWasLoaded( SmartPtr<IGameScene> scene ) = 0;

            /**
             * @brief Callback invoked when the actor's position in the scene hierarchy changes.
             */
            virtual void hierarchyChanged() = 0;

            /**
             * @brief Callback invoked when a child is directly added to this actor.
             * @param child Smart pointer to the added child actor.
             */
            virtual void childAdded( SmartPtr<IGameActor> child ) = 0;

            /**
             * @brief Callback invoked when a child is directly removed from this actor.
             * @param child Smart pointer to the removed child actor.
             */
            virtual void childRemoved( SmartPtr<IGameActor> child ) = 0;

            /**
             * @brief Callback invoked when a child is added anywhere within this actor's subtree.
             * @param child Smart pointer to the added child actor.
             */
            virtual void childAddedInHierarchy( SmartPtr<IGameActor> child ) = 0;

            /**
             * @brief Callback invoked when a child is removed from anywhere within this actor's subtree.
             * @param child Smart pointer to the removed child actor.
             */
            virtual void childRemovedInHierarchy( SmartPtr<IGameActor> child ) = 0;

            /**
             * @brief Returns whether the actor is marked as perpetual.
             * @return True if the actor should persist across scene loads, false otherwise.
             */
            virtual bool getPerpetual() const = 0;

            /**
             * @brief Sets whether the actor is marked as perpetual.
             * @param perpetual True to make the actor persist across scene loads.
             * @param cascade If true, the perpetual flag is applied to all children recursively.
             */
            virtual void setPerpetual( bool perpetual, bool cascade = false ) = 0;

            /**
             * @brief Adds a component instance to the actor.
             * @param component Smart pointer to the component to be added.
             */
            virtual void addComponentInstance( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Removes a component instance from the actor.
             * @param component Smart pointer to the component to be removed.
             */
            virtual void removeComponentInstance( SmartPtr<IComponent> component ) = 0;

            /** @brief Checks if the actor has a component associated with the given handle. */
            virtual bool hasComponent( hash_type id ) = 0;

            /** @brief Retrieves a component by its handle. Returns nullptr if no component is found. */
            virtual SmartPtr<IComponent> getComponent( hash_type id ) const = 0;

            /**
             * @brief Retrieves all components attached to the actor.
             * @return An array of smart pointers to the actor's components.
             */
            virtual Array<SmartPtr<IComponent>> getComponents() const = 0;

            /**
             * @brief Finds a child actor by its name.
             * @param name The name of the child actor to search for.
             * @param cascade If true, the search will recursively traverse the entire child subtree.
             * @return Smart pointer to the matching child actor, or nullptr if not found.
             * @note This function is not thread-safe.
             */
            virtual SmartPtr<IGameActor> findChildByName( const String &name,
                                                          bool cascade = true ) const = 0;

            /**
             * @brief Finds a child actor by its name and returns a raw pointer.
             * @param name The name of the child actor to search for.
             * @param cascade If true, the search will recursively traverse the entire child subtree.
             * @return Raw pointer to the matching child actor, or nullptr if not found.
             * @note This function is not thread-safe.
             */
            virtual IGameActor *findChildByNamePtr( const String &name, bool cascade = true ) const = 0;

            /**
             * @brief Retrieves a child actor by its index in the child list.
             * @param index The zero-based index of the child.
             * @return Smart pointer to the child actor at the specified index.
             */
            virtual SmartPtr<IGameActor> getChildByIndex( u32 index ) const = 0;

            /**
             * @brief Returns the total number of direct children.
             * @return The number of child actors.
             */
            virtual u32 getNumChildren() const = 0;

            /**
             * @brief Returns the index of this actor relative to its siblings.
             * @return The sibling index.
             */
            virtual s32 getSiblingIndex() const = 0;

            /**
             * @brief Triggers the enter event for a component collision.
             * @param collision The component that collided.
             */
            virtual void triggerEnter( SmartPtr<IComponent> collision ) = 0;

            /**
             * @brief Triggers the leave event for a component collision.
             * @param collision The component that collided.
             */
            virtual void triggerLeave( SmartPtr<IComponent> collision ) = 0;

            /**
             * @brief Called when a component is loaded.
             * @param loadedComponent The loaded component.
             */
            virtual void componentLoaded( SmartPtr<IComponent> loadedComponent ) = 0;

            /**
             * @brief Compares the tag with the actor's tag.
             * @param tag The tag to compare.
             * @return True if the tag matches, false otherwise.
             */
            virtual bool compareTag( const String &tag ) const = 0;

            /**
             * @brief Gets the actor's tags.
             * @return An array of tags.
             */
            virtual Array<String> getTags() const = 0;

            /**
             * @brief Sets the actor's tags.
             * @param tags The array of tags to set.
             */
            virtual void setTags( const Array<String> &tags ) = 0;

            /**
             * @brief Adds a tag to the actor.
             * @param tag The tag to add.
             */
            virtual void addTag( const String &tag ) = 0;

            /**
             * @brief Removes a tag from the actor.
             * @param tag The tag to remove.
             */
            virtual void removeTag( const String &tag ) = 0;

            /**
             * @brief Checks if a tag exists on the actor.
             * @param tag The tag to check.
             * @return True if the tag exists, false otherwise.
             */
            virtual bool hasTag( const String &tag ) const = 0;

            /**
             * @brief Clears all tags from the actor.
             */
            virtual void clearTags() = 0;

            /**
             * @brief Gets the actor's layer.
             * @return The actor's layer.
             */
            virtual String getLayer() const = 0;

            /**
             * @brief Sets the actor's layer.
             * @param layer The new layer.
             */
            virtual void setLayer( const String &layer ) = 0;

            /**
             * @brief Gets the parent actor as a raw pointer.
             * @return Pointer to the parent actor.
             */
            virtual IGameActor *getParentPtr() const = 0;

            /**
             * @brief Gets the parent actor as a smart pointer.
             * @return Smart pointer to the parent actor.
             */
            virtual SmartPtr<IGameActor> getParent() const = 0;

            /**
             * @brief Sets the parent actor. Used internally.
             * @param parent The new parent actor.
             */
            virtual void setParent( SmartPtr<IGameActor> parent ) = 0;

            /**
             * @brief Adds a child actor to this actor.
             * @param child The child actor to add.
             */
            virtual void addChild( SmartPtr<IGameActor> child ) = 0;

            /**
             * @brief Removes a child actor from this actor.
             * @param child The child actor to remove.
             */
            virtual void removeChild( SmartPtr<IGameActor> child ) = 0;

            /**
             * @brief Removes all children from this actor.
             */
            virtual void removeChildren() = 0;

            /**
             * @brief Destroys all children of this actor.
             */
            virtual void destroyChildren() = 0;

            /**
             * @brief Returns the child with the specified name, or nullptr if not found.
             * @param name The name of the child.
             * @return Smart pointer to the child actor.
             */
            virtual SmartPtr<IGameActor> findChild( const String &name ) = 0;

            /**
             * @brief Gets all children of this actor.
             * @return An array of child actors.
             */
            virtual Array<SmartPtr<IGameActor>> getChildren() const = 0;

            /**
             * @brief Gets all children recursively.
             * @return An array of all child actors.
             */
            virtual Array<SmartPtr<IGameActor>> getAllChildren() const = 0;

            /**
             * @brief Sets the sibling index of this actor.
             * @param index The new sibling index.
             */
            virtual void setSiblingIndex( s32 index ) = 0;

            /**
             * @brief Sets the sibling index of a specific child actor.
             * @param child Smart pointer to the child actor.
             * @param index The new sibling index.
             */
            virtual void setChildSiblingIndex( SmartPtr<IGameActor> child, s32 index ) = 0;

            /**
             * @brief Returns whether this actor is owned by the local client in a network game.
             * @return True if the actor is client-owned, false otherwise.
             */
            virtual bool isMine() const = 0;

            /**
             * @brief Sets whether the actor is owned by the local client in a network game.
             * @param mine True if the actor should be marked as client-owned.
             */
            virtual void setMine( bool mine ) = 0;

            /**
             * @brief Returns whether the actor is marked as static.
             * @return True if the actor is static (non-moving), false otherwise.
             */
            virtual bool isStatic() const = 0;

            /**
             * @brief Sets whether the actor is static.
             * @param isstatic True if the actor is static.
             */
            virtual void setStatic( bool isstatic, bool cacade = true ) = 0;

            /**
             * @brief Returns whether the actor is currently enabled within the scene context.
             * @return True if enabled in the scene, false otherwise.
             */
            virtual bool isEnabledInScene() const = 0;

            /**
             * @brief Returns whether the actor is enabled.
             * @return True if the actor is enabled, false otherwise.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Sets whether the actor is enabled.
             * @param enabled True if the actor is enabled.
             */
            virtual void setEnabled( bool enabled, bool cacade = false ) = 0;

            /**
             * @brief Returns whether the actor is visible.
             * @return True if the actor is visible, false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Sets whether the actor is visible.
             * @param visible True to make the actor visible.
             * @param cascade If true, applies this status to all children recursively.
             */
            virtual void setVisible( bool visible, bool cacade = true ) = 0;

            /**
             * @brief Returns whether the actor is marked as dirty.
             * @return True if the actor requires an update, false otherwise.
             */
            virtual bool isDirty() const = 0;

            /**
             * @brief Sets whether the actor is marked as dirty.
             * @param dirty True if the actor is dirty.
             */
            virtual void setDirty( bool dirty, bool cacade = false ) = 0;

            /**
             * @brief Returns whether the actor uses smooth motion interpolation.
             * @return True if smooth motion is enabled, false otherwise.
             */
            virtual bool isSmoothMotion() const = 0;

            /**
             * @brief Sets whether the actor uses smooth motion interpolation.
             * @param smoothMotion True to enable smooth motion.
             */
            virtual void setSmoothMotion( bool smoothMotion, bool cascade = false ) = 0;

            /**
             * @brief Gets the actor collision mask used for physics filtering.
             * @return Collision mask bitfield.
             */
            virtual u32 getCollisionMask() const = 0;

            /**
             * @brief Sets the actor collision mask used for physics filtering.
             * @param collisionMask Collision mask bitfield.
             * @param cascade If true, applies to children as well.
             */
            virtual void setCollisionMask( u32 collisionMask, bool cascade = false ) = 0;

            /**
             * @brief Updates the actor and component transformations.
             */
            virtual void updateTransform() = 0;

            /**
             * @brief Gets the root actor attached to the scene.
             * @return Smart pointer to the root actor.
             */
            virtual SmartPtr<IGameActor> getSceneRoot() const = 0;

            /**
             * @brief Gets the root actor attached to the scene as a raw pointer.
             * @return Pointer to the root actor.
             */
            virtual IGameActor *getSceneRootPtr() const = 0;

            /**
             * @brief Gets the scene depth of this actor in the hierarchy.
             * @return The scene depth.
             */
            virtual u32 getSceneLevel() const = 0;

            /**
             * @brief Gets the actor's transform as a raw pointer.
             * @return Pointer to the transform.
             */
            virtual ITransform *getTransformPtr() const = 0;

            /**
             * @brief Gets the actor's transform as a smart pointer (modifiable).
             * @return Reference to a smart pointer to the transform.
             */
            virtual SmartPtr<ITransform> getTransform() const = 0;

            /**
             * @brief Sets the actor's transform.
             * @param transform The new transform.
             */
            virtual void setTransform( SmartPtr<ITransform> transform ) = 0;

            /**
             * @brief Sets the state of the actor.
             * @param state The new state to transition to.
             * @param cascade If true, applies this state to all children recursively.
             */
            virtual void setState( State state, bool cacade = true ) = 0;

            /**
             * @brief Returns the current state of the actor.
             * @return The actor's current state.
             */
            virtual State getState() const = 0;

            /**
             * @brief Returns the parent actor of this actor.
             * @return Smart pointer to the parent actor.
             */
            virtual u32 getPreviousFlags() const = 0;

            /**
             * @brief Returns the current flags of the actor.
             * @param flags The current flags.
             */
            virtual void setPreviousFlags( u32 flags ) = 0;

            /**
             * @brief Returns the current status flags of the actor.
             * @return The flags as a bitmask.
             */
            virtual u32 getFlags() const = 0;

            /**
             * @brief Sets the status flags of the actor.
             * @param flags The new flags bitmask.
             */
            virtual void setFlags( u32 flags ) = 0;

            /**
             * @brief Returns whether a specific flag is set.
             * @param flag The flag to check.
             * @return True if the flag is set, false otherwise.
             */
            virtual bool getFlag( u32 flag ) const = 0;

            /**
             * @brief Sets a specific flag of the actor.
             * @param flag The flag to modify.
             * @param value The new value of the flag.
             * @param cascade If true, applies this change to all children recursively.
             */
            virtual void setFlag( u32 flag, bool value, bool cascade = false ) = 0;

            /**
             * @brief Returns the scene this actor belongs to as a raw pointer.
             * @return Raw pointer to the scene.
             */
            virtual IGameScene *getScenePtr() const = 0;

            /**
             * @brief Returns the scene this actor belongs to as a smart pointer.
             * @return Smart pointer to the scene.
             */
            virtual SmartPtr<IGameScene> getScene() const = 0;

            /**
             * @brief Assigns the scene this actor belongs to.
             * @param scene Smart pointer to the scene.
             */
            virtual void setScene( SmartPtr<IGameScene> scene ) = 0;

            /**
             * @brief Updates the actor's visibility flags.
             */
            virtual void updateVisibility() = 0;

            /**
             * @brief Updates the rendering or processing order of components.
             * @param cascade If true, applies the update to all children recursively.
             */
            virtual void updateOrder( bool cascade = true ) = 0;

            /**
             * @brief Processes an incoming event.
             * @param eventType The type of event.
             * @param eventValue The value associated with the event.
             * @param arguments Additional arguments passed with the event.
             * @param sender The object that sent the event.
             * @param object The target object of the event.
             * @param event The event object itself.
             * @return The result of the event processing.
             */
            virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                           const Array<Parameter> &arguments,
                                           SmartPtr<ISharedObject> sender,
                                           SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

            /**
             * @brief Dispatches an event to a target object.
             * @param eventType The type of event to dispatch.
             * @param eventValue The value associated with the event.
             * @param arguments Additional arguments passed with the event.
             * @param sender The object sending the event.
             * @param object The target object receiving the event.
             * @param event The event object itself.
             * @return The result of the event dispatch.
             */
            virtual Parameter sendEvent( EventType eventType, hash_type eventValue,
                                         const Array<Parameter> &arguments,
                                         SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                         SmartPtr<IEvent> event ) = 0;

            /** @brief Synchronizes components' enabled/active state with the actor's state. */
            virtual void updateComponentsState() = 0;

            virtual AABB3<real_Num> getLocalAABB() const = 0;

            virtual void setLocalAABB( const AABB3<real_Num> &localAABB ) = 0;

            /** @brief Returns the axis-aligned bounds enclosing this actor in world space. */
            virtual AABB3<real_Num> getWorldAABB() const = 0;

            /** @brief Returns the actor's local bounds placed by its world transform. */
            virtual OBB3<real_Num> getWorldOBB() const = 0;

            /**
             * Submits both world AABB and OBB representations to a debug renderer.

             * * @param debug Debug renderer that receives the wireframe primitives.
             *
             * @param aabbColour Packed RGBA colour for the axis-aligned bounds.
             * @param
             * obbColour Packed RGBA colour for the oriented bounds.
             */
            virtual void drawDebugBounds( render::IDebug &debug, u32 aabbColour = 0x00ff00ffu,
                                          u32 obbColour = 0x00ffffffu ) const = 0;

            /** @brief Returns whether this actor submits its bounds every frame. */
            virtual bool isDebugDrawEnabled() const = 0;

            /** @brief Enables or disables automatic per-frame actor bounds drawing. */
            virtual void setDebugDrawEnabled( bool enabled ) = 0;

            virtual f32 getRadius() const = 0;

            virtual void setRadius( f32 radius ) = 0;

            /**
             * @brief Adds a component of type T to the actor.
             * @tparam T The component class type.
             * @return Smart pointer to the newly created component.
             */
            template <class T>
            SmartPtr<T> addComponent();

            /**
             * @brief Adds a component of type T to the actor and returns a raw pointer.
             * @tparam T The component class type.
             * @return Raw pointer to the newly created component.
             */
            template <class T>
            T *addComponentPtr();

            /**
             * @brief Retrieves a component of type T by its unique identifier.
             * @tparam T The component class type.
             * @param id The unique ID of the component.
             * @return Smart pointer to the component, or nullptr if not found.
             */
            template <class T>
            SmartPtr<T> getComponentById( s32 id ) const;

            /**
             * @brief Retrieves a component of type T.
             * @tparam T The component class type.
             * @return Smart pointer to the component, or nullptr if not found.
             */
            template <class T>
            SmartPtr<T> getComponent() const;

            /**
             * @brief Retrieves a component of type T as a raw pointer.
             * @tparam T The component class type.
             * @return Raw pointer to the component, or nullptr if not found.
             */
            template <class T>
            T *getComponentPtr() const;

            /**
             * @brief Gets a component of type T in the actor's children.
             * @tparam T The type of component.
             * @return Smart pointer to the component, or nullptr if not found.
             */
            template <class T>
            SmartPtr<T> getComponentInChildren() const;

            /**
             * @brief Gets a component of type T in this actor and its children.
             * @tparam T The type of component.
             * @return Smart pointer to the component, or nullptr if not found.
             */
            template <class T>
            SmartPtr<T> getComponentInThisAndChildren() const;

            /**
             * @brief Gets all components of type T in the actor's children.
             * @tparam T The type of component.
             * @return Array of smart pointers to components.
             */
            template <class T>
            Array<SmartPtr<T>> getComponentsInChildren() const;

            /**
             * @brief Gets all components of type T in the actor's children as raw pointers.
             * @tparam T The type of component.
             * @return Array of raw pointers to components.
             */
            template <class T>
            List<T *> getComponentsInChildrenPtr() const;

            /**
             * @brief Gets all components of type T in all children recursively.
             * @tparam T The type of component.
             * @return Array of smart pointers to components.
             */
            template <class T>
            Array<SmartPtr<T>> getAllComponentsInChildren() const;

            /**
             * @brief Gets all components of type T in all children recursively as raw pointers.
             * @tparam T The type of component.
             * @return Array of raw pointers to components.
             */
            template <class T>
            Array<T *> getAllComponentsInChildrenPtr() const;

            /**
             * @brief Gets all components of type T in this actor and its children.
             * @tparam T The type of component.
             * @return Array of smart pointers to components.
             */
            template <class T>
            Array<SmartPtr<T>> getComponentsAndInChildren() const;

            /**
             * @brief Gets all components of type T in this actor and all children recursively.
             * @tparam T The type of component.
             * @return Array of smart pointers to components.
             */
            template <class T>
            Array<SmartPtr<T>> getAllComponentsAndInChildren() const;

            /**
             * @brief Gets all components of type T in this actor and all children recursively as raw
             * pointers.
             * @tparam T The type of component.
             * @return Array of raw pointers to components.
             */
            template <class T>
            Array<T *> getAllComponentsAndInChildrenPtr() const;

            /**
             * @brief Gets all components of type T in this actor.
             * @tparam T The type of component.
             * @return Array of smart pointers to components.
             */
            template <class T>
            Array<SmartPtr<T>> getComponentsByType() const;

            /**
             * @brief Gets all components of type T in this actor as raw pointers.
             * @tparam T The type of component.
             * @return Array of raw pointers to components.
             */
            template <class T>
            List<T *> getComponentsByTypePtr() const;

            /**
             * @brief Checks if the actor has a component of type T.
             * @tparam T The type of component.
             * @return True if the component exists, false otherwise.
             */
            template <class T>
            bool hasComponent() const;

            /**
             * @brief Gets a component of type T in the parent actor.
             * @tparam T The type of component.
             * @return Smart pointer to the component, or nullptr if not found.
             */
            template <typename T>
            SmartPtr<T> getComponentInParent();

            // 'c' style linked list.
            AtomicRawPtr<IGameActor> next;

            // 'c' style linked list.
            AtomicRawPtr<IGameActor> nextSibling;

            // 'c' style linked list.
            AtomicRawPtr<IGameActor> nextChild;

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        SmartPtr<T> IGameActor::addComponent()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager->getFactoryManagerPtr();

            auto component = factoryManager->make_ptr<T>();
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( component ) );

            addComponentInstance( component );
            component->load( nullptr );
            return component;
        }

        template <class T>
        T *IGameActor::addComponentPtr()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager->getFactoryManagerPtr();

            auto component = factoryManager->make_ptr<T>();
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( component ) );

            addComponentInstance( component );
            component->load( nullptr );
            return component.get();
        }

        template <class T>
        SmartPtr<T> IGameActor::getComponentById( s32 id ) const
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component )
                {
                    auto derivedComponent = workphone::dynamic_pointer_cast<T>( component );
                    if( derivedComponent )
                    {
                        auto handle = derivedComponent->getHandle();
                        WP_ASSERT( handle );

                        if( handle->getId() == id )
                        {
                            return derivedComponent;
                        }
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<T> IGameActor::getComponent() const
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component->isDerived<T>() )
                {
                    return SmartPtr<T>( component );
                }
            }

            return nullptr;
        }

        template <class T>
        T *IGameActor::getComponentPtr() const
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component->isDerived<T>() )
                {
                    return (T *)component.get();
                }
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<T> IGameActor::getComponentInChildren() const
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    if( auto component = child->getComponent<T>() )
                    {
                        return component;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<T> IGameActor::getComponentInThisAndChildren() const
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component )
                {
                    if( auto derivedComponent = workphone::dynamic_pointer_cast<T>( component ) )
                    {
                        return derivedComponent;
                    }
                }
            }

            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    if( auto component = child->getComponentInThisAndChildren<T>() )
                    {
                        return component;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        Array<SmartPtr<T>> IGameActor::getComponentsInChildren() const
        {
            Array<SmartPtr<T>> componentsInChildren;
            componentsInChildren.reserve( 32 );

            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    auto components = child->getComponentsByType<T>();
                    std::copy( components.begin(), components.end(),
                               std::back_inserter( componentsInChildren ) );

                    auto childComponents = child->getComponentsInChildren<T>();
                    std::copy( childComponents.begin(), childComponents.end(),
                               std::back_inserter( componentsInChildren ) );
                }
            }

            return componentsInChildren;
        }

        template <class T>
        List<T *> IGameActor::getComponentsInChildrenPtr() const
        {
            auto children = getChildren();

            List<T *> componentsInChildren;

            for( auto &child : children )
            {
                if( child )
                {
                    auto components = child->getComponentsByTypePtr<T>();
                    for( auto &component : components )
                    {
                        componentsInChildren.push_back( component );
                    }

                    auto childComponents = child->getComponentsInChildrenPtr<T>();
                    for( auto &component : childComponents )
                    {
                        componentsInChildren.push_back( component );
                    }
                }
            }

            return componentsInChildren;
        }

        template <class T>
        Array<SmartPtr<T>> IGameActor::getAllComponentsInChildren() const
        {
            Array<SmartPtr<T>> componentsInChildren;
            componentsInChildren.reserve( 32 );

            auto children = getAllChildren();
            for( auto &child : children )
            {
                auto childComponents = child->getComponentsByType<T>();
                std::copy( childComponents.begin(), childComponents.end(),
                           std::back_inserter( componentsInChildren ) );
            }

            return componentsInChildren;
        }

        template <class T>
        Array<T *> IGameActor::getAllComponentsInChildrenPtr() const
        {
            Array<T *> componentsInChildren;
            componentsInChildren.reserve( 32 );

            auto children = getAllChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    auto components = child->getComponentsByTypePtr<T>();
                    std::copy( components.begin(), components.end(),
                               std::back_inserter( componentsInChildren ) );
                }
            }

            return componentsInChildren;
        }

        template <class T>
        Array<SmartPtr<T>> IGameActor::getComponentsAndInChildren() const
        {
            const auto components = getComponents();

            Array<SmartPtr<T>> componentsInChildren;
            componentsInChildren.reserve( components.size() );

            for( auto &component : components )
            {
                if( component )
                {
                    if( auto derivedComponent = workphone::dynamic_pointer_cast<T>( component ) )
                    {
                        componentsInChildren.push_back( derivedComponent );
                    }
                }
            }

            auto children = getChildren();
            for( auto &child : children )
            {
                auto childComponents = child->getComponentsInChildren<T>();
                std::copy( childComponents.begin(), childComponents.end(),
                           std::back_inserter( componentsInChildren ) );
            }

            return componentsInChildren;
        }

        template <class T>
        Array<SmartPtr<T>> IGameActor::getAllComponentsAndInChildren() const
        {
            const auto components = getComponents();

            Array<SmartPtr<T>> componentsInChildren;
            componentsInChildren.reserve( components.size() );

            for( const auto &component : components )
            {
                if( component && component->isDerived<T>() )
                {
                    componentsInChildren.push_back( component );
                }
            }

            auto children = getAllChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    const auto childComponents = child->getComponents();
                    for( const auto &childComponent : childComponents )
                    {
                        if( childComponent && childComponent->isDerived<T>() )
                        {
                            componentsInChildren.push_back( childComponent );
                        }
                    }
                }
            }

            return componentsInChildren;
        }

        template <class T>
        Array<T *> IGameActor::getAllComponentsAndInChildrenPtr() const
        {
            Array<T *> componentsInChildren;
            componentsInChildren.reserve( 32 );

            auto components = getComponents();
            for( auto &component : components )
            {
                if( component->isDerived<T>() )
                {
                    componentsInChildren.push_back( (T *)component );
                }

                component = static_cast<IComponent *>( component->next.get() );
            }

            auto children = getAllChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    auto childComponents = child->getComponents();
                    for( auto &childComponent : childComponents )
                    {
                        if( childComponent->isDerived<T>() )
                        {
                            componentsInChildren.push_back( (T *)childComponent.get() );
                        }
                    }
                }
            }

            return componentsInChildren;
        }

        template <class T>
        Array<SmartPtr<T>> IGameActor::getComponentsByType() const
        {
            Array<SmartPtr<T>> componentsByType;
            componentsByType.reserve( 12 );

            auto components = this->getComponents();
            for( auto component : components )
            {
                if( component && component->isDerived<T>() )
                {
                    auto derivedComponent = workphone::static_pointer_cast<T>( component );
                    componentsByType.push_back( derivedComponent );
                }
            }

            return componentsByType;
        }

        template <class T>
        List<T *> IGameActor::getComponentsByTypePtr() const
        {
            List<T *> componentsByType;

            auto components = getComponents();
            for( auto &component : components )
            {
                if( component->isDerived<T>() )
                {
                    componentsByType.push_back( (T *)component.get() );
                }
            }

            return componentsByType;
        }

        template <class T>
        bool IGameActor::hasComponent() const
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component->isDerived<T>() )
                {
                    return true;
                }
            }

            return false;
        }

        template <typename T>
        SmartPtr<T> IGameActor::getComponentInParent()
        {
            auto currentActor = getSharedFromThis<IGameActor>();

            while( currentActor != nullptr )
            {
                auto componentInParent = currentActor->getComponent<T>();
                if( componentInParent != nullptr )
                {
                    return componentInParent;
                }

                currentActor = currentActor->getParent();
            }

            return nullptr;
        }
    }  // namespace scene
}  // namespace workphone

#endif  // IActor_h__
