#ifndef __OgreGfxPrerequisites__H
#define __OgreGfxPrerequisites__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <WPGraphicsOgre/BuildSettings/win32/OgreBuildSettings.h>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Memory/WeakPtr.hpp>

class DeferredShadingSystem;
class TiXmlElement;
class TiXmlElement;
class AssimpLoader;

namespace Forests
{
    class PagedGeometry;
}

namespace Forests
{
    class GeometryPageManager;
    class PageLoader;
    class PagedGeometry;
    class GrassLoader;
}  // namespace Forests

class Plot;

// forward declarations
namespace Ogre
{
    class AnimationState;
    class Billboard;
    class BillboardSet;
    class Camera;
    class D3D9Plugin;
    class D3D11Plugin;
    class Entity;
    class FrameListener;
    class Frustum;
    class GLPlugin;
    class GL3PlusPlugin;
    class GLES2Plugin;
    class Light;
    class InstancedEntity;
    class InstanceManager;
    class MovableObject;
    class MetalPlugin;
    class Overlay;
    class OverlayElement;
    class OverlayContainer;
    class OverlayManager;
    class OverlaySystem;
    class ParticleSystem;
    class ParticleSystemManager;
    class Pass;
    class PageManager;
    class Profiler;
    class PsoCacheHelper;
    class PSSMShadowCameraSetup;
    class Root;
    class RenderTarget;
    class RenderWindow;
    class ResourceGroupManager;
    class ResourceManager;
    class RenderSystem;
    class ScreenSpaceEffect;
    class SceneManager;
    class SceneNode;
    class StaticPluginLoader;
    class ScriptLoader;
    class Terrain;
    class TerrainGlobalOptions;
    class TerrainGroup;
    class TerrainPaging;
    class TerrainLayerBlendMap;
    class TextureUnitState;
    class TextureUnitState;
    class TextAreaOverlayElement;
    class TextAreaOverlayElement;
    class Viewport;
    class Technique;

}  // namespace Ogre

namespace OgreBites
{

    class SGTechniqueResolverListener;

}

namespace Ogitors
{
    class OgitorsRoot;
    class CBaseSerializer;
    class CViewportEditor;
    class OgitorsPropertySetListener;
}  // namespace Ogitors

namespace workphone
{
    class ResourceGroupHelper;
    class ImGuiManagerOgre;

    namespace render
    {
        class DynamicLinesOgre;

        class LightDepthMapRttListener;

        class SSEffectRenderer;

        class CInstanceObjectOld;

        class DecalCursor;
        class DecalCursorOgre;

        class CSceneNodeOgre;
        class CompositorManager;
        class ScreenSpaceEffect;
        class SSEffectRenderer;
        class sseffectManager;

        class LodManager;
        class LodObject;
        class LodPage;

        class CMaterialTechniqueOgre;
        class CInstanceManager;
        class CInstancedObject;

        class CDeferredShadingSystem;

        class CCameraOgre;

        class CameraVisibilityState;
        class DynamicLines;

        class DepthOfFieldEffect;
        class HDRListener;

        class CDebugOgre;
        class CDebugLineOgreNext;

        class CTextureOgre;
        class CTerrainOgre;

        class WaterMesh;

        class ResourceLoadingListener;
        class MaterialListener;
        class CellSceneManagerFactory;
        class BasicSceneManagerFactory;

        class WindowWin32;
        class WindowMacOS;

        class Texture3D;
        class ParticleTextureAtlas;

    }  // end namespace render
}  // namespace workphone

#endif
