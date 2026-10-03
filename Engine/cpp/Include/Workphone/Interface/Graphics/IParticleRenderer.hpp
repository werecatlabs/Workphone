#ifndef IParticleRenderer_h__
#define IParticleRenderer_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleNode.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Particle renderer interface.
         */
        class WPCore_API IParticleRenderer : public IParticleNode
        {
        public:
            /**
             * @brief Destructor.
             */
            ~IParticleRenderer() override;

            /**
             * @brief Set the name of the material to use for rendering particles.
             *
             * @param materialName The name of the material.
             * @param index The index of the sub-material to use, or -1 to set the name for all
             * sub-materials.
             */
            virtual void setMaterialName( const String &materialName, s32 index = -1 ) = 0;

            /**
             * @brief Get the name of the material used for rendering particles.
             *
             * @param index The index of the sub-material to get, or -1 to get the name of the first
             * sub-material.
             * @return The name of the material.
             */
            virtual String getMaterialName( s32 index = -1 ) const = 0;

            /**
             * @brief Add a particle to the renderer.
             *
             * @param particle The particle to add.
             */
            virtual void addParticle( SmartPtr<IParticle> particle ) = 0;

            /**
             * @brief Remove a particle from the renderer.
             *
             * @param particle The particle to remove.
             */
            virtual void removeParticle( SmartPtr<IParticle> particle ) = 0;

            /**
             * @brief Remove all particles from the renderer.
             */
            virtual void clearParticles() = 0;

            /**
             * @brief Set the visibility flags for the renderer.
             *
             * @param flags The visibility flags.
             */
            virtual void setVisibilityFlags( u32 flags ) = 0;

            /**
             * @brief Get the visibility flags for the renderer.
             *
             * @return The visibility flags.
             */
            virtual u32 getVisibilityFlags() const = 0;

            /**
             * @brief Get the parent scene node.
             *
             * @return A smart pointer to the parent scene node.
             */
            virtual SmartPtr<IGraphicsSceneNode> getParentSceneNode() const = 0;

            /**
             * @brief Set the parent scene node.
             *
             * @param parentSceneNode A smart pointer to the parent scene node.
             */
            virtual void setParentSceneNode( SmartPtr<IGraphicsSceneNode> parentSceneNode ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IParticleRenderer_h__
