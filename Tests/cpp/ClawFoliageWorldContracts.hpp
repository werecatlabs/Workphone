#pragma once
#include "ClawFoliageContracts.hpp"
#include "ClawFoliageSceneContracts.hpp"
#include <WPGraphics/ClawFoliageWorld.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/WPClawHammer.hpp>
#include <Workphone/Graphics/FoliageImpostorBaker.hpp>
#include <filesystem>
#include <fstream>
#include <thread>

namespace claw_foliage_world_contracts
{
    using namespace claw_foliage_contracts;
    struct NativeShadowScope
    {
        wp_renderer_dx11 *renderer;
        ~NativeShadowScope(){wp_renderer_dx11_end_shadow_map(renderer);}
    };
    inline std::shared_ptr<FoliageSpecies> species()
    {
        auto s=std::make_shared<FoliageSpecies>();s->id="paged-contract-species";
        s->boundsMinimum={-.02f,0,0};s->boundsMaximum={.02f,.04f,0};
        s->maximumDistance=10;s->shadowDistance=10;s->shadowLodBias=0;
        s->materials.resize(2);s->materials[0].tint={1,.1f,.1f,1};s->materials[1].tint={.1f,1,.1f,1};
        FoliageMeshData mesh;mesh.positions={{-.02f,0,0},{.02f,0,0},{.02f,.04f,0},{-.02f,.04f,0}};
        mesh.normals.assign(4,{0,0,1});mesh.uvs={{0,1},{1,1},{1,0},{0,0}};
        mesh.indices={0,1,2,0,2,3};mesh.sections={{0,3,0},{3,3,1}};
        s->lods={mesh,mesh,mesh};return s;
    }
    inline void prepare(ClawFoliageWorld &world,ClawRendererDX11 &renderer,const ClawFoliageView &view,u64 revision)
    {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);String error;
        do
        {
            require(world.prepare(renderer,{view},0,error),error.c_str());
            if(world.diagnostics().sourceRevision==revision&&world.diagnostics().currentGenerationReady)return;
            std::this_thread::yield();
        }while(std::chrono::steady_clock::now()<deadline);
        require(false,"foliage pages must complete within bounded worker wait");
    }
    inline void wind(wp_renderer_dx11 *native)
    {
        setup(native);
        auto shared=geometry(native,{{{-.1f,0,0},{0,0,1},{0,1},0xffffffff},
            {{.1f,0,0},{0,0,1},{1,1},0xffffffff},{{0,.7f,0},{0,0,1},{.5f,0},0xffffffff}},{0,1,2});
        auto plain=material({1,1,1,1});wp_renderer_dx11_set_material(native,&plain);
        wp_foliage_instance_dx11 tree{};tree.instance=instance(0,-.4f,1,{1,1,1,1});tree.uv_rect={0,0,1,1};
        wp_foliage_parameters_dx11 parameters{{0,1,0,0},{0,.7f,0,0}};
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,3,&tree,1,&parameters)==1,"foliage stream shader must compile and draw");
        const auto still=pixels(native);tree.wind={.45f,0,1.5707963f,0};
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,3,&tree,1,&parameters)==1,"wind draw must submit");
        const auto bent=pixels(native);size_t changed=0;for(size_t i=0;i<still.size();i+=4)changed+=still[i]!=bent[i];
        require(changed>150,"wind must visibly bend the crown while retaining the root");
        tree.wind={};tree.interaction={.45f,0,0,0};
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,3,&tree,1,&parameters)==1,"interaction draw must submit");
        equivalent(bent,pixels(native),"same bounded wind/interaction displacement must match pixels");
        const auto light=identity();require(wp_renderer_dx11_begin_shadow_map(native,&light,128),"foliage wind shadow must begin");
        NativeShadowScope shadowScope{native};
        wp_renderer_dx11_set_depth_test_enabled(native,1);wp_renderer_dx11_set_depth_write_enabled(native,1);
        require(wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,3,&tree,1,&parameters)==1,"wind shadow stream must draw");
        // At row 64 the bent triangle's rasterized edges interpolate between
        // base x=[-.1,.1] and crown x=.45, covering x=.21..30 (pixel 80).
        const auto crownDepth=shadowDepth(native,80,64),rootColumnDepth=shadowDepth(native,64,64);
        std::printf("Foliage bent shadow depth: crown=%g, original column=%g\n",crownDepth,rootColumnDepth);
        require(crownDepth<.51f&&rootColumnDepth>.99f,
            "shadow silhouette must follow the same bent crown as colour");
        wp_renderer_dx11_end_shadow_map(native);setup(native);
        tree.uv_rect.z=2;const auto before=statistics(native);
        require(!wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,3,&tree,1,&parameters)&&statistics(native).draws==before.draws,
            "invalid atlas coordinates must reject before draw");
        ComPtr<ID3D11Buffer> buffer;UINT stride=0,offset=0;
        static_cast<ID3D11DeviceContext*>(wp_renderer_dx11_get_context(native))->IAGetVertexBuffers(1,1,buffer.GetAddressOf(),&stride,&offset);
        require(!buffer,"foliage instance stream must unbind after draw");
    }
    inline void atlas(wp_renderer_dx11 *native)
    {
        setup(native);auto device=static_cast<ID3D11Device*>(wp_renderer_dx11_get_device(native));
        auto context=static_cast<ID3D11DeviceContext*>(wp_renderer_dx11_get_context(native));
        const u32 colours[]={0xff0000ff,0xff00ff00}; // RGBA8 red, green.
        D3D11_TEXTURE2D_DESC desc{};desc.Width=2;desc.Height=1;desc.MipLevels=desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA input{colours,8,0};ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;
        require(SUCCEEDED(device->CreateTexture2D(&desc,&input,&texture))&&SUCCEEDED(device->CreateShaderResourceView(texture.Get(),nullptr,&view)),"atlas texture must upload");
        auto shared=geometry(native,{{{-.25f,-.25f,0},{0,0,1},{0,1},0xffffffff},{{.25f,-.25f,0},{0,0,1},{1,1},0xffffffff},
            {{.25f,.25f,0},{0,0,1},{1,0},0xffffffff},{{-.25f,.25f,0},{0,0,1},{0,0},0xffffffff}},{0,1,2,0,2,3});
        wp_foliage_instance_dx11 cards[2]{};cards[0].instance=instance(-.5f,0,1,{1,1,1,1});cards[1].instance=instance(.5f,0,1,{1,1,1,1});
        cards[0].uv_rect={0,0,.5f,1};cards[1].uv_rect={.5f,0,.5f,1};
        wp_foliage_parameters_dx11 parameters{{0,1,0,0},{0,1,0,0}};
        auto plain=material({1,1,1,1});plain.controls.z=.5f;wp_renderer_dx11_set_material(native,&plain);
        wp_renderer_dx11_set_texture_native(native,view.Get());wp_renderer_dx11_set_material_sampler(native,1,1,0,1);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,6,cards,2,&parameters)==2,"atlas cards must share one instanced draw");
        auto output=pixels(native);const auto left=(64*128+32)*4,right=(64*128+96)*4;
        require(output[left]>200&&output[left+1]<10&&output[right]<10&&output[right+1]>200,"per-instance atlas rectangles must select distinct source views");
        const u32 cutout[]={0xff0000ff,0x0000ff00};context->UpdateSubresource(texture.Get(),0,nullptr,cutout,8,0);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,6,cards,2,&parameters);output=pixels(native);
        require(output[left]>200&&output[right+1]<10,"textured atlas leaf alpha must cut out colour");
        const auto unit=identity();require(wp_renderer_dx11_begin_shadow_map(native,&unit,128),"atlas shadow must begin");
        NativeShadowScope shadowScope{native};
        wp_renderer_dx11_set_depth_test_enabled(native,1);wp_renderer_dx11_set_depth_write_enabled(native,1);
        wp_renderer_dx11_draw_foliage_instances(native,shared.get(),0,6,cards,2,&parameters);
        require(shadowDepth(native,32,64)<.51f&&shadowDepth(native,96,64)>.99f,"atlas alpha must match colour and shadow depth");
        wp_renderer_dx11_end_shadow_map(native);wp_renderer_dx11_set_texture_native(native,nullptr);
        ID3D11ShaderResourceView *empty=nullptr;context->PSSetShaderResources(0,1,&empty);setup(native);
    }
    class TerrainScene : public ClawScene
    {
    public:
        void attach(SmartPtr<ClawTerrain> value){m_terrains.push_back(value);}
    };
    inline void cpuTexture(wp_renderer_dx11 *native)
    {
        claw_foliage_scene_contracts::Fixture fixture;
        fixture.services.factory->load(nullptr);
        struct FactoryScope{SmartPtr<FactoryManager> factory;~FactoryScope(){factory->unload(nullptr);}}factoryScope{fixture.services.factory};
        auto plugin=make_ptr<ClawHammer>();plugin->load(nullptr);
        require(bool(getFoliageImpostorTextureResolver()),"graphics plugin must install the CPU foliage baker texture resolver");
        auto texture=make_ptr<ClawTexture>();u32 bgra=0xff123456;texture->copyData(&bgra,{1,1});String error;
        const auto before=WorldStreamingScheduler::shared()->stats().used[size_t(StreamMemoryClass::SourceCpu)];
        auto pixels=texture->getCpuBaseMip(error);require(pixels&&pixels->width==1&&pixels->bgra[0]==0x56,"CPU baker must receive retained BGRA pixels");
        require(WorldStreamingScheduler::shared()->stats().used[size_t(StreamMemoryClass::SourceCpu)]>before,"escaped CPU texture snapshot must retain a budget lease");
        bgra=0xffabcdef;texture->copyData(&bgra,{1,1});require(pixels->bgra[0]==0x56,"texture authoring must not mutate retained bake input");
        TextureMipLevel cooked{1,1,{9,8,7,255}};TextureMipSettings settings;
        require(texture->uploadCookedMips({cooked},settings,native,error),error.c_str());
        auto cookedPixels=texture->getCpuBaseMip(error);require(cookedPixels&&cookedPixels->bgra==cooked.bgra,"cooked base mip must be available without GPU readback");
        texture=nullptr;require(pixels->bgra[0]==0x56&&cookedPixels->bgra[0]==9,"bake inputs must outlive texture destruction");
        pixels.reset();cookedPixels.reset();
        require(WorldStreamingScheduler::shared()->stats().used[size_t(StreamMemoryClass::SourceCpu)]==before,"last CPU pixel owner must return its reservation");
        plugin->unload(nullptr);require(!getFoliageImpostorTextureResolver(),"plugin unload must remove callback into unloaded backend");
    }
    inline void sceneIntegration(ClawRendererDX11 &renderer,wp_renderer_dx11 *native)
    {
        claw_foliage_scene_contracts::Fixture fixture;auto scene=make_ptr<TerrainScene>();
        scene->setEnableShadows(false);scene->setAmbientLight(ColourF::White);
        auto data=std::make_shared<FoliageWorldData>();data->revision=42;data->species={species()};
        FoliageInstance instance;instance.id=42;instance.scale={10,10,10};data->instances={instance};
        String error;auto standalone=std::make_shared<ClawFoliageWorld>();
        require(standalone->setSource(data,{},error)&&scene->addFoliageWorld(standalone,error),error.c_str());
        auto terrain=make_ptr<ClawTerrain>();workphone::render::TerrainData emptySurface;emptySurface.holes={1};
        require(terrain->applyTerrainData(emptySurface,error)&&terrain->setFoliageSnapshot(data,error),error.c_str());scene->attach(terrain);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);u64 submitted=0;
        do{const auto before=statistics(native);renderer.clear(ColourF::Black);scene->render(static_cast<IRenderer*>(&renderer));
            submitted=statistics(native).instances-before.instances;if(submitted==4)break;std::this_thread::yield();}
        while(std::chrono::steady_clock::now()<deadline);
        require(submitted==4&&scene->getFoliageError().empty(),"real scene must render terrain snapshot plus standalone world with two sections each");
        require(terrain->getActiveFoliage().snapshot==data,"scene must publish the same active foliage generation through the terrain gate");
        require(terrain->setFoliageForcedLod(2,error),error.c_str());scene->render(static_cast<IRenderer*>(&renderer));
        const auto diagnostics=terrain->getFoliageDiagnostics();
        require(diagnostics.currentGenerationReady&&diagnostics.lodInstances[2]==1&&diagnostics.actualDraws==2,
            "terrain forced LOD and diagnostics must report actual backend rendering");
        require(!scene->addFoliageWorld(standalone,error),"duplicate standalone world must reject");
        require(terrain->setFoliageSnapshot({},error),error.c_str());
        require(!terrain->getActiveFoliage().snapshot,"clearing authoring foliage must clear its active publication");
        const auto before=statistics(native);scene->render(static_cast<IRenderer*>(&renderer));
        require(statistics(native).instances-before.instances==2,"clearing terrain foliage snapshot must detach only its retained world");
        require(scene->removeFoliageWorld(standalone),"standalone world must detach");
        const auto detached=statistics(native);scene->render(static_cast<IRenderer*>(&renderer));
        require(statistics(native).instances==detached.instances,"detached worlds must issue no foliage draw");
        scene->clear();terrain=nullptr;scene=nullptr;
    }
    inline void deformationBounds(ClawRendererDX11 &renderer,wp_renderer_dx11 *native)
    {
        setup(native);
        auto source=species();source->boundsMinimum={-1,0,0};source->boundsMaximum={1,2,0};
        for(auto &lod:source->lods)for(auto &p:lod.positions){p.x*=50;p.y*=50;}
        source->wind.amplitude=.8f;source->wind.bendEnd=2;
        auto data=std::make_shared<FoliageWorldData>();data->species={source};data->revision=1;
        FoliageInstance plant;plant.id=701;plant.position={1.3f,-.15f,0};plant.scale={.1f,.1f,.1f};data->instances={plant};
        ClawFoliageWorld world;String error;require(world.setSource(data,{},error),error.c_str());
        ClawFoliageView view;view.forcedLod=0;view.orthographic=true;
        prepare(world,renderer,view,1);
        const double phase=double(foliageStableHash(plant.id)&65535u)/65535.0*6.283185307;
        const auto seconds=f32(std::fmod(4.712388980+6.283185307-phase,6.283185307));
        require(world.prepare(renderer,{view},seconds,error),error.c_str());
        renderer.setSceneLighting(ColourF::White,{0,0,-1},ColourF::White,0);
        auto uncull=view;uncull.cull=false;
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,uncull,error),error.c_str());
        const auto reference=pixels(native);size_t covered=0;
        for(size_t i=0;i<reference.size();i+=4)covered+=reference[i]>20||reference[i+1]>20||reference[i+2]>20;
        std::printf("Foliage scaled sway: covered=%zu, draws=%llu, instances=%llu, phase=%.5f, time=%.5f\n",
            covered,static_cast<unsigned long long>(world.diagnostics().actualDraws),
            static_cast<unsigned long long>(world.diagnostics().lodInstances[0]),phase,seconds);
        require(covered>12,"small foliage outside its static bounds must sway into visible pixels");
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,view,error),error.c_str());
        require(world.diagnostics().lodInstances[0]==1,"page and instance culling must retain world-space sway at subunit scale");
        require(reference==pixels(native),"deformed bounds must match uncull reference pixels");
        auto placement=Matrix4F::identity();placement[0][0]=placement[1][1]=placement[2][2]=.5f;placement[0][3]=.65f;
        require(world.setPlacement(placement,error)&&world.prepare(renderer,{view},seconds,error),error.c_str());
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,uncull,error),error.c_str());
        const auto placed=pixels(native);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,view,error),error.c_str());
        require(world.diagnostics().lodInstances[0]==1,"placement changes must rebuild world deformation page bounds");
        require(placed==pixels(native),"scaled placement culling must match its uncull reference");
        world.clear();

        // An off-centre capture has a different card centre and authored pivot.
        // The interaction must continue sampling the plant's original root.
        auto card=species();card->boundsMinimum={.1f,0,0};card->boundsMaximum={.5f,.4f,0};
        for(auto &lod:card->lods)for(auto &p:lod.positions){p.x=p.x*10+.3f;p.y*=10;}
        card->wind.maximumInteractionBend=.6f;card->wind.bendEnd=.4f;card->farBillboard=true;
        card->impostor.albedo="pivot-atlas";card->impostor.width=card->impostor.height=4;
        card->impostor.views.push_back({});
        auto texture=make_ptr<ClawTexture>();TextureMipLevel white{4,4,std::vector<u8>(64,255)};TextureMipSettings settings;
        require(texture->uploadCookedMips({white},settings,native,error),error.c_str());
        auto cardData=std::make_shared<FoliageWorldData>();cardData->species={card};cardData->revision=2;
        plant.position={0,0,0};plant.scale={1,1,1};cardData->instances={plant};
        require(world.setSource(cardData,[texture](const String &,String &message)->SmartPtr<ITexture>{message.clear();return texture;},error),error.c_str());
        require(world.setPlacement(Matrix4F::identity(),error),error.c_str());view.forcedLod=2;view.position={.3f,.2f,1};
        prepare(world,renderer,view,2);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,view,error),error.c_str());const auto still=pixels(native);
        require(world.setInteractions({{{-.05f,0,0},.15f,1}},error),error.c_str());
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,view,error),error.c_str());const auto bent=pixels(native);
        size_t changed=0;for(size_t i=0;i<still.size();i+=4)changed+=still[i]!=bent[i];
        require(changed>20,"far impostor interaction must sample authored root, not shifted capture centre");
    }
    inline void sourceImpostorReopen(ClawRendererDX11 &renderer,wp_renderer_dx11 *native)
    {
        namespace fs=std::filesystem;
        claw_foliage_scene_contracts::Fixture fixture;
        const auto folder=fs::temp_directory_path()/
            ("workphone_foliage_source_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        require(fs::create_directory(folder),"source fixture must create its own project root");
        const auto oldRoot=fixture.services.application->getProjectPath();
        struct Cleanup
        {
            SmartPtr<core::IApplicationManager> app;String root;fs::path folder;
            ~Cleanup()
            {
                app->setProjectPath(root);std::error_code ignored;
                for(const char *channel:{"albedo","normal","orm","depth"})
                    for(const char *extension:{".texres",".tga"})
                        fs::remove(folder/"bake"/(String("plant_")+channel+extension).c_str(),ignored);
                fs::remove(folder/"bake"/"species.folspec",ignored);fs::remove(folder/"bake",ignored);fs::remove(folder,ignored);
            }
        }cleanup{fixture.services.application,oldRoot,folder};
        fixture.services.application->setProjectPath(folder.u8string().c_str());
        auto plant=species();FoliageImpostorBakeOptions options;options.tileSize=32;options.gutter=2;
        options.directions={{0,0,1}};options.resourcePrefix="data://bake/plant";
        FoliageImpostorBake baked;String error,path;
        require(bakeFoliageImpostor(*plant,{},options,baked,error),error.c_str());
        require(saveFoliageImpostorSource(*plant,baked,(folder/"bake").u8string().c_str(),path,error),error.c_str());
        std::ifstream stream(fs::u8path(path.c_str()),std::ios::binary);
        const std::string serialized((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());
        FoliageSpecies restored;require(deserializeFoliageSpecies(serialized.c_str(),restored,error),error.c_str());
        auto data=std::make_shared<FoliageWorldData>();data->revision=73;data->species={std::make_shared<const FoliageSpecies>(restored)};
        FoliageInstance item;item.id=73;item.scale={10,10,10};data->instances={item};
        ClawFoliageView view;view.forcedLod=2;view.orthographic=true;view.position={0,.2f,1};
        auto world=std::make_unique<ClawFoliageWorld>();require(world->setSource(data,{},error),error.c_str());prepare(*world,renderer,view,73);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world->render(renderer,view,error),error.c_str());
        const auto original=pixels(native);size_t covered=0;for(size_t i=0;i<original.size();i+=4)covered+=original[i]>20||original[i+1]>20;
        require(world->diagnostics().actualDraws==1&&covered>100,"saved textured impostor must render through the default project descriptor resolver");
        world.reset();world=std::make_unique<ClawFoliageWorld>();
        require(world->setSource(data,{},error),error.c_str());prepare(*world,renderer,view,73);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world->render(renderer,view,error),error.c_str());
        equivalent(original,pixels(native),"fresh foliage owner must reopen saved species and texture descriptors without preview cache");
        fixture.services.factory->load(nullptr);
        struct FactoryScope{SmartPtr<FactoryManager> factory;~FactoryScope(){factory->unload(nullptr);}}factoryScope{fixture.services.factory};
        auto plugin=make_ptr<ClawHammer>();plugin->load(nullptr);
        const auto resolver=getFoliageImpostorTextureResolver();auto cpu=resolver(restored.impostor.albedo,error);
        require(cpu&&cpu->width==baked.metadata.width&&cpu->bgra==baked.albedo.front().bgra,
            "reopened descriptor must also resolve CPU pixels for a subsequent impostor bake");
        plugin->unload(nullptr);
        {std::ofstream broken(folder/"bake"/"plant_albedo.texres",std::ios::binary|std::ios::trunc);broken<<"{}";}
        require(world->setSource(data,{},error)&&!world->prepare(renderer,{view},0,error),
            "malformed reloaded texture descriptor must reject candidate preparation");
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world->render(renderer,view,error),error.c_str());
        equivalent(original,pixels(native),"failed source descriptor reload must retain the previous complete GPU material");
        world.reset();
    }
    inline void extentPaging(ClawRendererDX11 &renderer,wp_renderer_dx11 *native)
    {
        setup(native);String error;auto tree=species();
        tree->maximumDistance=tree->shadowDistance=1;
        tree->boundsMinimum.x-=10;tree->boundsMaximum.x-=10;
        for(auto &lod:tree->lods)for(auto &vertex:lod.positions)vertex.x-=10;
        auto data=std::make_shared<FoliageWorldData>();data->species={tree};data->pageSize=1;data->revision=1;
        FoliageInstance plant;plant.id=1;plant.position={10,0,0};data->instances={plant};
        ClawFoliageWorld world;require(world.setSource(data,{},error),error.c_str());
        ClawFoliageView view;view.key=411;view.position={0,0,1};view.forcedLod=0;
        prepare(world,renderer,view,1);wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(world.render(renderer,view,error),error.c_str());
        require(world.diagnostics().residentInstances==1&&world.diagnostics().lodInstances[0]==1&&
            world.diagnostics().actualDraws==2,"Visible off-centre foliage must load its distant origin cell");
    }
    inline bool run(ClawRendererDX11 &renderer)
    {
        struct Scope
        {
            ClawRendererDX11&r;
            ~Scope()
            {
                auto native=wp_renderer_get_dx11(r.getNativeRenderer());
                wp_renderer_dx11_end_shadow_map(native);setup(native);
                r.setCamera(nullptr);r.setViewport(nullptr);r.setRenderTarget(nullptr);r.disableShadows();
            }
        }scope{renderer};
        try
        {
            auto target=make_ptr<ClawRenderTarget>();target->setSize({128,128});renderer.setRenderTarget(target);
            renderer.setCamera(nullptr);renderer.setViewport(nullptr);renderer.disableShadows();
            auto native=wp_renderer_get_dx11(renderer.getNativeRenderer());wind(native);atlas(native);cpuTexture(native);deformationBounds(renderer,native);sourceImpostorReopen(renderer,native);extentPaging(renderer,native);
            auto data=std::make_shared<FoliageWorldData>();data->pageSize=1;data->revision=1;auto sourceSpecies=species();data->species={sourceSpecies};
            for(u32 n=0;n<5000;++n){FoliageInstance i;i.id=n+1;i.seed=n;i.position={f32(n%100)*.016f-.8f,f32(n/100)*.016f-.4f,0};data->instances.push_back(i);}
            String error;ClawFoliageWorld world;require(world.setSource(data,{},error),error.c_str());
            ClawFoliageView view;view.forcedLod=0;view.orthographic=true;view.projectionScale=1;
            prepare(world,renderer,view,1);renderer.setSceneLighting(ColourF::White,{0,0,-1},ColourF::White,0);
            wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,view,error),error.c_str());
            auto stats=world.diagnostics();require(stats.residentPages>=2&&stats.lodInstances[0]==5000,"multiple sparse cells must be resident and independently instanced");
            require(stats.groups==2&&stats.actualDraws==4&&stats.predictedDraws==4&&stats.submittedInstances==10000,
                "cross-page grouping must issue two sections times two capacity chunks");
            require(stats.uploadBytes==10000*160,"reported uploads must match native foliage stream bytes");
            const auto covered=pixels(native);require(std::count_if(covered.begin(),covered.end(),[](u8 v){return v>40;})>128*128,"resident world must visibly render");
            bool permit=false;u32 commits=0;
            require(world.setPublicationGate([&](FoliageSnapshot,const std::function<void()> &commit,String &message)
            {if(!permit){message="fixture collision preparation rejected";return false;}commit();++commits;message.clear();return true;},error),error.c_str());
            auto displaced=Matrix4F::identity();displaced[0][3]=100;require(world.setPlacement(displaced,error),error.c_str());
            require(!world.prepare(renderer,{view},0,error)&&commits==0,"consumer gate failure must not publish staged residency or placement");
            require(world.render(renderer,view,error)&&world.diagnostics().lodInstances[0]==5000,
                "failed placement publication must retain previous visible GPU positions and population");
            permit=true;require(world.setPlacement(Matrix4F::identity(),error),error.c_str());prepare(world,renderer,view,1);
            require(commits>0&&world.setPublicationGate({},error),"successful consumer gate must run renderer commit before publication completes");
            auto second=view;second.key=2;second.forcedLod=2;require(world.render(renderer,second,error),error.c_str());
            require(world.diagnostics().lodInstances[2]==5000,"second camera must select its own far LOD");
            require(world.render(renderer,view,error)&&world.diagnostics().lodInstances[0]==5000,"second camera must not override first camera LOD");
            auto hysteresis=view;hysteresis.forcedLod=-1;hysteresis.projectionScale=4;
            require(world.render(renderer,hysteresis,error)&&world.diagnostics().lodInstances[0]==5000,"per-instance LOD must retain near detail inside hysteresis band");
            hysteresis.key=3;require(world.render(renderer,hysteresis,error)&&world.diagnostics().lodInstances[1]==5000,"fresh view must not inherit another camera hysteresis");
            hysteresis.key=view.key;hysteresis.projectionScale=3;
            require(world.render(renderer,hysteresis,error)&&world.diagnostics().lodInstances[1]==5000,"LOD must downgrade beyond its hysteresis threshold");
            auto culled=view;culled.worldToClip[0][3]=100;require(world.render(renderer,culled,error),error.c_str());
            require(world.diagnostics().actualDraws==0&&world.diagnostics().culledInstances==5000,"page frustum culling must issue no draw");
            auto invalid=std::make_shared<FoliageWorldData>(*data);invalid->instances[0].scale.x=0;
            require(!world.setSource(invalid,{},error)&&world.source().get()==data.get(),"invalid source must retain published world");
            auto tiny=std::make_shared<FoliageWorldData>(*data);tiny->instances.resize(1);tiny->revision=9;
            tiny->instances[0].scale={1.e-40f,1.e-40f,1.e-40f};
            require(world.setSource(tiny,{},error),error.c_str());bool nativeRejected=false;
            const auto nativeDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
            while(std::chrono::steady_clock::now()<nativeDeadline)
            {if(!world.prepare(renderer,{view},0,error)){nativeRejected=true;break;}std::this_thread::yield();}
            require(nativeRejected&&error.find("native DX11 limits")!=String::npos,
                "Finite transforms with unrepresentable native normals must reject before publication");
            require(world.render(renderer,view,error)&&world.diagnostics().lodInstances[0]==5000,
                "Unsupported native transform must retain the previous complete visible generation");
            auto grass=std::make_shared<FoliageSpecies>(*sourceSpecies);grass->kind=FoliageKind::Grass;grass->densityFractions={1,.5f,.25f};
            auto replacement=std::make_shared<FoliageWorldData>(*data);replacement->species={grass};replacement->revision=2;
            require(world.setSource(replacement,{},error),error.c_str());
            const auto budgetState=WorldStreamingScheduler::shared()->stats();const auto resultCategory=size_t(StreamMemoryClass::ResultCpu);
            auto pressure=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::ResultCpu,
                budgetState.limits[resultCategory]-budgetState.used[resultCategory]-256,error);require(bool(pressure),error.c_str());
            require(!world.prepare(renderer,{second},0,error)&&!world.diagnostics().currentGenerationReady,
                "page replacement must report shared budget admission failure");
            require(world.render(renderer,view,error)&&world.diagnostics().lodInstances[0]==5000,
                "budget failure must retain the complete previous population");
            pressure.reset();prepare(world,renderer,second,2);
            require(world.render(renderer,second,error),error.c_str());const auto sparse=world.diagnostics().lodInstances[2];
            require(sparse>1000&&sparse<1500,"grass far LOD must select a stable bounded density subset");
            second.position.x+=.01f;require(world.render(renderer,second,error)&&world.diagnostics().lodInstances[2]==sparse,
                "camera motion must not reshuffle stable grass density");
            auto outOfRange=std::make_shared<FoliageWorldData>(*replacement);auto distant=std::make_shared<FoliageSpecies>(*grass);distant->maximumDistance=100;
            outOfRange->species={distant};require(!world.setSource(outOfRange,{},error)&&error.find("30 local page widths")!=String::npos,
                "unsupported paging range must reject before replacing the working source");
            auto empty=std::make_shared<FoliageWorldData>();empty->revision=3;
            require(world.setSource(empty,{},error),error.c_str());prepare(world,renderer,view,3);
            require(world.diagnostics().residentInstances==0&&world.diagnostics().geometryBytes==0,"empty world publication must remove old pages and GPU geometry");
            world.clear();require(!world.source()&&world.diagnostics().residentPages==0,"unload must release resident pages");
            sceneIntegration(renderer,native);
            std::puts("Paged foliage: 5000 instances, >=2 pages, 2 material groups, 4 real draws; independent views, grass density and wind passed.");return true;
        }catch(const std::exception &e){std::fprintf(stderr,"Paged foliage GPU contract failed: %s\n",e.what());return false;}
    }
}
