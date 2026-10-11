#pragma once
#include <Workphone/Graphics/FoliagePageStream.hpp>
#include <WorkphoneGraphics/workphone_graphics_paged_geometry.h>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <chrono>
#include <future>
#include <map>
#include <stdexcept>
#include <thread>

namespace foliage_page_stream_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require(bool ok,const String &message){if(!ok)throw std::runtime_error(message.c_str());}
    inline std::shared_ptr<FoliageWorldData> source()
    {
        auto s=std::make_shared<FoliageSpecies>();s->id="fixture-tree";s->boundsMinimum={-.5f,0,0};s->boundsMaximum={.5f,1,0};s->materials.resize(1);
        for(auto &mesh:s->lods){mesh.positions={{-.5f,0,0},{.5f,0,0},{0,1,0}};mesh.normals={{0,0,1},{0,0,1},{0,0,1}};mesh.uvs={{0,1},{1,1},{.5f,0}};mesh.indices={0,1,2};}
        auto world=std::make_shared<FoliageWorldData>();world->species.push_back(s);
        for(u32 i=0;i<3;++i){FoliageInstance v;v.id=i+1;v.position={i==0?-1.f:i==1?1.f:129.f,0,1};world->instances.push_back(v);}return world;
    }
    inline FoliagePageResult take(FoliagePageStream &stream)
    {
        FoliagePageResult result;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!stream.take(result)){require(std::chrono::steady_clock::now()<deadline,"Foliage page preparation timeout");std::this_thread::yield();}return result;
    }
    struct Native
    {
        struct Node{Native *owner;};
        struct Requested{wp_geometry_page *page;uint64_t generation;FoliageCellKey cell;};
        wp_camera *camera=nullptr;wp_paged_geometry *pager=nullptr;
        std::vector<Requested> requests;std::vector<FoliageCellKey> unloaded;
        u32 created=0,destroyed=0,built=0;
        ~Native(){wp_paged_geometry_destroy(pager);wp_camera_destroy(camera);}
        static void *create(void *p){auto self=static_cast<Native*>(p);++self->created;return new Node{self};}
        static void destroy(void *p){auto n=static_cast<Node*>(p);++n->owner->destroyed;delete n;}
        static void build(void *p){++static_cast<Node*>(p)->owner->built;}
        static void load(void *p,wp_geometry_page *page,wp_geometry_page_info *info)
        {auto self=static_cast<Native*>(p);self->requests.push_back({page,wp_geometry_page_get_load_generation(page),{info->x_index,info->z_index}});info->user_data=self;wp_geometry_page_defer_load(page);}
        static void unload(void *p,wp_geometry_page_info *info)
        {static_cast<Native*>(p)->unloaded.push_back({info->x_index,info->z_index});}
    };
    inline void nativeContracts()
    {
        Native n;n.camera=wp_camera_create();n.pager=wp_paged_geometry_create(n.camera,64);
        require(n.camera&&n.pager,"Native page fixture allocation");
        wp_geometry_page_callbacks pages{};pages.create_page=Native::create;pages.destroy_page=Native::destroy;pages.build=Native::build;
        wp_page_loader_callbacks loader{};loader.load_page=Native::load;loader.unload_page=Native::unload;
        wp_paged_geometry_set_page_loader(n.pager,&loader,&n);
        require(wp_paged_geometry_add_detail_level(n.pager,96,0,&pages,&n,nullptr,0)!=nullptr,"Native detail grid");
        wp_camera_set_position(n.camera,{-1,0,-1});wp_paged_geometry_update(n.pager,16);
        require(!n.requests.empty()&&n.built==0,"Deferred native pages must not build before completion");
        const auto first=n.requests.front();require(!wp_geometry_page_is_loaded(first.page)&&wp_geometry_page_get_load_state(first.page)==WORKPHONE_PAGE_LOADING,"Deferred page readiness");
        require(wp_geometry_page_complete_load(first.page,first.generation,1)&&n.built==1,"Explicit completion builds once");
        require(!wp_geometry_page_complete_load(first.page,first.generation,1)&&n.built==1,"Duplicate completion rejected");
        wp_paged_geometry_reload_page(n.pager,wp_geometry_page_get_center(first.page));
        require(!wp_geometry_page_complete_load(first.page,first.generation,1),"Reload cancels old native generation");
        n.requests.clear();wp_paged_geometry_update(n.pager,16);require(!n.requests.empty(),"Reload retries deferred page");
        const auto pending=n.requests.front();const auto count=n.created;
        wp_camera_set_position(n.camera,{4096,0,-4096});wp_paged_geometry_update(n.pager,16);
        require(n.created==count&&!wp_geometry_page_complete_load(pending.page,pending.generation,1),"Teleport reuses slots and rejects old generation");
        bool oldCell=false;for(auto key:n.unloaded)oldCell=oldCell||key==pending.cell;require(oldCell,"Unload receives the previous cell identity");
        const auto unloaded=n.unloaded.size();wp_paged_geometry_set_page_loader(n.pager,nullptr,nullptr);
        require(n.unloaded.size()>unloaded,"Replacing loader unloads through the old callbacks");
        wp_paged_geometry_destroy(n.pager);n.pager=nullptr;require(n.created==n.destroyed,"Native slot lifecycle balances");
    }
    inline void run()
    {
        nativeContracts();String error;auto scheduler=WorldStreamingScheduler::shared();const auto before=scheduler->stats();
        FoliagePageStream stream;auto world=source();require(stream.setSource(world,error),error);
        require(stream.setDemand({{-1,0},{0,0},{2,0},{999,999},{-1,0}},error),error);
        require(stream.demand().size()==3,"Canonical demand removes duplicates and absent cells");
        Array<FoliagePageResult> held;for(int i=0;i<3;++i){auto page=take(stream);require(page.page&&page.error.empty(),page.error);held.push_back(std::move(page));}
        const auto completed=stream.stats().completed;
        require(stream.setDemand(stream.demand(),error)&&stream.stats().pending==0,"Retained pages suppress repeated preparation");
        require(stream.updateViews({{{-1,0,1},48},{{129,0,1},48}},error),error);
        require(stream.demand().size()==3,"Two views retain the union of negative and positive cells");
        require(stream.updateViews({{{129,0,1},48},{{-1,0,1},48}},error)&&stream.stats().completed==completed,"View reorder does not recook resident pages");
        require(stream.updateViews({{{0,0,0},1}},error)&&
            std::find(stream.demand().begin(),stream.demand().end(),FoliageCellKey{0,0})!=stream.demand().end(),
            "A short range at a page corner must retain the cell containing nearby instances");
        auto invalid=world;invalid=std::make_shared<FoliageWorldData>(*world);invalid->pageSize=0;
        const auto prior=stream.source();require(!stream.setSource(invalid,error)&&stream.source()==prior,"Invalid source retains previous generation");
        stream.drain();require(!stream.source()&&stream.demand().empty(),"Drain releases pager source and demand");
        require(scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]>before.used[size_t(StreamMemoryClass::ResultCpu)],"Taken page alias retains result lease after drain");
        held.clear();
        const auto limits=scheduler->stats().limits;const auto used=scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)];
        require(scheduler->setBudget(StreamMemoryClass::ResultCpu,used,error),error);
        struct Restore{std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~Restore(){String error;scheduler->setBudget(StreamMemoryClass::ResultCpu,limit,error);}}restore{scheduler,limits[size_t(StreamMemoryClass::ResultCpu)]};
        require(stream.setSource(world,error),error);require(!stream.setDemand({{0,0}},error)&&stream.demand().empty(),"Impossible resident demand rejects instead of waiting forever");
        require(scheduler->setBudget(StreamMemoryClass::ResultCpu,restore.limit,error),error);
        require(stream.setDemand({{0,0}},error),error);auto retry=take(stream);require(retry.page!=nullptr,"Budget release permits demand retry");
        stream.drain();retry={};
    }
}
