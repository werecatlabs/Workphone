#pragma once
#include <Workphone/Interface/Procedural/ProceduralTextureData.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>

namespace procedural_buffers_budget_contracts
{
    using namespace workphone;
    using namespace workphone::procedural;
    using namespace workphone::scene::race;
    inline void require(bool value,const char *message)
    { if(!value) throw std::runtime_error(message); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler=WorldStreamingScheduler::shared();
        static constexpr auto source=size_t(StreamMemoryClass::SourceCpu);
        u64 original=scheduler->stats().limits[source];
        u64 used() const {return scheduler->stats().used[source];}
        void set(u64 value) {String error;require(scheduler->setBudget(StreamMemoryClass::SourceCpu,value,error),"procedural Source budget applies");}
        void restore(){set(original);}
        ~Budget(){restore();}
    };
    template<class T> inline u64 bytes(size_t count)
    {return count ? sourceBudgetStorageBytes(count*sizeof(T),alignof(T)):0;}
    template<class F> inline void denied(F operation,const char *message)
    {bool rejected=false;try{operation();}catch(const std::bad_alloc&){rejected=true;}require(rejected,message);}
    template<class T> inline void bodyAdmission(Budget &budget)
    {
        const auto baseline=budget.used();
        const auto body=sourceBudgetStorageBytes(sizeof(T),alignof(std::max_align_t));
        budget.set(baseline+body-1);
        denied([]{auto candidate=std::make_unique<T>();},"procedural heap body cannot bypass Source admission");
        require(budget.used()==baseline,"denied procedural body unwinds");
        budget.set(baseline+body);auto owner=std::make_unique<T>();
        require(budget.used()==baseline+body,"empty procedural heap body charges its physical allocation");
        budget.set(budget.used());owner.reset();
        require(budget.used()==baseline,"procedural body retirement needs no replacement allocation");
    }
    struct ThrowingValue
    {
        bool reject=false;
        ThrowingValue()=default;
        ThrowingValue(const ThrowingValue &other):reject(other.reject)
        {if(reject) throw std::runtime_error("injected element copy failure");}
        ThrowingValue &operator=(const ThrowingValue&)=default;
    };
    class RetiringTexture : public render::ClawTexture
    {
    public:
        scene::ProceduralRaceScene *owner=nullptr;
        bool detached=false;
        int callbacks=0;
        void unload(SmartPtr<ISharedObject>) override
        {
            ++callbacks;
            detached=owner && owner->getAssets().textures.empty() && owner->getAssets().actors.empty();
            throw std::bad_alloc(); // Optional notification denied after manager detachment.
        }
    };

