#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/RetargetResourceCompiler.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Animation/ActorAnimationTrack.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <map>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace workphone::resource
{
    namespace
    {
        constexpr size_t maximumBytes=256u<<10;
        void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
        void report(String &error,const char *message) noexcept
        {try{error=message;}catch(...){try{error="Retarget resource failed";}catch(...){error.clear();}}}
        struct Clip {String source,target;};
        struct Mapping {String source,target;f32 scale=1;};
        struct Definition
        {
            String source,target;
            Array<Clip> clips=makeSourceBudgetSnapshot<Clip>();
            Array<Mapping> bones=makeSourceBudgetSnapshot<Mapping>();
        };
        void name(const String &value,size_t bound=128)
        {require(!value.empty()&&value.size()<=bound,"Retarget name exceeds its bound");for(unsigned char c:value)require(c>=32&&c<=126,"Retarget names require printable ASCII without NUL");}
        ResourceID rigId(const String &value)
        {name(value,1024);ResourceID id;String error;require(id.set(value,&error)&&id.str()==value&&!id.isSubResource()&&id.type()==ResourceTypeID("skelres"),"Retarget rigs require canonical root skelres ResourceIDs");return id;}
        void validate(const Definition &value)
        {
            rigId(value.source);rigId(value.target);require(value.source!=value.target,"Retarget source and target must be distinct rig resources");
            require(!value.clips.empty()&&value.clips.size()<=64&&!value.bones.empty()&&value.bones.size()<=256,"Retarget requires 1-64 clips and 1-256 bone mappings");
            std::set<String> sourceClips,targetClips,sourceBones,targetBones;
            for(const auto &clip:value.clips){name(clip.source);name(clip.target);require(sourceClips.insert(clip.source).second&&targetClips.insert(clip.target).second,"Retarget clip aliases must be one-to-one");}
            for(const auto &bone:value.bones){name(bone.source);name(bone.target);require(sourceBones.insert(bone.source).second&&targetBones.insert(bone.target).second&&std::isfinite(bone.scale)&&bone.scale>0&&bone.scale<=1000,"Retarget bone mappings must be one-to-one with scale in (0,1000]");}
        }
        void fields(const rapidjson::Value &value,std::initializer_list<const char*> expected)
        {
            require(value.IsObject(),"Retarget record must be an object");std::set<String> seen;
            for(auto i=value.MemberBegin();i!=value.MemberEnd();++i){require(!std::memchr(i->name.GetString(),0,i->name.GetStringLength()),"Retarget field contains NUL");String key(i->name.GetString(),i->name.GetStringLength());bool known=false;for(auto name:expected)known|=key==name;require(known&&seen.insert(key).second,"Unknown or duplicate retarget field");}
            for(auto key:expected)require(seen.count(key)!=0,"Missing retarget field");
        }
        String retargetText(const rapidjson::Value &value,size_t maximum=128)
        {require(value.IsString(),"Retarget string field has wrong type");String result(value.GetString(),value.GetStringLength());name(result,maximum);return result;}
        Definition parse(const String &text)
        {
            rapidjson::Document root;root.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(text.data(),text.size());require(!root.HasParseError(),"Invalid retarget JSON/UTF8");
            fields(root,{"version","source","target","clips","bones"});require(root["version"].IsUint()&&root["version"].GetUint()==1,"Unsupported retarget source version");Definition result;
            // Explicit lengths keep embedded NULs visible to validation.
            result.source=retargetText(root["source"],1024);
            result.target=retargetText(root["target"],1024);
            require(root["clips"].IsArray()&&root["clips"].Size()<=64&&root["bones"].IsArray()&&root["bones"].Size()<=256,"Retarget record count exceeds bounds");
            for(const auto &entry:root["clips"].GetArray()){fields(entry,{"source","target"});result.clips.push_back({retargetText(entry["source"]),retargetText(entry["target"])});}
            for(const auto &entry:root["bones"].GetArray()){fields(entry,{"source","target","translationScale"});require(entry["translationScale"].IsNumber()&&std::isfinite(entry["translationScale"].GetDouble()),"Retarget scale must be finite");result.bones.push_back({retargetText(entry["source"]),retargetText(entry["target"]),f32(entry["translationScale"].GetDouble())});}
            validate(result);return result;
        }
        struct Input {StreamBudget work;String text;};
        Input source(const String &path)
        {
            std::ifstream file(std::filesystem::u8path(path.c_str()),std::ios::binary|std::ios::ate);const auto bytes=file?file.tellg():std::streampos(-1);require(bytes>0&&bytes<=maximumBytes,"Retarget source requires 1-262144 bytes");
            Input value;String error;value.work=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*u64(bytes)+65536,error);require(bool(value.work),"Retarget parser working admission denied");value.text.resize(size_t(bytes));file.seekg(0);file.read(value.text.data(),value.text.size());require(bool(file)&&value.text.find('\0')==String::npos,"Incomplete retarget source or NUL");return value;
        }
        void word(std::ostream &out,u32 value){for(unsigned i=0;i<4;++i)out.put(char(value>>(i*8)));}
        void string(std::ostream &out,const String &value){word(out,u32(value.size()));out.write(value.data(),value.size());}
        void scalar(std::ostream &out,f32 value){u32 bits;std::memcpy(&bits,&value,4);word(out,bits);}
        struct Reader
        {
            const Array<u8> &bytes;size_t at=0;
            u32 word(){require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated retarget record");u32 value=0;for(unsigned i=0;i<4;++i)value|=u32(bytes[at++])<<(i*8);return value;}
            u32 count(u32 maximum){const auto result=word();require(result&&result<=maximum,"Retarget count exceeds bounds");return result;}
            String string(size_t maximum=128){const auto count=word();require(count&&count<=maximum&&count<=bytes.size()-at,"Retarget string exceeds its encoded extent");String value(reinterpret_cast<const char*>(bytes.data()+at),count);at+=count;name(value,maximum);return value;}
            f32 scalar(){const auto bits=word();f32 result;std::memcpy(&result,&bits,4);require(std::isfinite(result),"Nonfinite retarget scale");return result;}
        };
        StreamBudget admitted(const RuntimeResource &value,String &error)
        {
            require(value.payload.size()>=8&&value.payload.size()<=maximumBytes&&value.header.resourceId.str().size()<=1024&&value.header.installDependencies.size()==2&&value.dependencies.size()<=2&&value.header.installArtifacts.size()<=2,"Retarget extent/dependency count exceeds bounds");
            for(const auto &id:value.header.installDependencies)require(id.str().size()<=1024,"Retarget rig identity exceeds bound");for(const auto &artifact:value.header.installArtifacts)require(artifact.path.size()<=4096,"Retarget artifact path exceeds bound");
            auto work=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*value.payload.size()+65536,error);require(bool(work),"Retarget decode working admission denied");RetargetResourceCompiler compiler;
            require(value.header.isValid()&&value.header.resourceId.type()==ResourceTypeID("aretgt")&&value.header.resourceType==ResourceTypeID("aretgt")&&!value.header.resourceId.isSubResource()&&value.header.compilerVersion==compiler.versionFor(ResourceTypeID("aretgt")),"Unsupported retarget identity/compiler version");
            require(value.header.payloadSize==value.payload.size()&&hashBytes(value.payload.data(),value.payload.size())==value.header.payloadHash&&std::memcmp(value.payload.data(),"WPRT",4)==0,"Retarget payload integrity/magic mismatch");return work;
        }
        Definition read(const RuntimeResource &value)
        {
            Reader input{value.payload,4};require(input.word()==1,"Unsupported retarget encoding version");Definition result;result.source=input.string(1024);result.target=input.string(1024);
            auto size=input.count(64);for(u32 i=0;i<size;++i){Clip clip;clip.source=input.string();clip.target=input.string();result.clips.push_back(std::move(clip));}
            size=input.count(256);for(u32 i=0;i<size;++i){Mapping bone;bone.source=input.string();bone.target=input.string();bone.scale=input.scalar();result.bones.push_back(std::move(bone));}
            require(input.at==value.payload.size(),"Trailing retarget record data");validate(result);return result;
        }
        std::shared_ptr<const RuntimeResource> dependency(const RuntimeResource &parent,const String &id)
        {for(const auto &child:parent.dependencies)if(child&&child->header.resourceId.str()==id)return child;throw std::runtime_error("Retarget is missing its exact rig dependency");}
        void pins(const RuntimeResource &parent,const Definition &definition)
        {
            require(parent.dependencies.size()==2&&parent.header.installDependencies.size()==2,"Retarget requires exactly two installed rig dependencies");std::set<String> expected{definition.source,definition.target},actual;
            for(const auto &id:parent.header.installDependencies)require(expected.count(id.str())&&actual.insert(id.str()).second,"Retarget declared rig identities mismatch");actual.clear();
            for(const auto &child:parent.dependencies)require(child&&expected.count(child->header.resourceId.str())&&actual.insert(child->header.resourceId.str()).second,"Retarget installed rig identities mismatch");
            if(parent.header.formatVersion>=2)for(size_t i=0;i<2;++i){const auto child=dependency(parent,parent.header.installDependencies[i].str());const auto &pin=parent.header.installArtifacts[i];require(pin.sourceHash==child->header.sourceHash&&pin.payloadHash==child->header.payloadHash,"Retarget rig generation pin mismatch");}
        }
        Quaternion<real_Num> normalized(Quaternion<real_Num> value)
        {
            const auto norm=value.W()*value.W()+value.X()*value.X()+value.Y()*value.Y()+value.Z()*value.Z();
            require(std::isfinite(norm)&&norm>1e-12,"Retarget quaternion is degenerate/nonfinite");value.normalise();require(std::isfinite(value.W())&&std::isfinite(value.X())&&std::isfinite(value.Y())&&std::isfinite(value.Z()),"Retarget quaternion cannot be normalized");return value;
        }
        void position(const Vector3<real_Num> &value)
        {for(const auto axis:{value.X(),value.Y(),value.Z()})require(std::isfinite(axis)&&std::abs(axis)<=1000000,"Retarget transformed position exceeds finite native bounds");}
        void transfer(const Definition &definition,const std::shared_ptr<const RuntimeResource> &sourceResource,const std::shared_ptr<const RuntimeResource> &targetResource,RetargetAnimationResource &output,String &error)
        {
            require(sourceResource->header.payloadSize<=(2u<<20)&&targetResource->header.payloadSize<=(2u<<20),"Retarget rig payload exceeds skeleton bounds");
            auto work=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,2*(sourceResource->header.payloadSize+targetResource->header.payloadSize)+65536,error);require(bool(work),"Retarget transfer working admission denied");
            auto source=SkeletonResourceCompiler::decode(*sourceResource,error);require(bool(source),error.empty()?"Invalid source retarget rig":error.c_str());auto target=SkeletonResourceCompiler::decode(*targetResource,error);require(bool(target),error.empty()?"Invalid target retarget rig":error.c_str());
            struct Binding {SmartPtr<Bone> source,target;f32 scale;};Array<Binding> bindings=makeSourceBudgetSnapshot<Binding>();std::map<String,size_t> mapping;
            for(const auto &entry:definition.bones){auto from=dynamic_pointer_cast<Bone>(source->getBone(entry.source)),to=dynamic_pointer_cast<Bone>(target->getBone(entry.target));require(from&&to,"Retarget mapping references an unknown native bone");
                mapping.emplace(entry.source,bindings.size());bindings.push_back({from,to,entry.scale});position(from->getBindingPosition());position(to->getBindingPosition());normalized(from->getBindingOrientation());normalized(to->getBindingOrientation());}
            for(const auto &binding:bindings){auto from=binding.source,to=binding.target;auto a=from->getParent(),b=to->getParent();require(bool(a)==bool(b),"Retarget mapped roots/parents differ");
                if(a){const auto found=mapping.find(a->getName());require(found!=mapping.end()&&bindings[found->second].target.get()==b.get(),"Retarget immediate source parent must map to immediate target parent");}}
            Array<SmartPtr<IAnimation>> clips=makeSourceBudgetSnapshot<SmartPtr<IAnimation>>();size_t totalKeys=0,totalTracks=0;
            for(const auto &clip:definition.clips){auto sourceClip=source->getAnimation(clip.source);require(sourceClip&&!target->hasAnimation(clip.target),"Retarget source clip is missing or target alias collides");auto targetClip=target->createAnimation(clip.target,sourceClip->getLength());require(bool(targetClip),"Cannot allocate target retarget clip");
                targetClip->setInterpolationMode(sourceClip->getInterpolationMode());targetClip->setRotationInterpolationMode(sourceClip->getRotationInterpolationMode());
                for(const auto &pair:sourceClip->_getNodeTrackList()){auto track=dynamic_cast<ActorAnimationTrack*>(pair.second);require(track&&!track->getActor()&&track->getTrackType()==IActorAnimationTrack::TrackType::Transform&&++totalTracks<=2048,"Retarget supports bounded native bone transform tracks only");
                    auto boneName=track->getPropertyName();if(boneName.empty())if(auto bone=track->getBone())boneName=bone->getName();const auto found=mapping.find(boneName);require(found!=mapping.end(),"Every animated source bone requires an explicit retarget mapping");auto binding=bindings[found->second];
                    auto destination=targetClip->addTrackByType<ActorAnimationTrack>(binding.target->getBoneHandle(),binding.target);require(bool(destination),"Cannot allocate native retarget track");destination->setPropertyName(binding.target->getName());destination->setTrackType(IActorAnimationTrack::TrackType::Transform);
                    const auto sourceBind=normalized(binding.source->getBindingOrientation()),targetBind=normalized(binding.target->getBindingOrientation());
                    for(const auto &frame:track->getKeyFrames()){auto key=dynamic_pointer_cast<KeyFrameTransform3>(frame);require(key&&++totalKeys<=16384,"Retarget supports at most16384 native transform keys");
                        const auto scale=key->getScale();require(scale==Vector3<real_Num>::unit(),"Retarget scale channels are unsupported");
                        const auto translated=binding.target->getBindingPosition()+(key->getPosition()-binding.source->getBindingPosition())*binding.scale;position(translated);
                        const auto rotation=normalized(targetBind*sourceBind.inverse()*normalized(key->getOrientation()));auto result=dynamic_pointer_cast<KeyFrameTransform3>(destination->createKeyFrame(key->getTime()));require(bool(result),"Cannot allocate retarget transform key");result->setPosition(translated);result->setOrientation(rotation);result->setScale(Vector3<real_Num>::unit());}
                }
                clips.push_back(targetClip);
            }
            output.skeleton=std::move(target);output.targetRig=targetResource;output.clips.swap(clips);
        }
        struct Artifact {StreamBudget work;std::shared_ptr<RuntimeResource> value;};
        Artifact artifact(const CompileContext &context,const String &id)
        {
            const auto found=context.dependencyArtifacts.find(id);require(found!=context.dependencyArtifacts.end(),"Retarget rig has no pinned compiled artifact");std::error_code ec;const auto bytes=std::filesystem::file_size(std::filesystem::u8path(found->second.c_str()),ec);require(!ec&&bytes<=(12u<<20),"Retarget rig artifact exceeds its bound");
            Artifact result;String error;result.work=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*bytes+65536,error);require(bool(result.work),"Retarget artifact working admission denied");result.value=std::make_shared<RuntimeResource>();const bool loaded=CompiledResourceIO::read(found->second,*result.value,error,2u<<20);require(loaded,error.empty()?"Cannot read retarget rig artifact":error.c_str());require(result.value->header.resourceId.str()==id,"Retarget pinned rig identity differs");return result;
        }
    }
    String RetargetResourceCompiler::name() const{return "WorkphoneLocalBindRetarget";}
    Array<CompilerOutput> RetargetResourceCompiler::outputs() const{return {{ResourceTypeID("aretgt"),1}};}
    bool RetargetResourceCompiler::getDependencies(const CompileContext &context,DependencySet &output,String &error) const
    {
        error.clear();try{require(context.resourceId.type()==ResourceTypeID("aretgt")&&!context.resourceId.isSubResource(),"Retarget compiler requires a root aretgt resource");auto input=source(context.sourcePath);const auto definition=parse(input.text);
            for(const auto &id:{definition.source,definition.target}){const ResourceID resource(id);output.compileDependencies.push_back(CompileDependency::resource(resource));output.installDependencies.push_back(resource);}return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    CompilationStatus RetargetResourceCompiler::compile(const CompileContext &context,std::ostream &out,Array<String> &messages) const
    {
        try{require(context.resourceId.type()==ResourceTypeID("aretgt")&&!context.resourceId.isSubResource(),"Retarget compiler requires a root aretgt resource");auto input=source(context.sourcePath);const auto value=parse(input.text);auto from=artifact(context,value.source),to=artifact(context,value.target);String error;RetargetAnimationResource proof;transfer(value,from.value,to.value,proof,error);
            out.write("WPRT",4);word(out,1);string(out,value.source);string(out,value.target);word(out,u32(value.clips.size()));for(const auto &clip:value.clips){string(out,clip.source);string(out,clip.target);}word(out,u32(value.bones.size()));for(const auto &bone:value.bones){string(out,bone.source);string(out,bone.target);scalar(out,bone.scale);}require(bool(out),"Cannot write typed retarget payload");return CompilationStatus::Success;
        }catch(const std::exception &failure){messages.push_back(failure.what());return CompilationStatus::Failure;}
    }
    bool RetargetResourceCompiler::decode(const RuntimeResource &value,RetargetAnimationResource &output,String &error)
    {
        error.clear();try{auto work=admitted(value,error);const auto definition=read(value);pins(value,definition);RetargetAnimationResource candidate;transfer(definition,dependency(value,definition.source),dependency(value,definition.target),candidate,error);output=std::move(candidate);return true;}
        catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    bool RetargetResourceCompiler::targetRigIdentity(const RuntimeResource &value,ResourceID &output,String &error)
    {
        error.clear();try{auto work=admitted(value,error);const auto definition=read(value);std::set<String> expected{definition.source,definition.target},seen;
            for(const auto &id:value.header.installDependencies)require(expected.count(id.str())&&seen.insert(id.str()).second,"Retarget declared rig identities mismatch");auto candidate=rigId(definition.target);output=std::move(candidate);return true;}
        catch(const std::exception &failure){report(error,failure.what());return false;}
    }
}
