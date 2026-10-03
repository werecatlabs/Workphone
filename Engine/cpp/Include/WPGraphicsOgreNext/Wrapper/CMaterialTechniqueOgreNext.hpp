#ifndef CMaterialTechniqueOgreNext_h__
#define CMaterialTechniqueOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Ogre-next implementation of the MaterialTechnique interface.
         *
         * This class wraps an `Ogre::Technique` and adapts it to the engine's
         * `MaterialTechnique` abstraction. It manages the lifetime reference to
         * the underlying Ogre technique and provides factory functionality for
         * creating material passes that belong to this technique.
         */
        class CMaterialTechniqueOgreNext : public MaterialTechnique
        {
        public:
            /**
             * @brief Construct a new CMaterialTechniqueOgreNext instance.
             */
            CMaterialTechniqueOgreNext();

            /**
             * @brief Destroy the CMaterialTechniqueOgreNext instance.
             */
            ~CMaterialTechniqueOgreNext() override;

            /**
             * @brief Load the technique from serialized/shared data.
             * @param data Shared object containing serialized technique data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reload or refresh the technique using new data.
             * @param data Shared object containing updated technique data.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release any resources associated with the technique.
             * @param data Optional shared object used during unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Initialise this wrapper with an existing Ogre technique.
             * @param technique Pointer to an existing `Ogre::Technique`.
             *
             * The wrapper will reference the provided technique but does not
             * take full ownership — the caller is responsible for managing
             * Ogre resource lifetimes as required by the engine conventions.
             */
            void initialise( Ogre::Technique *technique );

            /**
             * @brief Get the scheme identifier associated with this technique.
             * @return hash32 Scheme id used to group or select techniques.
             */
            hash32 getScheme() const override;

            /**
             * @brief Set the scheme identifier for this technique.
             * @param scheme A hashed identifier representing the rendering scheme.
             */
            void setScheme( hash32 scheme ) override;

            /**
             * @brief Create a new material pass object for this technique.
             * @return SmartPtr<IMaterialPass> Newly created pass instance.
             */
            SmartPtr<IMaterialPass> createPass() override;

            /**
             * @copydoc MaterialTechnique::getChildObjects
             *
             * Returns child objects owned by this technique wrapper (for
             * example passes) so they can be inspected or serialized by
             * higher-level systems.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the underlying Ogre technique pointer.
             * @return Ogre::Technique* Raw pointer to the wrapped Ogre technique.
             */
            Ogre::Technique *getTechnique() const;

            /**
             * @brief Set the underlying Ogre technique pointer.
             * @param technique Pointer to the Ogre technique to wrap.
             */
            void setTechnique( Ogre::Technique *technique );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Raw pointer to the wrapped Ogre technique.
             *
             * This pointer may be null when no technique has been assigned.
             */
            Ogre::Technique *m_technique = nullptr;

            /**
             * @brief Hashed identifier for the rendering scheme this technique
             * belongs to.
             */
            hash32 m_scheme = 0;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CMaterialTechniqueOgreNext_h__
