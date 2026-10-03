#ifndef IMaterialNode_h__
#define IMaterialNode_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /** An interface for a material node. */
        class WPCore_API IMaterialNode : public ISharedObject
        {
        public:
            IMaterialNode();

            /** Virtual destructor. */
            ~IMaterialNode() override;

            /** Adds a child to this material node.
             * @param child The child to be added to the node.
             */
            virtual void addChild( SmartPtr<IMaterialNode> child ) = 0;

            /** Adds a child to this material node at the specified index.
             * @param child The child to be added to the node.
             * @param index The index at which to add the child.
             */
            virtual void addChild( SmartPtr<IMaterialNode> child, int index ) = 0;

            /** Removes a child from this material node.
             * @param child The child to be removed from the node.
             */
            virtual void removeChild( SmartPtr<IMaterialNode> child ) = 0;

            /** Removes this material node from its parent. */
            virtual void remove() = 0;

            /** Removes all children from this material node. */
            virtual void removeAllChildren() = 0;

            /** Gets the number of children of this material node.
             * @return The number of children.
             */
            virtual u32 getNumChildren() const = 0;

            /** Gets a child of this material node by its index.
             * @param index The index of the child to get.
             * @return A smart pointer to the child at the specified index.
             */
            virtual SmartPtr<IMaterialNode> getChildByIndex( u32 index ) const = 0;

            /** Gets a child of this material node by its ID.
             * @param id The ID of the child to get.
             * @return A smart pointer to the child with the specified ID.
             */
            virtual SmartPtr<IMaterialNode> getChildById( hash_type id ) const = 0;

            /**
             * @brief Gets an array of all children of this material node.
             * @return An array of smart pointers to the children of this node.
             */
            virtual Array<SmartPtr<IMaterialNode>> getChildren() const = 0;

            /**
             * @brief Sets the children of this material node.
             * @param children An array of smart pointers to the new children of this node.
             */
            virtual void setChildren( const Array<SmartPtr<IMaterialNode>> &children ) = 0;

            /**
             * @brief Get the parent node.
             * @return Pointer to the parent IMaterialNode, or nullptr if none.
             */
            virtual IMaterialNode *getParentPtr() const = 0;

            /**
             * @brief Gets the parent of this material node.
             * @return A smart pointer to the parent of this node. This can be null.
             */
            virtual SmartPtr<IMaterialNode> getParent() const = 0;

            /** Sets the parent of this material node.
             * @param parent A smart pointer to the parent of this node. This can be null.
             */
            virtual void setParent( SmartPtr<IMaterialNode> parent ) = 0;

            /** Gets the owner material of this node.
             * @return A smart pointer to the parent material of this node. This can be null.
             */
            virtual SmartPtr<IMaterial> getMaterial() const = 0;

            /** Sets the owner material of this node.
             * @param material A smart pointer to the parent material of this node. This can be null.
             */
            virtual void setMaterial( SmartPtr<IMaterial> material ) = 0;

            /** Gets the state object associated with this material node.
             * @return A smart pointer to the state object associated with this material node.
             */
            virtual SmartPtr<IStateContext> getStateContext() const = 0;

            /**
             * Sets the state object associated with this overlay element.
             * @param stateContext A pointer to the state object.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

            /**
             * Gets the state listener for this overlay element.
             * @return A pointer to the state listener.
             */
            virtual SmartPtr<IStateListener> getStateListener() const = 0;

            /**
             * Sets the state listener for this overlay element.
             * @param stateListener A pointer to the state listener.
             */
            virtual void setStateListener( SmartPtr<IStateListener> stateListener ) = 0;

            /**
             * @brief Handle an incoming state message.
             * @param message The state message to handle.
             * @return true if the message was handled and should not propagate.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Handle a full state change.
             * @param state The new state object.
             * @return true if the change was handled.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            /**
             * Checks if this overlay element is enabled.
             * @return `true` if the overlay element is enabled, `false` otherwise.
             */
            virtual bool isEnabled() const = 0;

            /**
             * Sets the enabled status for this overlay element.
             * @param enabled The enabled status to set. `true` to enable the overlay element, `false` to
             * disable it.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Gets the children of this material node that are of a specific type.
             * @return An array of smart pointers to the children of this material node that are of the
             * specified type.
             * @tparam T The type of children to get. This should be a type derived from IMaterialNode.
             * @note This function performs a dynamic cast on each child to check if it is of the
             * specified type. If a child is not of the specified type, it will be skipped and not
             * included in the returned array.
             * @note The returned array may be empty if no children of the specified type are found.
             * @note This function does not modify the children of this material node; it only filters
             * them by type.
             * @note The caller is responsible for ensuring that T is a valid type that can be used with
             * dynamic casting. If T is not a valid type, this function may throw an exception or exhibit
             * undefined behavior.
             */
            template <class T>
            Array<SmartPtr<T>> getChildrenByType() const;

            /**
             * @brief Sets the children of this material node to the given array of children of a
             * specific type.
             * @tparam T The type of children to set. This should be a type derived from IMaterialNode.
             * @param children An array of smart pointers to the new children of this material node. All
             * children in the array must be of the specified type.
             */
            template <class T>
            void setChildrenByType( const Array<SmartPtr<T>> &children );

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        void IMaterialNode::setChildrenByType( const Array<SmartPtr<T>> &children )
        {
            Array<SmartPtr<IMaterialNode>> newChildren;
            newChildren.reserve( children.size() );

            for( auto &child : children )
            {
                newChildren.push_back( child );
            }

            setChildren( newChildren );
        }

        template <class T>
        Array<SmartPtr<T>> IMaterialNode::getChildrenByType() const
        {
            auto children = getChildren();

            Array<SmartPtr<T>> result;
            result.reserve( children.size() );

            for( auto &child : children )
            {
                if( child->isDerived<T>() )
                {
                    auto derivedChild = workphone::static_pointer_cast<T>( child );
                    result.push_back( derivedChild );
                }
            }

            return result;
        }

    }  // end namespace render
}  // namespace workphone

#endif  // IMaterialNode_h__
