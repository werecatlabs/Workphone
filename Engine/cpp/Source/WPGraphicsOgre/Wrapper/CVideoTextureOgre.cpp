#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CVideoTextureOgre.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        void CVideoTextureOgre::_copyFrameData( void *frameData, const Vector2I &size )
        {
            auto currentTaskId = Thread::getCurrentTask();
            if( currentTaskId == TaskId::Render )
            {
                auto texData = static_cast<u8 *>(
                    m_texture->getBuffer()->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );
                auto videoData = static_cast<u8 *>( frameData );

                int width = OgreUtil::calculateNearest2Pow( size.X() );
                int height = OgreUtil::calculateNearest2Pow( size.Y() );

                Memory::Memcpy( texData, videoData, width * height * 4 );
                // Memory::Memcpy(texData, videoData, width*height*4);
                m_texture->getBuffer()->unlock();
            }
            else
            {
                ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
                auto texData = static_cast<u8 *>(
                    m_texture->getBuffer()->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );
                auto videoData = static_cast<u8 *>( frameData );
                Memory::Memcpy( texData, videoData, size.X() * size.Y() * 4 );
                // Memory::Memcpy(texData, videoData, size.X()*size.Y()*4);
                m_texture->getBuffer()->unlock();
            }
        }

        void CVideoTextureOgre::_getObject( void **object )
        {
            *object = m_texture.getPointer();
        }

        void CVideoTextureOgre::_getObject( void **ppObject ) const
        {
        }

        Vector2I CVideoTextureOgre::getSize() const
        {
            return m_size;
        }

        CVideoTextureOgre::CVideoTextureOgre()
        {
        }

        CVideoTextureOgre::~CVideoTextureOgre()
        {
        }

        void CVideoTextureOgre::initialise( const String &name, const Vector2I &size )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            int width = OgreUtil::calculateNearest2Pow( size.X() );
            int height = OgreUtil::calculateNearest2Pow( size.Y() );

            Ogre::TexturePtr t = Ogre::TextureManager::getSingleton().createManual(
                name.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D,
                width, height, 1, 0, Ogre::PF_A8R8G8B8, Ogre::TU_DYNAMIC_WRITE_ONLY );

            t->load( true );

            Ogre::PixelFormat format = t->getFormat();
            if( format != Ogre::PF_A8R8G8B8 )
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    "ERROR: Pixel format is not PF_A8R8G8B8 which is what was requested!" );
            }

            auto texData =
                static_cast<u8 *>( t->getBuffer()->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );
            memset( texData, 0x0, width * height * 4 );
            t->getBuffer()->unlock();

            m_texture = t;
        }

        void CVideoTextureOgre::update()
        {
        }

        void CVideoTextureOgre::setSize( const Vector2I &size )
        {
            m_size = size;
        }

        SmartPtr<IRenderTarget> CVideoTextureOgre::getRenderTarget() const
        {
            return nullptr;
        }

        void CVideoTextureOgre::setRenderTarget( SmartPtr<IRenderTarget> rt )
        {
        }

        void CVideoTextureOgre::copyToTexture( SmartPtr<ITexture> &target )
        {
        }

        void CVideoTextureOgre::copyData( void *data, const Vector2I &size )
        {
            auto currentTaskId = Thread::getCurrentTask();
            if( currentTaskId == TaskId::Render )
            {
                auto texData = static_cast<u8 *>(
                    m_texture->getBuffer()->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );
                auto videoData = static_cast<u8 *>( data );

                int width = OgreUtil::calculateNearest2Pow( size.X() );
                int height = OgreUtil::calculateNearest2Pow( size.Y() );

                Memory::Memcpy( texData, videoData, width * height * 4 );
                // Memory::Memcpy(texData, videoData, width*height*4);
                m_texture->getBuffer()->unlock();
            }
            else
            {
                ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
                auto texData = static_cast<u8 *>(
                    m_texture->getBuffer()->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );
                auto videoData = static_cast<u8 *>( data );
                Memory::Memcpy( texData, videoData, size.X() * size.Y() * 4 );
                // Memory::Memcpy(texData, videoData, size.X()*size.Y()*4);
                m_texture->getBuffer()->unlock();
            }
        }

        Vector2I CVideoTextureOgre::getActualSize() const
        {
            return Vector2I();
        }

        void CVideoTextureOgre::getTextureGPU( void **ppTexture ) const
        {
        }

        void CVideoTextureOgre::getTextureFinal( void **ppTexture ) const
        {
        }

        size_t CVideoTextureOgre::getTextureHandle() const
        {
            return 0;
        }

        u32 CVideoTextureOgre::getUsageFlags() const
        {
            return 0;
        }

        void CVideoTextureOgre::setUsageFlags( u32 usageFlags )
        {
        }

        void CVideoTextureOgre::load()
        {
        }

        void CVideoTextureOgre::reload()
        {
        }

        void CVideoTextureOgre::unload()
        {
        }

        void CVideoTextureOgre::save()
        {
        }

        UUID CVideoTextureOgre::getFileSystemId() const
        {
            return {};
        }

        void CVideoTextureOgre::setFileSystemId( UUID id )
        {
        }

        SmartPtr<Properties> CVideoTextureOgre::getProperties() const
        {
            return nullptr;
        }

        void CVideoTextureOgre::setProperties( SmartPtr<Properties> properties )
        {
        }

    }  // namespace render
}  // namespace workphone
