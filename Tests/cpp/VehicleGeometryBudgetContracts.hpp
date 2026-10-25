#pragma once
#include <Workphone/Interface/Procedural/VehicleGeometryTypes.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>
#include <WPProcedural/CVehicleDynamics.hpp>
#include <WPProcedural/CVehicleDamage.hpp>
#include <WPProcedural/CVehiclePresentation.hpp>
#include <Workphone/Scene/Components/ProceduralVehicle.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace vehicle_geometry_budget_contracts
{
    using namespace workphone;
    using namespace workphone::procedural;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t(StreamMemoryClass::SourceCpu);
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        { String error; require(scheduler->setBudget(StreamMemoryClass::SourceCpu,value,error),"vehicle geometry budget applies"); }
        void restore() { set(original); }
        ~Budget() { restore(); }
    };
    template<class T> inline u64 bytes( size_t capacity )
    { return capacity ? sourceBudgetStorageBytes(capacity*sizeof(T),alignof(T)) : 0; }
    template<class F> inline void denied( F operation, const char *message )
    { bool rejected=false;try{operation();}catch(const std::bad_alloc&){rejected=true;}require(rejected,message); }
    inline u64 nameBytes( const SourceBudgetStdString &name )
    {
        const SourceBudgetStdString empty;
        return name.capacity()>empty.capacity() ? bytes<char>(name.capacity()+1) : 0;
    }
    inline u64 resident( const VehicleMeshSection &section )
    { return nameBytes(section.name)+bytes<VehicleVertex>(section.vertices.capacity())+bytes<u32>(section.indices.capacity()); }
    inline u64 resident( const VehicleLOD &lod )
    {
        auto total=bytes<VehicleMeshSection>(lod.sections.capacity());
        for(const auto &section:lod.sections) total+=resident(section);
        return total;
    }
    inline u64 resident( const VehicleGeometry &geometry )
    {
        auto total=bytes<VehicleLOD>(geometry.lods.capacity());
        for(const auto &lod:geometry.lods) total+=resident(lod);
        return total;
    }
    inline void triangle( VehicleMeshSection &section, size_t vertexCapacity, size_t indexCapacity )
    {
        section.vertices.reserve(vertexCapacity);
        section.indices.reserve(indexCapacity);
        section.vertices.resize(3);
        section.vertices[0].position={0,0,0};
        section.vertices[1].position={1,0,0};
        section.vertices[2].position={0,1,0};
        section.indices.push_back(0);section.indices.push_back(1);section.indices.push_back(2);
    }
    template<class T> inline void bodyAdmission( Budget &budget )
    {
        const auto baseline=budget.used();
        const auto cost=sourceBudgetStorageBytes(sizeof(T),alignof(std::max_align_t));
        budget.set(baseline+cost-1);
        denied([]{auto candidate=std::make_unique<T>();},"owning geometry body requires complete Source admission");
        require(budget.used()==baseline,"denied geometry body leaves no lease");
        budget.set(baseline+cost);auto owner=std::make_unique<T>();
        require(budget.used()==baseline+cost,"empty owning geometry body has its own physical reservation");
        budget.set(budget.used());owner.reset();
        require(budget.used()==baseline,"geometry heap body retires at a full budget");
    }
    inline void inventories()
    {
        Budget budget;
        const auto baseline=budget.used();
        VehicleGeometry source;
        source.lods.reserve(5);source.lods.resize(2);
        source.boundsMin={-1,-2,-3};source.boundsMax={4,5,6};
        source.centreOfMass={7,8,9};source.frontAxle={10,11,12};
        source.rearAxle={13,14,15};source.cockpitEye={16,17,18};
        source.lods[0].level=2;source.lods[0].suggestedScreenCoverage=.125f;
        source.lods[0].sections.reserve(7);source.lods[0].sections.resize(2);
        auto &first=source.lods[0].sections[0];
        std::string foreignName;foreignName.reserve(137);foreignName="admitted panel";
        first.name=foreignName;first.material=VehicleMaterial::CarbonGloss;
        triangle(first,37,41);
        auto &second=source.lods[0].sections[1];
        second.name="another admitted material section";triangle(second,19,23);
        source.lods[1].sections.reserve(3);source.lods[1].sections.resize(1);
        triangle(source.lods[1].sections[0],11,13);
        const auto incoming=resident(source);
        require(budget.used()==baseline+incoming && first.name.capacity()>=foreignName.capacity(),
                "section names, spare vertices/indices and both outer geometry layers charge physical storage");
        const auto finalIndexStage=bytes<VehicleLOD>(source.lods.capacity())+
            bytes<VehicleMeshSection>(source.lods[0].sections.capacity())+nameBytes(first.name)+
            bytes<VehicleVertex>(first.vertices.capacity())+bytes<u32>(first.indices.capacity());
        VehicleGeometry previous;
        previous.boundsMin={91,92,93};previous.cockpitEye={94,95,96};
        previous.lods.reserve(3);previous.lods.resize(1);previous.lods[0].level=99;
        previous.lods[0].sections.reserve(2);previous.lods[0].sections.resize(1);
        previous.lods[0].sections[0].name="old";
        triangle(previous.lods[0].sections[0],5,7);
        const auto oldCost=resident(previous),beforeCopy=budget.used();
        const auto oldLODs=previous.lods.data();
        const auto oldVertices=previous.lods[0].sections[0].vertices.data();
        for(const auto room:{u64(0),bytes<VehicleLOD>(source.lods.capacity()),finalIndexStage-1,incoming-1})
        {
            budget.set(beforeCopy+room);
            denied([&]{previous=source;},"geometry copy must admit every nested owner before publication");
            require(budget.used()==beforeCopy && previous.lods.data()==oldLODs &&
                    previous.lods[0].sections[0].vertices.data()==oldVertices &&
                    previous.lods[0].sections[0].name=="old" && previous.lods[0].level==99 &&
                    previous.boundsMin.x==91 && previous.cockpitEye.z==96,
                    "partial geometry copy denial preserves old names, topology, spare storage and bounds");
        }
        budget.set(beforeCopy+incoming);previous=source;
        require(budget.used()==beforeCopy+incoming-oldCost && previous.lods.capacity()==5 &&
                previous.lods[0].sections.capacity()==7 && previous.lods[0].sections[0].vertices.capacity()==37 &&
                previous.lods[0].sections[0].indices.capacity()==41 &&
                previous.lods[0].sections[0].name.capacity()==first.name.capacity() &&
                previous.lods[0].sections[0].vertices.data()!=first.vertices.data() &&
                previous.boundsMin.z==-3 && previous.rearAxle.y==14 && previous.cockpitEye.z==18,
                "geometry retry publishes an independent complete generation with every physical spare capacity");
        budget.set(budget.used()+incoming);
        std::optional<VehicleGeometry> snapshot(previous);
        const auto retained=budget.used();budget.set(retained);
        const auto denials=sourceBudgetDenialCount();
        first.name.clear();first.vertices.clear();first.indices.clear();
        require(budget.used()==retained,"logical geometry clear retains all physical spare-capacity charges");
        source={};previous={};
        require(budget.used()==baseline+incoming && snapshot->hasGeometry() &&
                snapshot->lods[0].sections[0].vertices.size()==3 &&
                snapshot->lods[0].sections[0].indices[2]==2 &&
                snapshot->lods[0].sections[0].name=="admitted panel",
                "caller snapshot retains nested geometry after producer retirement");
        std::thread finalOwner([owned=std::move(snapshot)]() mutable {owned.reset();});
        finalOwner.join();snapshot.reset();
        require(budget.used()==baseline && sourceBudgetDenialCount()==denials,
                "last nested geometry owner can retire on another thread at a full budget");
        bodyAdmission<VehicleMeshSection>(budget);
        bodyAdmission<VehicleLOD>(budget);
        bodyAdmission<VehicleGeometry>(budget);
    }
    inline void generation()
    {
        Budget budget;
        const auto baseline=budget.used();
        for(const auto style:{VehicleBodyStyle::GrandPrix,VehicleBodyStyle::GT,VehicleBodyStyle::RoadSport,
                              VehicleBodyStyle::HelicopterLittleBird,VehicleBodyStyle::Warbird})
        {
            budget.restore();
            VehicleGeometryConfig config;config.bodyStyle=style;
            config.generateThreeLODs=false;config.includeFineDetails=false;
            auto geometry=WPVehicleGeometry::generate(config);
            const auto footprint=resident(geometry);
            require(geometry.hasGeometry() && budget.used()==baseline+footprint,
                    "generated vehicle output owns admitted geometry after all tangent/station/surface scratch retires");
            const auto old=geometry.lods.data();
            for(const auto room:{u64(0),bytes<VehicleLOD>(1),footprint/2,footprint-1})
            {
                budget.set(baseline+footprint+room);
                denied([&]{geometry=WPVehicleGeometry::generate(config);},
                       "partial vehicle generation requires output and overlapping scratch admission");
                require(geometry.lods.data()==old && geometry.hasGeometry() && budget.used()==baseline+footprint,
                        "failed generator stages unwind without replacing last-good geometry");
            }
            budget.restore();auto repeated=WPVehicleGeometry::generate(config);
            require(resident(repeated)==footprint && repeated.lods[0].sections.size()==geometry.lods[0].sections.size() &&
                    repeated.boundsMin.x==geometry.boundsMin.x && repeated.boundsMax.z==geometry.boundsMax.z,
                    "retry preserves each existing vehicle preset's geometry shape and ownership");
            for(size_t section=0;section<geometry.lods[0].sections.size();++section)
            {
                const auto &a=geometry.lods[0].sections[section],&b=repeated.lods[0].sections[section];
                require(a.indices==b.indices && a.vertices.size()==b.vertices.size() && a.name==b.name,
                        "budgeted generation preserves section names and complete indexed topology");
                for(size_t vertex=0;vertex<a.vertices.size();++vertex)
                    require(a.vertices[vertex].position==b.vertices[vertex].position &&
                            a.vertices[vertex].normal==b.vertices[vertex].normal &&
                            a.vertices[vertex].tangent==b.vertices[vertex].tangent &&
                            a.vertices[vertex].uv==b.vertices[vertex].uv,
                            "deterministic generation retains positions, normals, tangents and UVs");
            }
            budget.set(budget.used());const auto denials=sourceBudgetDenialCount();
            geometry={};repeated={};
            require(budget.used()==baseline && sourceBudgetDenialCount()==denials,
                    "each generated preset retires its complete admitted output at the cap");
        }
    }
    class Generator : public IVehicleGenerator
    {
    public:
        GeneratedVehicle value;
        GeneratedVehicle generate(const VehicleGenerationConfig &) override {return value;}
        Array<VehicleGenerationIssue> validate(const GeneratedVehicle &) override
        {return Array<VehicleGenerationIssue>(Allocator<VehicleGenerationIssue>(sourceBudget));}
        VehicleMaterialSlot materialSlotFor(VehicleMaterial) override {return VehicleMaterialSlot::BodyPaint;}
    };
    class Effects : public IVehicleEffects
    {
    public:
        bool foreign=true;
        VehicleEffectsState reset(std::uint64_t seed) override
        {VehicleEffectsState state;state.randomState=seed;return state;}
        bool emit(const VehicleEffectConfig &,const VehiclePhysicsConfig &,const VehicleDynamicsState &,
                  const VehicleDynamicsTelemetry &,const VehiclePresentationState &,
                  const VehicleDamageTelemetry &,const VehicleEffectEnvironment &,double,
                  VehicleEffectsState &state,Array<VehicleEffectEvent> &events) override
        {
            if(foreign)
            {
                Array<VehicleEffectEvent> output;
                output.reserve(37);output.resize(1);
                events=std::move(output);
            }
            else {events.reserve(37);events.resize(1);}
            events[0].intensity=double(++state.randomState);
            return true;
        }
    };
    class Component : public scene::ProceduralVehicle
    {
    public:
        using scene::ProceduralVehicle::buildMesh;
        auto &materials() {return m_generatedMaterials;}
        auto &events() {return m_events;}
        auto &scratch() {return m_eventScratch;}
        auto &vehicle() {return m_vehicle;}
        auto &effectState() {return m_effectsState;}
        bool detached() const
        {return m_generatedMaterials.empty() && m_events.empty() && m_eventScratch.empty() &&
                m_vehicle.geometry.lods.empty() && !m_generator && !m_dynamics && !m_damage &&
                !m_presentation && !m_effects;}
        void simulateWith(SmartPtr<Effects> effects)
        {
            m_vehicle.physics=WPVehiclePhysics::generate(VehiclePhysicsPreset::GrandPrix,73);
            m_vehicle.geometry.lods.resize(1);m_vehicle.geometry.lods[0].sections.resize(1);
            triangle(m_vehicle.geometry.lods[0].sections[0],3,3);
            m_dynamics=make_ptr<CVehicleDynamics>();m_damage=make_ptr<CVehicleDamage>();
            m_presentation=make_ptr<CVehiclePresentation>();m_effects=effects;
            m_dynamicsState=m_dynamics->reset(m_vehicle.physics);
            m_damageState=m_damage->reset(73);m_presentationState=m_presentation->reset(m_vehicle.physics);
            m_effectsState=m_effects->reset(73);
        }
    };
    class RetiringMaterial : public scene::Material
    {
    public:
        Component *owner=nullptr;
        int callbacks=0;
        bool detached=false,reenter=false;
        void unload(SmartPtr<ISharedObject>) override
        {
            ++callbacks;detached=owner && owner->detached();
            if(reenter) owner->unload(nullptr);
            throw std::bad_alloc();
        }
    };
    inline void component()
    {
        Budget budget;
        {
            Component component;
            auto generator=make_ptr<Generator>();
            generator->value.geometry.lods.resize(1);
            generator->value.geometry.lods[0].sections.resize(1);
            triangle(generator->value.geometry.lods[0].sections[0],3,3);
            generator->value.contentHash=11;
            component.setGenerator(generator);
            auto mesh=component.buildMesh();
            require(mesh && mesh->getNumSubMeshes()==1 && component.getGeneratedVehicle().contentHash==11,
                    "scene adapter builds executable mesh geometry from admitted vehicle payloads");
            const auto oldGeometry=component.getGeneratedVehicle().geometry.lods.data();
            const auto baseline=budget.used(),incoming=resident(generator->value.geometry);
            generator->value.contentHash=22;
            const auto scratch=bytes<u32>(3)+3*bytes<Vector3<real_Num>>(3)+bytes<Vector4F>(3);
            const auto meshBody=sourceBudgetStorageBytes(sizeof(workphone::Mesh),alignof(std::max_align_t));
            for(const auto room:{u64(0),incoming/2,incoming-1,incoming+meshBody+scratch-1})
            {
                budget.set(baseline+room);
                denied([&]{auto candidate=component.buildMesh();},
                       "scene conversion must admit generated data, mesh wrappers and every overlapping staging array");
                require(component.getGeneratedVehicle().contentHash==11 &&
                        component.getGeneratedVehicle().geometry.lods.data()==oldGeometry &&
                        mesh->getNumSubMeshes()==1 && budget.used()==baseline,
                        "denied scene conversion preserves the published producer and previous executable mesh");
            }
            budget.restore();auto retry=component.buildMesh();
            require(retry && retry!=mesh && component.getGeneratedVehicle().contentHash==22,
                    "scene mesh conversion retries once staging capacity returns");
            budget.set(budget.used());component.unload(nullptr);
            require(component.detached(),"scene adapter unload detaches generated payloads and services");
        }
        budget.restore();
        {
            Component component;
            auto effects=make_ptr<Effects>();component.simulateWith(effects);
            component.events().reserve(11);component.events().resize(1);component.events()[0].intensity=.42;
            const auto oldEvents=component.getEffects().data();
            const auto oldTick=component.getDynamicsState().simulationTick;
            const auto oldRandom=component.effectState().randomState;
            const auto before=budget.used();budget.set(before);
            VehicleControlInput input;input.throttle=.8;
            std::array<VehicleSurfaceSample,4> surfaces;
            denied([&]{component.stepFixed(input,surfaces,1.0/120.0);},
                   "foreign effect output must admit complete retained spare capacity before tick publication");
            require(component.getEffects().data()==oldEvents && component.getEffects()[0].intensity==.42 &&
                    component.getDynamicsState().simulationTick==oldTick &&
                    component.effectState().randomState==oldRandom && budget.used()==before,
                    "denied callback-output import preserves previous events, simulation tick and RNG state");
            const auto eventBytes=bytes<VehicleEffectEvent>(37);
            budget.set(before+eventBytes);
            require(component.stepFixed(input,surfaces,1.0/120.0) &&
                    component.getEffects().get_allocator().isSourceBudgeted() &&
                    component.getEffects().capacity()==37 &&
                    component.getDynamicsState().simulationTick==oldTick+1 &&
                    component.effectState().randomState==oldRandom+1,
                    "callback output canonicalizes spare capacity and commits a complete simulation generation");
            budget.restore();effects->foreign=false;
            require(component.stepFixed(input,surfaces,1.0/120.0),"second event buffer warms normally");
            std::optional<Array<VehicleEffectEvent>> escaped(component.getEffects());
            const auto oldIntensity=escaped->at(0).intensity;
            const auto warm=budget.used(),denials=sourceBudgetDenialCount();budget.set(warm);
            require(component.stepFixed(input,surfaces,1.0/120.0) && budget.used()==warm &&
                    sourceBudgetDenialCount()==denials && escaped->at(0).intensity==oldIntensity &&
                    component.getEffects()[0].intensity!=oldIntensity,
                    "warm fixed-step publication reuses admitted buffers at a full budget while snapshots stay immutable");
            component.unload(nullptr);
            require(component.detached() && component.events().capacity()==0 && component.scratch().capacity()==0 &&
                    escaped->capacity()==37 && escaped->at(0).intensity==oldIntensity,
                    "component unload retires both event generations while caller snapshots retain physical ownership");
            escaped.reset();
        }
        budget.restore();
        {
            Component component;
            auto first=make_ptr<RetiringMaterial>(),second=make_ptr<RetiringMaterial>();
            first->owner=&component;first->reenter=true;second->owner=&component;
            component.materials().reserve(7);component.materials().push_back(first);component.materials().push_back(second);
            component.events().reserve(11);component.events().resize(1);component.scratch().reserve(13);
            component.vehicle().appearance.textures.bodyLivery=TextureBuffer(128,128);
            budget.set(budget.used());component.unload(nullptr);
            require(first->callbacks==1 && second->callbacks==1 && first->detached && second->detached &&
                    component.detached() && component.materials().capacity()==0 && component.events().capacity()==0 &&
                    component.scratch().capacity()==0,
                    "scene inventory detaches before reentrant denied notifications and all later owners still retire");
            component.unload(nullptr);
            require(first->callbacks==1 && second->callbacks==1,"repeated unload cannot redispatch detached material owners");
        }
    }
    inline void run() { inventories();generation();component(); }
}
