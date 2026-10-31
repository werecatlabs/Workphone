#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Jobs/SceneLoadJob.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
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
        enum class Role { Scene,Actor,Transform,Components,Component };
        using JSON=rapidjson::Value;
        const String bindingAttribute="compiledResourceId";
        String componentName(const String &name)
        {
            for(const auto &supported:Array<String>{"Mesh","MeshRenderer","Animator","Script"})
                if(name==supported||name=="workphone::scene::"+supported||name=="fb::scene::"+supported)
                    return supported;
            throw std::runtime_error("Unsupported cooked component type: "+std::string(name.c_str()));
        }
        bool assetField(const String &component,const String &field)
        {
            return (component=="Mesh"&&(field=="Mesh Resource"||field=="Skeleton"))||
                   (component=="MeshRenderer"&&field=="meshAssetUuid")||
                   (component=="Animator"&&field=="animationAssetUuid")||
                   (component=="Script"&&field=="scriptAssetUuid");
        }
        bool assetType(const String &component,const String &field,const ResourceTypeID &type)
        {
            if(component=="Mesh"&&field=="Skeleton"||component=="Animator"&&field=="animationAssetUuid")return type==ResourceTypeID("skelres");
            if(component=="Script"&&field=="scriptAssetUuid")return type==ResourceTypeID("lua");
            return (component=="Mesh"&&field=="Mesh Resource"||component=="MeshRenderer"&&field=="meshAssetUuid")&&
                (type==ResourceTypeID("mesh")||type==ResourceTypeID("skinres"));
        }
        void boolean(const String &value)
        { require(value=="0"||value=="1"||value=="true"||value=="false","Invalid cooked component boolean"); }
        void integer(const String &value,u64 maximum=0xffffffffull)
        {
            require(!value.empty()&&value.size()<=10&&value.find_first_not_of("0123456789")==String::npos,"Invalid cooked component integer");
            const auto parsed=std::stoull(value.c_str());require(parsed<=maximum,"Cooked component integer overflow");require(value==std::to_string(parsed).c_str(),"Noncanonical cooked component integer");
        }
        void scalar(const String &value,bool nonnegative=false)
        {
            std::istringstream stream(value.c_str());stream.imbue(std::locale::classic());double number=0;
            require(bool(stream>>number)&&std::isfinite(number)&&std::abs(number)<=std::numeric_limits<f32>::max()&&(!nonnegative||number>=0),"Invalid cooked component scalar");
            stream>>std::ws;require(stream.eof(),"Cooked component scalar has trailing values");
        }
        void componentField(const String &component,const String &key,const String &value)
        {
            if(key=="uuid"||key=="name"||key=="componentType")return;
            if(key=="Enabled"){boolean(value);return;}
            if(key=="componentFlags"){integer(value);return;}
            if(key=="state"){integer(value,5);require(value!="2","Destroyed components cannot be cooked");return;}
            if(assetField(component,key)){if(!value.empty())(void)uuid(value);return;}
            if(component=="Mesh") {
                if(key=="Mesh Path"){require(value.empty(),"Cooked Mesh requires a UUID, not Mesh Path");return;}
                if(key=="FSM Priority"){integer(value,0x7fffffffull);return;}
            }
            if(component=="MeshRenderer") {
                if(key=="materialName"){require(value.empty(),"Cooked renderer cannot load a loose material name");return;}
                if(key=="castShadows"||key=="reflections"||key=="occulsion"){integer(value,3);return;}
                if(key=="recieveShadows"){integer(value,1);return;}
                if(key=="zOrder"||key=="visibilityFlags"){integer(value);return;}
                if(key=="Update Materials"||key=="Update Bounds"){boolean(value);require(value=="0"||value=="false","Cooked renderer cannot invoke authoring actions");return;}
                if(key=="boundingBox")return; // Derived authoring display, never applied by Renderer.
            }
            if(component=="Script") {
                if(key=="className"){require(!value.empty()&&value.size()<=256,"Cooked Script requires a bounded className");return;}
                if(key=="updateInEditMode"||key=="updateInPlayMode"){boolean(value);return;}
            }
            if(component=="Animator") {
                if(key=="animationCachePath"){require(value.empty(),"Cooked Animator cannot open animationCachePath");return;}
                if(key=="looping"||key=="autoPlay"||key=="resetTimeOnPlay"||key=="resetTimeOnStop"||key=="clampTimeToClip"||key=="applyOnPropertyChange"){boolean(value);return;}
                if(key=="animationTime"||key=="animationSpeed"||key=="animationWeight"||key=="animationScale"){scalar(value,true);return;}
                if(key=="selectedAnimationIndex"){integer(value,4095);return;}
                if(key=="playbackState"){integer(value,2);return;}
                if(key=="Play"||key=="Pause"||key=="Stop"||key=="Rewind"){boolean(value);require(value=="0"||value=="false","Cooked Animator cannot invoke authoring actions");return;}
                if(key=="playbackStatus"||key=="selectedAnimationName"||key=="selectedAnimationLength"||key=="runtimeClipCount"||key=="editableClipCount")return;
            }
            throw std::runtime_error("Unsupported cooked "+std::string(component.c_str())+" property: "+key.c_str());
        }
        SmartPtr<Properties> parseNode(const JSON &node,Role role,size_t depth,size_t &nodes,std::set<String> &identities,
                                      const StreamBudget &lease={},bool cooked=false)
        {
            require(node.IsObject()&&depth<=maximumDepth&&++nodes<=maximumNodes,"Actor graph exceeds object/depth limits");
            SmartPtr<Properties> result;
            if(lease) result=make_ptr<LeasedActorGraphProperties>(lease);
            else result=make_ptr<Properties>();
            std::set<String> seen;
            std::map<String,String> bindings;
            String component;
            if(role==Role::Component) {
                require(node.HasMember("componentType"),"Cooked component requires componentType");component=componentName(text(node["componentType"]));
            }
            const std::set<String> actorFields={"uuid","name","label","static","enabled","visible","perpetual","smoothMotion","collisionMask","layer","tags"};
            for(auto i=node.MemberBegin();i!=node.MemberEnd();++i) {
                const auto key=text(i->name); require(seen.insert(key).second,"Duplicate actor graph field");
                if(key=="handle_name") { result->setName(text(i->value)); continue; }
                if(key=="children") { require(i->value.IsArray()&&i->value.Size()<=maximumNodes,"Invalid actor children"); continue; }
                if(key=="__wp_properties_v1") {
                    require(i->value.IsArray()&&i->value.Size()<=32,"Actor metadata exceeds limits");
                    for(const auto &entry:i->value.GetArray()) {
                        require(entry.IsObject()&&entry.MemberCount()==4,"Invalid actor metadata"); std::set<String> fields;
                        String propertyName;const JSON *attributesValue=nullptr;
                        for(auto j=entry.MemberBegin();j!=entry.MemberEnd();++j) {
                            const auto field=text(j->name); require(fields.insert(field).second,"Duplicate actor metadata field");
                            if(field=="name") propertyName=text(j->value);
                            else if(field=="type") (void)text(j->value);
                            else if(field=="readOnly") require(j->value.IsBool(),"Invalid actor metadata flag");
                            else if(field=="attributes") {
                                attributesValue=&j->value;
                                require(j->value.IsObject()&&j->value.MemberCount()<=16,"Invalid actor attributes"); std::set<String> attributes;
                                for(auto k=j->value.MemberBegin();k!=j->value.MemberEnd();++k) { require(attributes.insert(text(k->name)).second,"Duplicate actor attribute"); (void)text(k->value); }
                            } else require(false,"Unknown actor metadata field");
                        }
                        if(cooked&&attributesValue&&attributesValue->HasMember(bindingAttribute.c_str()))
                            require(bindings.emplace(propertyName,text((*attributesValue)[bindingAttribute.c_str()])).second,"Duplicate cooked asset binding");
                    }
                    continue; // Authoring type/Inspector attributes are not runtime object bindings.
                }
                const auto value=text(i->value);
                if(role==Role::Scene) { require(key=="lighting"&&value.empty(),"Scene property requires an unsupported typed installer"); continue; }
                if(role==Role::Components)require(false,"Component container cannot contain properties");
                if(role==Role::Transform) {
                    require(key=="Position"||key=="Scale"||key=="Orientation","Unknown transform field");
                    vector(value,key=="Orientation"?4:3,key=="Orientation");
                } else if(role==Role::Actor) {
                    require(actorFields.count(key)!=0,"Actor property requires an unsupported typed installer");
                    if(key=="uuid") require(identities.insert(uuid(value)).second,"Duplicate actor UUID");
                    if(key=="static"||key=="enabled"||key=="visible"||key=="perpetual"||key=="smoothMotion")
                        require(value=="0"||value=="1"||value=="true"||value=="false","Invalid actor boolean");
                    if(key=="collisionMask") { require(!value.empty()&&value.size()<=10&&value.find_first_not_of("0123456789")==String::npos,"Invalid collision mask"); require(std::stoull(value.c_str())<=0xffffffffull,"Collision mask overflow"); }
                } else if(role==Role::Component) {
                    componentField(component,key,value);
                    if(key=="uuid")require(identities.insert(uuid(value)).second,"Duplicate component UUID");
                }
                result->setProperty(key,key=="uuid"||assetField(component,key)&&!value.empty()?uuid(value):key=="componentType"?"workphone::scene::"+component:value);
            }
            require(seen.count("children")!=0,"Actor graph requires children array");
            if(role==Role::Actor) require(seen.count("uuid")!=0,"Actor graph requires durable actor UUIDs");
            if(role==Role::Component) {
                require(seen.count("uuid")!=0,"Cooked components require durable UUIDs");
                if(component=="Script")require(seen.count("className")&&seen.count("scriptAssetUuid")&&!result->getProperty("scriptAssetUuid").empty(),"Cooked Script requires className and scriptAssetUuid");
            }
            for(const auto &binding:bindings) {
                require(role==Role::Component&&assetField(component,binding.first)&&seen.count(binding.first)&&!result->getProperty(binding.first).empty(),"Unexpected cooked asset binding");
                ResourceID id(binding.second);require(id.isValid()&&!id.isSubResource()&&assetType(component,binding.first,id.type()),"Cooked asset binding has the wrong type/selector");
                result->getPropertyObject(binding.first).setAttribute(bindingAttribute,id.str());
            }
            std::set<String> transforms;
            for(const auto &child:node["children"].GetArray()) {
                require(child.IsObject()&&child.HasMember("handle_name"),"Actor child requires a group name");
                const auto name=text(child["handle_name"]); Role childRole=Role::Actor;
                if(role==Role::Scene) require(name=="actors","Scene child requires an unsupported typed installer");
                else if(role==Role::Actor) {
                    if(name=="localTransform"||name=="worldTransform") { require(transforms.insert(name).second,"Duplicate actor transform"); childRole=Role::Transform; }
                    else if(name=="component"||name=="components")childRole=child.HasMember("componentType")?Role::Component:Role::Components;
                    else require(name=="child","Actor child requires an unsupported typed installer");
                } else if(role==Role::Components) {
                    require(name=="component"&&child.HasMember("componentType"),"Invalid cooked component container");childRole=Role::Component;
                } else require(false,"Transform cannot contain child groups");
                result->addChild(parseNode(child,childRole,depth+1,nodes,identities,lease,cooked));
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
        using AssetBindings=std::map<String,ResourceID>;
        void inspectAssets(const SmartPtr<Properties> &data,const std::function<void(Properties &,const String &,const String &,const String &)> &inspect)
        {
            if(data->hasProperty("componentType")) {
                const auto component=componentName(data->getProperty("componentType"));
                for(const auto &property:data->getPropertiesAsArray())
                    if(assetField(component,property.getName())&&!property.getValue().empty())inspect(*data,component,property.getName(),uuid(property.getValue()));
            }
            for(const auto &child:data->getChildren())inspectAssets(child,inspect);
        }
        void inspectRelationships(const SmartPtr<Properties> &data)
        {
            if(data->hasProperty("uuid")&&!data->hasProperty("componentType")) {
                std::map<String,SmartPtr<Properties>> components;
                auto collect=[&](const SmartPtr<Properties> &child) {
                    const auto component=componentName(child->getProperty("componentType"));
                    require(components.emplace(component,child).second,"Duplicate cooked component type on an actor");
                };
                for(const auto &child:data->getChildren()) {
                    if(child->hasProperty("componentType"))collect(child);
                    else if(child->getName()=="components")for(const auto &component:child->getChildren())collect(component);
                }
                const auto mesh=components.find("Mesh");
                auto meshRef=[&](const String &field) { return mesh!=components.end()&&!mesh->second->getProperty(field).empty(); };
                if(mesh!=components.end())require(meshRef("Mesh Resource"),"Cooked Mesh requires a typed mesh UUID");
                if(auto renderer=components.find("MeshRenderer");renderer!=components.end())
                    require(!renderer->second->getProperty("meshAssetUuid").empty()||meshRef("Mesh Resource"),"Cooked MeshRenderer requires meshAssetUuid or a typed Mesh sibling");
                if(auto animator=components.find("Animator");animator!=components.end())
                    require(!animator->second->getProperty("animationAssetUuid").empty()||meshRef("Skeleton"),"Cooked Animator requires animationAssetUuid or a typed Mesh skeleton");
            }
            for(const auto &child:data->getChildren())inspectRelationships(child);
        }
        struct ParsedSource { StreamBudget budget; SmartPtr<Properties> data; AssetBindings bindings; };
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
            auto data=parseJSON(bytes,context.resourceId.type()==ResourceTypeID("fbscene"));inspectRelationships(data);
            AssetBindings bindings;
            std::set<String> objectIdentities;
            auto collectIdentities=[&](auto &&self,const SmartPtr<Properties> &node)->void {
                if(node->hasProperty("uuid"))objectIdentities.insert(node->getProperty("uuid"));
                for(const auto &child:node->getChildren())self(self,child);
            };collectIdentities(collectIdentities,data);
            inspectAssets(data,[&](Properties &properties,const String &component,const String &field,const String &identity) {
                require(!objectIdentities.count(identity),"Cooked asset UUID collides with actor/component identity");
                require(context.assetBindings!=nullptr,"Cooked component requires captured asset bindings");
                const auto found=context.assetBindings->find(identity);
                require(found!=context.assetBindings->end(),"Cooked component UUID is missing from captured asset bindings");
                const auto &id=found->second;
                require(id.isValid()&&!id.isSubResource()&&assetType(component,field,id.type()),"Captured cooked component UUID has the wrong resource type/selector");
                auto entry=bindings.emplace(identity,id);require(entry.second||entry.first->second==id,"Conflicting captured cooked UUID binding");
                properties.getPropertyObject(field).setAttribute(bindingAttribute,id.str());
            });
            return {std::move(budget),std::move(data),std::move(bindings)};
        }
        // Allocation-free validation of the existing Properties binary envelope
        // before its generic reader may allocate property/child arrays.
        void utf8(const u8 *bytes,size_t size)
        {
            for(size_t at=0;at<size;) {
                const auto lead=bytes[at++];if(lead<0x80)continue;
                const unsigned count=lead>=0xc2&&lead<=0xdf?1:lead>=0xe0&&lead<=0xef?2:lead>=0xf0&&lead<=0xf4?3:0;
                require(count&&count<=size-at,"Invalid UTF-8 binary actor string");u32 code=lead&((1u<<(6-count))-1u);
                for(unsigned index=0;index<count;++index){const auto next=bytes[at++];require((next&0xc0)==0x80,"Invalid UTF-8 binary continuation");code=(code<<6)|(next&0x3f);}
                require((count!=2||code>=0x800)&&(count!=3||code>=0x10000)&&code<=0x10ffff&&!(code>=0xd800&&code<=0xdfff),"Noncanonical UTF-8 binary actor string");
            }
        }
        struct Preflight
        {
            const Array<u8> &bytes; size_t at=20,nodes=0;
            u32 number() { require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated binary actor graph"); u32 value=0; for(int i=0;i<4;++i)value|=u32(bytes[at++])<<(8*i); return value; }
            void string() { auto size=number(); require(size<=4096&&size<=bytes.size()-at,"Binary actor string exceeds limit"); require(!size||!std::memchr(bytes.data()+at,0,size),"NUL in binary actor string"); utf8(bytes.data()+at,size);at+=size; }
            void node(size_t depth) {
                require(depth<=maximumDepth&&++nodes<=maximumNodes,"Binary actor graph exceeds limits"); string();
                auto fields=number(); require(fields<=32,"Binary actor field limit exceeded");
                for(u32 i=0;i<fields;++i) { string();string();string();require(at<bytes.size(),"Truncated actor flag"); require(bytes[at++]<=1,"Invalid actor flag"); auto attributes=number();require(attributes<=16,"Actor attribute limit exceeded");for(u32 a=0;a<attributes;++a){string();string();} }
                auto children=number(); require(children<=maximumNodes,"Actor child limit exceeded"); for(u32 i=0;i<children;++i)node(depth+1);
            }
        };
        void asJSON(const Properties &data,JSON &out,rapidjson::Document::AllocatorType &allocator,size_t depth=0,size_t *count=nullptr,size_t *projectedBytes=nullptr)
        {
            size_t localCount=0;if(!count)count=&localCount;
            size_t localBytes=0;if(!projectedBytes)projectedBytes=&localBytes;
            require(depth<=maximumDepth&&++*count<=maximumNodes&&data.getNumProperties()<=32,"Actor graph JSON projection exceeds limits");
            out.SetObject(); JSON name; const auto group=data.getName();require(group.size()<=4096&&*projectedBytes<=maximumBytes-group.size(),"Actor group name exceeds limits");*projectedBytes+=group.size();name.SetString(group.data(),static_cast<rapidjson::SizeType>(group.size()),allocator); out.AddMember("handle_name",name,allocator);
            JSON metadata(rapidjson::kArrayType);
            for(const auto &property:data.getPropertiesAsArray()) {
                JSON key,value; const auto field=property.getName(), text=property.getValue();
                require(field.size()<=4096&&text.size()<=4096&&*projectedBytes<=maximumBytes-field.size()-text.size(),"Actor graph JSON projection exceeds string/byte limits");
                *projectedBytes+=field.size()+text.size();
                key.SetString(field.data(),static_cast<rapidjson::SizeType>(field.size()),allocator); value.SetString(text.data(),static_cast<rapidjson::SizeType>(text.size()),allocator); out.AddMember(key,value,allocator);
                const auto binding=property.getAttribute(bindingAttribute);
                if(!binding.empty()) {
                    require(binding.size()<=4096&&*projectedBytes<=maximumBytes-binding.size(),"Actor graph binding exceeds byte limit");*projectedBytes+=binding.size();
                    JSON entry(rapidjson::kObjectType),entryName,attributes(rapidjson::kObjectType),bound;
                    entryName.SetString(field.data(),static_cast<rapidjson::SizeType>(field.size()),allocator);entry.AddMember("name",entryName,allocator);
                    entry.AddMember("type",JSON("resource",allocator),allocator);entry.AddMember("readOnly",false,allocator);
                    bound.SetString(binding.data(),static_cast<rapidjson::SizeType>(binding.size()),allocator);attributes.AddMember(JSON(bindingAttribute.c_str(),allocator),bound,allocator);
                    entry.AddMember("attributes",attributes,allocator);metadata.PushBack(entry,allocator);
                }
            }
            out.AddMember("__wp_properties_v1",metadata,allocator);
            JSON children(rapidjson::kArrayType); for(const auto &child:data.getChildren()) { require(child!=nullptr,"Null actor group"); JSON value; asJSON(*child,value,allocator,depth+1,count,projectedBytes);children.PushBack(value,allocator); } out.AddMember("children",children,allocator);
        }
        void estimateProjection(const Properties &data,size_t depth,size_t &nodes,u64 &bytes)
        {
            require(depth<=maximumDepth&&++nodes<=maximumNodes&&data.getNumProperties()<=32,"Cooked document validation exceeds node/field limits");
            const auto name=data.getName();require(name.size()<=4096,"Cooked document group exceeds string limit");bytes+=name.size()+128;
            // Retire property snapshots before descending, so one bounded scratch
            // lease covers this preflight even at the maximum tree depth.
            {
                const auto fields=data.getPropertiesAsArray();
                for(const auto &property:fields) {
                    const auto key=property.getName(),value=property.getValue(),binding=property.getAttribute(bindingAttribute);
                    require(key.size()<=4096&&value.size()<=4096&&binding.size()<=4096,"Cooked document field exceeds string limit");bytes+=key.size()+value.size()+binding.size()+128;
                    require(bytes<=maximumBytes,"Cooked document projection exceeds byte limit");
                }
            }
            for(const auto &child:data.getChildren()){require(child!=nullptr,"Null cooked actor group");estimateProjection(*child,depth+1,nodes,bytes);}
        }
    }
    class CookedActorGraph
    {
    public:
        std::shared_ptr<const RuntimeResource> resource;
        StreamBudget metadataBudget;
        SmartPtr<Properties> data;
        std::map<String,std::shared_ptr<const RuntimeResource>> assets;
        bool scene=false;
        bool requiresScripts=false;
    };
    String SceneResourceCompiler::name() const { return "WorkphoneActorGraphBinary"; }
    Array<CompilerOutput> SceneResourceCompiler::outputs() const { return {{ResourceTypeID("fbscene"),2},{ResourceTypeID("prefab"),2}}; }
    bool SceneResourceCompiler::getDependencies(const CompileContext &context,DependencySet &dependencies,String &error) const
    { error.clear();try { const auto parsed=source(context);std::set<ResourceID> unique;for(const auto &binding:parsed.bindings)if(unique.insert(binding.second).second){dependencies.compileDependencies.push_back(CompileDependency::resource(binding.second));dependencies.installDependencies.push_back(binding.second);}return true; }catch(const std::exception &e){error=e.what();return false;} }
    CompilationStatus SceneResourceCompiler::compile(const CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try { auto data=source(context);const auto bytes=PropertiesBinarySerializer::serialize(*data.data);require(bytes.size()<=maximumBytes,"Cooked actor graph exceeds byte limit");output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());return output?CompilationStatus::Success:CompilationStatus::Failure; }
        catch(const std::exception &e){messages.push_back(e.what());return CompilationStatus::Failure;}
    }
    std::shared_ptr<const CookedActorGraph> decodeActorGraphResource(std::shared_ptr<const RuntimeResource> resource,String &error)
    {
        error.clear();try {
            require(resource!=nullptr,"Missing cooked actor graph"); const auto type=resource->header.resourceType; const SceneResourceCompiler compiler;
            const auto legacyVersion=combineHash(combineHash(combineHash(14695981039346656037ull,1),1),type.value());
            const bool legacy=resource->header.compilerVersion==legacyVersion;
            require(resource->header.isValid()&&(type==ResourceTypeID("fbscene")||type==ResourceTypeID("prefab"))&&!resource->header.resourceId.isSubResource()&&resource->header.resourceId.type()==type&&(legacy||resource->header.compilerVersion==compiler.versionFor(type)),"Unsupported cooked actor graph header/type/version/selector");
            if(legacy)require(resource->header.installDependencies.empty()&&resource->dependencies.empty(),"Legacy actor graphs cannot bind typed dependencies");
            require(resource->header.installDependencies.size()==resource->dependencies.size(),"Cooked graph dependency closure is incomplete");
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
            if(legacy) {
                auto check=[&](auto &&self,const SmartPtr<Properties> &node)->void {
                    require(!node->hasProperty("componentType")&&node->getName()!="component"&&node->getName()!="components","Legacy actor graphs support transforms and identity only");
                    for(const auto &child:node->getChildren())self(self,child);
                };check(check,data);
            }
            rapidjson::Document doc; asJSON(*data,doc,doc.GetAllocator());size_t nodes=0;std::set<String> identities;
            auto result=std::make_shared<CookedActorGraph>();result->resource=std::move(resource);result->metadataBudget=retained;result->scene=type==ResourceTypeID("fbscene");result->data=parseNode(doc,result->scene?Role::Scene:Role::Actor,0,nodes,identities,retained,true);
            inspectRelationships(result->data);
            std::map<ResourceID,std::shared_ptr<const RuntimeResource>> dependencies;
            for(size_t i=0;i<result->resource->dependencies.size();++i) {
                const auto &dependency=result->resource->dependencies[i];const auto &id=result->resource->header.installDependencies[i];
                require(dependency&&dependency->header.isValid()&&dependency->header.resourceId==id&&!id.isSubResource()&&dependency->header.resourceType==id.type(),"Cooked graph dependency identity mismatch");
                require(dependencies.emplace(id,dependency).second,"Duplicate cooked graph dependency");
                if(result->resource->header.formatVersion>=2) {
                    const auto &pin=result->resource->header.installArtifacts[i];
                    require(pin.sourceHash==dependency->header.sourceHash&&pin.payloadHash==dependency->header.payloadHash,"Cooked graph dependency generation mismatch");
                }
            }
            std::set<ResourceID> used;
            inspectAssets(result->data,[&](Properties &properties,const String &component,const String &field,const String &identity) {
                const auto binding=properties.getPropertyObject(field).getAttribute(bindingAttribute);
                require(!binding.empty(),"Cooked component lacks its captured asset binding");
                const ResourceID id(binding);const auto dependency=dependencies.find(id);
                require(id.isValid()&&assetType(component,field,id.type())&&dependency!=dependencies.end(),"Cooked component binding is outside its pinned closure");
                require(!identities.count(identity),"Cooked asset UUID collides with actor/component identity");
                auto entry=result->assets.emplace(identity,dependency->second);
                require(entry.second||entry.first->second==dependency->second,"Conflicting cooked asset UUID binding");used.insert(id);
                result->requiresScripts|=component=="Script";
            });
            require(used.size()==dependencies.size(),"Undeclared extra dependency in cooked graph");return result;
        } catch(const std::exception &e){error=e.what();return {};}
    }
    bool actorGraphRequiresScripts(const std::shared_ptr<const CookedActorGraph> &graph)
    { return graph&&graph->requiresScripts; }
    bool validateCookedActorGraphData(SmartPtr<Properties> data,bool scene,std::shared_ptr<const RuntimeAssetContext> context,String &error)
    {
        error.clear();try {
            require(data!=nullptr,"Missing cooked actor document");
            auto preflight=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,1ull<<20,error);
            require(preflight!=nullptr,"Cooked document validation preflight budget exhausted");
            size_t estimatedNodes=0;u64 estimatedBytes=0;estimateProjection(*data,0,estimatedNodes,estimatedBytes);
            auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,estimatedBytes*48+(1ull<<20),error);
            require(scratch!=nullptr,"Cooked document validation budget exhausted");
            rapidjson::Document doc;asJSON(*data,doc,doc.GetAllocator());size_t nodes=0;std::set<String> identities;
            auto validated=parseNode(doc,scene?Role::Scene:Role::Actor,0,nodes,identities,{},true);inspectRelationships(validated);
            inspectAssets(validated,[&](Properties &properties,const String &component,const String &field,const String &identity) {
                require(context&&context->resolve,"Cooked actor document requires a pinned asset context");
                const ResourceID id(properties.getPropertyObject(field).getAttribute(bindingAttribute));
                auto asset=context->resolve(identity,error);
                require(!identities.count(identity)&&id.isValid()&&asset&&asset->header.resourceId==id&&assetType(component,field,id.type()),"Cooked actor document reference is outside its typed context");
                if(component=="Script")require(context->scripts&&context->scripts->isLoaded(),"Cooked Script requires its private loaded VM");
            });return true;
        }catch(const std::exception &exception){error=exception.what();return false;}
    }
    std::shared_ptr<RuntimeAssetContext> createActorGraphRuntimeContext(std::shared_ptr<const CookedActorGraph> graph,const String &target,bool packagedBuild,String &error)
    {
        error.clear();try {
            require(graph&&!target.empty(),"Cooked graph context requires a graph and target");
            auto context=std::make_shared<RuntimeAssetContext>();context->target=target;context->packagedBuild=packagedBuild;context->requiresScripts=graph->requiresScripts;
            struct DetachedAssets { StreamBudget lease;std::map<String,std::shared_ptr<const RuntimeResource>> values; };
            auto assets=std::make_shared<DetachedAssets>();assets->lease=graph->metadataBudget;assets->values=graph->assets;
            context->resolve=[assets](const String &identity,String &error)->std::shared_ptr<const RuntimeResource> {
                error.clear();const auto found=assets->values.find(StringUtil::make_lower(identity));
                if(found==assets->values.end()){error="UUID is outside the cooked graph's captured asset closure";return {};}
                return found->second;
            };
            require(prepareRuntimeAssetScripts(context,error),"Cannot prepare the cooked graph's private script VM");return context;
        }catch(const std::exception &exception){error=exception.what();return {};}
    }
    bool installCookedSceneGraph(std::shared_ptr<const CookedActorGraph> graph,SmartPtr<scene::IGameScene> target,String &error)
    { return installCookedSceneGraph(std::move(graph),std::move(target),{},error); }
    bool installCookedSceneGraph(std::shared_ptr<const CookedActorGraph> graph,SmartPtr<scene::IGameScene> target,std::shared_ptr<const RuntimeAssetContext> context,String &error)
    {
        SmartPtr<scene::GameScene> concrete;u64 generation=0;bool previousReady=false;
        error.clear();try {
            require(graph&&graph->scene&&target&&target->isLoaded(),"Cooked scene requires a loaded target scene");
            require(graph->assets.empty()||context&&context->resolve,"Mixed cooked scene requires its pinned runtime asset context");
            require(!graph->requiresScripts||context&&context->scripts&&context->scripts->isLoaded(),"Cooked Script scene requires a private loaded runtime VM");
            if(context)for(const auto &asset:graph->assets) {
                auto resolved=context->resolve(asset.first,error);
                require(resolved&&resolved->header.resourceId==asset.second->header.resourceId&&resolved->header.sourceHash==asset.second->header.sourceHash&&resolved->header.payloadHash==asset.second->header.payloadHash,"Scene context does not match its captured resource generation");
            }
            if(context)context=std::make_shared<const RuntimeAssetContext>(*context);
            concrete=dynamic_pointer_cast<scene::GameScene>(target);require(concrete!=nullptr,"Cooked scene requires the native GameScene loader");
            SmartPtr<core::IApplicationManager> app=core::IApplicationManager::instancePtr();
            auto manager=app?app->getGameManager():nullptr;require(manager!=nullptr,"Cooked scene requires its game manager");
            SmartPtr<ITask> renderTask;
            if(auto tasks=app->getTaskManager())if(auto task=tasks->getTask(TaskId::Render);task&&task->isLoaded())renderTask=task;
            // Stop before owning either manager lock: the retiring frame may need
            // both. Nested native loader task locks then preserve this stop.
            TaskLock renderPause(renderTask);
            ScopedLock managerLock(manager);
            auto graphics=app->getGraphicsSystem();ScopedLock graphicsLock(graphics);
            auto job=make_ptr<SceneLoadJob>();job->setPreparedData(graph->data);job->setRuntimeAssets(context);
            {
                ScopedLock lock(concrete);
                previousReady=target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded;
                generation=concrete->beginSceneLoad();
                job->setScene(target);job->setLightingDirector(target->getLightingDirector());
            }
            struct ResolutionScope {
                Properties::ObjectResolver previous;
                bool fallback;
                ~ResolutionScope(){Properties::exchangeObjectResolver(std::move(previous));Properties::exchangeObjectResolverFallback(fallback);}
            } resolution{Properties::exchangeObjectResolver(context?context->objects:Properties::ObjectResolver{}),Properties::exchangeObjectResolverFallback(false)};
            job->execute();
            bool committed=false;
            {
                ScopedLock lock(concrete);
                committed=concrete->getLoadGeneration()==generation&&target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded;
                if(!committed&&previousReady)concrete->restoreReadySceneAfterFailedAppend(generation);
            }
            require(committed,"Cooked scene graph commit failed or was superseded");return true;
        }catch(const std::exception &e){
            if(concrete&&previousReady)concrete->restoreReadySceneAfterFailedAppend(generation);
            error=e.what();return false;
        }
    }
    SmartPtr<scene::IGamePrefab> installCookedPrefabGraph(std::shared_ptr<const CookedActorGraph> graph,SmartPtr<scene::GamePrefabManager> manager,String &error)
    { return installCookedPrefabGraph(std::move(graph),std::move(manager),{},error); }
    SmartPtr<scene::IGamePrefab> installCookedPrefabGraph(std::shared_ptr<const CookedActorGraph> graph,SmartPtr<scene::GamePrefabManager> manager,std::shared_ptr<const RuntimeAssetContext> context,String &error)
    {
        error.clear();try {
            require(graph&&!graph->scene&&manager,"Cooked prefab requires the native prefab manager");
            if(!context&&graph->assets.empty())context=createActorGraphRuntimeContext(graph,"pc",true,error);
            require(validateCookedActorGraphData(graph->data,false,context,error),"Cooked prefab requires its validated runtime asset context");
            if(context)context=std::make_shared<const RuntimeAssetContext>(*context);
            const auto bound=graph->resource->payload.size()*48+(1ull<<20);
            auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,bound,error);
            require(scratch!=nullptr,"Prefab clone preparation budget exhausted");
            auto retained=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,bound,error);
            require(retained!=nullptr,"Prefab clone residency budget exhausted");
            auto copy=scene::GameActorUtil::createInstanceData(graph->data);require(copy!=nullptr,"Cannot clone cooked prefab graph");
            rapidjson::Document doc;asJSON(*copy,doc,doc.GetAllocator());size_t nodes=0;std::set<String> identities;
            auto data=parseNode(doc,Role::Actor,0,nodes,identities,retained,true);
            auto prefab=dynamic_pointer_cast<scene::GamePrefab>(manager->create(StringUtil::getUUID()));require(prefab!=nullptr,"Cannot create cooked prefab");prefab->setData(data);prefab->setRuntimeAssets(context);return prefab;
        }catch(const std::exception &e){error=e.what();return {};}
    }
}