    inline void buffers()
    {
        Budget budget;
        const auto baseline=budget.used();
        const auto pixelBytes=bytes<u8>(17*19*4);
        budget.set(baseline+pixelBytes-1);
        denied([]{TextureBuffer candidate(17,19);},"texture allocation requires complete physical admission");
        require(budget.used()==baseline,"denied pixels leave no lease");
        budget.set(baseline+pixelBytes);
        TextureBuffer pixels(17,19);
        pixels.pixel(16,18)[3]=93;
        require(budget.used()==baseline+pixelBytes && pixels.sizeBytes()==17*19*4,"RGBA storage charges physical capacity");
        budget.restore();
        std::vector<u8> raw;raw.reserve(5003);raw.resize(4,71);
        TextureBuffer incoming;incoming.width=incoming.height=1;incoming.pixels=std::move(raw);
        const auto spare=bytes<u8>(raw.capacity());
        require(raw.size()==4 && incoming.pixels.capacity()==raw.capacity() && incoming.pixels.data()!=raw.data(),
                "foreign rvalue imports spare storage without adopting an uncharged block");
        const auto resident=budget.used();const auto old=pixels.data();
        budget.set(resident+spare-1);
        denied([&]{pixels=incoming;},"texture assignment stages the full incoming capacity");
        require(pixels.data()==old && pixels.width==17 && pixels.height==19 && pixels.pixel(16,18)[3]==93 && budget.used()==resident,
                "denied texture copy preserves complete dimensions and pixels");
        budget.set(resident+spare);
        std::optional<TextureBuffer> snapshot(incoming);
        require(snapshot->pixels.capacity()==5003 && snapshot->pixels[0]==71,"copy retains imported spare capacity");
        budget.set(budget.used());const auto denials=sourceBudgetDenialCount();
        incoming=TextureBuffer{};pixels=TextureBuffer{};
        require(budget.used()==baseline+spare && snapshot->pixels[0]==71,"caller snapshot owns storage beyond producer reset");
        std::thread finalOwner([owned=std::move(snapshot)]() mutable {owned.reset();});
        finalOwner.join();snapshot.reset();
        require(budget.used()==baseline && sourceBudgetDenialCount()==denials,"last cross-thread pixel release needs no admission");

        const auto heightBytes=bytes<real_Num>(17*19);
        budget.set(baseline+heightBytes-1);
        denied([]{HeightBuffer candidate(17,19);},"height storage uses the same Source quota");
        budget.set(baseline+heightBytes);
        HeightBuffer height(17,19);height.values.back()=6;
        require(height.at(999,999)==6 && budget.used()==baseline+heightBytes,"height lookup clamps using wide row offsets");
        const auto heightPointer=height.values.data();
        denied([&]{height.values.resize(9999);},"height growth is denied at the cap");
        require(height.values.data()==heightPointer && height.at(16,18)==6,"denied resize retains valid height data");
        height=HeightBuffer{};
        bool invalid=false;try{height.at(0,0);}catch(const std::out_of_range&){invalid=true;}
        require(invalid,"empty heights reject invalid lookups");
        const auto beforeInvalid=sourceBudgetDenialCount();
        invalid=false;try{TextureBuffer huge(std::numeric_limits<u32>::max(),2);}catch(const std::length_error&){invalid=true;}
        require(invalid && sourceBudgetDenialCount()==beforeInvalid,"texture overflow fails before allocator admission");
        invalid=false;try{HeightBuffer huge(std::numeric_limits<u32>::max(),std::numeric_limits<u32>::max());}catch(const std::length_error&){invalid=true;}
        require(invalid && budget.used()==baseline,"height byte overflow leaves no allocation");

        const auto body=sourceBudgetStorageBytes(sizeof(TextureBuffer),alignof(std::max_align_t));
        budget.set(baseline+body+pixelBytes-1);
        denied([]{auto candidate=std::make_unique<TextureBuffer>(17,19);},"heap wrapper and payload each require admission");
        require(budget.used()==baseline,"payload denial unwinds an admitted wrapper");
        budget.set(baseline+body+pixelBytes);
        auto heap=std::make_unique<TextureBuffer>(17,19);
        require(budget.used()==baseline+body+pixelBytes,"heap body and pixel capacity are independently charged");
        budget.set(budget.used());heap.reset();
        require(budget.used()==baseline,"heap pixel retirement returns both charges");
        bodyAdmission<HeightBuffer>(budget);
        bodyAdmission<SourceBudgetVector<int>>(budget);

        budget.restore();
        SourceBudgetVector<int> vector;vector.reserve(37);vector.push_back(8);
        const auto vectorCost=bytes<int>(37);const auto vectorResident=budget.used();
        budget.set(vectorResident);vector=vector;vector.clear();
        require(vector.capacity()==37 && budget.used()==baseline+vectorCost,"self assignment and clear retain physical spare capacity");
        denied([&]{vector={1,2,3};},"initializer replacement stages before publication");
        require(vector.empty() && vector.capacity()==37,"denied initializer replacement retains old storage");
        vector.releaseStorage();require(budget.used()==baseline,"explicit vector retirement frees spare capacity");
        budget.restore();
        SourceBudgetVector<ThrowingValue> elements;elements.reserve(37);elements.resize(2);elements.back().reject=true;
        const auto liveElements=budget.used();
        bool copyFailed=false;try{auto candidate=elements;}catch(const std::runtime_error&){copyFailed=true;}
        require(copyFailed && budget.used()==liveElements,"partial throwing element copy destroys admitted storage and constructed elements");
        budget.set(liveElements);elements.releaseStorage();
        require(budget.used()==baseline,"throwing element source can retire at the cap");
    }

