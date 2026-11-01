#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/AnimationResourceCompiler.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <map>
#include <cstring>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace workphone::resource
{
    namespace
    {
        constexpr size_t maximumBytes=256u<<10;
        using namespace animation;
        void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
        void report(String &error,const char *message) noexcept
        {try{error=message;}catch(...){try{error="Animation resource failed";}catch(...){error.clear();}}}
        struct Input {StreamBudget working;String text;};
        Input source(const String &path)
        {
            std::ifstream file(std::filesystem::u8path(path.c_str()),std::ios::binary|std::ios::ate);
            const auto size=file?file.tellg():std::streampos(-1);require(size>0&&size<=maximumBytes,"Animation JSON requires 1-262144 bytes");
            String error;Input result;result.working=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*u64(size)+65536,error);
            require(bool(result.working),"Animation JSON working admission denied");result.text.resize(size_t(size));file.seekg(0);file.read(result.text.data(),result.text.size());
            require(bool(file)&&result.text.find('\0')==String::npos,"Incomplete animation JSON or embedded NUL");return result;
        }
        void fields(const rapidjson::Value &object,std::initializer_list<const char*> required,std::initializer_list<const char*> optional={})
        {
            require(object.IsObject(),"Animation record must be an object");std::set<String> seen;
            for(auto i=object.MemberBegin();i!=object.MemberEnd();++i){
                require(std::memchr(i->name.GetString(),0,i->name.GetStringLength())==nullptr,"Animation field contains NUL");
                String name(i->name.GetString(),i->name.GetStringLength());bool known=false;
                for(auto key:required)known|=name==key;for(auto key:optional)known|=name==key;
                require(known&&seen.insert(name).second,"Unknown or duplicate animation field");}
            for(auto key:required)require(seen.count(key)!=0,"Missing animation field");
        }
        String text(const rapidjson::Value &value,size_t maximum=128,bool empty=false)
        {
            require(value.IsString()&&value.GetStringLength()<=maximum&&(empty||value.GetStringLength()),"Invalid animation string length");
            String result(value.GetString(),value.GetStringLength());
            for(unsigned char c:result)require(c>=32&&c<=126,"Animation names require printable ASCII without NUL");return result;
        }
        f32 number(const rapidjson::Value &value)
        {require(value.IsNumber()&&std::isfinite(value.GetDouble())&&std::abs(value.GetDouble())<=1000000,"Animation number must be finite and bounded");return f32(value.GetDouble());}
        bool boolean(const rapidjson::Value &value){require(value.IsBool(),"Animation flag requires a boolean");return value.GetBool();}
        u32 count(const rapidjson::Value &value,u32 maximum,bool empty=false)
        {require(value.IsArray()&&value.Size()<=maximum&&(empty||!value.Empty()),"Animation array exceeds bounds or is empty");return value.Size();}
        ResourceID identity(const String &value,const char *type)
        {ResourceID id;String error;require(value.size()<=1024&&id.set(value,&error)&&!id.isSubResource()&&id.type()==ResourceTypeID(type),"Animation dependency requires an explicit canonical root ResourceID of the right type");require(id.str()==value,"Animation dependency ResourceID must be canonical");return id;}
        struct Clip {String id,name;f32 speed=1;bool loop=true;};
        struct Mask {String rig;f32 base=1;Array<std::pair<String,f32>> bones=makeSourceBudgetSnapshot<std::pair<String,f32>>();};
        struct Graph
        {
            String rig,mask,initial;
            Array<Clip> clips=makeSourceBudgetSnapshot<Clip>();
            Array<AnimationGraphParameter> parameters=makeSourceBudgetSnapshot<AnimationGraphParameter>();
            Array<AnimationGraphPoseNodeDefinition> nodes=makeSourceBudgetSnapshot<AnimationGraphPoseNodeDefinition>();
            Array<AnimationGraphState> states=makeSourceBudgetSnapshot<AnimationGraphState>();
            Array<AnimationGraphTransition> transitions=makeSourceBudgetSnapshot<AnimationGraphTransition>();
        };
        const AnimationGraphParameter *parameter(const Graph &graph,const String &name)
        {for(const auto &entry:graph.parameters)if(entry.name==name)return &entry;return nullptr;}
        void validate(const Mask &mask)
        {
            identity(mask.rig,"skelres");require(std::isfinite(mask.base)&&mask.base>=0&&mask.base<=1,"Mask defaultWeight requires [0,1]");
            require(mask.bones.size()<=256,"Mask exceeds 256 bones");std::set<String> seen;
            for(const auto &bone:mask.bones)require(!bone.first.empty()&&seen.insert(bone.first).second&&std::isfinite(bone.second)&&bone.second>=0&&bone.second<=1,"Duplicate mask bone or weight outside [0,1]");
        }
        void validate(const Graph &graph)
        {
            identity(graph.rig,"skelres");if(!graph.mask.empty())identity(graph.mask,"amask");
            require(!graph.clips.empty()&&graph.clips.size()<=64&&!graph.states.empty()&&graph.states.size()<=128&&graph.nodes.size()<=128&&graph.parameters.size()<=64&&graph.transitions.size()<=256,"Animation graph record counts exceed bounds");
            std::set<String> clips,nodes,states,parameters;
            for(const auto &clip:graph.clips)require(!clip.id.empty()&&!clip.name.empty()&&clips.insert(clip.id).second&&std::isfinite(clip.speed)&&clip.speed>=0&&clip.speed<=16,"Invalid graph clip alias or speed");
            for(const auto &entry:graph.parameters)require(!entry.name.empty()&&parameters.insert(entry.name).second&&std::isfinite(entry.value),"Invalid or duplicate graph parameter");
            for(const auto &node:graph.nodes){require(nodes.insert(node.id).second&&node.isValid()&&node.speed<=16,"Invalid or duplicate pose node");
                if(node.type==AnimationGraphPoseNodeType::Clip)require(node.parameter.empty()&&node.childNodeIds.empty()&&node.blendRanges.empty(),"Clip pose node contains non-clip fields");
                else require(node.clipId.empty()&&node.speed==1&&node.loop&&(node.type!=AnimationGraphPoseNodeType::Selector||node.blendRanges.empty()),"Blend/selector pose node contains unsupported clip/range fields");}
            for(const auto &state:graph.states){require(states.insert(state.id).second&&state.isValid()&&state.speed<=16&&(state.clipId.empty()!=state.poseNodeId.empty()),"Invalid graph state or ambiguous pose binding");
                require(state.clipId.empty()?nodes.count(state.poseNodeId)!=0:clips.count(state.clipId)!=0,"State references an unknown clip or pose node");}
            require(states.count(graph.initial)!=0,"Initial graph state does not exist");
            std::map<String,u32> colors,heights;std::set<String> usedNodes,usedClips;
            std::function<void(const String&,u32)> visit=[&](const String &name,u32 depth){
                require(depth<=32,"Animation pose graph exceeds depth32");auto &color=colors[name];require(color!=1,"Cyclic animation pose graph");if(color==2){require(depth+heights[name]<=33,"Animation pose graph exceeds depth32");return;}color=1;
                const AnimationGraphPoseNodeDefinition *node=nullptr;for(const auto &entry:graph.nodes)if(entry.id==name)node=&entry;
                require(node!=nullptr,"Pose node references unknown child");usedNodes.insert(name);
                if(node->type==AnimationGraphPoseNodeType::Clip){require(clips.count(node->clipId)!=0,"Pose node references unknown clip");usedClips.insert(node->clipId);}
                else {const auto driving=parameter(graph,node->parameter);require(driving&&!driving->boolean&&node->childNodeIds.size()<=16,"Pose driving parameter must be a declared float with 1-16 children");
                    std::set<String> children;for(const auto &child:node->childNodeIds){require(children.insert(child).second,"Duplicate pose child");visit(child,depth+1);}
                    if(node->type==AnimationGraphPoseNodeType::Blend1D){require(node->blendRanges.size()+1==node->childNodeIds.size(),"Blend1D requires contiguous adjacent ranges");
                        for(size_t i=0;i<node->blendRanges.size();++i){const auto &range=node->blendRanges[i];require(range.isValid()&&range.inputIndex0==i&&range.inputIndex1==i+1&&(!i||range.parameterLow==node->blendRanges[i-1].parameterHigh),"Blend1D ranges must tile adjacent inputs without gaps or overlaps");}}}
                u32 height=1;for(const auto &child:node->childNodeIds)height=std::max(height,1+heights[child]);heights[name]=height;require(depth+height<=33,"Animation pose graph exceeds depth32");color=2;
            };
            for(const auto &state:graph.states)if(state.poseNodeId.empty())usedClips.insert(state.clipId);else visit(state.poseNodeId,1);
            require(usedNodes.size()==nodes.size()&&usedClips.size()==clips.size(),"Animation graph contains unreachable pose nodes or unused clip aliases");
            std::set<String> edges;
            for(const auto &transition:graph.transitions){require(transition.isValid()&&states.count(transition.fromState)&&states.count(transition.toState)&&transition.duration<=60&&
                    (transition.exitTime==-1||(transition.exitTime>=0&&transition.exitTime<=1))&&transition.priority>=-1024&&transition.priority<=1024&&transition.conditions.size()<=8,"Invalid graph transition endpoints, timing or priority");
                require(edges.insert(transition.fromState+"\n"+transition.toState).second,"Duplicate graph transition edge");
                for(const auto &condition:transition.conditions){const auto driving=parameter(graph,condition.parameter);require(driving&&driving->boolean==condition.isBoolean&&std::isfinite(condition.threshold)&&
                    u32(condition.comparison)<=u32(AnimationGraphComparison::LessOrEqual)&&(!condition.isBoolean||condition.comparison==AnimationGraphComparison::Equal||condition.comparison==AnimationGraphComparison::NotEqual),"Transition condition has an unknown/mismatched parameter or comparison");}}
            std::set<String> reachable{graph.initial};bool changed=true;while(changed){changed=false;for(const auto &edge:graph.transitions)if(reachable.count(edge.fromState))changed|=reachable.insert(edge.toState).second;}
            require(reachable.size()==states.size(),"Animation graph has states unreachable from its initial state");
        }
        rapidjson::Document document(const String &source)
        {rapidjson::Document value;value.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(source.data(),source.size());require(!value.HasParseError(),"Invalid animation JSON/UTF8");return value;}
        Mask parseMask(const rapidjson::Value &root)
        {
            fields(root,{"version","rig","defaultWeight","bones"});require(root["version"].IsUint()&&root["version"].GetUint()==1,"Unsupported animation mask version");
            Mask out;out.rig=text(root["rig"],1024);out.base=number(root["defaultWeight"]);count(root["bones"],256,true);
            for(const auto &entry:root["bones"].GetArray()){fields(entry,{"bone","weight"});out.bones.push_back({text(entry["bone"]),number(entry["weight"])});}validate(out);return out;
        }
        Graph parseGraph(const rapidjson::Value &root)
        {
            fields(root,{"version","rig","initialState","clips","parameters","states","transitions"},{"mask","nodes"});require(root["version"].IsUint()&&root["version"].GetUint()==1,"Unsupported animation graph version");
            Graph out;out.rig=text(root["rig"],1024);out.initial=text(root["initialState"]);if(root.HasMember("mask"))out.mask=text(root["mask"],1024);
            count(root["clips"],64);for(const auto &entry:root["clips"].GetArray()){fields(entry,{"id","clip","speed","loop"});out.clips.push_back({text(entry["id"]),text(entry["clip"]),number(entry["speed"]),boolean(entry["loop"])});}
            count(root["parameters"],64,true);for(const auto &entry:root["parameters"].GetArray()){fields(entry,{"name","type","value"});AnimationGraphParameter item;item.name=text(entry["name"]);const auto type=text(entry["type"]);
                require(type=="bool"||type=="float","Graph parameter type must be bool or float");item.boolean=type=="bool";if(item.boolean)item.booleanValue=boolean(entry["value"]);else item.value=number(entry["value"]);out.parameters.push_back(std::move(item));}
            if(root.HasMember("nodes")){count(root["nodes"],128,true);for(const auto &entry:root["nodes"].GetArray()){
                require(entry.IsObject()&&entry.HasMember("type"),"Pose node requires a type");const auto type=text(entry["type"]);AnimationGraphPoseNodeDefinition node;
                if(type=="clip"){fields(entry,{"id","type","clip","speed","loop"});node.type=AnimationGraphPoseNodeType::Clip;node.clipId=text(entry["clip"]);node.speed=number(entry["speed"]);node.loop=boolean(entry["loop"]);}
                else {require(type=="blend1d"||type=="selector","Unsupported graph pose node type");node.type=type=="blend1d"?AnimationGraphPoseNodeType::Blend1D:AnimationGraphPoseNodeType::Selector;
                    if(type=="blend1d")fields(entry,{"id","type","parameter","children","ranges"});else fields(entry,{"id","type","parameter","children"});
                    node.parameter=text(entry["parameter"]);count(entry["children"],16);for(const auto &child:entry["children"].GetArray())node.childNodeIds.push_back(text(child));
                    if(type=="blend1d"){count(entry["ranges"],15);for(const auto &range:entry["ranges"].GetArray()){fields(range,{"input0","input1","low","high"});require(range["input0"].IsUint()&&range["input1"].IsUint(),"Blend inputs require unsigned indices");node.blendRanges.push_back({range["input0"].GetUint(),range["input1"].GetUint(),number(range["low"]),number(range["high"])});}}}
                node.id=text(entry["id"]);out.nodes.push_back(std::move(node));}}
            count(root["states"],128);for(const auto &entry:root["states"].GetArray()){fields(entry,{"id","speed"},{"clip","node"});AnimationGraphState state;state.id=text(entry["id"]);state.speed=number(entry["speed"]);
                if(entry.HasMember("clip"))state.clipId=text(entry["clip"]);if(entry.HasMember("node"))state.poseNodeId=text(entry["node"]);out.states.push_back(std::move(state));}
            count(root["transitions"],256,true);for(const auto &entry:root["transitions"].GetArray()){
                fields(entry,{"from","to","duration","exitTime","priority","synchronize","conditions"});AnimationGraphTransition transition;
                transition.fromState=text(entry["from"]);transition.toState=text(entry["to"]);transition.duration=number(entry["duration"]);transition.exitTime=number(entry["exitTime"]);require(entry["priority"].IsInt(),"Transition priority requires an integer");transition.priority=entry["priority"].GetInt();transition.synchronize=boolean(entry["synchronize"]);
                count(entry["conditions"],8,true);for(const auto &value:entry["conditions"].GetArray()){fields(value,{"parameter","op","value"});AnimationGraphCondition condition;condition.parameter=text(value["parameter"]);const auto op=text(value["op"]);
                    const char *ops[]={"equal","notEqual","greater","greaterOrEqual","less","lessOrEqual"};size_t selected=0;while(selected<6&&op!=ops[selected])++selected;require(selected<6,"Unknown graph condition comparison");condition.comparison=AnimationGraphComparison(selected);
                    condition.isBoolean=value["value"].IsBool();if(condition.isBoolean)condition.booleanValue=boolean(value["value"]);else condition.threshold=number(value["value"]);transition.conditions.push_back(std::move(condition));}out.transitions.push_back(std::move(transition));}
            validate(out);return out;
        }
        void word(std::ostream &out,u32 value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));}
        void scalar(std::ostream &out,f32 value){u32 bits;std::memcpy(&bits,&value,4);word(out,bits);}
        void string(std::ostream &out,const String &value){word(out,u32(value.size()));out.write(value.data(),value.size());}
        struct Reader
        {
            const Array<u8> &bytes;size_t at=0;
            u32 word(){require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated typed animation record");u32 value=0;for(unsigned i=0;i<4;++i)value|=u32(bytes[at++])<<(8*i);return value;}
            u32 count(u32 maximum){const auto value=word();require(value<=maximum,"Typed animation count exceeds bound");return value;}
            bool boolean(){const auto value=word();require(value<=1,"Invalid typed animation flag");return value!=0;}
            f32 scalar(){const auto bits=word();f32 value;std::memcpy(&value,&bits,4);require(std::isfinite(value)&&std::abs(value)<=1000000,"Invalid typed animation float");return value;}
            String string(size_t maximum=128,bool empty=false){const auto size=count(u32(maximum));require((size||empty)&&size<=bytes.size()-at,"Invalid typed animation string extent");String value(reinterpret_cast<const char*>(bytes.data()+at),size);at+=size;
                for(unsigned char c:value)require(c>=32&&c<=126,"Invalid typed animation string");return value;}
        };
        void write(std::ostream &out,const Graph &graph)
        {
            out.write("WPAG",4);word(out,1);string(out,graph.rig);string(out,graph.mask);string(out,graph.initial);
            word(out,u32(graph.clips.size()));for(const auto &clip:graph.clips){string(out,clip.id);string(out,clip.name);scalar(out,clip.speed);word(out,clip.loop);}
            word(out,u32(graph.parameters.size()));for(const auto &parameter:graph.parameters){string(out,parameter.name);word(out,parameter.boolean);word(out,parameter.booleanValue);scalar(out,parameter.value);}
            word(out,u32(graph.nodes.size()));for(const auto &node:graph.nodes){string(out,node.id);word(out,u32(node.type));string(out,node.clipId);scalar(out,node.speed);word(out,node.loop);string(out,node.parameter);
                word(out,u32(node.childNodeIds.size()));for(const auto &child:node.childNodeIds)string(out,child);word(out,u32(node.blendRanges.size()));for(const auto &range:node.blendRanges){word(out,range.inputIndex0);word(out,range.inputIndex1);scalar(out,range.parameterLow);scalar(out,range.parameterHigh);}}
            word(out,u32(graph.states.size()));for(const auto &state:graph.states){string(out,state.id);string(out,state.clipId);string(out,state.poseNodeId);scalar(out,state.speed);}
            word(out,u32(graph.transitions.size()));for(const auto &transition:graph.transitions){string(out,transition.fromState);string(out,transition.toState);scalar(out,transition.duration);scalar(out,transition.exitTime);word(out,u32(transition.priority));word(out,transition.synchronize);
                word(out,u32(transition.conditions.size()));for(const auto &condition:transition.conditions){string(out,condition.parameter);word(out,u32(condition.comparison));scalar(out,condition.threshold);word(out,condition.booleanValue);word(out,condition.isBoolean);}}
        }
        Graph readGraph(Reader &in)
        {
            Graph graph;graph.rig=in.string(1024);graph.mask=in.string(1024,true);graph.initial=in.string();
            auto size=in.count(64);for(u32 i=0;i<size;++i){Clip clip;clip.id=in.string();clip.name=in.string();clip.speed=in.scalar();clip.loop=in.boolean();graph.clips.push_back(std::move(clip));}
            size=in.count(64);for(u32 i=0;i<size;++i){AnimationGraphParameter value;value.name=in.string();value.boolean=in.boolean();value.booleanValue=in.boolean();value.value=in.scalar();graph.parameters.push_back(std::move(value));}
            size=in.count(128);for(u32 i=0;i<size;++i){AnimationGraphPoseNodeDefinition node;node.id=in.string();const auto type=in.word();require(type<=u32(AnimationGraphPoseNodeType::Selector),"Unknown typed pose node kind");node.type=AnimationGraphPoseNodeType(type);
                node.clipId=in.string(128,true);node.speed=in.scalar();node.loop=in.boolean();node.parameter=in.string(128,true);auto nested=in.count(16);for(u32 j=0;j<nested;++j)node.childNodeIds.push_back(in.string());
                nested=in.count(15);for(u32 j=0;j<nested;++j){AnimationGraphBlendRange range;range.inputIndex0=in.word();range.inputIndex1=in.word();range.parameterLow=in.scalar();range.parameterHigh=in.scalar();node.blendRanges.push_back(range);}graph.nodes.push_back(std::move(node));}
            size=in.count(128);for(u32 i=0;i<size;++i){AnimationGraphState state;state.id=in.string();state.clipId=in.string(128,true);state.poseNodeId=in.string(128,true);state.speed=in.scalar();graph.states.push_back(std::move(state));}
            size=in.count(256);for(u32 i=0;i<size;++i){AnimationGraphTransition transition;transition.fromState=in.string();transition.toState=in.string();transition.duration=in.scalar();transition.exitTime=in.scalar();transition.priority=s32(in.word());transition.synchronize=in.boolean();const auto nested=in.count(8);
                for(u32 j=0;j<nested;++j){AnimationGraphCondition condition;condition.parameter=in.string();const auto op=in.word();require(op<=u32(AnimationGraphComparison::LessOrEqual),"Unknown typed condition operator");condition.comparison=AnimationGraphComparison(op);condition.threshold=in.scalar();condition.booleanValue=in.boolean();condition.isBoolean=in.boolean();transition.conditions.push_back(std::move(condition));}graph.transitions.push_back(std::move(transition));}validate(graph);return graph;
        }
        bool sameRig(const RuntimeResource &a,const RuntimeResource &b)
        {return a.header.resourceId==b.header.resourceId&&a.header.compilerVersion==b.header.compilerVersion&&a.header.sourceHash==b.header.sourceHash&&a.header.payloadHash==b.header.payloadHash&&a.header.payloadSize==b.header.payloadSize;}
        std::shared_ptr<const RuntimeResource> dependency(const RuntimeResource &parent,const String &id)
        {for(const auto &child:parent.dependencies)if(child&&child->header.resourceId.str()==id)return child;throw std::runtime_error("Typed animation is missing its exact installed dependency");}
        void dependencies(const RuntimeResource &value,const Array<String> &expected)
        {
            require(value.header.installDependencies.size()==expected.size()&&value.dependencies.size()==expected.size(),"Typed animation dependency count mismatch");std::set<String> seen;
            for(const auto &child:value.dependencies)require(child&&seen.insert(child->header.resourceId.str()).second,"Duplicate or null typed animation dependency");
            for(const auto &id:expected){require(seen.count(id)!=0,"Typed animation dependency identity mismatch");bool declared=false;for(const auto &header:value.header.installDependencies)declared|=header.str()==id;require(declared,"Undeclared typed animation dependency");}
            if(value.header.formatVersion>=2)for(size_t i=0;i<value.header.installDependencies.size();++i){const auto child=dependency(value,value.header.installDependencies[i].str());const auto &pin=value.header.installArtifacts[i];
                require(pin.sourceHash==child->header.sourceHash&&pin.payloadHash==child->header.payloadHash,"Typed animation dependency generation pin differs from its installed child");}
        }
        StreamBudget admitted(const RuntimeResource &value,const char *type,const char *magic,String &error)
        {
            require(value.payload.size()>=8&&value.payload.size()<=maximumBytes&&value.header.resourceId.str().size()<=1024&&value.header.installDependencies.size()<=2&&value.dependencies.size()<=2&&value.header.installArtifacts.size()<=2,"Typed animation extent/dependency count exceeds bounds");
            for(const auto &id:value.header.installDependencies)require(id.str().size()<=1024,"Typed animation dependency identity exceeds bound");
            for(const auto &artifact:value.header.installArtifacts)require(artifact.path.size()<=4096,"Typed animation artifact identity exceeds bound");
            auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*value.payload.size()+65536,error);require(bool(budget),"Typed animation decode admission denied");
            const AnimationResourceCompiler compiler;require(value.header.isValid()&&value.header.resourceId.type()==ResourceTypeID(type)&&value.header.resourceType==ResourceTypeID(type)&&!value.header.resourceId.isSubResource()&&value.header.compilerVersion==compiler.versionFor(ResourceTypeID(type)),"Unsupported typed animation identity/compiler version");
            require(value.payload.size()>=8&&value.payload.size()<=maximumBytes&&value.header.payloadSize==value.payload.size()&&hashBytes(value.payload.data(),value.payload.size())==value.header.payloadHash&&std::memcmp(value.payload.data(),magic,4)==0,"Typed animation payload integrity/magic mismatch");
            return budget;
        }
        Mask readMask(const RuntimeResource &value,String &error)
        {
            auto work=admitted(value,"amask","WPAM",error);Reader in{value.payload,4};require(in.word()==1,"Unsupported animation mask encoding");Mask mask;mask.rig=in.string(1024);mask.base=0;const auto size=in.count(256);
            for(u32 i=0;i<size;++i){auto name=in.string();const auto weight=in.scalar();mask.bones.push_back({std::move(name),weight});}require(in.at==value.payload.size(),"Trailing animation mask data");validate(mask);dependencies(value,{mask.rig});return mask;
        }
        IAnimationState::BoneBlendMask maskFor(const Mask &mask,MeshSkeleton &rig,bool compiled)
        {
            const auto &bones=rig.getBones();require(!bones.empty()&&bones.size()<=256&&(!compiled||mask.bones.size()==bones.size()),"Animation mask rig bone count mismatch");
            IAnimationState::BoneBlendMask result(bones.size(),mask.base);std::set<String> seen;
            for(const auto &entry:mask.bones){auto bone=rig.getBone(entry.first);require(bone&&bone->getBoneHandle()<bones.size()&&seen.insert(entry.first).second,"Mask references an unknown or duplicate rig bone");result[bone->getBoneHandle()]=entry.second;}
            if(compiled)for(size_t i=0;i<bones.size();++i)require(bones[i]&&bones[i]->getBoneHandle()==i&&mask.bones[i].first==bones[i]->getName(),"Compiled mask bone signature differs from rig");return result;
        }
        void bindGraphResources(const Graph &graph,const std::shared_ptr<const RuntimeResource> &rig,const RuntimeResource *mask,AnimationGraphResource &output,String &error)
        {
            auto skeleton=SkeletonResourceCompiler::decode(*rig,error);require(bool(skeleton),error.empty()?"Graph rig is invalid":error.c_str());AnimationGraphDefinition definition;
            for(const auto &clip:graph.clips){AnimationGraphClip native;native.id=clip.id;native.animation=skeleton->getAnimation(clip.name);native.speed=clip.speed;native.loop=clip.loop;require(definition.addClip(native),"Graph references a missing/nonplayable rig clip");}
            for(const auto &node:graph.nodes)require(definition.addPoseNode(node),"Invalid native graph pose node");for(const auto &state:graph.states)require(definition.addState(state),"Invalid native graph state");
            for(const auto &transition:graph.transitions)require(definition.addTransition(transition),"Invalid native graph transition");definition.setInitialState(graph.initial);
            if(mask){const auto decoded=readMask(*mask,error);require(decoded.rig==graph.rig&&sameRig(*dependency(*mask,decoded.rig),*rig),"Graph mask references a different rig generation");require(definition.setBoneBlendMask(maskFor(decoded,*skeleton,true)),"Invalid native graph bone mask");}
            require(definition.isValid(),"Decoded native graph is invalid");output.skeleton=std::move(skeleton);output.rig=rig;output.definition=std::move(definition);output.parameters=graph.parameters;
        }
        struct Artifact {StreamBudget working;RuntimeResource value;};
        Artifact artifact(const CompileContext &context,const String &id)
        {
            const auto found=context.dependencyArtifacts.find(id);require(found!=context.dependencyArtifacts.end(),"Animation dependency has no pinned compiled artifact");
            std::error_code status;const auto bytes=std::filesystem::file_size(std::filesystem::u8path(found->second.c_str()),status);require(!status&&bytes<=(12u<<20),"Animation dependency artifact exceeds bound");
            String error;Artifact result;result.working=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*bytes+65536,error);require(bool(result.working),"Animation dependency working admission denied");
            const bool loaded=CompiledResourceIO::read(found->second,result.value,error,2u<<20);require(loaded,error.empty()?"Cannot read animation dependency":error.c_str());require(result.value.header.resourceId.str()==id,"Pinned animation artifact identity differs from dependency");return result;
        }
    }

    String AnimationResourceCompiler::name() const{return "WorkphoneTypedAnimationGraphMask";}
    Array<CompilerOutput> AnimationResourceCompiler::outputs() const{return {{ResourceTypeID("agraph"),1},{ResourceTypeID("amask"),1}};}
    bool AnimationResourceCompiler::getDependencies(const CompileContext &context,DependencySet &output,String &error) const
    {
        error.clear();try{require(!context.resourceId.isSubResource(),"Animation subasset selectors are unsupported");auto input=source(context.sourcePath);auto root=document(input.text);Array<String> ids;
            if(context.resourceId.type()==ResourceTypeID("amask"))ids.push_back(parseMask(root).rig);
            else {require(context.resourceId.type()==ResourceTypeID("agraph"),"Unsupported typed animation resource");const auto graph=parseGraph(root);ids.push_back(graph.rig);if(!graph.mask.empty())ids.push_back(graph.mask);}
            for(const auto &id:ids){const ResourceID dependencyId(id);output.compileDependencies.push_back(CompileDependency::resource(dependencyId));output.installDependencies.push_back(dependencyId);}return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    CompilationStatus AnimationResourceCompiler::compile(const CompileContext &context,std::ostream &out,Array<String> &messages) const
    {
        try{require(!context.resourceId.isSubResource(),"Animation subasset selectors are unsupported");auto input=source(context.sourcePath);auto root=document(input.text);String error;
            if(context.resourceId.type()==ResourceTypeID("amask")){
                const auto mask=parseMask(root);auto compiled=artifact(context,mask.rig);auto rig=SkeletonResourceCompiler::decode(compiled.value,error);require(bool(rig),error.empty()?"Invalid mask rig":error.c_str());const auto weights=maskFor(mask,*rig,false);
                out.write("WPAM",4);word(out,1);string(out,mask.rig);word(out,u32(weights.size()));const auto &bones=rig->getBones();for(size_t i=0;i<weights.size();++i){const auto boneName=bones[i]->getName();for(unsigned char c:boneName)require(c>=32&&c<=126,"Typed mask rig bone names require printable ASCII");string(out,boneName);scalar(out,weights[i]);}
            }else {require(context.resourceId.type()==ResourceTypeID("agraph"),"Unsupported typed animation resource");const auto graph=parseGraph(root);auto compiled=artifact(context,graph.rig);auto rig=std::make_shared<RuntimeResource>(std::move(compiled.value));
                Artifact mask;if(!graph.mask.empty()){mask=artifact(context,graph.mask);for(const auto &id:mask.value.header.installDependencies)require(id.str()==graph.rig,"Graph mask has an incompatible rig dependency");mask.value.dependencies.push_back(rig);}
                AnimationGraphResource proof;bindGraphResources(graph,rig,graph.mask.empty()?nullptr:&mask.value,proof,error);write(out,graph);}
            require(bool(out),"Cannot write typed animation payload");return CompilationStatus::Success;
        }catch(const std::exception &failure){messages.push_back(failure.what());return CompilationStatus::Failure;}
    }
    bool AnimationResourceCompiler::decodeGraph(const RuntimeResource &value,AnimationGraphResource &output,String &error)
    {
        error.clear();try{auto work=admitted(value,"agraph","WPAG",error);Reader in{value.payload,4};require(in.word()==1,"Unsupported graph encoding");const auto graph=readGraph(in);require(in.at==value.payload.size(),"Trailing animation graph data");
            Array<String> expected{graph.rig};if(!graph.mask.empty())expected.push_back(graph.mask);dependencies(value,expected);auto rig=dependency(value,graph.rig);auto mask=graph.mask.empty()?nullptr:dependency(value,graph.mask);
            AnimationGraphResource candidate;bindGraphResources(graph,rig,mask.get(),candidate,error);output=std::move(candidate);return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    bool AnimationResourceCompiler::decodeMask(const RuntimeResource &value,IAnimationState::BoneBlendMask &output,String &error)
    {
        error.clear();try{auto work=admitted(value,"amask","WPAM",error);const auto mask=readMask(value,error);auto rig=SkeletonResourceCompiler::decode(*dependency(value,mask.rig),error);require(bool(rig),error.empty()?"Invalid animation mask rig":error.c_str());auto candidate=maskFor(mask,*rig,true);output.swap(candidate);return true;}
        catch(const std::exception &failure){report(error,failure.what());return false;}
    }
}
