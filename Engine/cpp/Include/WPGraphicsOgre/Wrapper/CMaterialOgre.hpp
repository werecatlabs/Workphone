#ifndef CMaterialOgre_h__
#define CMaterialOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <OgreMaterial.h>
#include <OgreRenderTargetListener.h>

namespace workphone
{
    namespace render
    {
        class CMaterialOgre : public Material
        {
        public:
            class MaterialOgreListener : public MaterialStateListener
            {
            public:
                MaterialOgreListener();
                ~MaterialOgreListener() override;
            };

            CMaterialOgre();
            ~CMaterialOgre() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void reload( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void setCubicTexture( const Array<SmartPtr<ITexture>> &textures, u32 layerIdx = 0 ) override;

            void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) override;

            void setFragmentParam( const String &name, f32 value ) override;
            void setFragmentParam( const String &name, const Vector2F &value ) override;
            void setFragmentParam( const String &name, const Vector3F &value ) override;
            void setFragmentParam( const String &name, const Vector4F &value ) override;
            void setFragmentParam( const String &name, const ColourF &value ) override;

            void setScale( const Vector3F &scale, u32 textureIndex = 0, u32 passIndex = 0,
                           u32 techniqueIndex = 0 ) override;

            SmartPtr<IMaterialNode> getRoot() const override;
            void setRoot( SmartPtr<IMaterialNode> root ) override;

            /** @copydoc IObject::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            Ogre::MaterialPtr getMaterial() const;

            void setMaterial( Ogre::MaterialPtr material );

            /** @copydoc IObject::isValid */
            bool isValid() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            class MaterialStateListener : public IStateListener
            {
            public:
                MaterialStateListener();
                MaterialStateListener( CMaterialOgre *material );
                ~MaterialStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                CMaterialOgre *getOwner() const;
                void setOwner( CMaterialOgre *owner );

            protected:
                CMaterialOgre *m_owner = nullptr;
            };

            class MaterialEvents : public Ogre::RenderTargetListener
            {
            public:
                MaterialEvents( CMaterialOgre *material );
                ~MaterialEvents() override;

                void preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;

                void postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;

                void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;

                void postViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;

                void viewportAdded( const Ogre::RenderTargetViewportEvent &evt ) override;

                void viewportRemoved( const Ogre::RenderTargetViewportEvent &evt ) override;

            private:
                CMaterialOgre *m_material = nullptr;
            };

            void createMaterialByType() override;

            Atomic<MaterialType> m_materialType = MaterialType::Standard;
            Ogre::MaterialPtr m_material;

            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IMaterial_h__
