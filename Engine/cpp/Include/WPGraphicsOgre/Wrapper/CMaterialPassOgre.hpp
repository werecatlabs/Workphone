#ifndef CMaterialPassOgre_h__
#define CMaterialPassOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IMaterialPass.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Core/Array.hpp>
#include <OgrePass.h>

namespace workphone
{
    namespace render
    {

        class CMaterialPassOgre : public MaterialPass
        {
        public:
            static const hash_type DIFFUSE_HASH;
            static const hash_type EMISSION_HASH;

            CMaterialPassOgre();
            ~CMaterialPassOgre() override;

            /** @copydoc IObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::initialise */
            void initialise( Ogre::Pass *pass );

            /** @copydoc IMaterialPass::setSceneBlending */
            void setSceneBlending( u32 blendType ) override;

            /** @copydoc IMaterialPass::isDepthCheckEnabled */
            bool isDepthCheckEnabled() const override;

            /** @copydoc IMaterialPass::setDepthCheckEnabled */
            void setDepthCheckEnabled( bool enabled ) override;

            /** @copydoc IMaterialPass::isDepthWriteEnabled */
            bool isDepthWriteEnabled() const override;

            /** @copydoc IMaterialPass::setDepthWriteEnabled */
            void setDepthWriteEnabled( bool enabled ) override;

            /** @copydoc IMaterialPass::getCullingMode */
            u32 getCullingMode() const override;

            /** @copydoc IMaterialPass::setCullingMode */
            void setCullingMode( u32 mode ) override;

            /** @copydoc IMaterialPass::toJson */
            SmartPtr<IMaterialTexture> createTextureUnit() override;

            void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 );

            /** @copydoc IMaterialPass::toJson */
            void setTexture( const String &fileName, u32 layerIdx = 0 ) override;

            /** @copydoc IMaterialPass::toJson */
            void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) override;

            /** @copydoc IMaterialPass::toJson */
            void setFragmentParam( const String &name, f32 value ) override;

            /** @copydoc IMaterialPass::toJson */
            void setFragmentParam( const String &name, const Vector2F &value ) override;

            /** @copydoc IMaterialPass::toJson */
            void setFragmentParam( const String &name, const Vector3F &value ) override;

            /** @copydoc IMaterialPass::toJson */
            void setFragmentParam( const String &name, const Vector4F &value ) override;

            /** @copydoc IMaterialPass::toJson */
            void setFragmentParam( const String &name, const ColourF &value ) override;

            /** @copydoc IResource::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IResource::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IResource::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            Ogre::Pass *getPass() const;
            void setPass( Ogre::Pass *pass );

            WP_CLASS_REGISTER_DECL;

        protected:
            class MaterialPassOgreStateListener : public MaterialPassStateListener
            {
            public:
                MaterialPassOgreStateListener() = default;
                ~MaterialPassOgreStateListener() override = default;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            void createTextureSlots() override;
            void setupMaterial() override;

            Ogre::Pass *m_pass = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CPass_h__
