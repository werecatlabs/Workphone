/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

/* Stable headers which will be used for precompilation if the compiler
   supports it. Add entries here when headers are unlikely to change.
   NB: a change to any of these headers will result in a full rebuild,
   so don't add things to this lightly.
*/

#ifndef __OgreStableHeaders__
#define __OgreStableHeaders__

extern "C" {
#   include <sys/types.h>
#   include <sys/stat.h>
}

#include "OgreConfig.hpp"
#include "OgreExports.hpp"
#include "OgrePrerequisites.hpp"
#include "OgrePlatform.hpp"
#include "OgreStdHeaders.hpp"
#include <iomanip>

#include "OgreAny.hpp"
#include "OgreArchive.hpp"
#include "OgreArchiveManager.hpp"
#include "OgreAxisAlignedBox.hpp"
#include "OgreBitwise.hpp"
#include "OgreBone.hpp"
#include "OgreCamera.hpp"
#include "OgreCodec.hpp"
#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreDataStream.hpp"
#include "OgreDefaultWorkQueue.hpp"
#include "OgreException.hpp"
#include "OgreFileSystem.hpp"
#include "OgreFrustum.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgreManualObject.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgreMath.hpp"
#include "OgreMatrix3.hpp"
#include "OgreMatrix4.hpp"
#include "OgreMesh.hpp"
#include "OgreMeshManager.hpp"
#include "OgreMeshSerializer.hpp"
#include "OgreMovableObject.hpp"
#include "OgreNode.hpp"
#include "OgreParticleSystemManager.hpp"
#include "OgrePass.hpp"
#include "OgrePlane.hpp"
#include "OgrePlatformInformation.hpp"
#include "OgreProfiler.hpp"
#include "OgreQuaternion.hpp"
#include "OgreRadixSort.hpp"
#include "OgreRay.hpp"
#include "OgreRectangle2D.hpp"
#include "OgreBuiltinMovableFactories.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreResource.hpp"
#include "OgreRoot.hpp"
#include "OgreShadowTextureManager.hpp"
#include "OgreSceneManager.hpp"
#include "OgreSceneNode.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreSerializer.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSimpleRenderable.hpp"
#include "OgreSimpleSpline.hpp"
#include "OgreSingleton.hpp"
#include "OgreSkeleton.hpp"
#include "OgreSphere.hpp"
#include "OgreStringConverter.hpp"
#include "OgreString.hpp"
#include "OgreStringInterface.hpp"
#include "OgreStringVector.hpp"
#include "OgreSubMesh.hpp"
#include "OgreTechnique.hpp"
#include "OgreTextureManager.hpp"
#include "Threading/OgreThreadHeaders.hpp"
#include "OgreUserObjectBindings.hpp"
#include "OgreVector.hpp"
#if OGRE_NO_ZIP_ARCHIVE == 0
#   include "OgreZip.h"
#endif

#define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#define OGRE_IGNORE_DEPRECATED_BEGIN __pragma(warning(push)) \
    __pragma(warning(disable:4996))
#define OGRE_IGNORE_DEPRECATED_END __pragma(warning(pop))
#else
#define OGRE_IGNORE_DEPRECATED_BEGIN _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define OGRE_IGNORE_DEPRECATED_END _Pragma("GCC diagnostic pop")
#endif

#endif 
