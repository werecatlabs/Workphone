#ifndef _CBillboardSet_H
#define _CBillboardSet_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/BillboardSet.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CBillboardOgreNext.hpp>
#include <vector>
#include <list>

namespace workphone
{
    namespace render
    {

        class CBillboardSetOgreNext : public CGraphicsObjectOgreNext<BillboardSet>
        {
        public:
            CBillboardSetOgreNext( SmartPtr<IGraphicsScene> creator );
            ~CBillboardSetOgreNext() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void initialise( Ogre::v1::BillboardSet *bbSet );

            void setMaterialName( const String &materialName, s32 index = -1 ) override;
            String getMaterialName( s32 index = -1 ) const override;

            /** @copydoc IBillboardSet::setMaterial */
            void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) override;

            /** @copydoc IBillboardSet::getMaterial */
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override;

            void setRenderQueueGroup( u8 renderQueue );

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            // IBillboardSet functions
            void clear() override;

            SmartPtr<IBillboard> createBillboard( const Vector3F &position = Vector3F::zero() ) override;
            bool removeBillboard( SmartPtr<IBillboard> billboard ) override;

            void setCullIndividually( bool cullIndividually ) override;
            void setSortingEnabled( bool sortingEnabled ) override;
            void setUseAccurateFacing( bool useAccurateFacing ) override;

            void setBounds( const AABB3F &box, f32 radius ) override;

            void setDefaultDimensions( const Vector2F &dimension );

            void setDefaultDimensions( const Vector3F &dimension ) override;

        protected:
            Array<SmartPtr<IBillboard>> getBillboards() const override;

            void handleEvent( SmartPtr<IEvent> event );

            Ogre::v1::BillboardSet *m_bbSet = nullptr;
            SmartPtr<IGraphicsScene> m_creator;
            String m_name;
            String m_materialName;
            SmartPtr<IMaterial> m_material;

            using BillboardList = std::list<SmartPtr<CBillboardOgreNext>>;
            BillboardList m_bbs;
        };
    }  // end namespace render
}  // namespace workphone

#endif
