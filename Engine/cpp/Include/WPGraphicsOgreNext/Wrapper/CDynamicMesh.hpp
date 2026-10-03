#ifndef _CDynamicMesh_H_
#define _CDynamicMesh_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/DynamicMesh.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        class CDynamicMesh : public CGraphicsObjectOgreNext<DynamicMesh>
        {
        public:
            CDynamicMesh();
            CDynamicMesh( SmartPtr<IGraphicsScene> creator );
            ~CDynamicMesh() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update();

            void setMaterialName( const String &materialName, s32 index = -1 );
            String getMaterialName( s32 index = -1 ) const;

            void setCastShadows( bool castShadows ) override;
            bool getCastShadows() const override;

            void setReceiveShadows( bool receiveShadows ) override;
            bool getReceiveShadows() const override;

            void setRecieveShadows( bool recieveShadows );
            bool getRecieveShadows() const;

            void setVisible( bool visible ) override;
            bool isVisible() const override;

            void setRenderQueueGroup( u32 renderQueue ) override;

            void setVisibilityFlags( u32 flags ) override;
            u32 getVisibilityFlags() const override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            void setMesh( SmartPtr<IMesh> mesh ) override;
            SmartPtr<IMesh> getMesh() const override;

            void setSubMesh( SmartPtr<ISubMesh> subMesh ) override;
            SmartPtr<ISubMesh> getSubMesh() const override;

            void setDirty( bool dirty ) override;

            void setOwner( SmartPtr<IGraphicsSceneNode> owner ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            DynamicMeshOgreNext *m_dynamicMesh = nullptr;

            SmartPtr<IGraphicsScene> m_creator;
            SmartPtr<IMesh> m_mesh;
            String m_materialName;
            bool m_receiveShadows = true;
        };
    }  // end namespace render
}  // namespace workphone

#endif
