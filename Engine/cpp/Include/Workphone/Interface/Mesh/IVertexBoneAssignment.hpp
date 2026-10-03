#ifndef IVertexBoneAssignment_h__
#define IVertexBoneAssignment_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @class IVertexBoneAssignment
     * @brief Interface for managing the relationship between a vertex and a bone in a skeletal animation
     * system.
     *
     * This interface defines the methods required to assign a vertex to a bone and specify the weight of
     * the influence that the bone has on the vertex. It is commonly used in skeletal animation systems
     * to define how vertices are influenced by bones during animations.
     */
    class WPCore_API IVertexBoneAssignment : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        ~IVertexBoneAssignment() override;

        /**
         * @brief Gets the index of the vertex associated with this bone assignment.
         *
         * @return The index of the vertex.
         */
        virtual u32 getVertexIndex() const = 0;

        /**
         * @brief Sets the index of the vertex associated with this bone assignment.
         *
         * @param vertexIndex The index of the vertex to associate with this bone assignment.
         */
        virtual void setVertexIndex( u32 vertexIndex ) = 0;

        /**
         * @brief Gets the index of the bone influencing the vertex.
         *
         * @return The index of the bone.
         */
        virtual u16 getBoneIndex() const = 0;

        /**
         * @brief Sets the index of the bone influencing the vertex.
         *
         * @param boneIndex The index of the bone to associate with this vertex.
         */
        virtual void setBoneIndex( u16 boneIndex ) = 0;

        /**
         * @brief Gets the weight of the bone's influence on the vertex.
         *
         * The weight determines how much influence the bone has on the vertex during skeletal animation.
         *
         * @return The weight of the bone's influence, typically in the range [0.0, 1.0].
         */
        virtual f32 getWeight() const = 0;

        /**
         * @brief Sets the weight of the bone's influence on the vertex.
         *
         * @param weight The weight of the bone's influence, typically in the range [0.0, 1.0].
         */
        virtual void setWeight( f32 weight ) = 0;

        /**
         * @brief Registers the class for runtime type information.
         *
         * This macro is used to enable runtime type identification and reflection for the class.
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IVertexBoneAssignment_h__
