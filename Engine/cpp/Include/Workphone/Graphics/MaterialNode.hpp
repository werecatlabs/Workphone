#ifndef CMaterialNode_h__
#define CMaterialNode_h__

#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Generic implementation of an IMaterialNode.
         *
         * @tparam T The concrete implementation type used by the SharedGraphicsObject base.
         *
         * MaterialNode implements a simple parent/child tree for material nodes, holds a
         * pointer to an IMaterial and exposes lifecycle and property query methods.
         *
         * Responsibilities:
         * - Manage child nodes (add/remove/get).
         * - Hold a reference to a material instance.
         * - Track enabled/disabled state.
         * - Provide basic load/reload/unload lifecycle hooks.
         *
         * The class is a template so different concrete specializations can provide
         * platform-specific behaviour via the SharedGraphicsObject<T> base.
         */
        template <class T>
        class MaterialNode : public T
        {
        public:
            /**
             * @brief Internal state listener used to tie node lifecycle to the state system.
             *
             * This listener is lightweight and primarily used to clear the owner pointer
             * when the listener is unloaded. It provides adapter methods for the state manager.
             */
            class MaterialNodeStateListener : public IStateListener
            {
            public:
                MaterialNodeStateListener();
                ~MaterialNodeStateListener() override;

                /**
                 * @brief Unload handler invoked by the state system.
                 *
                 * Clears the weak owner reference so the node can be freed independently.
                 *
                 * @param data Unused shared object data.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message State message.
                 * @return true if handled; false otherwise.
                 *
                 * Default behaviour does not handle messages.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Called when a state object has changed.
                 * @param state Changed state.
                 * @return true if handled; false otherwise.
                 *
                 * Default behaviour does not react to state changes.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owner.
                 */
                MaterialNode *getOwnerPtr() const;

                /**
                 * @brief Get the owner MaterialNode (strong pointer) if it still exists.
                 * @return SmartPtr<MaterialNode> to the owner, or nullptr if expired.
                 */
                SmartPtr<MaterialNode> getOwner() const;

                /**
                 * @brief Set the owner for this listener.
                 * @param owner SmartPtr to the owning MaterialNode.
                 */
                void setOwner( SmartPtr<MaterialNode> owner );

            protected:
                /// Weak atomic pointer to the owner MaterialNode to avoid reference cycles.
                AtomicWeakPtr<MaterialNode> m_owner;
            };

            /**
             * @brief Construct an empty material node with no children.
             */
            MaterialNode();

            /**
             * @brief Construct a material node pre-sized to contain @p numChildren slots.
             *
             * @param numChildren Initial number of child slots (they will be default-initialized).
             */
            MaterialNode( u32 numChildren );

            /**
             * @brief Destructor - ensures resources / state listeners are cleaned up.
             */
            ~MaterialNode() override;

            /** @copydoc IObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Add a child node to the end of the child array.
             *
             * The child's parent is set to this node.
             *
             * @param child Child node to add. If null, the call is ignored.
             */
            void addChild( SmartPtr<IMaterialNode> child ) override;

            /**
             * @brief Set a child at a specific index in the child array.
             *
             * The child's parent is set to this node. The caller must ensure index is valid.
             *
             * @param child Child node to assign.
             * @param index Index at which to place the child.
             */
            void addChild( SmartPtr<IMaterialNode> child, int index ) override;

            /**
             * @brief Remove a child node.
             *
             * If the child exists in this node's children it is removed and its parent is cleared.
             *
             * @param child Child node to remove.
             */
            void removeChild( SmartPtr<IMaterialNode> child ) override;

            /**
             * @brief Remove this node from its parent.
             * If this node has a parent, the parent will remove this node from its children.
             */
            void remove() override;

            /** @copydoc IMaterialNode::removeAllChildren */
            void removeAllChildren() override;

            /**
             * @brief Get the number of children currently present.
             * @return Number of children.
             */
            u32 getNumChildren() const override;

            /**
             * @brief Get a child by index.
             *
             * No bounds checking is performed (caller must ensure index is valid).
             *
             * @param index Index of the child to retrieve.
             * @return SmartPtr to the child node or null if none.
             */
            SmartPtr<IMaterialNode> getChildByIndex( u32 index ) const override;

            /**
             * @brief Find a child by its id.
             *
             * Default implementation returns nullptr. Concrete specializations may
             * override to perform id-based lookups.
             *
             * @param id Hash id of the child to find.
             * @return SmartPtr to the child node if found; nullptr otherwise.
             */
            SmartPtr<IMaterialNode> getChildById( hash_type id ) const override;

            /**
             * @brief Get a copy of the children array.
             * @return Array of SmartPtr<IMaterialNode> representing the children.
             */
            Array<SmartPtr<IMaterialNode>> getChildren() const override;

            /** @copydoc IMaterialNode::setChildren */
            void setChildren( const Array<SmartPtr<IMaterialNode>> &children );

            /**
             * @brief Get the parent node.
             * @return Pointer to the parent IMaterialNode, or nullptr if none.
             */
            IMaterialNode *getParentPtr() const override;

            /**
             * @brief Get the parent node.
             * @return SmartPtr to the parent IMaterialNode, or nullptr if none.
             */
            SmartPtr<IMaterialNode> getParent() const override;

            /**
             * @brief Set the parent node.
             * @param parent New parent for this node (may be nullptr).
             */
            void setParent( SmartPtr<IMaterialNode> parent ) override;

            /**
             * @brief Get the material associated with this node.
             * @return SmartPtr<IMaterial> referencing the material or nullptr.
             */
            SmartPtr<IMaterial> getMaterial() const override;

            /**
             * @brief Assign a material to this node.
             * @param material Material to assign (may be nullptr).
             */
            void setMaterial( SmartPtr<IMaterial> material ) override;

            /** @copydoc IResource::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IResource::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Query whether this node is enabled.
             * @return true if enabled; false otherwise.
             */
            bool isEnabled() const override;

            /**
             * @brief Enable or disable this node.
             * @param enabled true to enable; false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Get the raw pointer to the attached IStateContext.
             * Useful when C-style pointer access is required. The returned pointer is not
             * accompanied by ownership guarantees; prefer getStateContext() for ownership.
             * @return Raw IStateContext pointer or nullptr if none set.
             */
            IStateContext *getStateContextPtr() const;

            /**
             * @brief Get the SmartPtr to the attached IStateContext.
             * Returns the internal smart pointer that owns the state context.
             * @return SmartPtr<IStateContext> reference (may be nullptr).
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Attach a state context to this object.
             * The provided SmartPtr will be stored and later unloaded/removed by
             * destroyStateContext() during object unload.
             * @param stateContext Smart pointer to the state context to attach.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * Gets the state listener for this overlay element.
             * @return A pointer to the state listener.
             */
            SmartPtr<IStateListener> getStateListener() const;

            /**
             * Sets the state listener for this overlay element.
             * @param stateListener A pointer to the state listener.
             */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /**
             * @brief Handle an incoming state message.
             * @param message The state message to handle.
             * @return true if the message was handled and should not propagate.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handle a full state change.
             * @param state The new state object.
             * @return true if the change was handled.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Acquire the graphics system lock.
             *
             * Uses the global application manager to obtain the graphics system and call lock().
             * Asserts if the application manager or graphics system are not available.
             */
            void lock();

            /**
             * @brief Try to acquire the graphics system lock without blocking.
             *
             * Returns immediately with whether the lock was obtained.
             *
             * @return true if the lock was acquired, false otherwise.
             */
            bool try_lock();

            /**
             * @brief Release the graphics system lock.
             *
             * Calls unlock() on the graphics system obtained from the application manager.
             */
            void unlock();

            WP_CLASS_REGISTER_TEMPLATE_DECL( MaterialNode, T );

        protected:
            /// Whether this material node is enabled and should contribute to rendering.
            atomic_bool m_enabled = true;

            /// Material associated with this node (may be null).
            AtomicWeakPtr<IMaterial> m_material;

            /// Parent material node (may be null).
            AtomicWeakPtr<IMaterialNode> m_parent;

            /// Child material nodes.
            ConcurrentArray<SmartPtr<IMaterialNode>> m_children;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, MaterialNode, T, T );

        template <class T>
        MaterialNode<T>::MaterialNode() : m_parent( nullptr )
        {
        }

        template <class T>
        MaterialNode<T>::MaterialNode( u32 numChildren )
        {
            m_children.resize( numChildren );
        }

        template <class T>
        MaterialNode<T>::~MaterialNode()
        {
            unload( nullptr );
        }

        template <class T>
        void MaterialNode<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void MaterialNode<T>::reload( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void MaterialNode<T>::unload( SmartPtr<ISharedObject> data )
        {
            m_children.clear();
            m_parent = nullptr;
        }

        template <class T>
        void MaterialNode<T>::addChild( SmartPtr<IMaterialNode> child )
        {
            if( child )
            {
                m_children.push_back( child );
                child->setParent( this );
            }
        }

        template <class T>
        void MaterialNode<T>::addChild( SmartPtr<IMaterialNode> child, int index )
        {
            if( child )
            {
                m_children[index] = child;
                child->setParent( this );
            }
        }

        template <class T>
        void MaterialNode<T>::removeChild( SmartPtr<IMaterialNode> child )
        {
            auto it = std::find( m_children.begin(), m_children.end(), child );
            if( it != m_children.end() )
            {
                m_children.erase( it );
                child->setParent( nullptr );
            }
        }

        template <class T>
        void MaterialNode<T>::remove()
        {
            auto parent = getParent();
            if( parent )
            {
                parent->removeChild( this );
            }
        }

        template <class T>
        void MaterialNode<T>::removeAllChildren()
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setParent( nullptr );
                }
            }

            m_children.clear();
        }

        template <class T>
        auto MaterialNode<T>::getNumChildren() const -> u32
        {
            return (u32)m_children.size();
        }

        template <class T>
        auto MaterialNode<T>::getChildByIndex( u32 index ) const -> SmartPtr<IMaterialNode>
        {
            return m_children[index];
        }

        template <class T>
        auto MaterialNode<T>::getChildById( hash_type id ) const -> SmartPtr<IMaterialNode>
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child && child->getId() == id )
                {
                    child->setParent( nullptr );
                }
            }

            return nullptr;
        }

        template <class T>
        auto MaterialNode<T>::getChildren() const -> Array<SmartPtr<IMaterialNode>>
        {
            return m_children.snapshot();
        }

        template <class T>
        void MaterialNode<T>::setChildren( const Array<SmartPtr<IMaterialNode>> &children )
        {
            m_children = { children.begin(), children.end() };
        }

        template <class T>
        auto MaterialNode<T>::getParentPtr() const -> IMaterialNode *
        {
            return m_parent.get();
        }

        template <class T>
        auto MaterialNode<T>::getParent() const -> SmartPtr<IMaterialNode>
        {
            auto p = m_parent.load();
            return p.lock();
        }

        template <class T>
        void MaterialNode<T>::setParent( SmartPtr<IMaterialNode> parent )
        {
            m_parent = parent;
        }

        template <class T>
        auto MaterialNode<T>::getMaterial() const -> SmartPtr<IMaterial>
        {
            auto p = m_material.load();
            return p.lock();
        }

        template <class T>
        void MaterialNode<T>::setMaterial( SmartPtr<IMaterial> material )
        {
            m_material = material;
        }

        template <class T>
        auto MaterialNode<T>::getProperties() const -> SmartPtr<Properties>
        {
            auto properties = workphone::make_ptr<Properties>();
            properties->setProperty( ISharedObject::loadedStr, this->isLoaded() );
            properties->setProperty( ISharedObject::referencesStr, this->getReferences() );
            properties->setProperty( ISharedObject::weakReferencesStr, this->getWeakReferences() );
            return properties;
        }

        template <class T>
        void MaterialNode<T>::setProperties( SmartPtr<Properties> properties )
        {
        }

        template <class T>
        bool MaterialNode<T>::isEnabled() const
        {
            return m_enabled;
        }

        template <class T>
        void MaterialNode<T>::setEnabled( bool enabled )
        {
            m_enabled = enabled;
        }

        template <typename T>
        IStateContext *MaterialNode<T>::getStateContextPtr() const
        {
            if( auto material = getMaterial() )
            {
                return material->getStateContextPtr();
            }

            return nullptr;
        }

        template <typename T>
        SmartPtr<IStateContext> MaterialNode<T>::getStateContext() const
        {
            if( auto material = getMaterial() )
            {
                return material->getStateContext();
            }

            return nullptr;
        }

        template <typename T>
        void MaterialNode<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
        }

        template <typename T>
        SmartPtr<IStateListener> MaterialNode<T>::getStateListener() const
        {
            return nullptr;
        }

        template <typename T>
        void MaterialNode<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
        }

        template <typename T>
        bool MaterialNode<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    if( child->handleStateMessage( message ) )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <typename T>
        bool MaterialNode<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    if( child->handleStateChanged( state ) )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <typename T>
        void MaterialNode<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                graphicsSystem->lock();
            }
        }

        template <typename T>
        bool MaterialNode<T>::try_lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                return graphicsSystem->try_lock();
            }

            return false;
        }

        template <typename T>
        void MaterialNode<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                graphicsSystem->unlock();
            }
        }

        template <typename T>
        MaterialNode<T>::MaterialNodeStateListener::MaterialNodeStateListener() = default;

        template <typename T>
        MaterialNode<T>::MaterialNodeStateListener::~MaterialNodeStateListener() = default;

        template <typename T>
        void MaterialNode<T>::MaterialNodeStateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        template <typename T>
        bool MaterialNode<T>::MaterialNodeStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <typename T>
        bool MaterialNode<T>::MaterialNodeStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        template <typename T>
        MaterialNode<T> *MaterialNode<T>::MaterialNodeStateListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

        template <typename T>
        SmartPtr<MaterialNode<T>> MaterialNode<T>::MaterialNodeStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <typename T>
        void MaterialNode<T>::MaterialNodeStateListener::setOwner( SmartPtr<MaterialNode> owner )
        {
            m_owner = owner;
        }

    }  // namespace render
}  // namespace workphone

#endif  // CMaterialComponent_h__
