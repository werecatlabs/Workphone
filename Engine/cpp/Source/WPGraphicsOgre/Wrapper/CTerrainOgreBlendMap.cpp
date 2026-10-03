#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainOgreBlendMap.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainOgre.hpp>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrain.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainGroup.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainQuadTreeNode.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainMaterialGeneratorA.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainPaging.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainLayerBlendMap.h>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreSharedPtr.h>
#include <OgreShadowCameraSetup.h>

namespace workphone
{
    namespace render
    {
        CTerrainOgreBlendMap::CTerrainOgreBlendMap() : m_terrain( nullptr ), m_blendMap( nullptr )
        {
            auto engine = core::IApplicationManager::instance();

            // m_stateContext = platformMgr->createStateObject();
            // m_stateContext->add();

            m_stateListener = SmartPtr<IStateListener>( new TerrainBlendMapStateListener( this ) );
            m_stateContext->addStateListener( m_stateListener );
        }

        CTerrainOgreBlendMap::~CTerrainOgreBlendMap()
        {
        }

        void CTerrainOgreBlendMap::initialise( SmartPtr<IGraphicsTerrain> terrain, u32 index )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            ScopedLock lock( graphicsSystem );

            m_terrain = static_cast<CTerrainOgre *>( terrain.get() );

            Ogre::TerrainGroup *terrainGroup = m_terrain->getTerrainGroup();

            Ogre::TerrainGroup::TerrainIterator ti = terrainGroup->getTerrainIterator();
            while( ti.hasMoreElements() )
            {
                Ogre::Terrain *t = ti.getNext()->instance;
                m_blendMap = t->getLayerBlendMap( index );
                break;
            }

            m_index = index;
        }

        void CTerrainOgreBlendMap::initialise( Ogre::TerrainLayerBlendMap *blendMap )
        {
            m_blendMap = blendMap;
        }

        void CTerrainOgreBlendMap::updateModifications()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            ScopedLock lock( graphicsSystem );
            m_blendMap->update();
        }

        void CTerrainOgreBlendMap::loadImage( const String &fileName,
                                              const String &path /*= StringUtil::EmptyString*/ )
        {
        }

        void CTerrainOgreBlendMap::saveImage( const String &fileName,
                                              const String &path /*= StringUtil::EmptyString*/ )
        {
        }

        f32 CTerrainOgreBlendMap::getBlendValue( u32 x, u32 y )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            ScopedLock lock( graphicsSystem );

            return m_blendMap->getBlendValue( x, y );
        }

        void CTerrainOgreBlendMap::setBlendValue( u32 x, u32 y, f32 blendValue )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            ScopedLock lock( graphicsSystem );

            Ogre::TerrainGroup *terrainGroup = m_terrain->getTerrainGroup();
            Ogre::TerrainGroup::TerrainIterator ti = terrainGroup->getTerrainIterator();
            while( ti.hasMoreElements() )
            {
                Ogre::Terrain *t = ti.getNext()->instance;
                m_blendMap = t->getLayerBlendMap( m_index );
                break;
            }

            m_blendMap->setBlendValue( x, y, blendValue );
        }

        u32 CTerrainOgreBlendMap::getSize() const
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            ScopedLock lock( graphicsSystem );

            return m_blendMap->getParent()->getLayerBlendMapSize();
        }

        SmartPtr<IGraphicsTerrain> CTerrainOgreBlendMap::getOwner() const
        {
            return m_terrain;
        }

        Ogre::TerrainLayerBlendMap *CTerrainOgreBlendMap::getBlendMap() const
        {
            return m_blendMap;
        }

        void CTerrainOgreBlendMap::setBlendMap( Ogre::TerrainLayerBlendMap *blendMap )
        {
            m_blendMap = blendMap;
        }

        CTerrainOgreBlendMap::TerrainBlendMapStateListener::TerrainBlendMapStateListener(
            CTerrainOgreBlendMap *terrainBlendMap ) :
            m_terrainBlendMap( terrainBlendMap )
        {
        }

        CTerrainOgreBlendMap::TerrainBlendMapStateListener::~TerrainBlendMapStateListener()
        {
        }

        bool CTerrainOgreBlendMap::TerrainBlendMapStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            // if(message->isExactly(StateMessageBlendMapValue::TYPE_INFO))
            //{
            //	StateMessageBlendMapValuePtr msg = message;
            //	Vector2I vec = msg->getCoordinates();
            //	m_terrainBlendMap->setBlendValue(vec.X(), vec.Y(), msg->getBlendValue());
            // }
            // else if(message->isExactly(StateMessageDirty::TYPE_INFO))
            //{
            //	m_terrainBlendMap->updateModifications();
            // }

            return false;
        }

        bool CTerrainOgreBlendMap::TerrainBlendMapStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            return false;
        }

    }  // end namespace render
}  // namespace workphone
