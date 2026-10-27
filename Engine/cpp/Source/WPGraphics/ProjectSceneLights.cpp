#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ProjectSceneLights.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <WorkphoneGraphics/workphone_graphics_light_grid.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace workphone::render
{
    bool uploadProjectSceneLights( void *nativeRenderer, SmartPtr<IGraphicsScene> scene,
        SmartPtr<IGraphicsCamera> camera, SmartPtr<Properties> settings )
    {
#if !defined( WP_PLATFORM_WIN32 ) || !WP_BUILD_RENDERER_DX11
        (void)nativeRenderer;(void)scene;(void)camera;(void)settings;return false;
#else
        if(!nativeRenderer || !camera || !scene)return false;
        std::array<wp_local_light,WP_LIGHT_GRID_MAX_LIGHTS> lights{};u32 count=0;
        for(auto light:scene->getObjectByType<IGraphicsLight>())
        {
            if(!light || !light->isVisible() || light->getType()!=LightTypes::LT_POINT || count==lights.size())continue;
            const auto position=light->getPosition();const auto color=light->getDiffuseColour();
            const auto radius=light->getAttenuationRange(),power=light->getPowerScale();
            if(!std::isfinite(radius) || radius<=0 || !std::isfinite(power) || power<0)continue;
            wp_local_light value{};bool finite=true;
            for(u32 axis=0;axis<3;++axis){value.position[axis]=static_cast<float>(position[axis]);finite=finite && std::isfinite(value.position[axis]);}
            value.radius=radius;value.radiance[0]=std::max(0.f,color.r*power);value.radiance[1]=std::max(0.f,color.g*power);value.radiance[2]=std::max(0.f,color.b*power);
            if(!finite || !std::isfinite(value.radiance[0]) || !std::isfinite(value.radiance[1]) || !std::isfinite(value.radiance[2]))continue;
            lights[count++]=value;
        }
        wp_light_grid_camera view{};
        const Matrix4F viewMatrix(camera->getViewMatrix().ptr()),projection(camera->getProjectionMatrix().ptr());
        const auto inverseView=viewMatrix.inverse(),viewProjection=projection*viewMatrix;
        for(u32 row=0;row<4;++row)for(u32 col=0;col<4;++col)view.view_projection[row*4+col]=viewProjection[row][col];
        auto forward=camera->getDirection();
        if(forward.lengthSquared()<.000001)forward=Vector3<real_Num>(-inverseView[0][2],-inverseView[1][2],-inverseView[2][2]);
        forward.normalise();
        for(u32 axis=0;axis<3;++axis){view.position[axis]=inverseView[axis][3];view.forward[axis]=static_cast<float>(forward[axis]);}
        view.z_near=std::max(.0001f,camera->getNearClipDistance());view.z_far=camera->getFarClipDistance();
        const bool clustered=settings && settings->getPropertyAsInt("graphics.lightingMode",1)==3;
        const bool tiled=clustered || (settings && settings->getPropertyAsInt("graphics.renderPipeline",1)==1);
        const u32 gx=tiled?16:1,gy=tiled?9:1,gz=clustered?16:1,cells=gx*gy*gz;
        std::vector<u32> ranges(cells*2),indices(cells*WP_LIGHT_GRID_CELL_LIGHTS);u32 indexCount=0,discarded=0;
        if(!wp_light_grid_build(lights.data(),count,&view,gx,gy,gz,ranges.data(),cells,indices.data(),static_cast<u32>(indices.size()),&indexCount,&discarded))return false;
        // The finite, documented cell budget is exposed by the C builder for
        // diagnostic clients; shader list traversal never exceeds 32 entries.
        return wp_renderer_dx11_set_local_lights(static_cast<wp_renderer_dx11 *>(nativeRenderer),
            reinterpret_cast<const wp_f32 *>(lights.data()),count,ranges.data(),indices.data(),indexCount,
            gx,gy,gz,view.z_near,view.z_far,view.forward)!=0;
#endif
    }
}
