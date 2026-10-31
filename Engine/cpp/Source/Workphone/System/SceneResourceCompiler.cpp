#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Jobs/SceneLoadJob.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <locale>
#include <set>
#include <cmath>
#include <cstring>
#include <limits>

namespace workphone::resource
{
    // Bases retire in reverse declaration order: the Properties storage must
    // release before its shared conservative metadata reservation does.
    struct ActorGraphLeaseOwner
    {
        StreamBudget lease;
        ActorGraphLeaseOwner()=default;
        explicit ActorGraphLeaseOwner(StreamBudget value):lease(std::move(value)){}
    };
    class LeasedActorGraphProperties final : private ActorGraphLeaseOwner, public Properties
    {
    public:
        LeasedActorGraphProperties()=default;
        explicit LeasedActorGraphProperties(StreamBudget lease):ActorGraphLeaseOwner(std::move(lease)){}
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED(workphone::resource,LeasedActorGraphProperties,Properties);
    namespace
    {
        constexpr size_t maximumBytes=8ull<<20, maximumNodes=4096, maximumDepth=64;
        void require(bool condition,const char *error) { if(!condition) throw std::runtime_error(error); }
        String text(const rapidjson::Value &value)
        {
            require(value.IsString()&&value.GetStringLength()<=4096,"Actor graph fields require bounded strings");
            String result(value.GetString(),value.GetStringLength());
            require(result.find('\0')==String::npos,"Actor graph contains NUL");
            for(size_t at=0;at<result.size();) {
                const auto lead=static_cast<unsigned char>(result[at++]); if(lead<0x80) continue;
                unsigned count=lead>=0xc2&&lead<=0xdf?1:lead>=0xe0&&lead<=0xef?2:lead>=0xf0&&lead<=0xf4?3:0;
                require(count&&count<=result.size()-at,"Invalid UTF-8 actor graph string");
                u32 code=lead&((1u<<(6-count))-1u);
                for(unsigned i=0;i<count;++i) { const auto next=static_cast<unsigned char>(result[at++]);require((next&0xc0)==0x80,"Invalid UTF-8 continuation");code=(code<<6)|(next&0x3f); }
                require((count!=2||code>=0x800)&&(count!=3||code>=0x10000)&&code<=0x10ffff&&!(code>=0xd800&&code<=0xdfff),"Noncanonical UTF-8 actor graph string");
            }
            return result;
        }
        String uuid(const String &value)
        {
            require(value.size()==36,"Actor graph requires canonical UUIDs");
            bool nonzero=false;
            for(size_t i=0;i<value.size();++i) {
                const auto c=value[i];
                if(i==8||i==13||i==18||i==23) require(c=='-',"Invalid actor UUID separators");
                else { require((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'),"Invalid actor UUID"); nonzero|=c!='0'; }
            }
            require(nonzero,"Nil actor UUID is unsupported"); return StringUtil::make_lower(value);
        }
        void vector(const String &value,size_t count,bool orientation=false)
        {
            std::istringstream stream(value.c_str()); stream.imbue(std::locale::classic());
            double norm=0;
            for(size_t i=0;i<count;++i) { double number=0; require(bool(stream>>number)&&std::isfinite(number)&&std::abs(number)<=std::numeric_limits<real_Num>::max(),"Invalid or overflowing actor transform"); if(orientation)norm+=number*number; }
            stream>>std::ws; require(stream.eof(),"Actor transform has trailing values");
            if(orientation) require(std::abs(norm-1.0)<0.001,"Actor orientation must be a unit quaternion");
        }
        enum class Role { Scene,Actor,Transform };
        using JSON=rapidjson::Value;
        SmartPtr<Properties> parseNode(const JSON &node,Role role,size_t depth,size_t &nodes,std::set<String> &identities,
                                      const StreamBudget &lease={})
        {
            require(node.IsObject()&&depth<=maximumDepth&&++nodes<=maximumNodes,"Actor graph exceeds object/depth limits");
            SmartPtr<Properties> result;
            if(lease) result=make_ptr<LeasedActorGraphProperties>(lease);
            else result=make_ptr<Properties>();
            std::set<String> seen;
            const std::set<String> actorFields={"uuid","name","label","static","enabled","visible","perpetual","smoothMotion","collisionMask","layer","tags"};
            for(auto i=node.MemberBegin();i!=node.MemberEnd();++i) {
                const auto key=text(i->name); require(seen.insert(key).second,"Duplicate actor graph field");
                if(key=="handle_name") { result->setName(text(i->value)); continue; }
                if(key=="children") { require(i->value.IsArray()&&i->value.Size()<=maximumNodes,"Invalid actor children"); continue; }
                if(key=="__wp_properties_v1") {
                    require(i->value.IsArray()&&i->value.Size()<=32,"Actor metadata exceeds limits");
                    for(const auto &entry:i->value.GetArray()) {
                        require(entry.IsObject()&&entry.MemberCount()==4,"Invalid actor metadata"); std::set<String> fields;
                        for(auto j=entry.MemberBegin();j!=entry.MemberEnd();++j) {
                            const auto field=text(j->name); require(fields.insert(field).second,"Duplicate actor metadata field");
                            if(field=="name"||field=="type") (void)text(j->value);
                            else if(field=="readOnly") require(j->value.IsBool(),"Invalid actor metadata flag");
                            else if(field=="attributes") {
                                require(j->value.IsObject()&&j->value.MemberCount()<=16,"Invalid actor attributes"); std::set<String> attributes;
                                for(auto k=j->value.MemberBegin();k!=j->value.MemberEnd();++k) { require(attributes.insert(text(k->name)).second,"Duplicate actor attribute"); (void)text(k->value); }
                            } else require(false,"Unknown actor metadata field");
                        }
                    }
                    continue; // Authoring type/Inspector attributes are not runtime object bindings.
                }
                const auto value=text(i->value);
                if(role==Role::Scene) { require(key=="lighting"&&value.empty(),"Scene property requires an unsupported typed installer"); continue; }
                if(role==Role::Transform) {
                    require(key=="Position"||key=="Scale"||key=="Orientation","Unknown transform field");
                    vector(value,key=="Orientation"?4:3,key=="Orientation");
                } else {
                    require(actorFields.count(key)!=0,"Actor property requires an unsupported typed installer");
                    if(key=="uuid") require(identities.insert(uuid(value)).second,"Duplicate actor UUID");
                    if(key=="static"||key=="enabled"||key=="visible"||key=="perpetual"||key=="smoothMotion")
                        require(value=="0"||value=="1"||value=="true"||value=="false","Invalid actor boolean");
                    if(key=="collisionMask") { require(!value.empty()&&value.size()<=10&&value.find_first_not_of("0123456789")==String::npos,"Invalid collision mask"); require(std::stoull(value.c_str())<=0xffffffffull,"Collision mask overflow"); }
                }
                result->setProperty(key,key=="uuid"?uuid(value):value);
            }
            require(seen.count("children")!=0,"Actor graph requires children array");
            if(role==Role::Actor) require(seen.count("uuid")!=0,"Actor graph requires durable actor UUIDs");
            std::set<String> transforms;
            for(const auto &child:node["children"].GetArray()) {
                require(child.IsObject()&&child.HasMember("handle_name"),"Actor child requires a group name");
                const auto name=text(child["handle_name"]); Role childRole=Role::Actor;
                if(role==Role::Scene) require(name=="actors","Scene child requires an unsupported typed installer");
                else if(role==Role::Actor) {
                    if(name=="localTransform"||name=="worldTransform") { require(transforms.insert(name).second,"Duplicate actor transform"); childRole=Role::Transform; }
                    else require(name=="child","Actor components require typed runtime integration");
                } else require(false,"Transform cannot contain child groups");
                result->addChild(parseNode(child,childRole,depth+1,nodes,identities,lease));
            }
            return result;
        }
        SmartPtr<Properties> parseJSON(const String &bytes,bool scene)
        {
            require(bytes.size()<=maximumBytes&&bytes.find('\0')==String::npos,"Actor graph exceeds byte limit or contains NUL");
            rapidjson::Document doc; doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(bytes.data(),bytes.size());
            require(!doc.HasParseError(),"Invalid UTF-8 actor graph JSON"); size_t nodes=0; std::set<String> identities;
            return parseNode(doc,scene?Role::Scene:Role::Actor,0,nodes,identities);
        }
        struct ParsedSource { StreamBudget budget; SmartPtr<Properties> data; };
        ParsedSource source(const CompileContext &context)
        {
            require(!context.resourceId.isSubResource(),"Actor graph sub-resource selectors are unsupported");
            std::ifstream file(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
            const auto size=file?file.tellg():std::streampos(-1);
            require(size>0&&size<=maximumBytes,"Invalid actor graph source size");
            String error;
            auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,
                static_cast<u64>(size)*48+(1ull<<20),error);
            require(budget!=nullptr,"Actor graph source preparation budget exhausted");
            String bytes(static_cast<size_t>(size),'\0'); file.seekg(0); file.read(bytes.data(),bytes.size());
            require(bool(file),"Incomplete actor graph source read");
            return {std::move(budget),parseJSON(bytes,context.resourceId.type()==ResourceTypeID("fbscene"))};
        }
        // Allocation-free validation of the existing Properties binary envelope
        // before its generic reader may allocate property/child arrays.
        struct Preflight
        {
            const Array<u8> &bytes; size_t at=20,nodes=0;
            u32 number() { require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated binary actor graph"); u32 value=0; for(int i=0;i<4;++i)value|=u32(bytes[at++])<<(8*i); return value; }
            void string() { auto size=number(); require(size<=4096&&size<=bytes.size()-at,"Binary actor string exceeds limit"); require(!size||!std::memchr(bytes.data()+at,0,size),"NUL in binary actor string"); at+=size; }
            void node(size_t depth) {
                require(depth<=maximumDepth&&++nodes<=maximumNodes,"Binary actor graph exceeds limits"); string();
                auto fields=number(); require(fields<=32,"Binary actor field limit exceeded");
                for(u32 i=0;i<fields;++i) { string();string();string();require(at<bytes.size(),"Truncated actor flag"); require(bytes[at++]<=1,"Invalid actor flag"); auto attributes=number();require(attributes<=16,"Actor attribute limit exceeded");for(u32 a=0;a<attributes;++a){string();string();} }
                auto children=number(); require(children<=maximumNodes,"Actor child limit exceeded"); for(u32 i=0;i<children;++i)node(depth+1);
            }
        };
        void asJSON(const Properties &data,JSON &out,rapidjson::Document::AllocatorType &allocator)
        {
            out.SetObject(); JSON name; const auto group=data.getName(); name.SetString(group.data(),static_cast<rapidjson::SizeType>(group.size()),allocator); out.AddMember("handle_name",name,allocator);
            for(const auto &property:data.getPropertiesAsArray()) {
                JSON key,value; const auto field=property.getName(), text=property.getValue();
                key.SetString(field.data(),static_cast<rapidjson::SizeType>(field.size()),allocator); value.SetString(text.data(),static_cast<rapidjson::SizeType>(text.size()),allocator); out.AddMember(key,value,allocator);
            }
            JSON children(rapidjson::kArrayType); for(const auto &child:data.getChildren()) { require(child!=nullptr,"Null actor group"); JSON value; asJSON(*child,value,allocator);children.PushBack(value,allocator); } out.AddMember("children",children,allocator);
        }
    }
    class CookedActorGraph
    {
    public:
        std::shared_ptr<const RuntimeResource> resource;
        StreamBudget metadataBudget;
        SmartPtr<Properties> data;
        bool scene=false;
    };
    String SceneResourceCompiler::name() const { return "WorkphoneActorGraphBinary"; }
    Array<CompilerOutput> SceneResourceCompiler::outputs() const { return {{ResourceTypeID("fbscene"),1},{ResourceTypeID("prefab"),1}}; }
    bool SceneResourceCompiler::getDependencies(const CompileContext &context,DependencySet &,String &error) const
    { error.clear();try { (void)source(context);return true; }catch(const std::exception &e){error=e.what();return false;} }
    CompilationStatus SceneResourceCompiler::compile(const CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try { auto data=source(context);const auto bytes=PropertiesBinarySerializer::serialize(*data.data);require(bytes.size()<=maximumBytes,"Cooked actor graph exceeds byte limit");output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());return output?CompilationStatus::Success:CompilationStatus::Failure; }
        catch(const std::exception &e){messages.push_back(e.what());return CompilationStatus::Failure;}
    }
    std::shared_ptr<const CookedActorGraph> decodeActorGraphResource(std::shared_ptr<const RuntimeResource> resource,String &error)
    {
        error.clear();try {
            require(resource!=nullptr,"Missing cooked actor graph"); const auto type=resource->header.resourceType; const SceneResourceCompiler compiler;
            require(resource->header.isValid()&&(type==ResourceTypeID("fbscene")||type==ResourceTypeID("prefab"))&&!resource->header.resourceId.isSubResource()&&resource->header.resourceId.type()==type&&resource->header.compilerVersion==compiler.versionFor(type),"Unsupported cooked actor graph header/type/version/selector");
            require(resource->header.installDependencies.empty()&&resource->dependencies.empty(),"Actor graph requires unsupported typed dependencies");
            const auto &bytes=resource->payload;
            require(bytes.size()>=20&&bytes.size()<=maximumBytes,"Invalid binary actor graph size");
            require(resource->header.payloadSize==bytes.size()&&hashBytes(bytes.data(),bytes.size())==resource->header.payloadHash,"Cooked actor graph payload integrity mismatch");
            require(PropertiesBinarySerializer::hasBinaryHeader(bytes.data(),bytes.size()),"Invalid binary actor graph envelope");
            Preflight preflight{bytes};preflight.node(0);require(preflight.at==bytes.size(),"Trailing binary actor graph data");
            auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,
                bytes.size()*48+(1ull<<20),error);require(budget!=nullptr,"Actor graph decode budget exhausted");
            auto retained=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,
                bytes.size()*48+(1ull<<20),error);require(retained!=nullptr,"Actor graph metadata residency budget exhausted");
            auto data=make_ptr<Properties>();require(PropertiesBinarySerializer::deserialize(bytes,*data,&error),"Invalid binary actor graph");
            rapidjson::Document doc; asJSON(*data,doc,doc.GetAllocator());size_t nodes=0;std::set<String> identities;
            auto result=std::make_shared<CookedActorGraph>();result->resource=std::move(resource);result->metadataBudget=retained;result->scene=type==ResourceTypeID("fbscene");result->data=parseNode(doc,result->scene?Role::Scene:Role::Actor,0,nodes,identities,retained);return result;
        } catch(const std::exception &e){error=e.what();return {};}
    }
    bool installCookedSceneGraph(std::shared_ptr<const CookedActorGraph> graph,SmartPtr<scene::IGameScene> target,String &error)
    {
        SmartPtr<scene::GameScene> concrete;u64 generation=0;bool previousReady=false;
        error.clear();try {
            require(graph&&graph->scene&&target&&target->isLoaded(),"Cooked scene requires a loaded target scene");
            concrete=dynamic_pointer_cast<scene::GameScene>(target);require(concrete!=nullptr,"Cooked scene requires the native GameScene loader");
            SmartPtr<SceneLoadJob> job;
            {
                ScopedLock lock(concrete);
                previousReady=target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded;
                generation=concrete->beginSceneLoad();
                job=make_ptr<SceneLoadJob>();job->setScene(target);job->setLightingDirector(target->getLightingDirector());job->setPreparedData(graph->data);
            }
            job->execute();
            bool committed=false;
            {
                ScopedLock lock(concrete);
                committed=concrete->getLoadGeneration()==generation&&target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded;
            }
            require(committed,"Cooked scene graph commit failed or was superseded");return true;
        }catch(const std::exception &e){
            if(concrete&&previousReady)concrete->restoreReadySceneAfterFailedAppend(generation);
            error=e.what();return false;
        }
    }
    SmartPtr<scene::IGamePrefab> installCookedPrefabGraph(std::shared_ptr<const CookedActorGraph> graph,SmartPtr<scene::GamePrefabManager> manager,String &error)
    {
        error.clear();try {
            require(graph&&!graph->scene&&manager,"Cooked prefab requires the native prefab manager");
            const auto bound=graph->resource->payload.size()*48+(1ull<<20);
            auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,bound,error);
            require(scratch!=nullptr,"Prefab clone preparation budget exhausted");
            auto retained=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,bound,error);
            require(retained!=nullptr,"Prefab clone residency budget exhausted");
            auto copy=scene::GameActorUtil::createInstanceData(graph->data);require(copy!=nullptr,"Cannot clone cooked prefab graph");
            rapidjson::Document doc;asJSON(*copy,doc,doc.GetAllocator());size_t nodes=0;std::set<String> identities;
            auto data=parseNode(doc,Role::Actor,0,nodes,identities,retained);
            auto prefab=dynamic_pointer_cast<scene::IGamePrefab>(manager->create(StringUtil::getUUID()));require(prefab!=nullptr,"Cannot create cooked prefab");prefab->setData(data);return prefab;
        }catch(const std::exception &e){error=e.what();return {};}
    }
}
