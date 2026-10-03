#ifndef INode_h__
#define INode_h__

#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    /**
     * @brief Generic node mixin for building parent/child hierarchies.
     *
     * Node<T> is a templated class that extends T to provide a simple
     * tree-like parent/child relationship. It stores an id and a type
     * (both atomic), a pointer to the parent node and a list of children.
     *
     * The template parameter T is expected to provide getSharedFromThis<T>
     * and an unload(SmartPtr<T>) method which Node will call when the node
     * is unloaded. Node does not assume ownership semantics beyond using
     * SmartPtr for references to nodes.
     */
    template <class T>
    class Node : public T
    {
    public:
        using NodeList = Array<SmartPtr<Node<T>>>;

        /**
         * @brief Construct a Node with default id and type (0).
         */
        Node() : m_nodeId( 0 ), m_nodeType( 0 )
        {
        }

        /**
         * @brief Virtual destructor.
         */
        ~Node() override
        {
        }

        /**
         * @brief Recursively unload this node and all children.
         *
         * Calls unload(data) on every child, clears the children list,
         * resets the parent pointer and forwards the unload call to T.
         *
         * @param data Optional data forwarded to the underlying unload implementation.
         */
        void unload( SmartPtr<T> data ) override
        {
            for( auto &child : m_children )
            {
                child->unload( data );
            }

            m_children.clear();

            m_parent = nullptr;
            T::unload( data );
        }

        /**
         * @brief Get the node id.
         * @return Current node id.
         */
        u32 getNodeId() const
        {
            return m_nodeId;
        }

        /**
         * @brief Set the node id. Intended for internal use (prefixed with _).
         * @param nodeId New id to assign.
         */
        void _setNodeId( u32 nodeId )
        {
            m_nodeId = nodeId;
        }

        /**
         * @brief Get the node type.
         * @return Current node type value.
         */
        u32 getNodeType() const
        {
            return m_nodeType;
        }

        /**
         * @brief Set the node type value.
         * @param nodeType New type value.
         */
        void setNodeType( u32 nodeType )
        {
            m_nodeType = nodeType;
        }

        /**
         * @brief Returns true if this node has a parent.
         * @return true when a parent is set, false otherwise.
         */
        virtual bool hasParent() const
        {
            return m_parent != nullptr;
        }

        /**
         * @brief Returns a SmartPtr to the parent node.
         * @return Parent node or nullptr if none.
         */
        virtual SmartPtr<Node> getParent() const
        {
            return m_parent;
        }

        /**
         * @brief Add a child node to this node.
         *
         * The child is first removed from any existing parent, then appended
         * to this node's children list. The child's parent pointer is updated
         * to point to this node.
         *
         * @param child Child node to add (SmartPtr).
         */
        virtual void addChild( SmartPtr<Node> child )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            child->remove();  // remove from current parent
            m_children.push_back( child );

            auto pThisNode = T::template getSharedFromThis<Node<T>>();
            child->m_parent = pThisNode;
        }

        /**
         * @brief Remove a specific child from this node.
         *
         * Currently this function only clears the child's parent and is
         * expected to be extended to erase the child from m_children.
         *
         * @param child Child node to remove.
         * @return true if removal succeeded, false otherwise.
         */
        virtual bool removeChild( SmartPtr<Node> child )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            child->m_parent = nullptr;
            return false;  // m_children.erase_element(child);
        }

        /**
         * @brief Find a direct child by node id.
         * @param id Node id to search for.
         * @return SmartPtr to the child if found, nullptr otherwise.
         */
        virtual SmartPtr<Node> findChild( u32 id ) const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            for( u32 i = 0; i < m_children.size(); ++i )
            {
                const SmartPtr<Node> &child = m_children[i];
                if( child->getNodeId() == id )
                {
                    return child;
                }
            }

            return nullptr;
        }

        /**
         * @brief Detach this node from its parent.
         *
         * If this node has a parent, it will call the parent's removeChild
         * with a SmartPtr to this node. The parent's implementation is
         * responsible for actually removing the child from its list.
         */
        virtual void remove()
        {
            if( m_parent )
            {
                auto pThisNode = T::template getSharedFromThis<Node<T>>();
                m_parent->removeChild( pThisNode );
            }
        }

        /**
         * @brief Remove all direct children from this node.
         *
         * Each child is removed via removeChild which will clear the child's
         * parent pointer. The children container is iterated under lock.
         */
        virtual void removeAllChildren()
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            for( u32 i = 0; i < m_children.size(); ++i )
            {
                removeChild( m_children[i] );
            }
        }

        /**
         * @brief Returns a copy of the children list.
         *
         * The returned Array contains SmartPtr references to each child.
         * The operation is performed under the node mutex to ensure thread-safety.
         *
         * @return Array of SmartPtr<Node> representing direct children.
         */
        virtual Array<SmartPtr<Node>> getChildren() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            Array<SmartPtr<Node>> children;
            children.reserve( m_children.size() );

            for( u32 i = 0; i < m_children.size(); ++i )
                children.push_back( m_children[i] );

            return children;
        }

        WP_CLASS_REGISTER_TEMPLATE_DECL( Node, T );

    protected:
        atomic_u32 m_nodeId;   /**< Numeric id for this node (atomic). */
        atomic_u32 m_nodeType; /**< Numeric type/category for this node (atomic). */

        SmartPtr<Node> m_parent;          /**< Pointer to parent node or nullptr. */
        Array<SmartPtr<Node>> m_children; /**< List of direct children. */

        mutable RecursiveMutex m_mutex; /**< Mutex protecting children/parent. */
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, Node, T, T );

}  // namespace workphone

#endif  // INode_h__
