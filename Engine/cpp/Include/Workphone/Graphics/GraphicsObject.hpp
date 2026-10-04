#ifndef __CGraphicsObject_h__
#define __CGraphicsObject_h__

#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class GraphicsObject
         * @brief Base template implementation for scene-level graphics objects.
         *
         * GraphicsObject<T> provides a common base that manages state data, properties,
         * parent/owner relationships and integration with the engine's state system.
         * Derived classes provide concrete graphics-specific behaviour (e.g. mesh, light).
         *
         * The class encapsulates:
         * - access to a state context (GraphicsObjectData) for thread-safe property/state changes,
         * - owner/creator references (scene node / graphics scene),
         * - a raw pointer to the underlying renderer-specific object,
         * - common operations such as visibility, shadow flags, render queue and AABB.
         *
         * @tparam T Derived type (CRTP style).
         */
        template <class T>
        class GraphicsObject : public SharedGraphicsObject<T>
        {
        public:
            /**
             * @brief Constructs a GraphicsObject.
             *
             * The constructor sets the default event task flags (Render thread).
             */
            GraphicsObject();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup runs correctly via virtual dispatch.
             */
            ~GraphicsObject() override;

            /**
             * @brief Unload the graphics object and release runtime resources.
             *
             * This will remove any attached state listener, unload the state context and
             * release references to owner/creator objects. Exceptions are caught and logged.
             *
             * @param data Optional data parameter, currently ignored by default implementation.
             */
            virtual void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Attach this graphics object to a parent scene node.
             *
             * Called when this object becomes a child of a scene node. Derived classes
             * should override to perform renderer specific attach steps (e.g. attach to a
             * scene graph or node).
             *
             * @param parent Scene node to attach to. May be null for detach operations.
             */
            virtual void attachToParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Detach this graphics object from a parent scene node.
             *
             * Called when this object is removed from a scene node. Derived classes should
             * override to perform renderer specific detach steps (e.g. detach from scene graph).
             *
             * @param parent Scene node to detach from. May be null.
             */
            virtual void detachFromParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Enable or disable casting shadows for this object.
             *
             * This updates the flag stored in the object's state data. Concrete renderers
             * may observe the state change and update GPU-side state accordingly.
             *
             * @param castShadows True to enable shadow casting; false to disable.
             */
            virtual void setCastShadows( bool castShadows ) override;

            /**
             * @brief Query whether this object casts shadows.
             * @return True if the object is set to cast shadows, false otherwise.
             */
            virtual bool getCastShadows() const override;

            /**
             * @brief Enable or disable receiving shadows on this object.
             *
             * Updates the underlying state flag controlling whether the object's material
             * should receive shadows.
             *
             * @param receiveShadows True to receive shadows; false to ignore shadows.
             */
            virtual void setReceiveShadows( bool receiveShadows );

            /**
             * @brief Query whether this object receives shadows.
             * @return True if receive-shadows flag is set; false otherwise.
             */
            virtual bool getReceiveShadows() const;

            /**
             * @brief Mark the object visible or invisible.
             *
             * Visibility is stored in the object's state context and is intended to be
             * consumed by the renderer / scene traversal.
             *
             * @param visible True to make visible; false to hide.
             */
            virtual void setVisible( bool visible ) override;

            /**
             * @brief Test if the object is currently visible.
             * @return True if visible; false otherwise.
             */
            virtual bool isVisible() const override;

            /**
             * @brief Set the Z-order used by some renderers to sort objects with identical
             *        render queue grouping.
             *
             * @param zOrder Z-order index (higher values typically render later / on top).
             */
            virtual void setZOrder( u32 zOrder ) override;

            /**
             * @brief Get the configured Z-order value.
             * @return Current Z-order.
             */
            virtual u32 getZOrder() const override;

            /**
             * @brief Retrieve the object's local axis-aligned bounding box (AABB).
             *
             * Returns the bounding box expressed in the object's local space. The value
             * is read from the state context.
             *
             * @return Local-space AABB. If no state data is available an empty AABB is returned.
             */
            virtual AABB3<real_Num> getLocalAABB() const override;

            /**
             * @brief Set the object's local-axis aligned bounding box (AABB).
             *
             * The box is stored in the object's state data and is used for culling,
             * intersection tests and selection.
             *
             * @param localAABB AABB in local/object space.
             */
            virtual void setLocalAABB( const AABB3<real_Num> &localAABB ) override;

            /**
             * @brief Query whether this graphics object is currently attached to an owner node.
             * @return True if attached to a scene node; false otherwise.
             */
            virtual bool isAttached() const override;

            /**
             * @brief Set the attached state for this object.
             *
             * This is a logical flag; actual attach/detach logic is performed by
             * setOwner/attachToParent/detachFromParent methods.
             *
             * @param attached True to mark as attached; false to mark as detached.
             */
            virtual void setAttached( bool attached ) override;

            /**
             * @brief Get the render technique identifier (hash) used by this object.
             * @return Hash identifying the render technique / material pipeline.
             */
            virtual hash_type getRenderTechnique() const override;

            /**
             * @brief Set the render technique identifier (hash).
             * @param renderTechnique Hash that identifies the render technique.
             */
            virtual void setRenderTechnique( hash_type renderTechnique ) override;

            /**
             * @brief Set the render queue group this object belongs to.
             *
             * Render queues are used to group objects for ordering (opaque, transparent, UI, etc).
             *
             * @param queueID Render queue group identifier.
             */
            virtual void setRenderQueueGroup( u32 queueID );

            /**
             * @brief Get the render queue group id.
             * @return Current render queue group id.
             */
            virtual u32 getRenderQueueGroup() const;

            /**
             * @brief Set visibility mask flags for selective visibility/culling.
             *
             * Visibility flags are a bitmask used by the visibility/culling system to
             * include/exclude objects from specific camera passes or layers.
             *
             * @param flags Bitmask representing visibility groups.
             */
            virtual void setVisibilityFlags( u32 flags ) override;

            /**
             * @brief Get the object's visibility mask flags.
             * @return Visibility bitmask.
             */
            virtual u32 getVisibilityFlags() const override;

            /**
             * @brief Retrieve the raw underlying object pointer.
             *
             * The pointer is renderer-specific (for example an Ogre or BGFX object).
             * The raw pointer may be null.
             *
             * @param ppObject Output pointer to receive the raw pointer (optional).
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Set a custom boolean flag on this object.
             * @param flag Index/bit position of the flag.
             * @param value Boolean value to set.
             */
            virtual void setFlag( u32 flag, bool value ) override;

            /**
             * @brief Read a custom boolean flag from this object.
             * @param flag Index/bit position of the flag.
             * @return Boolean value of the flag.
             */
            virtual bool getFlag( u32 flag ) const override;

            /**
             * @brief Retrieve all flags as a single bitmask value.
             * @return Combined flags bitmask.
             */
            u32 getFlags() const;

            /**
             * @brief Overwrite all flags with the provided bitmask.
             * @param flags New combined flags bitmask.
             */
            void setFlags( u32 flags );

            /**
             * @brief Get the scene node that currently owns this object.
             * @return Pointer to the owner IGraphicsSceneNode, or nullptr if not attached.
             */
            IGraphicsSceneNode *getOwnerPtr() const override;

            /**
             * @brief Get the owner scene node (if any) that this object is attached to.
             * @return Smart pointer to the owner IGraphicsSceneNode or null if none.
             */
            virtual SmartPtr<IGraphicsSceneNode> getOwner() const override;

            /**
             * @brief Set the owner scene node for this object.
             *
             * This will detach from the previous owner (if present) and attach to the new one.
             *
             * @param sceneNode Smart pointer to the new owner scene node (may be null).
             */
            virtual void setOwner( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /**
             * @brief Export this object's properties into a Properties container.
             *
             * The returned Properties instance contains common properties such as visibility,
             * shadow flags, render technique, queue group, z-order and more.
             *
             * @return SmartPtr<Properties> containing a snapshot of the object's properties.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties from a Properties container to this object.
             *
             * Only supported properties are read and applied. Any missing properties are left unchanged.
             *
             * @param properties Properties container containing values to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the creator / owning graphics scene for this object.
             * @return Smart pointer to the IGraphicsScene that created this object (may be null).
             */
            SmartPtr<IGraphicsScene> getCreator() const;

            /**
             * @brief Set the creator / owning graphics scene.
             * @param creator Smart pointer to the graphics scene.
             */
            void setCreator( SmartPtr<IGraphicsScene> creator );

            /**
             * @brief Mark the object dirty so that dependent systems will update it.
             *
             * Derived classes should override to request a refresh/update of renderer-side
             * resources or to notify the scene of changes.
             */
            void makeDirty();

            /**
             * @brief Ray intersection test (fast boolean test).
             * @param ray Ray in world space.
             * @return True if ray intersects the object, false otherwise.
             */
            virtual bool intersects( const Ray3<real_Num> &ray ) const;

            /**
             * @brief Ray intersection test returning distance to hit.
             * @param ray Ray in world space.
             * @param distance Output distance from ray origin to hit point when true.
             * @return True if ray intersects the object, false otherwise.
             */
            virtual bool intersects( const Ray3<real_Num> &ray, real_Num &distance ) const;

            /**
             * @brief Ray intersection test returning hit point and distance.
             * @param ray Ray in world space.
             * @param hitPoint Output hit point (world space) if intersection occurs.
             * @param distance Output distance from ray origin to hit point when true.
             * @return True if ray intersects the object, false otherwise.
             */
            virtual bool intersects( const Ray3<real_Num> &ray, Vector3<real_Num> &hitPoint,
                                     real_Num &distance ) const;

            /**
             * @brief Process a state message sent to this graphics object.
             *
             * Messages are delivered by the engine state system and can be used to apply
             * asynchronous updates. Derived classes can handle specific message types.
             *
             * @param message Message to process.
             * @return True if the message was handled; false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Called when an attached IState object changes.
             *
             * This callback is intended for derived classes to react to state changes.
             *
             * @param state The state that changed.
             * @return True if the change was handled; false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Get the raw renderer-specific object pointer.
             * @return Void pointer to the underlying graphics object (may be null).
             */
            void *getGraphicsObject() const;

            /**
             * @brief Set the raw renderer-specific object pointer.
             *
             * The pointer is stored without ownership semantics here; the lifetime management
             * remains the responsibility of the concrete implementation.
             *
             * @param graphicsObject Raw pointer to set (may be null).
             */
            void setGraphicsObject( void *graphicsObject );

            /**
             * @brief Cast the raw graphics pointer to a concrete type.
             *
             * Convenience helper to avoid manual casts at call sites.
             *
             * @tparam U Target pointer type.
             * @return Pointer cast to U or null if m_graphicsObject is null.
             */
            template <class U>
            U *getGraphicsObjectByType() const;

            WP_CLASS_REGISTER_TEMPLATE_DECL( GraphicsObject, T );

        protected:
            /**
             * @brief Ensure a state context exists for this graphics object.
             *
             * If no state context is present, this method will request one from the
             * application state manager and attach it to the shared base.
             *
             * This method catches and logs exceptions; it does not throw.
             */
            virtual void setupStateObject();

            /**
             * @brief Atomic weak reference to the scene node that owns/contains this object.
             *
             * Use getOwner() to obtain a strong SmartPtr before use to prevent lifetime races.
             */
            AtomicWeakPtr<IGraphicsSceneNode> m_owner;

            /**
             * @brief Atomic weak reference to the graphics scene (scene manager) that created this
             * object.
             *
             * Use getCreator() to obtain a strong SmartPtr before use.
             */
            AtomicWeakPtr<IGraphicsScene> m_creator;

            /**
             * @brief Raw pointer to the renderer-specific implementation object.
             *
             * Example: an Ogre movable object, a BGFX handle, or other platform-specific type.
             * This pointer is stored without transfer of ownership.
             */
            void *m_graphicsObject = nullptr;

            /**
             * @brief Id generation
             */
            static hash_type m_idExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, GraphicsObject, T, T );

        template <class T>
        hash_type GraphicsObject<T>::m_idExt = 0;

        template <class T>
        GraphicsObject<T>::GraphicsObject()
        {
            auto typeManager = TypeManager::instance();

            auto typeinfo = typeInfo();

            auto name = typeManager->getName( typeinfo ) + m_idExt++;
            auto id = StringUtil::getHash( name );

            this->setName( name );
            this->setId( id );

            GraphicsObject<T>::setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
            GraphicsObject<T>::setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
            GraphicsObject<T>::setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
            GraphicsObject<T>::setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

            auto flags = Thread::Application_Flag | Thread::Render_Flag;
            GraphicsObject<T>::setEventTaskFlags( flags );
        }

        template <class T>
        GraphicsObject<T>::~GraphicsObject()
        {
            // Owners and creators may come from pooled factories and can already
            // have been reclaimed when an unloaded graphics object is destroyed.
            // These are bookkeeping-only weak references, so do not dereference
            // their raw targets while releasing them.
            m_owner.forceReset();
            m_creator.forceReset();
        }

        template <class T>
        void GraphicsObject<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                m_owner.forceReset();
                m_creator.forceReset();

                SharedGraphicsObject<T>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void GraphicsObject<T>::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
        }

        template <class T>
        void GraphicsObject<T>::detachFromParent( SmartPtr<IGraphicsSceneNode> parent )
        {
        }

        template <class T>
        AABB3<real_Num> GraphicsObject<T>::getLocalAABB() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return stateData->localAABB;
                }
            }

            return {};
        }

        template <class T>
        void GraphicsObject<T>::setLocalAABB( const AABB3<real_Num> &localAABB )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->localAABB = localAABB;
                }
            }
        }

        template <class T>
        bool GraphicsObject<T>::isAttached() const
        {
            return getOwner() != nullptr;
        }

        template <class T>
        void GraphicsObject<T>::setAttached( bool attached )
        {
        }

        template <class T>
        hash_type GraphicsObject<T>::getRenderTechnique() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return stateData->renderTechnique;
                }
            }

            return 0;
        }

        template <class T>
        void GraphicsObject<T>::setRenderTechnique( hash_type renderTechnique )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->renderTechnique = renderTechnique;
                }
            }
        }

        template <class T>
        void GraphicsObject<T>::setRenderQueueGroup( u32 queueID )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->renderQueueGroup = queueID;
                }
            }
        }

        template <class T>
        u32 GraphicsObject<T>::getRenderQueueGroup() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return stateData->renderQueueGroup;
                }
            }

            return 0;
        }

        template <class T>
        void GraphicsObject<T>::setCastShadows( bool castShadows )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->flags = BitUtil::setFlagValue(
                        stateData->flags, IGraphicsObject::castShadowsFlag, castShadows );
                }
            }
        }

        template <class T>
        bool GraphicsObject<T>::getCastShadows() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return BitUtil::getFlagValue( stateData->flags, IGraphicsObject::castShadowsFlag );
                }
            }

            return false;
        }

        template <class T>
        void GraphicsObject<T>::setReceiveShadows( bool recieveShadows )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->flags = BitUtil::setFlagValue(
                        stateData->flags, IGraphicsObject::receiveShadowsFlag, recieveShadows );
                }
            }
        }

        template <class T>
        bool GraphicsObject<T>::getReceiveShadows() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return BitUtil::getFlagValue( stateData->flags,
                                                  IGraphicsObject::receiveShadowsFlag );
                }
            }

            return false;
        }

        template <class T>
        void GraphicsObject<T>::setZOrder( u32 zOrder )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->zorder = zOrder;
                }
            }
        }

        template <class T>
        u32 GraphicsObject<T>::getZOrder() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return stateData->zorder;
                }
            }

            return 0;
        }

        template <class T>
        void GraphicsObject<T>::setVisibilityFlags( u32 flags )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->visibilityMask = flags;
                }
            }
        }

        template <class T>
        u32 GraphicsObject<T>::getVisibilityFlags() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return stateData->visibilityMask;
                }
            }

            return 0;
        }

        template <class T>
        void GraphicsObject<T>::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = m_graphicsObject;
            }
        }

        template <class T>
        void GraphicsObject<T>::setVisible( bool visible )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->flags =
                        BitUtil::setFlagValue( stateData->flags, IGraphicsObject::visibleFlag, visible );
                }
            }
        }

        template <class T>
        bool GraphicsObject<T>::isVisible() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return BitUtil::getFlagValue( stateData->flags, IGraphicsObject::visibleFlag );
                }
            }

            return false;
        }

        template <class T>
        void GraphicsObject<T>::setFlag( u32 flag, bool value )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->flags = BitUtil::setFlagValue( stateData->flags, flag, value );
                }
            }
        }

        template <class T>
        bool GraphicsObject<T>::getFlag( u32 flag ) const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return BitUtil::getFlagValue( stateData->flags, flag );
                }
            }

            return false;
        }

        template <class T>
        u32 GraphicsObject<T>::getFlags() const
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData =
                        stateContext->template getStateDataById<GraphicsObjectData>( this->getId() ) )
                {
                    return stateData->flags;
                }
            }

            return 0;
        }

        template <class T>
        void GraphicsObject<T>::setFlags( u32 flags )
        {
            if( auto stateContext = GraphicsObject<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateDataById<GraphicsObjectData>(
                        this->getId() ) )
                {
                    stateData->flags = flags;
                }
            }
        }

        template <class T>
        IGraphicsSceneNode *GraphicsObject<T>::getOwnerPtr() const
        {
            auto p = m_owner.load();
            return p.get();
        }

        template <class T>
        SmartPtr<IGraphicsSceneNode> GraphicsObject<T>::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <class T>
        void GraphicsObject<T>::setOwner( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            if( auto owner = getOwner() )
            {
                detachFromParent( owner );
            }

            m_owner = sceneNode;

            if( auto owner = getOwner() )
            {
                attachToParent( owner );
            }
        }

        template <class T>
        SmartPtr<Properties> GraphicsObject<T>::getProperties() const
        {
            auto properties = SharedGraphicsObject<T>::getProperties();

            auto name = SharedGraphicsObject<T>::getName();
            auto visible = GraphicsObject<T>::isVisible();
            auto castShadows = GraphicsObject<T>::getCastShadows();
            auto receiveShadows = GraphicsObject<T>::getReceiveShadows();
            auto renderTechnique = static_cast<u32>( GraphicsObject<T>::getRenderTechnique() );
            auto renderQueueGroup = GraphicsObject<T>::getRenderQueueGroup();
            auto zOrder = GraphicsObject<T>::getZOrder();
            auto visibilityMask = GraphicsObject<T>::getVisibilityFlags();
            auto localAABB = GraphicsObject<T>::getLocalAABB();
            auto attached = GraphicsObject<T>::isAttached();
            auto flags = getFlags();

            properties->setProperty( IGraphicsObject::namePropertyStr, name );
            properties->setProperty( IGraphicsObject::visiblePropertyStr, visible );
            properties->setProperty( IGraphicsObject::castShadowsPropertyStr, castShadows );
            properties->setProperty( IGraphicsObject::receiveShadowsPropertyStr, receiveShadows );
            properties->setProperty( IGraphicsObject::renderTechniquePropertyStr, renderTechnique );
            properties->setProperty( IGraphicsObject::renderQueueGroupPropertyStr, renderQueueGroup );
            properties->setProperty( IGraphicsObject::zOrderPropertyStr, zOrder );
            properties->setProperty( IGraphicsObject::visibilityMaskPropertyStr, visibilityMask );
            // properties->setProperty( IGraphicsObject::localAABBPropertyStr, localAABB );
            properties->setProperty( IGraphicsObject::attachedPropertyStr, attached );
            properties->setProperty( IGraphicsObject::flagsPropertyStr, flags );

            return properties;
        }

        template <class T>
        void GraphicsObject<T>::setProperties( [[maybe_unused]] SmartPtr<Properties> properties )
        {
            SharedGraphicsObject<T>::setProperties( properties );

            auto visible = GraphicsObject<T>::isVisible();
            auto castShadows = GraphicsObject<T>::getCastShadows();
            auto receiveShadows = GraphicsObject<T>::getReceiveShadows();
            auto renderTechnique = static_cast<u32>( GraphicsObject<T>::getRenderTechnique() );
            auto renderQueueGroup = GraphicsObject<T>::getRenderQueueGroup();
            auto zOrder = GraphicsObject<T>::getZOrder();
            auto visibilityMask = GraphicsObject<T>::getVisibilityFlags();
            auto localAABB = GraphicsObject<T>::getLocalAABB();
            auto attached = GraphicsObject<T>::isAttached();
            auto flags = getFlags();

            properties->getPropertyValue( IGraphicsObject::visiblePropertyStr, visible );
            properties->getPropertyValue( IGraphicsObject::castShadowsPropertyStr, castShadows );
            properties->getPropertyValue( IGraphicsObject::receiveShadowsPropertyStr, receiveShadows );
            properties->getPropertyValue( IGraphicsObject::renderTechniquePropertyStr, renderTechnique );
            properties->getPropertyValue( IGraphicsObject::renderQueueGroupPropertyStr,
                                          renderQueueGroup );
            properties->getPropertyValue( IGraphicsObject::zOrderPropertyStr, zOrder );
            properties->getPropertyValue( IGraphicsObject::visibilityMaskPropertyStr, visibilityMask );
            // properties->getPropertyValue( IGraphicsObject::localAABBPropertyStr, localAABB );
            properties->getPropertyValue( IGraphicsObject::attachedPropertyStr, attached );
            properties->getPropertyValue( IGraphicsObject::flagsPropertyStr, flags );

            GraphicsObject<T>::setVisible( visible );
            GraphicsObject<T>::setCastShadows( castShadows );
            GraphicsObject<T>::setReceiveShadows( receiveShadows );
            GraphicsObject<T>::setRenderTechnique( renderTechnique );
            GraphicsObject<T>::setRenderQueueGroup( renderQueueGroup );
            GraphicsObject<T>::setZOrder( zOrder );
            GraphicsObject<T>::setVisibilityFlags( visibilityMask );
            GraphicsObject<T>::setLocalAABB( localAABB );
            GraphicsObject<T>::setAttached( attached );
            setFlags( flags );
        }

        template <class T>
        SmartPtr<IGraphicsScene> GraphicsObject<T>::getCreator() const
        {
            auto p = m_creator.load();
            return p.lock();
        }

        template <class T>
        void GraphicsObject<T>::setCreator( SmartPtr<IGraphicsScene> creator )
        {
            m_creator = creator;
        }

        template <class T>
        void GraphicsObject<T>::setupStateObject()
        {
            try
            {
                auto stateContext = SharedGraphicsObject<T>::getStateContext();
                if( !stateContext )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto factoryManager = applicationManager->getFactoryManagerPtr();
                    WP_ASSERT( factoryManager );

                    auto stateManager = applicationManager->getStateManagerPtr();
                    WP_ASSERT( stateManager );

                    auto stateContext = stateManager->addStateContext();
                    SharedGraphicsObject<T>::setStateContext( stateContext );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void GraphicsObject<T>::makeDirty()
        {
        }

        template <class T>
        bool GraphicsObject<T>::intersects( const Ray3<real_Num> &ray ) const
        {
            return false;
        }

        template <class T>
        bool GraphicsObject<T>::intersects( const Ray3<real_Num> &ray, real_Num &distance ) const
        {
            return false;
        }

        template <class T>
        bool GraphicsObject<T>::intersects( const Ray3<real_Num> &ray, Vector3<real_Num> &hitPoint,
                                            real_Num &distance ) const
        {
            return false;
        }

        template <class T>
        bool GraphicsObject<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool GraphicsObject<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        template <class T>
        void *GraphicsObject<T>::getGraphicsObject() const
        {
            return m_graphicsObject;
        }

        template <class T>
        void GraphicsObject<T>::setGraphicsObject( void *graphicsObject )
        {
            m_graphicsObject = graphicsObject;
        }

        template <class T>
        template <class U>
        U *GraphicsObject<T>::getGraphicsObjectByType() const
        {
            return static_cast<U *>( m_graphicsObject );
        }

    }  // namespace render
}  // namespace workphone

#endif  // CGraphicsObject_h__
