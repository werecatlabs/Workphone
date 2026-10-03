#ifndef IParticleAffector_h__
#define IParticleAffector_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleNode.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for particle affectors.
         */
        class WPCore_API IParticleAffector : public IParticleNode
        {
        public:
            /** Destructor. */
            ~IParticleAffector() override;

            /**
             * @brief Calculates the state of a particle using the given index.
             *
             * @param particle The particle whose state to calculate.
             * @param stateIndex The index of the state to calculate.
             * @param data Optional data to use in the calculation.
             */
            virtual void calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                         void *data = nullptr ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IParticleAffector_h__
