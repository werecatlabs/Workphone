#ifndef __OgreNextGfxPrerequisites__H
#define __OgreNextGfxPrerequisites__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <OgreBuildSettings.h>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextConfig.hpp>

// forward decs
class DeferredShadingSystem;
class DecalCursor;
class Plot;
class WaterMesh;

namespace Colibri
{
    class CustomShape;
    class ColibriManager;
    class Widget;
    class Window;
    class Label;
}  // namespace Colibri

namespace Forests
{
    class GeometryPageManager;
    class PageLoader;
    class PagedGeometry;
    class GrassLoader;
}  // namespace Forests

// forward declaration
namespace Ogre
{
    class HlmsColibriDatablock;
    class HlmsPbsDatablock;

    class AnimationState;
    class Billboard;
    class BillboardSet;
    class Camera;
    class CompositorManager2;
    class CompositorWorkspace;
    class CompositorPassDef;
    class CompositorPassSceneDef;
    class CompositorTargetDef;
    class CompositorWorkspaceDef;
    class D3D11Plugin;
    class Entity;
    class FrameListener;
    class Frustum;
    class GL3PlusPlugin;
    class GLES2Plugin;
    class Light;
    class ManualObject;
    class MovableObject;

    class Pass;
    class Particle;
    class ParticleSystem;
    class Profiler;
    class PsoCacheHelper;
    class Rectangle2D;
    class RenderTarget;
    class RenderWindow;
    class ResourceGroupManager;
    class ResourceManager;
    class SceneManager;
    class SceneNode;
    class TextureUnitState;
    class TextureGpu;
    class TextAreaOverlayElement;
    class Viewport;
    class Technique;

    class Terra;
    class ShadowMapper;
    class TerraWorkspaceListener;

    class Terrain;
    class TerrainGlobalOptions;
    class TerrainGroup;
    class TerrainLayerBlendMap;
    class TerrainPaging;
    class PageManager;

    class InstancedEntity;
    class InstanceManager;

    class RenderSystem;
    class TextureUnitState;
    class TextureGpu;

    class StaticPluginLoader;

    class HlmsDatablock;
    class HlmsUnlitDatablock;
    class HlmsPbsTerraShadows;

    class Root;

    class Entity;
    class ParticleSystem;
    class RenderSystem;
    class OverlaySystem;

    class MetalPlugin;

    class HlmsJsonListener;

    class CompositorWorkspace;
    class Entity;
    class Pass;
    class ParticleSystem;
    class ParticleSystemManager;
    class Mesh;
    class IndexBufferPacked;
    class Item;
    class ScriptLoader;
    class Window;

    struct GpuTrackedResource;

    class GL3PlusPlugin;
    class MetalPlugin;

    class ObjectMemoryManager;

}  // namespace Ogre

namespace Ogre
{
    namespace v1
    {
        class AnimationState;
        class Billboard;
        class BillboardSet;
        class Entity;
        class TextAreaOverlayElement;
        class Overlay;
        class OverlayElement;
        class OverlayContainer;
        class OverlayManager;
        class OverlaySystem;
        class WireBoundingBox;
    }  // namespace v1
}  // namespace Ogre

class TiXmlElement;
class AssimpLoader;

namespace workphone
{
    namespace render
    {
        class CompositorPassProvider;

        class CBillboardOgreNext;
        class CBillboardSetOgreNext;

        class DynamicMeshOgreNext;

        class WaterMesh;
        class HDRListener;

        class CCameraOgreNext;
        class Compositor;
        class CompositorManager;
        class CDeferredShadingSystem;
        class CSceneNodeOgreNext;
        class CGraphicsSceneOgreNext;
        class CViewportOgreNext;
        class CMaterialTechniqueOgreNext;

        class CellSceneManagerFactory;

        class DynamicLinesOgreNext;
        class DepthOfFieldEffect;
        class ResourceLoadingListener;
        class MaterialListener;

        class ScreenSpaceEffect;
        class SSEffectRenderer;
        class sseffectManager;

        class LodManager;
        class LodObject;
        class LodPage;

        class DeferredShadingSystemTemplate;

        class SSEffectRenderer;
        class Quad;
        class WindowMacOS;

        class SkyRenderer;
        class SkyBoxRenderer;

        class UIRenderer;

        class WindowWin32;
        class WindowWin32Alt;
        class WindowMacOS;

    }  // end namespace render

    namespace ui
    {
        class UIManagerCore;
        class UIButtonCore;
        class UIImageCore;
        class UILayoutCore;
        class UISliderCore;
        class UITextCore;
        class UIToggleCore;
    }  // namespace ui
}  // namespace workphone

#endif
