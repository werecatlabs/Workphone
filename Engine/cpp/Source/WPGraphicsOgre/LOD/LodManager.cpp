#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/LOD/LodManager.hpp"
#include "WPGraphicsOgre/LOD/LodObject.hpp"
#include "WPGraphicsOgre/LOD/LodPage.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        LodManager::LodManager()
        {
            m_grid.setCellSize( Vector2F::UNIT * 25.0f );
            m_grid.setCenter( Vector2F::ZERO );
            m_grid.setExtents( Vector2F::UNIT * 500.0f );
        }

        LodManager::~LodManager()
        {
        }

        void LodManager::addObject( SmartPtr<LodObject> lodObject )
        {
            Vector3F position = lodObject->getPosition();

            Vector2I cellIndex;
            m_grid.calculateCell( Vector2F( position.X(), position.Z() ), cellIndex );

            SmartPtr<LodPage> page = getPage( cellIndex );
            page->addObject( lodObject );
        }

        void LodManager::removeObject( SmartPtr<LodObject> lodObject )
        {
            // SmartPtr<LodPage> page = lodObject->getLodPage();
            // page->removeObject(lodObject);
        }

        SmartPtr<LodPage> LodManager::getPage( const Vector2I &index )
        {
            LodPages::iterator it = m_lodPages.find( index );
            if( it != m_lodPages.end() )
            {
                return it->second;
            }

            SmartPtr<LodPage> lodPage( new LodPage );
            m_lodPages[index] = lodPage;
            return lodPage;
        }

    }  // namespace render
}  // namespace workphone
