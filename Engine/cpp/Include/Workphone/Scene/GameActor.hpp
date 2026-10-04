#ifndef __WP_GameActor_h__
#define __WP_GameActor_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentFixedArrayGrowable.hpp>
#include <Workphone/System/FSMListener.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class GameActor
         * @brief Concrete implementation of the IGameActor interface.
         *
         * GameActor is the primary scene node used by the engine. Each actor owns a transform
         * and a set of components, participates in a parent/child hierarchy and can be
         * enabled/disabled, visible/invisible, static/dynamic and tagged with arbitrary strings.
         *
         * Core responsibilities:
         *  - Lifetime management and (de)serialization via Resource<IGameActor> base class.
         *  - Maintain local and world transforms and provide time-sampled access for interpolation.
         *  - Host components which provide rendering, physics and gameplay behaviour.
         *  - Participate in scene graph operations: parenting, child management and cascading state.
         *
         * Concurrency:
         *  - Components and children use intrusive lists protected by the actor mutex; tags use
         *    a concurrent container.
         *  - A mutex API (lock/try_lock/unlock and shared variants) is provided for callers that
         *    require stronger external synchronization for multi-step operations.
         *
         * The public API mostly mirrors IGameActor; many methods are documented via @copydoc
         * to keep interface documentation centralized.
         */
        class WPCore_API GameActor : public Resource<IGameActor>
        {
        public:
            /**
             * @brief FSM listener that forwards state-machine events to its owning actor.
             *
             * The listener keeps only a weak reference to the GameActor to avoid creating
             * reference cycles between the actor and its internal FSM. When FSM events
             * arrive they are translated and dispatched on behalf of the actor instance.
             */
            class FsmListener : public FSMListener
            {
            public:
                FsmListener();
                ~FsmListener() override;

                /**
                 * @brief Called when the FSM is being loaded.
                 * @param data Optional data passed by the FSM.
                 */
                void load( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Called when the FSM is being unloaded.
                 * @param data Optional data passed by the FSM.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle a state machine event for the actor FSM.
                 * @param state Current state id.
                 * @param eventType Event being handled.
                 * @return FSMReturnType result for the FSM processing.
                 */
                FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

                /**
                 * @brief Return the owning GameActor.
                 * @return SmartPtr<GameActor> smart pointer to the owner or null if the owner
                 *         has expired or was never set.
                 */
                SmartPtr<GameActor> getOwner() const;

                /**
                 * @brief Assign the owning GameActor for this listener.
                 * @param owner Smart pointer to the actor to set as owner. May be null.
                 */
                void setOwner( SmartPtr<GameActor> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<GameActor> m_owner;
            };

            /**
             * @brief Default constructor.
             *
             * Constructs an actor with default properties. The actor will not be attached
             * to any scene or parent and will contain no components.
             */
            GameActor();

            /**
             * @brief Construct an actor with a pre-defined id.
             * @param id Unique identifier to assign to this actor. This is primarily used
             *           when creating actors from persisted data where the id must be preserved.
             */
            GameActor( s32 id );

            /**
             * @brief Destructor.
             *
             * Ensures components are cleaned up and the actor is detached from its scene and
             * parent prior to destruction.
             */
            ~GameActor() override;

            /** @copydoc IActor::getLocalTransform */
            Transform3<real_Num> getLocalTransform() const override;

            /** @copydoc IActor::getWorldTransform */
            Transform3<real_Num> getWorldTransform() const override;

            /** @copydoc IActor::getLocalTransform */
            Transform3<real_Num> getLocalTransform( time_interval t ) const override;

            /** @copydoc IActor::getWorldTransform */
            Transform3<real_Num> getWorldTransform( time_interval t ) const override;

            /** @copydoc IActor::getLocalPosition */
            Vector3<real_Num> getLocalPosition() const override;

            /** @copydoc IActor::setLocalPosition */
            void setLocalPosition( const Vector3<real_Num> &localPosition ) override;

            /** @copydoc IActor::getLocalScale */
            Vector3<real_Num> getLocalScale() const override;

            /** @copydoc IActor::setLocalScale */
            void setLocalScale( const Vector3<real_Num> &localScale ) override;

            /** @copydoc IActor::getLocalOrientation */
            Quaternion<real_Num> getLocalOrientation() const override;

            /** @copydoc IActor::setLocalOrientation */
            void setLocalOrientation( const Quaternion<real_Num> &localOrientation ) override;

            /** @copydoc IActor::getLocalRotation */
            Vector3<real_Num> getLocalRotation() const override;

            /** @copydoc IActor::setLocalRotation */
            void setLocalRotation( const Vector3<real_Num> &localRotation ) override;

            /** @copydoc IActor::getPosition */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Make this actor look at another actor.
             * @param actor Target actor to look at. If null, behaviour is undefined.
             */
            void lookAt( SmartPtr<IGameActor> actor ) override;

            /**
             * @brief Make this actor look at a world position.
             * @param position Target world position.
             */
            void lookAt( const Vector3<real_Num> &position ) override;

            /**
             * @brief Make this actor look at a world position using a custom yaw axis.
             * @param position Target world position.
             * @param yawAxis Axis to use for the yaw calculation.
             */
            void lookAt( const Vector3<real_Num> &position, const Vector3<real_Num> &yawAxis ) override;

            /** @copydoc IActor::setPosition */
            void setPosition( const Vector3<real_Num> &position ) override;

            /** @copydoc IActor::getScale */
            Vector3<real_Num> getScale() const override;

            /** @copydoc IActor::setScale */
            void setScale( const Vector3<real_Num> &scale ) override;

            /** @copydoc IActor::getOrientation */
            Quaternion<real_Num> getOrientation() const override;

            /** @copydoc IActor::setOrientation */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

            /** @copydoc IActor::getRotation */
            Vector3<real_Num> getRotation() const override;

            /** @copydoc IActor::setRotation */
            void setRotation( const Vector3<real_Num> &rotation ) override;

            /** @copydoc IActor::levelWasLoaded */
            void levelWasLoaded( SmartPtr<IGameScene> scene ) override;

            /** @copydoc IActor::hierarchyChanged */
            void hierarchyChanged() override;

            /** @copydoc IActor::childAdded */
            void childAdded( SmartPtr<IGameActor> child ) override;

            /** @copydoc IActor::childRemoved */
            void childRemoved( SmartPtr<IGameActor> child ) override;

            /** @copydoc IActor::updateDirty */
            void updateDirty( u32 flags, u32 oldFlags ) override;

            /** @copydoc IActor::preUpdate */
            void preUpdate() override;

            /** @copydoc IActor::update */
            void update() override;

            /** @copydoc IActor::postUpdate */
            void postUpdate() override;

            /** @copydoc Resource<IGameActor>::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource<IGameActor>::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource<IGameActor>::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource<IGameActor>::addComponentInstance */
            void addComponentInstance( SmartPtr<IComponent> component ) override;

            /** @copydoc Resource<IGameActor>::removeComponent */
            void removeComponentInstance( SmartPtr<IComponent> component ) override;

            /**
             * @brief Check whether the actor contains a component with the specified id.
             * @param id Component handle (hash) to search for.
             * @return true if a matching component is attached to this actor.
             */
            bool hasComponent( hash_type id ) override;

            /**
             * @brief Retrieve a component attached to this actor by its handle.
             * @param id Component handle (hash) to search for.
             * @return SmartPtr<IComponent> pointer to the component, or null if not found.
             */
            SmartPtr<IComponent> getComponent( hash_type id ) const override;

            /** @copydoc IActor::getComponents */
            Array<SmartPtr<IComponent>> getComponents() const override;

            /** @copydoc IActor::getPerpetual */
            bool getPerpetual() const override;

            /**
             * @brief Set perpetual flag on this actor.
             * @param perpetual If true, actor will persist across certain scene operations.
             * @param cascade If true, apply the change to child actors as well.
             */
            void setPerpetual( bool perpetual, bool cascade = false ) override;

            /** @copydoc IActor::getScenePtr */
            IGameScene *getScenePtr() const override;

            /** @copydoc IActor::getScene */
            SmartPtr<IGameScene> getScene() const override;

            /**
             * @brief Assign this actor to a scene.
             * @param scene Scene to assign. Passing null detaches the actor from its scene.
             */
            void setScene( SmartPtr<IGameScene> scene ) override;

            /** @copydoc IActor::triggerEnter */
            void triggerEnter( SmartPtr<IComponent> collision ) override;

            /** @copydoc IActor::triggerLeave */
            void triggerLeave( SmartPtr<IComponent> collision ) override;

            /** @copydoc IActor::componentLoaded */
            void componentLoaded( SmartPtr<IComponent> loadedComponent ) override;

            /** @copydoc IActor::findChildByName */
            SmartPtr<IGameActor> findChildByName( const String &name,
                                                  bool cascade = true ) const override;

            /** @copydoc IActor::findChildByNamePtr */
            IGameActor *findChildByNamePtr( const String &name, bool cascade = true ) const override;

            /** @copydoc IActor::getChildByIndex */
            SmartPtr<IGameActor> getChildByIndex( u32 index ) const override;

            /** @copydoc IActor::getNumChildren */
            u32 getNumChildren() const override;

            /** @copydoc IActor::getSiblingIndex */
            s32 getSiblingIndex() const override;

            /**
             * @brief Add a child actor to this actor.
             * @param child Actor to add. If child already has a parent it will be reparented.
             */
            void addChild( SmartPtr<IGameActor> child ) override;

            /**
             * @brief Remove a child actor from this actor.
             * @param child Actor to remove. Does not destroy the child.
             */
            void removeChild( SmartPtr<IGameActor> child ) override;

            /** @brief Called when a child is added somewhere in the hierarchy. */
            void childAddedInHierarchy( SmartPtr<IGameActor> child ) override;

            /** @brief Called when a child is removed somewhere in the hierarchy. */
            void childRemovedInHierarchy( SmartPtr<IGameActor> child ) override;

            /**
             * @brief Remove all direct children from this actor.
             *
             * This detaches direct child actors without destroying them. Children are
             * removed from this actor's children collection and their parent pointer is
             * cleared.
             */
            void removeChildren() override;

            /** @copydoc IActor::destroyChildren */
            void destroyChildren() override;

            /**
             * @brief Find a direct child by name.
             * @param name Name of the child to find.
             * @return SmartPtr to the child or null if not found.
             */
            SmartPtr<IGameActor> findChild( const String &name ) override;

            /** @copydoc IActor::getChildren */
            Array<SmartPtr<IGameActor>> getChildren() const override;

            /** @copydoc IActor::getAllChildren */
            Array<SmartPtr<IGameActor>> getAllChildren() const override;

            /** @copydoc IActor::getAllChildren */
            Array<SmartPtr<IGameActor>> getAllChildren( SmartPtr<IGameActor> parent ) const;

            /** @copydoc IActor::setSiblingIndex */
            void setSiblingIndex( s32 index ) override;

            /** @copydoc IActor::setChildSiblingIndex */
            void setChildSiblingIndex( SmartPtr<IGameActor> child, s32 index ) override;

            /** @copydoc IActor::toData */
            SmartPtr<ISharedObject> toData() const override;

            /** @copydoc IActor::fromData */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IActor::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IActor::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IActor::isMine */
            bool isMine() const override;

            /** @copydoc IActor::setMine */
            void setMine( bool mine ) override;

            /** @copydoc IActor::isStatic */
            bool isStatic() const override;

            /** @copydoc IActor::setStatic */
            void setStatic( bool isstatic, bool cascade = true ) override;

            /** @copydoc IActor::isEnabledInScene */
            bool isEnabledInScene() const override;

            /** @brief Return whether the actor is enabled (logical enabled state). */
            bool isEnabled() const override;

            /**
             * @brief Enable or disable the actor.
             * @param enabled True to enable, false to disable.
             */
            void setEnabled( bool enabled, bool cascade = false ) override;

            /** @copydoc IActor::isVisible */
            bool isVisible() const override;

            /** @copydoc IActor::setVisible */
            void setVisible( bool visible, bool cascade = true ) override;

            /** @copydoc IActor::isDirty */
            bool isDirty() const override;

            /** @copydoc IActor::setDirty */
            void setDirty( bool dirty, bool cascade = false ) override;

            /** @copydoc IActor::isSmoothMotion */
            bool isSmoothMotion() const override;

            /** @copydoc IActor::setSmoothMotion */
            void setSmoothMotion( bool smoothMotion, bool cascade = false ) override;

            /** @copydoc IActor::getCollisionMask */
            u32 getCollisionMask() const override;

            /** @copydoc IActor::setCollisionMask */
            void setCollisionMask( u32 collisionMask, bool cascade = false ) override;

            /** @copydoc ISharedObject::isValid */
            bool isValid() const override;

            /** @copydoc IActor::updateTransform */
            void updateTransform() override;

            /** @copydoc IActor::getParentPtr */
            IGameActor *getParentPtr() const override;

            /** @copydoc IActor::getParent */
            SmartPtr<IGameActor> getParent() const override;

            /** @copydoc IActor::setParent */
            void setParent( SmartPtr<IGameActor> parent ) override;

            /** @copydoc IActor::compareTag */
            bool compareTag( const String &tag ) const override;

            /** @copydoc IActor::getTags */
            Array<String> getTags() const override;

            /** @copydoc IActor::setTags */
            void setTags( const Array<String> &tags ) override;

            /** @copydoc IActor::addTag */
            void addTag( const String &tag ) override;

            /** @copydoc IActor::removeTag */
            void removeTag( const String &tag ) override;

            /** @copydoc IActor::hasTag */
            bool hasTag( const String &tag ) const override;

            /** @copydoc IActor::clearTags */
            void clearTags() override;

            /** @copydoc IActor::getLayer */
            String getLayer() const override;

            /** @copydoc IActor::setLayer */
            void setLayer( const String &layer ) override;

            /** @copydoc IActor::getSceneRoot */
            SmartPtr<IGameActor> getSceneRoot() const override;

            /** @copydoc IActor::getSceneRootPtr */
            IGameActor *getSceneRootPtr() const override;

            /** @copydoc IActor::getSceneLevel */
            u32 getSceneLevel() const override;

            /** @copydoc IActor::getTransformPtr */
            ITransform *getTransformPtr() const override;

            /** @copydoc IActor::getTransform */
            SmartPtr<ITransform> getTransform() const override;

            /** @copydoc IActor::setTransform */
            void setTransform( SmartPtr<ITransform> transform ) override;

            /** @copydoc IActor::setState */
            void setState( State state, bool cascade = true ) override;

            /** @copydoc IActor::setState */
            State getState() const override;

            /** @copydoc IActor::getPreviousFlags */
            u32 getPreviousFlags() const override;

            /** @copydoc IActor::setPreviousFlags */
            void setPreviousFlags( u32 flags ) override;

            /** @copydoc IActor::getFlags */
            u32 getFlags() const override;

            /** @copydoc IActor::setFlags */
            void setFlags( u32 flags ) override;

            /** @copydoc IActor::getFlag */
            bool getFlag( u32 flag ) const override;

            /**
             * @brief Set or clear an individual flag.
             * @param flag Flag bit to modify.
             * @param value True to set, false to clear.
             * @param cascade If true, apply to children as well.
             */
            void setFlag( u32 flag, bool value, bool cascade = false ) override;

            /** @copydoc IActor::updateVisibility */
            void updateVisibility() override;

            /**
             * @brief Update the stored order of components. Optionally cascade to children.
             * @param cascade If true, update order on child actors recursively.
             */
            void updateOrder( bool cascade = true ) override;

            /** @copydoc IActor::handleEvent */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /** @copydoc IActor::sendEvent */
            Parameter sendEvent( EventType eventType, hash_type eventValue,
                                 const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                 SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Synchronize attached components' enabled/active state with this actor.
             *
             * Components that are sensitive to the actor's enabled/visible flags will be
             * enabled or disabled according to the actor's current state. This is called
             * automatically when the actor's state changes.
             */
            void updateComponentsState() override;

            /**
             * @brief Get the actor's local axis-aligned bounding box.
             * @return The AABB in local space.
             */
            AABB3<real_Num> getLocalAABB() const override;

            /**
             * @brief Set the actor's local axis-aligned bounding box.
             * @param localAABB The new AABB in local space.
             */
            void setLocalAABB( const AABB3<real_Num> &localAABB ) override;

            /** @copydoc IGameActor::getWorldAABB */
            AABB3<real_Num> getWorldAABB() const override;

            /** @copydoc IGameActor::getWorldOBB */
            OBB3<real_Num> getWorldOBB() const override;

            /** @copydoc IGameActor::drawDebugBounds */
            void drawDebugBounds( render::IDebug &debug, u32 aabbColour = 0x00ff00ffu,
                                  u32 obbColour = 0x00ffffffu ) const override;

            /** @copydoc IGameActor::isDebugDrawEnabled */
            bool isDebugDrawEnabled() const override;

            /** @copydoc IGameActor::setDebugDrawEnabled */
            void setDebugDrawEnabled( bool enabled ) override;

            /**
             * @brief Get the bounding radius of the actor.
             * @return The radius as a float.
             */
            f32 getRadius() const override;

            /**
             * @brief Set the bounding radius of the actor.
             * @param radius The new radius value.
             */
            void setRadius( f32 radius ) override;

            /**
             * @brief Recalculate the local bounds from attached components and child actors.
             *
             * Finite component bounding boxes are merged in actor-local space. Each child's
             * local bounding box is transformed by the child's local transform before it is
             * merged. The bounding radius is then derived from the resulting AABB.
             */
            void updateBounds();

            /**
             * @brief Acquire the actor's exclusive mutex.
             *
             * Callers performing several related operations that must be atomic should
             * obtain this lock to prevent concurrent modifications. A shared-lock API
             * (lock_shared/unlock_shared) is provided for read-dominated access patterns.
             */
            void lock() override;

            /** @copydoc IActor::lock_shared */
            void lock_shared() override;

            /** @brief Try to acquire the actor mutex without blocking. */
            bool try_lock() override;

            /** @brief Release the actor mutex. */
            void unlock() override;

            /** @copydoc IActor::unlock_shared */
            void unlock_shared() override;

#ifdef _DEBUG
            s32 addReference();
            bool removeReference();
#endif

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal handler for actor FSM events.
             * @param state Current state id.
             * @param eventType Event being handled.
             * @return FSMReturnType result for the FSM processing.
             */
            FSMReturnType handleActorEvent( u32 state, FSMEvent eventType );

            /** Applies this actor's collision mask to a single physics-aware component. */
            void applyCollisionMaskToComponent( SmartPtr<IComponent> component, bool force );

            /**
             * @brief Create a copy of the currently attached components.\n
             * @return An array containing smart pointers to the components.\n
             */
            Array<SmartPtr<IComponent>> snapshotComponents() const;

            /**
             * @brief Clear the internal representation of component nodes.\n
             */
            void clearComponentNodes();

            /**\n
             * @brief Reorder the component nodes to match the provided array order.\n
             * @param components The ordered list of components to apply.
             */
            void reorderComponentNodes( const Array<SmartPtr<IComponent>> &components );

            /**
             * @brief Create a copy of the currently attached child actors.
             * @return An array containing smart pointers to the children.
             */
            Array<SmartPtr<IGameActor>> snapshotChildren() const;

            /**
             * @brief Clear the internal representation of child nodes.
             */
            void clearChildNodes();

            /**
             * @brief Reorder the child nodes to match the provided array order.
             * @param children The ordered list of child actors to apply.
             */
            void reorderChildNodes( const Array<SmartPtr<IGameActor>> &children );

            /**
             * @brief Apply this actor's collision mask to all attached physics-aware components.
             *
             * This iterates attached components and updates their collision settings so they
             * match the actor-level collision mask. Typically invoked when the actor's
             * collision mask changes.
             */
            void applyCollisionMaskToPhysicsComponents( bool force );

            //! The scene this actor belongs to (weak reference).
            AtomicWeakPtr<IGameScene> m_scene;

            //! The parent actor (weak reference). Null if this actor is a root.
            AtomicWeakPtr<IGameActor> m_parent;

            //! The local transform object for this actor.
            AtomicSmartPtr<ITransform> m_transform;

            //! Factory manager used to create components and subobjects (weak reference).
            AtomicWeakPtr<IFactoryManager> m_factoryManager;

            //! Local axis-aligned bounding box of the actor.
            AABB3<real_Num> m_localAABB;

            //! Bounding radius of the actor.
            atomic_f32 m_radius = 0.0f;

            //! When enabled the actor submits its AABB and OBB to the debug renderer each frame.
            atomic_bool m_debugDrawEnabled = false;

            //! Bitmask flags representing actor state.
            AtomicObject<u32> m_flags;

            //! Snapshot of previous flags used for change detection.
            AtomicObject<u32> m_previousFlags;

            //! Physics collision mask propagated to physics-aware components.
            AtomicObject<u32> m_collisionMask = 0;

            //! Null-terminated, C-style linked list of components attached to this actor.
            IComponent *m_componentsHead = nullptr;
            IComponent *m_componentsTail = nullptr;
            mutable RecursiveSpinMutex m_componentsMutex;

            //! Null-terminated, C-style linked list of direct child actors.
            IGameActor *m_childrenHead = nullptr;
            IGameActor *m_childrenTail = nullptr;
            u32 m_numChildren = 0;
            mutable RecursiveSpinMutex m_childrenMutex;

            //! Arbitrary string tags attached to the actor for lookup and filtering.
            ConcurrentFixedArrayGrowable<FixedString<128>, WP_MAX_TAGS> m_tags;

            //! Name of the layer this actor belongs to. Used for rendering and visibility groups.
            AtomicObject<FixedString<128>> m_layer;

            //! Static extension used to generate unique ids for newly created actors.
            static u32 m_idExt;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CActor_h__
