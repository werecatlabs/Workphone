#ifndef __WPOgreGfxAutolink__H
#define __WPOgreGfxAutolink__H

#include <Workphone/WorkphoneConfig.hpp>

#if WP_USE_AUTO_LINK
#    ifdef _DEBUG
#        pragma comment( lib, "WPGraphicsOgre_d.lib" )
#    elif NDEBUG
#        pragma comment( lib, "WPGraphicsOgre.lib" )
#    else
#        pragma comment( lib, "WPGraphicsOgre.lib" )
#    endif

#    ifdef _DEBUG
#        pragma comment( lib, "OgreMain_d.lib" )
#        pragma comment( lib, "OgreOverlay_d.lib" )
#        pragma comment( lib, "Plugin_ParticleFX_d.lib" )
#        pragma comment( lib, "ParticleUniverse_d.lib" )
#        pragma comment( lib, "OgreTerrain_d.lib" )
#        pragma comment( lib, "OgrePaging_d.lib" )
#        pragma comment( lib, "MeshSplitter_d.lib" )
#        pragma comment( lib, "assimp_d.lib" )
//#pragma comment(lib, "PagedGeometry.lib")
#    elif NDEBUG
#        pragma comment( lib, "OgreMain.lib" )
#        pragma comment( lib, "OgreOverlay.lib" )
#        pragma comment( lib, "Plugin_ParticleFX.lib" )
#        pragma comment( lib, "ParticleUniverse.lib" )
#        pragma comment( lib, "OgreTerrain.lib" )
#        pragma comment( lib, "OgrePaging.lib" )
#        pragma comment( lib, "MeshSplitter.lib" )
#        pragma comment( lib, "assimp.lib" )
#        pragma comment( lib, "PagedGeometry.lib" )
#    else
#        pragma comment( lib, "OgreMain.lib" )
#        pragma comment( lib, "OgreOverlay.lib" )
#        pragma comment( lib, "Plugin_ParticleFX.lib" )
#        pragma comment( lib, "ParticleUniverse.lib" )
#        pragma comment( lib, "OgreTerrain.lib" )
#        pragma comment( lib, "OgrePaging.lib" )
#        pragma comment( lib, "MeshSplitter.lib" )
#        pragma comment( lib, "assimp.lib" )
#        pragma comment( lib, "PagedGeometry.lib" )
#    endif
#endif

#endif
