#ifndef IParticleComponent_h__
#define IParticleComponent_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

/**
 * @namespace workphone::render
 * @brief Contains rendering-related interfaces and classes for the particle system.
 */
namespace workphone
{
    namespace render
    {

        /**
         * @class IParticleNode
         * @brief Abstract base class representing a node in a particle system hierarchy.
         *
         * A particle node can have a parent and multiple children, forming a tree structure.
         * Each node can represent a logical or spatial grouping within a particle system.
         * This interface provides methods for managing the node hierarchy and spatial properties.
         */
        class WPCore_API IParticleNode : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor for safe polymorphic deletion.
             */
            ~IParticleNode() override;

            /**
             * @brief Adds a child node to this node.
             * @param child The child node to add. Must not be null.
             *
             * The child will be appended to the end of the children list.
             */
            virtual void addChild( SmartPtr<IParticleNode> child ) = 0;

            /**
             * @brief Inserts a child node at the specified index.
             * @param child The child node to add. Must not be null.
             * @param index The position at which to insert the child.
             *
             * If the index is out of range, the child may be appended to the end.
             */
            virtual void addChild( SmartPtr<IParticleNode> child, s32 index ) = 0;

            /**
             * @brief Removes a specific child node from this node.
             * @param child The child node to remove. Must not be null.
             *
             * If the child is not found, this operation has no effect.
             */
            virtual void removeChild( SmartPtr<IParticleNode> child ) = 0;

            /**
             * @brief Removes this node from its parent and the particle system.
             *
             * After removal, the node may be deleted or orphaned depending on ownership semantics.
             */
            virtual void remove() = 0;

            /**
             * @brief Gets the number of child nodes attached to this node.
             * @return The number of children.
             */
            virtual u32 getNumChildren() const = 0;

            /**
             * @brief Gets the child node at the specified index.
             * @param index The index of the child node.
             * @return The child node at the given index, or nullptr if out of range.
             */
            virtual SmartPtr<IParticleNode> getChildByIndex( u32 index ) const = 0;

            /**
             * @brief Gets the child node with the specified unique ID.
             * @param id The unique identifier of the child node.
             * @return The child node with the given ID, or nullptr if not found.
             */
            virtual SmartPtr<IParticleNode> getChildById( hash32 id ) const = 0;

            /**
             * @brief Gets an array of all child nodes attached to this node.
             * @return An array of smart pointers to child nodes.
             */
            virtual Array<SmartPtr<IParticleNode>> getChildren() const = 0;

            /**
             * @brief Gets the parent node of this node.
             * @return The parent node, or nullptr if this is a root node.
             */
            virtual SmartPtr<IParticleNode> getParent() const = 0;

            /**
             * @brief Sets the parent node for this node.
             * @param parent The new parent node. Can be nullptr to make this a root node.
             */
            virtual void setParent( SmartPtr<IParticleNode> parent ) = 0;

            /**
             * @brief Sets the local position of this node relative to its parent.
             * @param position The new local position vector.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Gets the local position of this node relative to its parent.
             * @return The local position vector.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Gets the absolute/world position of this node.
             * @return The absolute position vector in world space.
             */
            virtual Vector3<real_Num> getAbsolutePosition() const = 0;

            /**
             * @brief Gets the particle system this node belongs to.
             * @return The particle system, or nullptr if not assigned.
             */
            virtual SmartPtr<IParticleSystem> getParticleSystem() const = 0;

            /**
             * @brief Sets the particle system this node belongs to.
             * @param particleSystem The particle system to assign.
             */
            virtual void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) = 0;

            /**
             * @brief Registers the class for reflection or serialization.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IParticleComponent_h__
