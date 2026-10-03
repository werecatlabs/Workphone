#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CBillboardSetOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        CBillboardSetOgre::CBillboardSetOgre( SmartPtr<IGraphicsScene> creator ) : m_creator( creator )
        {
        }

        CBillboardSetOgre::CBillboardSetOgre() = default;

        CBillboardSetOgre::~CBillboardSetOgre()
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            SmartPtr<CGraphicsSceneOgre> smgr;  // = m_creator;
            Ogre::SceneManager *ogreSmgr = smgr->getSceneManager();
            ogreSmgr->destroyBillboardSet( m_bbSet );
        }

        void CBillboardSetOgre::load( SmartPtr<ISharedObject> data )
        {
        }

        void CBillboardSetOgre::unload( SmartPtr<ISharedObject> data )
        {
        }

        void CBillboardSetOgre::initialise( Ogre::BillboardSet *bbSet )
        {
            m_bbSet = bbSet;
            expandPool<CBillboardOgre>( m_billboardPool, m_bbSet->getPoolSize() );
        }

        void CBillboardSetOgre::setMaterialName( const String &materialName, s32 index )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setMaterialName( materialName.c_str() );
        }

        String CBillboardSetOgre::getMaterialName( s32 index ) const
        {
            return StringUtil::EmptyString;
        }

        void CBillboardSetOgre::setCastShadows( bool castShadows )
        {
        }

        bool CBillboardSetOgre::getCastShadows() const
        {
            return false;
        }

        void CBillboardSetOgre::setReceiveShadows( bool recieveShadows )
        {
        }

        bool CBillboardSetOgre::getReceiveShadows() const
        {
            return false;
        }

        void CBillboardSetOgre::setVisible( bool isVisible )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setVisible( isVisible );
        }

        bool CBillboardSetOgre::isVisible() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_bbSet->isVisible();
        }

        void CBillboardSetOgre::setRenderQueueGroup( u8 renderQueue )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setRenderQueueGroup( renderQueue );
        }

        void CBillboardSetOgre::setVisibilityFlags( u32 flags )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setVisibilityFlags( flags );
        }

        u32 CBillboardSetOgre::getVisibilityFlags() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_bbSet->getVisibilityFlags();
        }

        SmartPtr<IGraphicsObject> CBillboardSetOgre::clone( const String &name ) const
        {
            return nullptr;
        }

        void CBillboardSetOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_bbSet;
        }

        void CBillboardSetOgre::clear()
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->clear();
            m_bbs.clear();
        }

        SmartPtr<IBillboard> CBillboardSetOgre::createBillboard( const Vector3F &position )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::Billboard *bb =
                m_bbSet->createBillboard( Ogre::Vector3( position.X(), position.Y(), position.Z() ) );

            SmartPtr<CBillboardOgre> *pBillboard;  // = m_billboardPool.getFreeElement();
            //if( !pBillboard )
            //{
            //    expandPool<CBillboardOgre>( m_billboardPool, 100 );
            //    pBillboard = m_billboardPool.getFreeElement();
            //}

            SmartPtr<CBillboardOgre> billboard = *pBillboard;

            billboard->initialise( bb );

            {
                SpinRWMutex::ScopedLock lock( BBSetMutex );
                m_bbs.push_back( billboard );
            }

            return billboard;
        }

        bool CBillboardSetOgre::removeBillboard( SmartPtr<IBillboard> billboard )
        {
            {
                ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
                Ogre::Billboard *bb = nullptr;
                billboard->_getObject( (void **)&bb );
                m_bbSet->removeBillboard( bb );
            }

            {
                SpinRWMutex::ScopedLock lock( BBSetMutex );
                auto it = m_bbs.begin();
                for( ; it != m_bbs.end(); ++it )
                {
                    SmartPtr<CBillboardOgre> &bb = *it;
                    if( bb == billboard )
                    {
                        m_bbs.erase( it );
                        break;
                    }
                }
            }

            return true;
        }

        void CBillboardSetOgre::setCullIndividually( bool cullIndividually )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setCullIndividually( cullIndividually );
        }

        void CBillboardSetOgre::setSortingEnabled( bool sortingEnabled )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setSortingEnabled( sortingEnabled );
        }

        void CBillboardSetOgre::setUseAccurateFacing( bool useAccurateFacing )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setUseAccurateFacing( useAccurateFacing );
        }

        void CBillboardSetOgre::setBounds( const AABB3F &box, f32 radius )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AxisAlignedBox ogreBox;  // (Ogre::Vector3(box.Min), Ogre::Vector3(box.Max));
            m_bbSet->setBounds( ogreBox, radius );
        }

        void CBillboardSetOgre::setDefaultDimensions( const Vector2F &dimension )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bbSet->setDefaultDimensions( dimension.X(), dimension.Y() );
        }

        void CBillboardSetOgre::setDefaultDimensions( const Vector3F &dimension )
        {
        }

        template <class T>
        void CBillboardSetOgre::expandPool( Pool<SmartPtr<CBillboardOgre>> &pool, u32 numNewElements )
        {
            // expand pool
            u32 numAllocated = 0;  //pool.getNumAllocated();
            //pool.setNumElements( numAllocated + numNewElements );
            u32 newNumAllocated = 0;  // pool.getNumAllocated();
            for( u32 i = 0; i < newNumAllocated; ++i )
            {
                /*			if(!pool[i])
                            {
                                SmartPtr<T> message = new T;
                                pool[i] = message;
                                RemoveReference(message);
                            }*/
            }
        }

        Array<SmartPtr<IBillboard>> CBillboardSetOgre::getBillboards() const
        {
            return Array<SmartPtr<IBillboard>>();
        }

        void CBillboardSetOgre::handleEvent( SmartPtr<IEvent> event )
        {
        }

        void CBillboardSetOgre::setZOrder( u32 zOrder )
        {
        }

        u32 CBillboardSetOgre::getZOrder() const
        {
            return 0;
        }

        AABB3F CBillboardSetOgre::getLocalAABB() const
        {
            return AABB3F();
        }

        void CBillboardSetOgre::setLocalAABB( const AABB3F &localAABB )
        {
        }

        void CBillboardSetOgre::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
        }

        void CBillboardSetOgre::detachFromParent()
        {
        }
    }  // namespace render
}  // namespace workphone
