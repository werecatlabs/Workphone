#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawCubemapTexture.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

namespace workphone
{
    namespace render
    {
        // Internal backend implementation; no additional public texture API.
        bool replaceClawTexturePixels( ClawTexture *texture, const void *input, const Vector2I &size,
                                       String &error );

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawTextureManager, TextureManager );

        ClawTextureManager::ClawTextureManager() = default;
        ClawTextureManager::~ClawTextureManager() = default;

        SmartPtr<ITexture> ClawTextureManager::createCubeMap( const Array<SmartPtr<ITexture>> &textures )
        {
            ScopedLock<TextureManager> lifecycle( this );
            auto texture = workphone::make_ptr<ClawCubemapTexture>();
            texture->setName( "GeneratedCubemap/" + StringUtil::getUUID() );
            texture->setFaces( textures );
            texture->load( nullptr );
            m_textures.push_back( texture );
            return texture;
        }

        void ClawTextureManager::load( SmartPtr<ISharedObject> data )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );
            TextureManager::load( data );
            setLoadingState( LoadingState::Loaded );
        }

        void ClawTextureManager::unload( SmartPtr<ISharedObject> data )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            auto textures = std::move( m_textures );
            for( auto &texture : textures )
            {
                if( texture )
                {
                    texture->unload( nullptr );
                }
            }
            m_renderTargets.releaseStorage();
            m_textures.releaseStorage();
            m_evictionCursor = 0;
            TextureManager::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        SmartPtr<ITexture> ClawTextureManager::createTexture( const String &uuid, const String &name )
        {
            auto texture = workphone::make_ptr<ClawTexture>();
            if( !texture )
            {
                return nullptr;
            }

            texture->setName( name );
            texture->setFilePath( name );
            if( auto handle = texture->getHandle() )
            {
                handle->setUUID( StringUtil::isNullOrEmpty( uuid ) ? StringUtil::getUUID() : uuid );
            }

            m_textures.push_back( texture );
            return texture;
        }

        SmartPtr<IResource> ClawTextureManager::create( const String &name )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( StringUtil::isNullOrEmpty( name ) )
            {
                return nullptr;
            }
            if( auto existing = getByName( name ) )
            {
                return existing;
            }
            return createTexture( String(), name );
        }

        SmartPtr<IResource> ClawTextureManager::create( const String &uuid, const String &name )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( StringUtil::isNullOrEmpty( name ) )
            {
                return nullptr;
            }
            if( !StringUtil::isNullOrEmpty( uuid ) )
            {
                if( auto existing = getById( uuid ) )
                {
                    return existing;
                }
            }
            if( auto existing = getByName( name ) )
            {
                return existing;
            }
            return createTexture( uuid, name );
        }

        SmartPtr<IResource> ClawTextureManager::loadResource( const String &name )
        {
            ScopedLock<TextureManager> lifecycle( this );
            auto resource = getByName( name );
            if( !resource )
            {
                resource = create( name );
            }
            if( resource && !resource->isLoaded() )
            {
                resource->load( nullptr );
            }
            return resource;
        }

        SmartPtr<IResource> ClawTextureManager::loadFromFile( const String &filePath )
        {
            ScopedLock<TextureManager> lifecycle( this );
            auto resource = loadResource( filePath );
            if( resource )
            {
                resource->setFilePath( filePath );
                resource->loadFromFile( filePath );
            }
            return resource;
        }

        SmartPtr<IResource> ClawTextureManager::cloneResource( SmartPtr<IResource> resource,
                                                               const String &clonedResourceName )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( !resource || !resource->isDerived<ITexture>() )
            {
                return nullptr;
            }
            return cloneTexture( workphone::static_pointer_cast<ITexture>( resource ),
                                 clonedResourceName );
        }

        SmartPtr<IResource> ClawTextureManager::cloneResource( const String &name,
                                                               const String &clonedResourceName )
        {
            return cloneResource( getByName( name ), clonedResourceName );
        }

        Pair<SmartPtr<IResource>, bool> ClawTextureManager::createOrRetrieve( const String &uuid,
                                                                              const String &path,
                                                                              const String & /*type*/ )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( !StringUtil::isNullOrEmpty( uuid ) )
            {
                if( auto resource = getById( uuid ) )
                {
                    return { resource, false };
                }
            }
            if( auto resource = getByName( path ) )
            {
                return { resource, false };
            }
            return { create( uuid, path ), true };
        }

        Pair<SmartPtr<IResource>, bool> ClawTextureManager::createOrRetrieve( const String &path )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( auto resource = getByName( path ) )
            {
                return { resource, false };
            }
            return { create( path ), true };
        }

        SmartPtr<ITexture> ClawTextureManager::createManual( const String &name,
                                                             const String & /*group*/, u8 texType,
                                                             u32 width, u32 height, u32 /*depth*/,
                                                             s32 /*numMips*/, u8 /*format*/, s32 usage )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( width > 16384 || height > 16384 )
            {
                return nullptr;
            }
            auto texture = workphone::dynamic_pointer_cast<ClawTexture>( create( name ) );
            if( texture )
            {
                // Manual textures have no file to decode. Initialise the pixel storage before
                // publishing dimensions/usage so a failed replacement preserves the old resource.
                const Vector2I size( static_cast<s32>( width ), static_cast<s32>( height ) );
                if( !( static_cast<u32>( usage ) & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
                {
                    String error;
                    if( texture->getRenderTarget() ||
                        !replaceClawTexturePixels( texture.get(), nullptr, size, error ) )
                    {
                        if( !error.empty() )
                        {
                            WP_LOG_WARNING( error );
                        }
                        return nullptr;
                    }
                }
                else
                {
                    texture->setSize( size );
                }
                texture->setUsageFlags( static_cast<u32>( usage ) );
                texture->setTextureType( static_cast<TextureType>( texType ) );
                texture->load( nullptr );
            }
            return texture;
        }

        SmartPtr<ITexture> ClawTextureManager::createRenderTexture()
        {
            ScopedLock<TextureManager> lifecycle( this );
            auto texture = workphone::make_ptr<ClawTexture>();
            texture->setName( "ClawRenderTexture_" + StringUtil::toString( m_renderTargets.size() ) );
            texture->setUsageFlags( static_cast<u32>( TextureUsage::TU_RENDERTARGET ) );
            texture->setRenderTarget( workphone::make_ptr<ClawRenderTarget>() );
            texture->load( nullptr );

            m_textures.push_back( texture );
            m_renderTargets.push_back( texture );
            return texture;
        }

        void ClawTextureManager::destroyRenderTexture( SmartPtr<ITexture> texture )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( !texture )
            {
                return;
            }

            texture->unload( nullptr );
            m_renderTargets.erase(
                std::remove( m_renderTargets.begin(), m_renderTargets.end(), texture ),
                m_renderTargets.end() );
            m_textures.erase( std::remove( m_textures.begin(), m_textures.end(), texture ),
                              m_textures.end() );
        }

        u32 ClawTextureManager::releaseUnusedResources( u32 maximumCandidates )
        {
            if( !maximumCandidates )
            {
                return 0;
            }
            ScopedLock<TextureManager> lifecycle( this );
            Array<SmartPtr<ITexture>> candidates;
            {
                // Copy only the bounded window. Do not retain the entire city cache,
                // or hold its array lock while destroying native graphics resources.
                ScopedLock<decltype( m_textures )> texturesLock( &m_textures );
                const auto size = m_textures.size();
                if( !size )
                {
                    m_evictionCursor = 0;
                    return 0;
                }
                const auto count = std::min<size_t>( maximumCandidates, size );
                candidates.reserve( count );
                for( size_t i = 0; i < count; ++i )
                {
                    m_evictionCursor %= size;
                    candidates.push_back( m_textures[m_evictionCursor++] );
                }
            }
            u32 released = 0;
            for( const auto &texture : candidates )
            {
                // One manager owner and one candidate owner are expected. Weak
                // clients may acquire a strong reference without the manager lock.
                if( !texture || texture->getReferences() != 2 || texture->getWeakReferences() != 0 ||
                    texture->isLoadLocked() )
                {
                    continue;
                }
                auto clawTexture = dynamic_cast<ClawTexture *>( texture.get() );
                if( clawTexture && clawTexture->releaseUnusedCacheData() )
                {
                    ++released;
                }
            }
            return released;
        }

        void ClawTextureManager::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = const_cast<ClawTextureManager *>( this );
            }
        }
    }  // namespace render
}  // namespace workphone
