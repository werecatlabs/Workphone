#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/VideoBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    using namespace render;

    IVideo *_addVideo( IVideoManager *mgr, const char *fileName )
    {
        auto video = mgr->addVideo( fileName );
        return video.get();
    }

    IVideo *_addVideoWithName( IVideoManager *mgr, const char *id, const char *fileName )
    {
        auto hash = static_cast<hash32>( StringUtil::getHash( id ) );
        auto video = mgr->addVideo( hash, fileName );
        return video.get();
    }

    IVideo *_addVideoAndTexture( IVideoManager *mgr, const char *id, const char *fileName,
                                 const char *textureName )
    {
        try
        {
            auto hash = static_cast<hash32>( StringUtil::getHash( id ) );
            auto video = mgr->addVideo( hash, fileName );
            if( video )
            {
                auto videoTexture = mgr->createVideoTexture( textureName );
                videoTexture->initialise( textureName, video->getSize() );
                video->setVideoTexture( videoTexture );
                return video.get();
            }
        }
        catch( Exception &e )
        {
            WP_LOG( e.what() );
        }
        catch( std::exception &e )
        {
            WP_LOG( e.what() );
        }
        catch( ... )
        {
            WP_LOG( "Unknown error." );
        }

        return nullptr;
    }

    SmartPtr<IVideo> _addVideoAndTextureSize( IVideoManager *mgr, const char *id, const char *fileName,
                                              const char *textureName, const Vector2I &size )
    {
        try
        {
            auto hash = static_cast<hash32>( StringUtil::getHash( id ) );
            auto video = mgr->addVideo( hash, fileName );
            if( video )
            {
                auto videoTexture = mgr->createVideoTexture( textureName );
                if( videoTexture )
                {
                    videoTexture->initialise( textureName, size );
                    video->setVideoTexture( videoTexture );
                }
            }

            return video;
        }
        catch( Exception &e )
        {
            WP_LOG( e.what() );
        }
        catch( std::exception &e )
        {
            WP_LOG( e.what() );
        }
        catch( ... )
        {
            WP_LOG( "Unknown error." );
        }

        return nullptr;
    }

    IVideo *_addVideoWithId( IVideoManager *mgr, hash32 hash, const char *fileName )
    {
        auto video = mgr->addVideo( hash, fileName );
        return video.get();
    }

    IVideo *_getVideoById( IVideoManager *mgr, lua_Integer id )
    {
        return mgr->getVideoById( static_cast<hash32>( id ) ).get();
    }

    IVideo *_getVideoByName( IVideoManager *mgr, const char *name )
    {
        auto id = StringUtil::getHash( name );
        return _getVideoById( mgr, id );
    }

    void _createTexture( IVideo *video, const char *textureName )
    {
        auto applicationManager = core::ApplicationManager::instance();
        auto videoMgr = applicationManager->getVideoManager();
        auto videoTexture = videoMgr->createVideoTexture( textureName );
        videoTexture->initialise( textureName, video->getSize() );
        video->setVideoTexture( videoTexture );
    }

    void bindVideo( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IVideo, ISharedObject, SmartPtr<IVideo>>( "Video" )
                        .def( "play", &IVideo::play )
                        .def( "stop", &IVideo::stop )
                        .def( "setLoop", &IVideo::setLoop )
                        .def( "getLoop", &IVideo::getLoop )
                        .def( "getSize", &IVideo::getSize )
                        .def( "setSize", &IVideo::setSize )];

        module( L )[class_<IVideoManager, ISharedObject, SmartPtr<IVideoManager>>( "VideoManager" )
                        .def( "addVideo", _addVideo )
                        .def( "addVideo", _addVideoWithName )
                        .def( "addVideo", _addVideoAndTexture )
                        .def( "addVideo", _addVideoAndTextureSize )
                        .def( "addVideo", _addVideoWithId )
                        .def( "getVideoById", _getVideoById )
                        .def( "getVideoByName", _getVideoByName )];
    }
} // namespace workphone
