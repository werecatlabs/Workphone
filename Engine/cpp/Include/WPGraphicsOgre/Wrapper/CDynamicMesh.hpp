#ifndef _CDynamicMesh_H_
#define _CDynamicMesh_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IDynamicMesh.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <DynamicMesh.hpp>

namespace workphone
{
    namespace render
    {

        class CDynamicMesh : public CGraphicsObjectOgre<IDynamicMesh>
        {
        public:
            CDynamicMesh() = default;
            CDynamicMesh( SmartPtr<IGraphicsScene> creator );
            ~CDynamicMesh() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void setMesh( SmartPtr<IMesh> mesh );
            SmartPtr<IMesh> getMesh() const;

            void setSubMesh( SmartPtr<ISubMesh> subMesh ) override;
            SmartPtr<ISubMesh> getSubMesh() const override;

            void setDirty( bool dirty ) override;

            void setOwner( SmartPtr<IGraphicsSceneNode> owner ) override;

            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            DynamicMesh *m_dynamicMesh = nullptr;
            Array<DynamicMesh *> m_subMeshes;
        };
    }  // end namespace render
}  // namespace workphone

#endif
