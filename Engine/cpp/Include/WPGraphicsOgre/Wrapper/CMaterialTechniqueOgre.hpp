#ifndef CMaterialTechniqueOgre_h__
#define CMaterialTechniqueOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Interface/Graphics/IMaterialTechnique.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        class CMaterialTechniqueOgre : public MaterialTechnique
        {
        public:
            CMaterialTechniqueOgre();
            ~CMaterialTechniqueOgre() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void reload( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void initialise( Ogre::Technique *technique );

            /** @copydoc IResource::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IResource::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IObject::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            Ogre::Technique *getTechnique() const;

            void setTechnique( Ogre::Technique *technique );

            WP_CLASS_REGISTER_DECL;

        protected:
            class MaterialTextureOgreStateListener : public MaterialTechniqueStateListener
            {
            public:
                MaterialTextureOgreStateListener() = default;
                ~MaterialTextureOgreStateListener() override = default;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            Ogre::Technique *m_technique = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTechnique_h__
