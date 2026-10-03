#ifndef IParticleManager_h__
#define IParticleManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        class WPCore_API IParticleManager : public ISharedObject
        {
        public:
            ~IParticleManager() override;

            /**
             * @brief Adds a new particle system with the given id.
             * @param id The unique identifier for the particle system.
             * @return A smart pointer to the newly created particle system.
             */
            virtual SmartPtr<IParticleSystem> addParticleSystem( hash32 id ) = 0;

            /**
             * @brief Removes a particle system by its id.
             * @param id The unique identifier of the particle system to remove.
             */
            virtual void removeParticleSystem( hash32 id ) = 0;

            /**
             * @brief Gets a particle system by its id.
             * @param id The unique identifier of the particle system.
             * @return A smart pointer to the particle system, or nullptr if not found.
             */
            virtual SmartPtr<IParticleSystem> getParticleSystem( hash32 id ) const = 0;

            /**
             * @brief Gets a particle system by its name.
             * @param name The name of the particle system.
             * @return A smart pointer to the particle system, or nullptr if not found.
             */
            virtual SmartPtr<IParticleSystem> getParticleSystemByName( const String &name ) const = 0;

            /**
             * @brief Gets a list of all managed particle systems.
             * @return An array of smart pointers to all particle systems.
             */
            virtual Array<SmartPtr<IParticleSystem>> getParticleSystems() const = 0;

            /**
             * @brief Removes all particle systems from the manager.
             */
            virtual void clear() = 0;

            /**
             * @brief Pauses all managed particle systems.
             */
            virtual void pause() = 0;

            /**
             * @brief Resumes all managed particle systems.
             */
            virtual void resume() = 0;

            /**
             * @brief Enables all managed particle systems.
             */
            virtual void enable() = 0;

            /**
             * @brief Disables all managed particle systems.
             */
            virtual void disable() = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IParticleManager_h__
