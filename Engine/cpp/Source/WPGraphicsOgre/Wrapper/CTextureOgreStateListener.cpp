#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgreStateListener.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgre.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, CTextureOgreStateListener, IStateListener );

        CTextureOgreStateListener::CTextureOgreStateListener() = default;

        CTextureOgreStateListener::~CTextureOgreStateListener() = default;

        void CTextureOgreStateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        bool CTextureOgreStateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isDerived<StateMessageVector2I>() )
                {
                    auto vector2Message =
                        workphone::static_pointer_cast<StateMessageVector2I>( message );
                    auto value = vector2Message->getValue();
                    auto type = vector2Message->getType();

                    if( type == ITexture::STATE_MESSAGE_TEXTURE_SIZE )
                    {
                        owner->setSize( value );
                    }
                }
            }

            return false;
        }

        bool CTextureOgreStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                if( owner->isLoaded() )
                {
                    auto textureState =
                        workphone::static_pointer_cast<TextureStateData>( state->getData() );
                    auto size = textureState->size;

                    auto texture = owner->getTexture();

                    if( texture )
                    {
                        auto currentSize = Vector2I( texture->getWidth(), texture->getHeight() );

                        if( currentSize != size )
                        {
                            return false;  // todo: handle this case

                            texture->setWidth( size.x );
                            texture->setHeight( size.y );

                            auto usageFlags = owner->getUsageFlags();

                            if( ( usageFlags & (u32)TextureUsage::TU_RENDERTARGET ) != 0 )
                            {
#if defined WP_PLATFORM_WIN32
                                owner->load( nullptr );
#elif defined WP_PLATFORM_APPLE
                                owner->load( nullptr );
#endif
                            }
                        }
                    }

                    state->setDirty( false );
                }
            }

            return false;
        }

        SmartPtr<CTextureOgre> CTextureOgreStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void CTextureOgreStateListener::setOwner( SmartPtr<CTextureOgre> owner )
        {
            m_owner = owner;
        }

    }  // namespace render
}  // namespace workphone
