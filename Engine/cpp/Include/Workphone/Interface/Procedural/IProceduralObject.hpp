#ifndef IProceduralObject_h__
#define IProceduralObject_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/Polygon3.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for procedural objects in the procedural system.
         *
         * This interface defines the contract for objects that can be procedurally generated or
         * manipulated within a scene. It provides methods for building, transforming, and managing
         * procedural nodes.
         */
        class WPCore_API IProceduralObject : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IProceduralObject() override;

            /**
             * @brief Builds or generates the procedural object.
             *
             * This method should be implemented to construct or update the geometry/data of the object.
             */
            virtual void build() = 0;

            /**
             * @brief Updates the bounding volume of the object.
             *
             * This should recalculate the object's bounds based on its current state.
             */
            virtual void updateBounds() = 0;

            /**
             * @brief Gets the world transform of the object.
             * @return The world transform as a Transform3.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Sets the world transform of the object.
             * @param transform The new world transform.
             */
            virtual void setWorldTransform( Transform3<real_Num> transform ) = 0;

            /**
             * @brief Sets the position of the object in world space.
             * @param position The new position vector.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Gets the position of the object in world space.
             * @return The position vector.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Gets the scale of the object.
             * @return The scale vector.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Sets the scale of the object.
             * @param scale The new scale vector.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /**
             * @brief Gets the orientation of the object as a quaternion.
             * @return The orientation quaternion.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Sets the orientation of the object.
             * @param orientation The new orientation quaternion.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Gets the procedural scene this object belongs to.
             * @return Smart pointer to the procedural scene.
             */
            virtual SmartPtr<IProceduralScene> getScene() const = 0;

            /**
             * @brief Sets the procedural scene for this object.
             * @param scene Smart pointer to the procedural scene.
             */
            virtual void setScene( SmartPtr<IProceduralScene> scene ) = 0;

            /**
             * @brief Creates a copy of this procedural object.
             * @return Smart pointer to the cloned object.
             */
            virtual SmartPtr<ISharedObject> clone() = 0;

            /**
             * @brief Adds a procedural node as a child to this object.
             * @param node Smart pointer to the node to add.
             */
            virtual void addNode( SmartPtr<IProceduralNode> node ) = 0;

            /**
             * @brief Removes a procedural node from this object.
             * @param node Smart pointer to the node to remove.
             */
            virtual void removeNode( SmartPtr<IProceduralNode> node ) = 0;

            /**
             * @brief Gets all child procedural nodes of this object.
             * @return Array of smart pointers to the child nodes.
             */
            virtual Array<SmartPtr<IProceduralNode>> getNodes() const = 0;

            /// @brief Macro for class registration and reflection.
            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralNode_h__
