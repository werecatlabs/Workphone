#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/LOD/LodPage.hpp"
#include "WPGraphicsOgre/LOD/LodObject.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        LodPage::LodPage()
        {
        }

        LodPage::~LodPage()
        {
        }

        void LodPage::addObject( SmartPtr<LodObject> lodObject )
        {
            lodObject->setLodPage( this );

            m_lodObjects[lodObject->getId()] = lodObject;
        }

        void LodPage::removeObject( SmartPtr<LodObject> lodObject )
        {
            lodObject->setLodPage( nullptr );
            m_lodObjects.erase( lodObject->getId() );
        }

    }  // namespace render
}  // namespace workphone
