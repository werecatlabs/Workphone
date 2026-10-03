#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialManagerOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreMaterialManager.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialManagerOgre, IMaterialManager );

        CMaterialManagerOgre::CMaterialManagerOgre()
        {
            m_materials.reserve( 1024 );
        }

        CMaterialManagerOgre::~CMaterialManagerOgre()
        {
            unload( nullptr );
        }

        void CMaterialManagerOgre::load( SmartPtr<ISharedObject> data )
        {
        }

        void CMaterialManagerOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                for( auto &material : m_materials )
                {
                    if( material )
                    {
                        material->unload( nullptr );
                    }
                }

                m_materials.clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IMaterial> CMaterialManagerOgre::cloneMaterial( SmartPtr<IMaterial> material,
                                                                 const String &clonedMaterialName )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto newMaterial = factoryManager->make_ptr<CMaterialOgre>();

                newMaterial->setParentPrototype( material );

                auto data = material->toData();
                newMaterial->fromData( data );
                newMaterial->setName( clonedMaterialName );

                m_materials.push_back( newMaterial );

                return newMaterial;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IMaterial> CMaterialManagerOgre::cloneMaterial( const String &name,
                                                                 const String &clonedMaterialName )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto materialResource = getByName( name );
            if( materialResource )
            {
                auto material = factoryManager->make_ptr<CMaterialOgre>();

                auto data = materialResource->toData();
                material->fromData( data );

                material->setName( clonedMaterialName );

                m_materials.push_back( material );

                return material;
            }

            return nullptr;
        }

        SmartPtr<IResource> CMaterialManagerOgre::create( const String &name )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto material = factoryManager->make_ptr<CMaterialOgre>();
            WP_ASSERT( material );

            material->setName( name );

            WP_ASSERT( material->getHandle() );
            if( auto handle = material->getHandle() )
            {
                auto uuid = StringUtil::getUUID();
                handle->setUUID( uuid );
            }

            material->setFilePath( name );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( name, fileInfo ) )
            {
                material->setFileSystemId( fileInfo.fileId );
            }

            m_materials.push_back( material );

            return material;
        }

        SmartPtr<IResource> CMaterialManagerOgre::create( const String &uuid, const String &name )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto material = factoryManager->make_ptr<CMaterialOgre>();
            WP_ASSERT( material );

            WP_ASSERT( material->getHandle() );

            material->setName( name );

            if( auto handle = material->getHandle() )
            {
                handle->setUUID( uuid );
            }

            material->setFilePath( name );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( name, fileInfo ) )
            {
                material->setFileSystemId( fileInfo.fileId );
            }

            m_materials.push_back( material );

            return material;
        }

        Pair<SmartPtr<IResource>, bool> CMaterialManagerOgre::createOrRetrieve( const String &uuid,
                                                                                const String &path,
                                                                                const String &type )
        {
            auto materialResource = getById( uuid );
            if( materialResource )
            {
                auto material = workphone::static_pointer_cast<CMaterialOgre>( materialResource );
                return Pair<SmartPtr<IResource>, bool>( material, false );
            }

            auto material = create( uuid, path );
            WP_ASSERT( material );

            return Pair<SmartPtr<IResource>, bool>( material, true );
        }

        Pair<SmartPtr<IResource>, bool> CMaterialManagerOgre::createOrRetrieve( const String &path )
        {
            auto materialResource = getByName( path );
            if( materialResource )
            {
                auto material = workphone::static_pointer_cast<CMaterialOgre>( materialResource );
                return Pair<SmartPtr<IResource>, bool>( material, false );
            }

            auto material = create( path );
            WP_ASSERT( material );

            return Pair<SmartPtr<IResource>, bool>( material, true );
        }

        void CMaterialManagerOgre::saveToFile( const String &filePath, SmartPtr<IResource> resource )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
                WP_ASSERT( resource );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                auto material = workphone::static_pointer_cast<IMaterial>( resource );
                auto pData = workphone::static_pointer_cast<Properties>( material->toData() );
                auto materialStr = DataUtil::toString( pData.get(), true );

                fileSystem->writeAllText( filePath, materialStr );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IResource> CMaterialManagerOgre::loadFromFile( const String &filePath )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                auto materialName = Path::getFileNameWithoutExtension( filePath );

                for( auto material : m_materials )
                {
                    auto currentMaterialName = material->getName();

                    if( materialName == currentMaterialName )
                    {
                        FileInfo fileInfo;
                        if( fileSystem->findFileInfo( filePath, fileInfo ) )
                        {
                            auto currentMaterialPath = String( fileInfo.filePath.c_str() );
                            auto currentFileId = material->getFileSystemId();

                            if( filePath == currentMaterialPath && fileInfo.fileId == currentFileId )
                            {
                                return material;
                            }
                        }
                    }
                }

                auto ext = Path::getFileExtension( filePath );
                if( ext == ".mat" )
                {
                    auto stream = fileSystem->open( filePath, true, false, false, false, false );
                    if( !stream )
                    {
                        stream = fileSystem->open( filePath, true, false, false, true, true );
                    }

                    if( stream )
                    {
                        auto materialStr = stream->getAsString();

                        auto materialData = workphone::make_ptr<Properties>();
                        DataUtil::parse( materialStr, materialData.get() );

                        auto material = create( materialName );

                        FileInfo fileInfo;
                        if( fileSystem->findFileInfo( filePath, fileInfo ) )
                        {
                            auto fileId = fileInfo.fileId;
                            material->setFileSystemId( fileId );
                        }

                        material->fromData( materialData );
                        material->load( nullptr );

                        m_materials.push_back( material );

                        return material;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CMaterialManagerOgre::load( const String &name )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

                auto materialResource = getByName( name );
                if( materialResource )
                {
                    auto material = workphone::static_pointer_cast<CMaterialOgre>( materialResource );
                    material->load( nullptr );
                    return material;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CMaterialManagerOgre::getByName( const String &name )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

                for( auto &material : m_materials )
                {
                    if( material->getName() == name )
                    {
                        return material;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CMaterialManagerOgre::getById( const String &uuid )
        {
            try
            {
                for( auto &material : m_materials )
                {
                    auto handle = material->getHandle();
                    if( handle->getUUIDAsString() == uuid )
                    {
                        return material;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CMaterialManagerOgre::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }
    }  // end namespace render
}  // namespace workphone
