#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CFontManager.hpp>
#include <WPGraphicsOgre/Wrapper/CFont.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreMaterialManager.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CFontManager, IMaterialManager );

        CFontManager::CFontManager()
        {
            m_fonts.reserve( 1024 );
        }

        CFontManager::~CFontManager()
        {
            unload( nullptr );
        }

        void CFontManager::load( SmartPtr<ISharedObject> data )
        {
        }

        void CFontManager::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                for( auto &font : m_fonts )
                {
                    font->unload( nullptr );
                    font = nullptr;
                }

                m_fonts.clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IResource> CFontManager::create( const String &name )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto font = factoryManager->make_ptr<CFont>();
            WP_ASSERT( font );

            auto handle = font->getHandle();
            WP_ASSERT( handle );

            setName( name );

            auto uuid = StringUtil::getUUID();
            handle->setUUID( uuid );

            m_fonts.push_back( font );

            return font;
        }

        SmartPtr<IResource> CFontManager::create( const String &uuid, const String &name )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto font = factoryManager->make_ptr<CFont>();
            WP_ASSERT( font );

            auto handle = font->getHandle();
            WP_ASSERT( handle );

            setName( name );

            handle->setUUID( uuid );

            m_fonts.push_back( font );

            return font;
        }

        Pair<SmartPtr<IResource>, bool> CFontManager::createOrRetrieve( const String &uuid,
                                                                        const String &path,
                                                                        const String &type )
        {
            auto fontResource = getById( uuid );
            if( fontResource )
            {
                auto font = workphone::static_pointer_cast<CFont>( fontResource );
                return Pair<SmartPtr<IResource>, bool>( font, false );
            }

            auto font = create( uuid, path );
            WP_ASSERT( font );

            return Pair<SmartPtr<IResource>, bool>( font, true );
        }

        Pair<SmartPtr<IResource>, bool> CFontManager::createOrRetrieve( const String &path )
        {
            auto fontResource = getByName( path );
            if( fontResource )
            {
                auto font = workphone::static_pointer_cast<CFont>( fontResource );
                return Pair<SmartPtr<IResource>, bool>( font, false );
            }

            auto font = create( path );
            WP_ASSERT( font );

            return Pair<SmartPtr<IResource>, bool>( font, true );
        }

        void CFontManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
        {
            try
            {
                //WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
                //WP_ASSERT( resource );

                //auto applicationManager = core::IApplicationManager::instance();
                //WP_ASSERT( applicationManager );

                //auto fileSystem = applicationManager->getFileSystem();
                //WP_ASSERT( fileSystem );

                //auto font = fb::static_pointer_cast<IFont>( resource );
                //auto pData = font->toData();

                //auto mat = pData->getDataAsType<data::font_graph>();
                //auto fontStr = DataUtil::toString( mat, true );

                //fileSystem->writeAllText( filePath, fontStr );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IResource> CFontManager::loadFromFile( const String &filePath )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                auto fontName = Path::getFileNameWithoutExtension( filePath );

                for( auto font : m_fonts )
                {
                    auto handle = font->getHandle();
                    auto currentMaterialName = font->getName();

                    if( fontName == currentMaterialName )
                    {
                        FileInfo fileInfo;
                        if( fileSystem->findFileInfo( filePath, fileInfo ) )
                        {
                            auto currentMaterialPath = String( fileInfo.filePath.c_str() );
                            auto currentFileId = font->getFileSystemId();

                            if( filePath == currentMaterialPath && fileInfo.fileId == currentFileId )
                            {
                                return font;
                            }
                        }
                    }
                }

                auto ext = Path::getFileExtension( filePath );
                if( ext == ".mat" )
                {
                    auto stream = fileSystem->open( filePath );
                    if( stream )
                    {
                        auto fontStr = stream->getAsString();

                        //data::font_graph fontData;
                        //DataUtil::parse( fontStr, &fontData );

                        auto font = create( fontName );

                        FileInfo fileInfo;
                        if( fileSystem->findFileInfo( filePath, fileInfo ) )
                        {
                            auto fileId = fileInfo.fileId;
                            font->setFileSystemId( fileId );
                        }

                        //auto data = fb::make_ptr<CData<data::font_graph>>();
                        //data->setData( &fontData );
                        //font->fromData( data );
                        //font->load( nullptr );

                        //m_fonts.push_back( font );

                        //return font;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CFontManager::loadResource( const String &name )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

                auto fontResource = getByName( name );
                if( fontResource )
                {
                    auto font = workphone::static_pointer_cast<CFont>( fontResource );
                    font->load( nullptr );
                    return font;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CFontManager::getByName( const String &name )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

                for( auto &font : m_fonts )
                {
                    if( font->getName() == name )
                    {
                        return font;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CFontManager::getById( const String &uuid )
        {
            try
            {
                for( auto &font : m_fonts )
                {
                    auto handle = font->getHandle();
                    if( handle->getUUIDAsString() == uuid )
                    {
                        return font;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CFontManager::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }
    }  // end namespace render
}  // namespace workphone
