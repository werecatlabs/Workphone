#ifndef GraphNode_h__
#define GraphNode_h__

#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/List.hpp>
#include <Workphone/Core/Any.hpp>

/**
 * @file GraphNode.hpp
 * @brief Template for a graph node that extends a user-defined type.
 *
 * The GraphNode template inherits from the user-specified type T and
 * augments it with graph-related functionality: a list of neighboring
 * nodes and an arbitrary user-data container (Any). This header provides
 * the declaration and inline definitions of the template members.
 */

namespace workphone
{

    /**
     * @brief Node wrapper that adds graph semantics to an arbitrary type.
     *
     * GraphNode<T> publicly inherits from T and introduces graph-related
     * members: a collection of neighbor pointers and an Any payload for
     * storing arbitrary user data. Typical use is to create graph structures
     * where nodes carry domain-specific data (provided by T) while the
     * GraphNode wrapper provides topology and cloning support.
     *
     * @tparam T The user-defined type that this node augments.
     */
    template <class T>
    class GraphNode : public T
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Constructs the underlying T subobject using its default constructor
         * and leaves the neighbor list and user data in their default state.
         */
        GraphNode();

        /**
         * @brief Virtual destructor.
         *
         * Declared virtual (override) to allow safe deletion through base
         * class pointers when T defines a virtual destructor.
         */
        ~GraphNode() override;

        /**
         * @brief Returns a copy of the neighbor list.
         *
         * The returned list contains SmartPtr references to adjacent
         * GraphNode<T> objects. The method returns by value to provide a
         * snapshot of the current neighbors.
         *
         * @return List of smart pointers to neighboring nodes.
         */
        List<SmartPtr<GraphNode<T>>> getNeighbors() const;

        /**
         * @brief Replaces the current neighbor list.
         *
         * @param neighbors New list of neighbors to assign.
         */
        void setNeighbors( const List<SmartPtr<GraphNode<T>>> &neighbors );

        /**
         * @brief Adds a single neighbor to this node.
         *
         * The neighbor is appended/added to the node's adjacency list.
         * @param neighbor Smart pointer to the node to add as a neighbor.
         */
        void addNeighbor( SmartPtr<GraphNode<T>> neighbor );

        /**
         * @brief Retrieves the opaque user data associated with the node.
         *
         * Any can store arbitrary types and is intended for client-specific
         * payloads that should be carried alongside the node.
         *
         * @return Copy of the stored Any payload.
         */
        Any getUserData() const;

        /**
         * @brief Assigns an opaque user data payload to the node.
         *
         * @param data Any containing the user payload to store.
         */
        void setUserData( const Any &data );

        /**
         * @brief Creates a deep-ish clone of this graph node.
         *
         * The default implementation constructs a new GraphNode<T>, copies
         * the Any user data and attempts to clone each neighbor and add it
         * to the cloned node. Subclasses may override this to change cloning
         * behavior. Note: cloning neighbors as done here may produce a new
         * topology dependent on GraphNode<T>::clone() implementations.
         *
         * @return A cloned GraphNode<T> instance (by value).
         */
        virtual GraphNode<T> clone() const;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Arbitrary user payload attached to this node.
         *
         * Clients can store any serializable/opaque data here. The Any type
         * provides type-erased storage for convenience.
         */
        Any m_userData;

        /**
         * @brief Adjacency list of this node.
         *
         * Each neighbor is held in a SmartPtr to manage lifetime and shared
         * ownership semantics within the graph.
         */
        List<SmartPtr<GraphNode<T>>> m_neighbors;
    };

    /**
     * @brief Register the GraphNode template with the runtime/type system.
     */
    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, GraphNode, T, T );

    // Out-of-class template member definitions
    template <class T>
    GraphNode<T>::GraphNode() = default;

    template <class T>
    GraphNode<T>::~GraphNode() = default;

    template <class T>
    List<SmartPtr<GraphNode<T>>> GraphNode<T>::getNeighbors() const
    {
        return m_neighbors;
    }

    template <class T>
    void GraphNode<T>::setNeighbors( const List<SmartPtr<GraphNode<T>>> &neighbors )
    {
        m_neighbors = neighbors;
    }

    template <class T>
    void GraphNode<T>::addNeighbor( SmartPtr<GraphNode<T>> neighbor )
    {
        m_neighbors = neighbor;
    }

    template <class T>
    Any GraphNode<T>::getUserData() const
    {
        return m_userData;
    }

    template <class T>
    void GraphNode<T>::setUserData( const Any &data )
    {
        m_userData = data;
    }

    template <class T>
    GraphNode<T> GraphNode<T>::clone() const
    {
        auto clonedNode = workphone::make_ptr<GraphNode<T>>();
        clonedNode->m_userData = this->m_userData;

        for( auto &neighbor : m_neighbors )
        {
            auto clonedNeighbor = neighbor->clone();
            clonedNode->addNeighbor( clonedNeighbor );
        }

        return clonedNode;
    }

}  // namespace workphone

#endif  // GraphNode_h__
