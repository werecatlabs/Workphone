#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ProjectIrradiance.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <mutex>
#include <cmath>
#include <algorithm>

namespace workphone
{
    namespace
    {
        struct Surface { ColourF albedo=ColourF(.6f,.6f,.6f,1), emission=ColourF::Black; };
        class LocalIrradiance final : public IProjectIrradianceProvider
        {
            struct Context
            {
                SmartPtr<physics::IPhysicsScene3> physics;
                SmartPtr<physics::RaycastHit> hit;
                std::map<physics::IRigidBody3 *,Surface> surfaces;
                ColourF upper=ColourF::Black, lower=ColourF::Black;
                bool staticOnly=false;
            };
            static wp_s32 trace( void *opaque, const float origin[3], const float direction[3],
                                 float distance, wp_irradiance_hit *output )
            {
                auto &context=*static_cast<Context *>(opaque);
                context.hit->setCheckStatic(true); context.hit->setCheckDynamic(!context.staticOnly);
                if(!context.physics->castRay(Ray3<real_Num>({origin[0],origin[1],origin[2]},
                    {direction[0],direction[1],direction[2]}),context.hit))return 0;
                const auto actual=context.hit->getDistance(); if(actual<0.0001f || actual>distance)return 0;
                const auto p=context.hit->getPoint(),n=context.hit->getNormal();
                output->distance=static_cast<float>(actual);
                output->position[0]=static_cast<float>(p.X());output->position[1]=static_cast<float>(p.Y());output->position[2]=static_cast<float>(p.Z());
                output->normal[0]=static_cast<float>(n.X());output->normal[1]=static_cast<float>(n.Y());output->normal[2]=static_cast<float>(n.Z());
                Surface surface;const auto found=context.surfaces.find(context.hit->getRigidBody().get());
                if(found!=context.surfaces.end())surface=found->second;
                output->albedo[0]=std::clamp(surface.albedo.r,0.f,1.f);output->albedo[1]=std::clamp(surface.albedo.g,0.f,1.f);output->albedo[2]=std::clamp(surface.albedo.b,0.f,1.f);
                output->emission[0]=std::max(surface.emission.r,0.f);output->emission[1]=std::max(surface.emission.g,0.f);output->emission[2]=std::max(surface.emission.b,0.f);
                return 1;
            }
            static void environment( void *opaque, const float direction[3], float radiance[3] )
            {
                const auto &context=*static_cast<Context *>(opaque);const auto t=std::clamp(direction[1]*.5f+.5f,0.f,1.f);
                radiance[0]=context.lower.r+(context.upper.r-context.lower.r)*t;
                radiance[1]=context.lower.g+(context.upper.g-context.lower.g)*t;
                radiance[2]=context.lower.b+(context.upper.b-context.lower.b)*t;
            }
        public:
            bool sample( const float position[3], u32 rays, s32 selection, bool sky,
                         wp_irradiance_probe &probe, String &error ) override
            {
                auto app=core::IApplicationManager::instancePtr();auto manager=app?app->getPhysicsManager():nullptr;
                Context context;context.physics=manager?manager->getRaycastScene():nullptr;
                if(!context.physics && manager)context.physics=manager->getPhysicsScene();
                if(!app || !context.physics){error="Local GI requires a collision/raycast scene or an injected irradiance provider";return false;}
                context.hit=make_ptr<physics::RaycastHit>();context.staticOnly=selection==1;
                auto graphics=app->getGraphicsSystem();auto graphicsScene=graphics?graphics->getGraphicsScene():nullptr;
                if(graphicsScene){context.upper=graphicsScene->getUpperHemisphere();context.lower=graphicsScene->getLowerHemisphere();}
                Array<wp_irradiance_light> lights;
                if(auto game=app->getGameManager())for(auto actor:game->getActors())
                {
                    Surface surface;
                    const auto renderers=actor->getComponentsByType<scene::Renderer>();
                    if(!renderers.empty())if(auto material=renderers.front()->getSharedMaterial())
                    {surface.albedo=material->getDiffuse();surface.emission=material->getEmissive();}
                    for(auto body:actor->getComponentsByType<scene::Rigidbody>())
                    {if(auto dynamic=body->getRigidDynamic())context.surfaces[dynamic.get()]=surface;
                     if(auto fixed=body->getRigidStatic())context.surfaces[fixed.get()]=surface;}
                    for(auto component:actor->getComponentsByType<scene::Light>())
                    {
                        if(!component->isEnabled() || lights.size()>=8)continue;
                        auto light=component->getLight();if(!light)continue;
                        const auto p=light->getPosition(),d=light->getDerivedDirection();const auto color=component->getDiffuseColour();
                        wp_irradiance_light output{};output.directional=component->getLightType()==LightTypes::LT_DIRECTIONAL;
                        output.dynamic=!actor->isStatic();output.range=light->getAttenuationRange();
                        output.position[0]=static_cast<float>(p.X());output.position[1]=static_cast<float>(p.Y());output.position[2]=static_cast<float>(p.Z());
                        output.direction[0]=static_cast<float>(d.X());output.direction[1]=static_cast<float>(d.Y());output.direction[2]=static_cast<float>(d.Z());
                        output.radiance[0]=color.r*component->getIntensity();output.radiance[1]=color.g*component->getIntensity();output.radiance[2]=color.b*component->getIntensity();lights.push_back(output);
                    }
                }
                wp_irradiance_scene source{};source.context=&context;source.trace=trace;source.environment=environment;
                source.lights=lights.data();source.light_count=static_cast<wp_u32>(lights.size());
                ScopedLock physicsLock(context.physics);
                if(!wp_irradiance_bake_probe(&source,position,rays,64,selection,sky?1:0,&probe))
                {error="Local irradiance ray integration failed";return false;}
                error.clear();return true;
            }
        };
        std::mutex providerMutex;
        std::shared_ptr<IProjectIrradianceProvider> provider;
        Array<wp_irradiance_probe> baked, live, published;
        s32 activeMode=0;
        size_t nextProbe=0;
        String activeRoot;
        String lastRefreshError;