    inline void inventories()
    {
        Budget budget;
        const auto baseline=budget.used();
        auto city=OpenCityLayout::generate(73,8,2);
        const auto lots=bytes<OpenCityLayout::Lot>(city.lots.capacity());
        const auto route=bytes<OpenCityLayout::Point>(city.route.capacity());
        require(budget.used()==baseline+lots+route && !city.route.empty(),"layout lot and navigation spare capacities share Source admission");
        OpenCityLayout old;old.blocks=6;old.spacing=7;old.lots.reserve(3);old.lots.push_back({1,2,3,4,5,false});
        const auto resident=budget.used();const auto pointer=old.lots.data();
        budget.set(resident+lots+route-1);
        denied([&]{old=city;},"route denial unwinds an already copied lot inventory");
        require(old.blocks==6 && old.spacing==7 && old.lots.data()==pointer && old.lots[0].height==5 && budget.used()==resident,
                "layout copy failure preserves scalars and every previous container");
        budget.set(resident+lots+route);
        old=city;
        require(old.route.capacity()==city.route.capacity() && old.route.data()!=city.route.data() && old.route[10].z==city.route[10].z,
                "layout retry has independent deterministic navigation storage");
        budget.set(budget.used());const auto denials=sourceBudgetDenialCount();
        denied([&]{auto candidate=OpenCityLayout::generate(73,8,2);},"new city generation must obtain Source admission");
        city={};old={};
        require(budget.used()==baseline && sourceBudgetDenialCount()==denials+1,"failed generation and old layout retire all leases");
        budget.restore();
        auto circuit=generateCircuit(73);
        const auto circuitCost=bytes<CircuitSample>(circuit.samples.capacity());
        require(budget.used()==baseline+circuitCost && circuit.length>0 && circuit.nearest(circuit.samples[5].position)==5,
                "circuit output owns only its retained samples after temporary generation buffers retire");
        const auto circuitPointer=circuit.samples.data();
        budget.set(budget.used());
        denied([&]{circuit=generateCircuit(74);},"denied circuit replacement unwinds its scratch");
        require(circuit.samples.data()==circuitPointer,"failed circuit generation preserves the previous path");
        circuit={};require(budget.used()==baseline,"circuit retirement frees sample capacity");

        budget.restore();
        SceneAssets assets;assets.openCity=true;assets.cityBlocks=10;assets.triangles=93;
        assets.vehicleMaterials.reserve(11);assets.materials.reserve(13);assets.textures.reserve(17);
        assets.meshes.reserve(19);assets.actors.reserve(23);assets.treeLODs.reserve(29);
        assets.circuit.samples.reserve(31);assets.circuit.length=51;
        assets.vehicle.appearance.textures.bodyLivery=TextureBuffer(17,19);
        const auto inventoriesCost=bytes<SmartPtr<render::IMaterial>>(11)+bytes<SmartPtr<render::IMaterial>>(13)+
            bytes<SmartPtr<render::ITexture>>(17)+bytes<SmartPtr<IMeshResource>>(19)+bytes<SmartPtr<scene::IGameActor>>(23)+
            bytes<SmartPtr<scene::LODGroup>>(29)+bytes<CircuitSample>(31)+bytes<u8>(17*19*4);
        require(budget.used()==baseline+inventoriesCost,"all city owner inventories and generated pixels charge retained spare capacity");
        SceneAssets previous;previous.cityBlocks=6;previous.triangles=42;previous.actors.reserve(3);
        const auto beforeCopy=budget.used();const auto previousPointer=previous.actors.data();
        for(const auto room:{u64(0),inventoriesCost/2,inventoriesCost-1})
        {
            budget.set(beforeCopy+room);
            denied([&]{previous=assets;},"complete scene copy requires all incoming allocations before publication");
            require(previous.cityBlocks==6 && previous.triangles==42 && previous.actors.data()==previousPointer && budget.used()==beforeCopy,
                    "partial scene copy denial unwinds payloads and leaves old ownership unchanged");
        }
        budget.set(beforeCopy+inventoriesCost);previous=assets;
        require(previous.openCity && previous.triangles==93 && previous.textures.capacity()==17 && previous.treeLODs.capacity()==29 &&
                previous.vehicle.appearance.textures.bodyLivery.data()!=assets.vehicle.appearance.textures.bodyLivery.data(),
                "complete scene copy preserves independent inventories, metadata and pixel owners");
        budget.set(budget.used());const auto beforeRetire=sourceBudgetDenialCount();
        assets={};previous={};
        require(budget.used()==baseline && sourceBudgetDenialCount()==beforeRetire,"full-cap scene reset retires every covered allocation without replacement admission");

        // Public Array callers can supply a foreign allocator; copying the scene
        // canonicalizes its complete physical spare capacity at the boundary.
        budget.restore();Array<SmartPtr<scene::LODGroup>> foreign;foreign.reserve(41);
        assets.treeLODs=foreign;const auto externalBaseline=budget.used();
        const auto treeCost=bytes<SmartPtr<scene::LODGroup>>(41);
        budget.set(externalBaseline+treeCost-1);
        denied([&]{SceneAssets candidate(assets);},"foreign tree inventory cannot bypass Source quota through an empty logical size");
        require(budget.used()==externalBaseline,"failed foreign tree import unwinds");
        budget.set(externalBaseline+treeCost);SceneAssets imported(assets);
        require(imported.treeLODs.get_allocator().isSourceBudgeted() && imported.treeLODs.capacity()==41 && budget.used()==externalBaseline+treeCost,
                "scene copy canonicalizes caller-owned tree spare capacity");
        budget.set(budget.used());imported={};assets={};
        require(budget.used()==baseline,"final foreign inventory release returns its physical charge");
        bodyAdmission<OpenCityLayout>(budget);
        bodyAdmission<Circuit>(budget);
        bodyAdmission<SceneAssets>(budget);

        budget.restore();
        scene::ProceduralRaceScene scene;
        auto texture=make_ptr<RetiringTexture>();texture->owner=&scene;
        auto &generation=const_cast<SceneAssets&>(scene.getAssets());
        generation.textures.push_back(texture);
        generation.vehicle.appearance.textures.bodyLivery=TextureBuffer(128,128);
        generation.circuit.samples.reserve(512);
        const auto producerPixels=generation.vehicle.appearance.textures.bodyLivery.data();
        require(producerPixels!=nullptr,"scene retirement fixture contains actual producer pixels");
        budget.set(budget.used());
        scene.clearGeneratedScene();
        require(texture->callbacks==1 && texture->detached && scene.getAssets().textures.empty() &&
                scene.getAssets().vehicle.appearance.textures.bodyLivery.pixels.empty(),
                "scene detaches generation and retires producer pixels before budget-denied texture callbacks");
        scene.clearGeneratedScene();
        require(texture->callbacks==1,"repeated scene retirement cannot redispatch the detached resource");
    }
}
