#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CBillboardOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreBillboard.h>

namespace workphone
{
    namespace render
    {

        CBillboardOgre::CBillboardOgre() : m_bb( nullptr )
        {
        }

        CBillboardOgre::~CBillboardOgre()
        {
        }

        void CBillboardOgre::load( SmartPtr<ISharedObject> data )
        {
        }

        void CBillboardOgre::unload( SmartPtr<ISharedObject> data )
        {
        }

        void CBillboardOgre::initialise( Ogre::Billboard *bb )
        {
            m_bb = bb;
        }

        void CBillboardOgre::setPosition( const Vector3F &position )
        {
            // RecursiveMutex::ScopedLock lock(OgreMutex);
            m_position = position;
            // m_bb->setPosition(Ogre::Vector3(m_position));
        }

        Vector3F CBillboardOgre::getPosition() const
        {
            return m_position;
        }

        void CBillboardOgre::setDimensions( const Vector2F &dimensions )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_bb->setDimensions( dimensions.X(), dimensions.Y() );
        }

        Vector2F CBillboardOgre::getDimensions() const
        {
            return Vector2F::ZERO;
        }

        void CBillboardOgre::setColour( const ColourF &colour )
        {
            m_bb->setColour( Ogre::ColourValue( colour.r, colour.g, colour.b, colour.a ) );
        }

        ColourF CBillboardOgre::getColour() const
        {
            return ColourF::Red;
        }

        void CBillboardOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_bb;
        }

        bool CBillboardOgre::IsFree( void *element )
        {
            // SmartPtr<CBillboardOgre> billboard = *(static_cast<SmartPtr<CBillboardOgre>*>(element));
            // if( billboard->getReferenceCount() == 2 )
            //	return true;

            return false;
        }

        void CBillboardOgre::setOrientation( const QuaternionF &orientation )
        {
        }

        QuaternionF CBillboardOgre::getOrientation() const
        {
            return QuaternionF::identity();
        }

        void CBillboardOgre::setScale( const Vector3F &dimensions )
        {
        }

        Vector3F CBillboardOgre::getScale() const
        {
            return Vector3F::zero();
        }

        void *CBillboardOgre::_getRenderSystemTransform() const
        {
            return nullptr;
        }

        void *CBillboardOgre::getRenderData() const
        {
            return nullptr;
        }

        void CBillboardOgre::setRenderData( void *renderData )
        {
        }
    }  // namespace render
}  // namespace workphone
