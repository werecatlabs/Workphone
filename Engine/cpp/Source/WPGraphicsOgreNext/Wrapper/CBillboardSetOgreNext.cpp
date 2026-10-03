#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CBillboardSetOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <Math/Simple/OgreAabb.h>
#include <OgreBillboardSet.h>
#include <OgreSceneManager.h>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <utility>

namespace workphone::render
{

    CBillboardSetOgreNext::CBillboardSetOgreNext( SmartPtr<IGraphicsScene> creator ) :
        m_creator( std::move( creator ) )
    {
        setCreator( m_creator );
    }

    CBillboardSetOgreNext::~CBillboardSetOgreNext()
    {
        unload( nullptr );
    }

    void CBillboardSetOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        if( m_bbSet )
        {
            {
                for( auto &billboard : m_bbs )
                {
                    if( billboard )
                    {
                        billboard->setRenderData( nullptr );
                    }
                }
                m_bbs.clear();
            }

            if( m_bbSet->isAttached() )
            {
                m_bbSet->detachFromParent();
            }

            Ogre::SceneManager *ogreSmgr = nullptr;
            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &ogreSmgr ) );
            }

            if( ogreSmgr )
            {
                ogreSmgr->destroyBillboardSet( m_bbSet );
            }

            m_bbSet = nullptr;
            setGraphicsObject( nullptr );
        }

        m_creator = nullptr;
        BillboardSet::unload( data );
    }

    void CBillboardSetOgreNext::initialise( Ogre::v1::BillboardSet *bbSet )
    {
        m_bbSet = bbSet;
        setGraphicsObject( bbSet );

    }

    void CBillboardSetOgreNext::setMaterialName( const String &materialName, s32 index )
    {
        if( index > 0 )
        {
            return;
        }

        m_materialName = materialName;
        m_material = nullptr;

        if( m_bbSet )
        {
            m_bbSet->setDatablockOrMaterialName(
                materialName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        }
    }

    auto CBillboardSetOgreNext::getMaterialName( s32 index ) const -> String
    {
        if( index > 0 )
        {
            return StringUtil::EmptyString;
        }

        if( !m_materialName.empty() )
        {
            return m_materialName;
        }

        if( m_material )
        {
            return m_material->getName();
        }

        if( m_bbSet )
        {
            return m_bbSet->getDatablockOrMaterialName().c_str();
        }

        return StringUtil::EmptyString;
    }

    void CBillboardSetOgreNext::setMaterial( SmartPtr<IMaterial> material, s32 index )
    {
        if( index > 0 )
        {
            return;
        }

        m_material = material;
        m_materialName = material ? material->getName() : StringUtil::EmptyString;

        if( !m_bbSet )
        {
            return;
        }

        if( !material )
        {
            m_bbSet->setDatablockOrMaterialName(
                StringUtil::EmptyString.c_str(),
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            return;
        }

        if( !material->isLoaded() )
        {
            material->load( nullptr );
        }

        auto ogreMaterial = workphone::dynamic_pointer_cast<CMaterialOgreNext>( material );
        if( ogreMaterial )
        {
            if( auto datablock = ogreMaterial->getHlmsDatablock() )
            {
                m_bbSet->setDatablock( datablock );
                return;
            }

            auto datablockName = ogreMaterial->getDatablockName();
            if( !datablockName.empty() )
            {
                m_bbSet->setDatablockOrMaterialName(
                    datablockName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                return;
            }
        }

        m_bbSet->setDatablockOrMaterialName( material->getName().c_str(),
                                             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
    }

    auto CBillboardSetOgreNext::getMaterial( s32 index ) const -> SmartPtr<IMaterial>
    {
        if( index > 0 )
        {
            return nullptr;
        }

        return m_material;
    }

    void CBillboardSetOgreNext::setRenderQueueGroup( u8 renderQueue )
    {
        if( m_bbSet )
        {
            m_bbSet->setRenderQueueGroup( renderQueue );
        }
    }

    auto CBillboardSetOgreNext::clone( const String &name ) const -> SmartPtr<IGraphicsObject>
    {
        return nullptr;
    }

    void CBillboardSetOgreNext::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_bbSet;
        }
    }

    void CBillboardSetOgreNext::clear()
    {
        if( m_bbSet )
        {
            m_bbSet->clear();
        }

        for( auto &billboard : m_bbs )
        {
            if( billboard )
            {
                billboard->setRenderData( nullptr );
            }
        }
        m_bbs.clear();
    }

    auto CBillboardSetOgreNext::createBillboard( const Vector3F &position ) -> SmartPtr<IBillboard>
    {
        if( !m_bbSet )
            return nullptr;

        Ogre::v1::Billboard *bb =
            m_bbSet->createBillboard( Ogre::Vector3( position.X(), position.Y(), position.Z() ) );
        if( !bb )
        {
            return nullptr;
        }

        auto billboard = workphone::make_ptr<CBillboardOgreNext>();
        if( billboard )
        {
            billboard->initialise( bb );

            m_bbs.push_back( billboard );
        }

        return billboard;
    }

    auto CBillboardSetOgreNext::removeBillboard( SmartPtr<IBillboard> billboard ) -> bool
    {
        if( !m_bbSet || !billboard )
            return false;

        SmartPtr<CBillboardOgreNext> billboardOgreNext;
        {
            auto it = std::find_if( m_bbs.begin(), m_bbs.end(), [&billboard]( const auto &bb ) {
                return bb == billboard;
            } );

            if( it == m_bbs.end() )
            {
                return false;
            }

            billboardOgreNext = *it;
            m_bbs.erase( it );
        }

        Ogre::v1::Billboard *bb = nullptr;
        billboard->_getObject( reinterpret_cast<void **>( &bb ) );
        if( bb )
        {
            m_bbSet->removeBillboard( bb );
        }

        if( billboardOgreNext )
        {
            billboardOgreNext->setRenderData( nullptr );
        }

        return true;
    }

    void CBillboardSetOgreNext::setCullIndividually( bool cullIndividually )
    {
        if( m_bbSet )
        {
            m_bbSet->setCullIndividually( cullIndividually );
        }
    }

    void CBillboardSetOgreNext::setSortingEnabled( bool sortingEnabled )
    {
        if( m_bbSet )
        {
            m_bbSet->setSortingEnabled( sortingEnabled );
        }
    }

    void CBillboardSetOgreNext::setUseAccurateFacing( bool useAccurateFacing )
    {
        if( m_bbSet )
        {
            m_bbSet->setUseAccurateFacing( useAccurateFacing );
        }
    }

    void CBillboardSetOgreNext::setBounds( const AABB3F &box, f32 radius )
    {
        if( m_bbSet && box.isFinite() )
        {
            auto minimum = box.getMinimum();
            auto maximum = box.getMaximum();
            auto ogreBox = Ogre::Aabb::newFromExtents(
                Ogre::Vector3( minimum.X(), minimum.Y(), minimum.Z() ),
                Ogre::Vector3( maximum.X(), maximum.Y(), maximum.Z() ) );

            m_bbSet->setBounds( ogreBox, radius >= 0.0f ? radius : ogreBox.getRadius() );
        }
    }

    void CBillboardSetOgreNext::setDefaultDimensions( const Vector2F &dimension )
    {
        if( m_bbSet )
        {
            m_bbSet->setDefaultDimensions( dimension.X(), dimension.Y() );
        }
    }

    void CBillboardSetOgreNext::setDefaultDimensions( const Vector3F &dimension )
    {
        if( m_bbSet )
        {
            m_bbSet->setDefaultDimensions( dimension.X(), dimension.Y() );
        }
    }

    auto CBillboardSetOgreNext::getBillboards() const -> Array<SmartPtr<IBillboard>>
    {
        Array<SmartPtr<IBillboard>> result;
        for( const auto &bb : m_bbs )
        {
            result.push_back( bb );
        }
        return result;
    }

    void CBillboardSetOgreNext::handleEvent( SmartPtr<IEvent> event )
    {
    }
}  // namespace workphone::render