        Array<wp_irradiance_probe> initialLocations()
        {
            Vector3F minimum(-4,0,-4),maximum(4,0,4);bool have=false;
            auto app=core::IApplicationManager::instancePtr();auto game=app?app->getGameManager():nullptr;
            if(game)for(auto actor:game->getActors())if(auto transform=actor->getTransform())
            {
                const auto p=transform->getWorldTransform().getPosition();
                Vector3F point(static_cast<float>(p.X()),static_cast<float>(p.Y()),static_cast<float>(p.Z()));
                if(!std::isfinite(point.X()) || !std::isfinite(point.Y()) || !std::isfinite(point.Z()))continue;
                if(!have){minimum=maximum=point;have=true;}
                else {minimum=Vector3F(std::min(minimum.X(),point.X()),std::min(minimum.Y(),point.Y()),std::min(minimum.Z(),point.Z()));
                      maximum=Vector3F(std::max(maximum.X(),point.X()),std::max(maximum.Y(),point.Y()),std::max(maximum.Z(),point.Z()));}
            }
            const auto center=(minimum+maximum)*.5f;
            const auto x=std::max((maximum.X()-minimum.X())*.25f,2.f),z=std::max((maximum.Z()-minimum.Z())*.25f,2.f);
            Array<wp_irradiance_probe> output(4);
            for(size_t i=0;i<4;++i){output[i]={};output[i].position[0]=center.X()+(i&1?x:-x);output[i].position[1]=center.Y()+1;
                output[i].position[2]=center.Z()+(i&2?z:-z);}
            return output;
        }
        void saveBake( const std::filesystem::path &path, const Array<wp_irradiance_probe> &probes )
        {
            auto data=make_ptr<Properties>();data->setProperty("irradianceVersion",s32(1));
            for(const auto &probe:probes)
            {
                auto item=make_ptr<Properties>();item->setName("Probe");
                item->setProperty("position",Vector3F(probe.position[0],probe.position[1],probe.position[2]));
                for(u32 i=0;i<9;++i)for(u32 c=0;c<3;++c)item->setProperty("sh"+StringUtil::toString(i)+"_"+StringUtil::toString(c),probe.sh[i][c]);
                data->addChild(item);
            }
            std::filesystem::create_directories(path.parent_path());std::ofstream stream(path,std::ios::binary|std::ios::trunc);
            stream.exceptions(std::ios::badbit|std::ios::failbit);stream<<DataUtil::toString(data.get(),true);stream.close();
        }
        bool loadBake( const std::filesystem::path &path, Array<wp_irradiance_probe> &probes )
        {
            if(!std::filesystem::is_regular_file(path) || std::filesystem::file_size(path)>65536)return false;
            std::ifstream stream(path,std::ios::binary);std::ostringstream text;text<<stream.rdbuf();auto data=make_ptr<Properties>();DataUtil::parse(text.str().c_str(),data.get());
            const auto children=data->getChildren();if(data->getPropertyAsInt("irradianceVersion")!=1 || children.empty() || children.size()>4)return false;
            Array<wp_irradiance_probe> result;
            for(auto item:children)
            {
                wp_irradiance_probe probe{};Vector3F p;if(!item->getPropertyValue("position",p))return false;
                probe.position[0]=p.X();probe.position[1]=p.Y();probe.position[2]=p.Z();
                if(!std::isfinite(probe.position[0]) || !std::isfinite(probe.position[1]) || !std::isfinite(probe.position[2]))return false;
                for(u32 i=0;i<9;++i)for(u32 c=0;c<3;++c)
                    if(!item->getPropertyValue("sh"+StringUtil::toString(i)+"_"+StringUtil::toString(c),probe.sh[i][c]) || !std::isfinite(probe.sh[i][c]))return false;
                result.push_back(probe);
            }
            probes=std::move(result);return true;
        }
    }
    void setProjectIrradianceProvider( std::shared_ptr<IProjectIrradianceProvider> value )
    { std::lock_guard<std::mutex> lock(providerMutex);provider=std::move(value); }
    bool applyProjectIrradianceSettings( SmartPtr<Properties> settings, const String &root, String &error )
    {
        try
        {
            const auto mode=settings?settings->getPropertyAsInt("graphics.gi",0):0;
            if(mode<0 || mode>3){error="Invalid GI mode";return false;}
            std::lock_guard<std::mutex> lock(providerMutex);
            if(mode==0){activeMode=0;baked.clear();live.clear();published.clear();error.clear();return true;}
            if(root.empty()){error="GI requires a project directory";return false;}
            if(!provider)provider=std::make_shared<LocalIrradiance>();
            auto locations=initialLocations();auto staticData=locations;
            String sceneKey=settings->getProperty("project.startupScene","scene");
            auto app=core::IApplicationManager::instancePtr();auto game=app?app->getGameManager():nullptr;
            if(game)if(auto scene=game->getCurrentScene())if(!scene->getFilePath().empty())sceneKey=scene->getFilePath();
            auto scenePath=std::filesystem::u8path(sceneKey.c_str());
            if(scenePath.is_absolute())
            {
                const auto relative=scenePath.lexically_normal().lexically_relative(std::filesystem::absolute(std::filesystem::u8path(root.c_str())).lexically_normal());
                if(!relative.empty() && *relative.begin()!="..")sceneKey=relative.generic_u8string().c_str();
            }
            const auto name=String(mode==3?"Static-":"Baked-")+StringUtil::toString(StringUtil::getHash(sceneKey))+".settings";
            const auto path=std::filesystem::u8path(root.c_str())/".workphone/lighting"/name.c_str();
            if(mode==1 || mode==3)
            {
                if(!loadBake(path,staticData))
                {
                    for(auto &probe:staticData)if(!provider->sample(probe.position,256,mode==3?1:0,true,probe,error))return false;
                    saveBake(path,staticData);
                }
                locations=staticData;
            }
            for(auto &probe:locations)for(auto &coefficient:probe.sh)for(auto &value:coefficient)value=0;
            auto visible=mode==1 || mode==3?staticData:locations;
            if(mode==2 || mode==3)
            {
                auto initial=locations[0];
                if(!provider->sample(initial.position,32,mode==3?2:0,mode==2,initial,error))return false;
                locations[0]=initial;
                if(mode==3)wp_irradiance_combine(&staticData[0],&initial,&visible[0]);else visible[0]=initial;
            }
            baked=std::move(staticData);live=std::move(locations);published=std::move(visible);
            activeMode=mode;activeRoot=root;nextProbe=0;error.clear();return true;
        }
        catch(const std::exception &e){error=e.what();return false;}
    }
    void updateProjectIrradiance()
    {
        std::lock_guard<std::mutex> lock(providerMutex);
        if((activeMode!=2 && activeMode!=3) || live.empty() || !provider)return;
        auto probe=live[nextProbe];String error;bool sampled=false;
        try{sampled=provider->sample(probe.position,32,activeMode==3?2:0,activeMode==2,probe,error);}
        catch(const std::exception &e){error=e.what();}
        if(sampled)
        {
            lastRefreshError.clear();
            live[nextProbe]=probe;
            if(activeMode==3)wp_irradiance_combine(&baked[nextProbe],&probe,&published[nextProbe]);else published[nextProbe]=probe;
        }
        else
        {
            if(error.empty())error="Irradiance provider could not refresh a local probe";
            if(error!=lastRefreshError){WP_LOG_ERROR("GI probe refresh failed: "+error);lastRefreshError=error;}
        }
        nextProbe=(nextProbe+1)%live.size();
    }
    Array<wp_irradiance_probe> getProjectIrradianceProbes()
    {std::lock_guard<std::mutex> lock(providerMutex);return published;}
    s32 getProjectIrradianceMode(){std::lock_guard<std::mutex> lock(providerMutex);return activeMode;}
    void releaseProjectIrradiance()
    {std::lock_guard<std::mutex> lock(providerMutex);activeMode=0;provider.reset();baked.clear();live.clear();published.clear();activeRoot.clear();lastRefreshError.clear();}
}
