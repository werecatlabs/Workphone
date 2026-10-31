#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/Mesh/XMLSkeletonSerializer.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <tinyxml.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <limits>
#include <stdexcept>

namespace workphone::resource
{
    namespace
    {
        constexpr size_t maximumBytes=2u<<20;
        using Names=std::set<std::string>;
        void require(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
        void preflight(const String &text)
        {
            require(!text.empty() && text.size()<=maximumBytes && text.find('\0')==String::npos,"Invalid skeleton XML size or NUL");
            for(size_t i=0;i<text.size();) {
                const auto first=static_cast<unsigned char>(text[i++]);
                if(first<128) { require(first>=32 || first==9 || first==10 || first==13,"Invalid XML control character"); continue; }
                unsigned count=0; unsigned value=0,minimum=0;
                if(first>=0xc2 && first<=0xdf) { count=1; value=first&31; minimum=128; }
                else if(first>=0xe0 && first<=0xef) { count=2; value=first&15; minimum=2048; }
                else if(first>=0xf0 && first<=0xf4) { count=3; value=first&7; minimum=65536; }
                else require(false,"Invalid skeleton UTF-8");
                require(i+count<=text.size(),"Truncated skeleton UTF-8");
                while(count--) { const auto next=static_cast<unsigned char>(text[i++]); require((next&0xc0)==0x80,"Invalid skeleton UTF-8 continuation"); value=(value<<6)|(next&63); }
                require(value>=minimum && value<=0x10ffff && (value<0xd800 || value>0xdfff),"Invalid skeleton Unicode scalar");
            }
            size_t position=0; unsigned depth=0;
            while((position=text.find('<',position))!=String::npos) {
                if(text.compare(position,4,"<!--")==0) {
                    const auto end=text.find("-->",position+4); require(end!=String::npos,"Unterminated XML comment"); position=end+3; continue;
                }
                require(position+1<text.size() && text[position+1]!='!',"Skeleton DTD/CDATA declarations are unsupported");
                if(text[position+1]=='?') {
                    const bool leading=position==0 || (position==3 && text.compare(0,3,"\xef\xbb\xbf")==0);
                    require(text.compare(position,5,"<?xml")==0 && leading,"Unsupported XML processing instruction");
                    const auto end=text.find("?>",position+5); require(end!=String::npos,"Unterminated XML declaration"); position=end+2; continue;
                }
                const bool closing=text[position+1]=='/'; char quote=0; size_t end=position+1;
                for(;end<text.size();++end) {
                    const char c=text[end]; if(quote) { if(c==quote) quote=0; }
                    else if(c=='\'' || c=='"') quote=c;
                    else if(c=='>') break;
                }
                require(end<text.size(),"Unterminated XML element");
                size_t last=end; while(last>position && std::isspace(static_cast<unsigned char>(text[last-1]))) --last;
                const bool empty=last>position && text[last-1]=='/';
                if(closing) { require(depth>0,"Invalid XML nesting"); --depth; }
                else if(!empty) require(++depth<=16,"Skeleton XML exceeds depth 16");
                position=end+1;
            }
            require(depth==0,"Unclosed XML elements");
        }
        void attributes(const TiXmlElement *node,const Names &allowed)
        {
            for(auto a=node->FirstAttribute();a;a=a->Next()) require(allowed.count(a->Name())!=0,"Unsupported skeleton XML attribute");
        }
        void children(const TiXmlElement *node,const Names &allowed,bool unique=true)
        {
            Names seen;
            for(auto child=node->FirstChild();child;child=child->NextSibling()) {
                if(const auto element=child->ToElement()) {
                    require(allowed.count(element->Value())!=0,"Unsupported skeleton XML element");
                    require(!unique || seen.insert(element->Value()).second,"Repeated singleton skeleton element");
                } else if(const auto text=child->ToText()) {
                    for(const unsigned char c:std::string(text->Value())) require(std::isspace(c)!=0,"Unexpected skeleton text");
                } else require(child->ToComment()!=nullptr,"Unsupported skeleton XML node");
            }
        }
        std::string name(const TiXmlElement *node,const char *key)
        {
            auto value=node->Attribute(key); require(value && *value && std::strlen(value)<=128,"Invalid skeleton name");
            return value;
        }
        double number(const TiXmlElement *node,const char *key)
        {
            const auto text=node?node->Attribute(key):nullptr; require(text && *text,"Missing skeleton number");
            char *end=nullptr; const auto value=std::strtod(text,&end);
            require(end!=text && end && *end=='\0' && std::isfinite(value) && std::abs(value)<=1000000,"Invalid finite skeleton number");
            return value;
        }
        int handle(const TiXmlElement *node,const char *key)
        {
            const auto text=node->Attribute(key); require(text && *text && std::strlen(text)<=3,"Invalid bone handle spelling");
            for(const auto c:std::string(text)) require(c>='0' && c<='9',"Bone handles require decimal integer spelling");
            const auto value=std::atoi(text); require(value>=0 && value<256,"Bone handle exceeds supported range"); return value;
        }
        void vector(const TiXmlElement *node,bool unit=false)
        {
            if(!node) return;
            attributes(node,{"x","y","z"}); children(node,{});
            for(const auto axis:{"x","y","z"}) require(!unit || number(node,axis)==1,"Bone scale requires a scale-capable runtime");
            (void)number(node,"x"); (void)number(node,"y"); (void)number(node,"z");
        }
        void rotation(const TiXmlElement *node)
        {
            if(!node) return;
            attributes(node,{"qw","qx","qy","qz"}); children(node,{}); double norm=0;
            for(const auto axis:{"qw","qx","qy","qz"}) { const auto value=number(node,axis); norm+=value*value; }
            require(std::abs(norm-1)<0.0001,"Skeleton rotations require normalized quaternions");
        }
        void validate(const String &text)
        {
            preflight(text); TiXmlDocument document; document.Parse(text.c_str(),nullptr,TIXML_ENCODING_UTF8);
            const auto root=document.RootElement();
            require(!document.Error() && root && std::string(root->Value())=="skeleton","Invalid skeleton document");
            for(auto node=document.FirstChild();node;node=node->NextSibling())
                require(node==root || node->ToComment()!=nullptr || node->ToDeclaration()!=nullptr,"Extra skeleton document content");
            attributes(root,{"blendmode"}); children(root,{"bones","bonehierarchy","animations"});
            if(auto mode=root->Attribute("blendmode")) require(std::string(mode)=="average" || std::string(mode)=="cumulative","Unsupported skeleton blend mode");
            auto bones=root->FirstChildElement("bones"); require(bones!=nullptr,"Skeleton has no bones");
            attributes(bones,{}); children(bones,{"bone"},false);
            std::map<std::string,int> ids; std::set<int> handles;
            for(auto bone=bones->FirstChildElement();bone;bone=bone->NextSiblingElement()) {
                require(ids.size()<256,"Skeleton exceeds 256 bones"); attributes(bone,{"id","name"}); children(bone,{"position","rotation","scale"});
                const auto id=handle(bone,"id");
                require(id==int(ids.size()) && handles.insert(id).second,"Bone handles must be contiguous and ordered");
                require(ids.emplace(name(bone,"name"),id).second,"Duplicate bone name");
                vector(bone->FirstChildElement("position")); rotation(bone->FirstChildElement("rotation")); vector(bone->FirstChildElement("scale"),true);
            }
            require(!ids.empty() && *handles.rbegin()==int(ids.size()-1),"Skeleton bone handles must be contiguous");
            std::map<std::string,std::string> parents;
            if(auto hierarchy=root->FirstChildElement("bonehierarchy")) {
                attributes(hierarchy,{}); children(hierarchy,{"boneparent"},false);
                for(auto link=hierarchy->FirstChildElement();link;link=link->NextSiblingElement()) {
                    attributes(link,{"bone","parent"}); children(link,{});
                    const auto bone=name(link,"bone"),parent=name(link,"parent");
                    require(ids.count(bone) && ids.count(parent) && bone!=parent && parents.emplace(bone,parent).second,"Invalid bone parent binding");
                }
                for(const auto &bone:ids) { Names visited; auto current=bone.first;
                    while(parents.count(current)) { require(visited.insert(current).second && visited.size()<64,"Cyclic/deep skeleton hierarchy"); current=parents.at(current); }
                }
            }
            size_t totalKeys=0,totalTracks=0;
            if(auto animations=root->FirstChildElement("animations")) {
                attributes(animations,{}); children(animations,{"animation"},false); Names clips;
                for(auto clip=animations->FirstChildElement();clip;clip=clip->NextSiblingElement()) {
                    require(clips.size()<64 && clips.insert(name(clip,"name")).second,"Duplicate or excess skeleton clips");
                    attributes(clip,{"name","length"}); children(clip,{"tracks"}); const auto length=number(clip,"length");
                    require(length>0 && length<=86400,"Invalid skeleton clip duration");
                    auto tracks=clip->FirstChildElement("tracks"); require(tracks!=nullptr,"Clip requires tracks"); attributes(tracks,{}); children(tracks,{"track"},false); Names names;
                    for(auto track=tracks->FirstChildElement();track;track=track->NextSiblingElement()) {
                        require(++totalTracks<=2048,"Excess skeleton tracks"); attributes(track,{"bone","handle"}); children(track,{"keyframes"});
                        const auto bone=name(track,"bone"); require(ids.count(bone) && names.insert(bone).second,"Missing or duplicate clip bone");
                        if(track->Attribute("handle")) require(handle(track,"handle")==ids.at(bone),"Track handle mismatch");
                        auto keys=track->FirstChildElement("keyframes"); require(keys!=nullptr,"Track requires keys"); attributes(keys,{}); children(keys,{"keyframe"},false);
                        float previous=-1;
                        for(auto key=keys->FirstChildElement();key;key=key->NextSiblingElement()) {
                            require(++totalKeys<=16384,"Excess skeleton keyframes"); attributes(key,{"time"}); children(key,{"translate","rotation","scale"});
                            const auto time=number(key,"time"); const auto runtimeTime=static_cast<float>(time);
                            // The native track coalesces keys within one absolute
                            // float epsilon. Distinct source keys must survive
                            // that insertion rule without replacing one another.
                            require(time>=0 && time<=length && runtimeTime>previous &&
                                (previous<0 || runtimeTime-previous>std::numeric_limits<f32>::epsilon()),
                                "Skeleton key times must remain distinct under native track insertion"); previous=runtimeTime;
                            vector(key->FirstChildElement("translate")); rotation(key->FirstChildElement("rotation")); vector(key->FirstChildElement("scale"),true);
                        }
                        require(previous>=0,"Track has no keyframes");
                    }
                    require(!names.empty(),"Clip has no tracks");
                }
            }
        }
        StreamBudget scratch(size_t bytes,String &error)
        { return WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,bytes*16ull+65536,error); }
    }
    String SkeletonResourceCompiler::name() const { return "WorkphoneSkeletonClipsXml"; }
    Array<CompilerOutput> SkeletonResourceCompiler::outputs() const { return {{ResourceTypeID("skelres"),1}}; }
    bool SkeletonResourceCompiler::validateSource(const String &text,String &error)
    {
        error.clear();
        if(text.size()>maximumBytes) { error="Skeleton source exceeds 2 MiB"; return false; }
        auto budget=scratch(text.size(),error); if(!budget) return false;
        try { validate(text); return true; } catch(const std::exception &e) { error=e.what(); return false; }
    }
    CompilationStatus SkeletonResourceCompiler::compile(const CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        if(context.resourceId.isSubResource()) { messages.push_back("Skeleton subasset selection is unsupported"); return CompilationStatus::Failure; }
        std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
        const auto length=input?input.tellg():std::streampos(-1);
        if(length<=0 || length>maximumBytes) { messages.push_back("Skeleton source requires 1 byte to 2 MiB"); return CompilationStatus::Failure; }
        String error; auto budget=scratch(static_cast<size_t>(length),error);
        if(!budget) { messages.push_back(error); return CompilationStatus::Failure; }
        String text(static_cast<size_t>(length),'\0'); input.seekg(0); input.read(&text[0],text.size());
        try { require(bool(input),"Incomplete skeleton source read"); validate(text); output.write(text.data(),text.size());
            return output?CompilationStatus::Success:CompilationStatus::Failure;
        } catch(const std::exception &e) { messages.push_back(e.what()); return CompilationStatus::Failure; }
    }
    SmartPtr<MeshSkeleton> SkeletonResourceCompiler::decode(const RuntimeResource &resource,String &error)
    {
        error.clear(); const SkeletonResourceCompiler compiler;
        if(!resource.header.isValid() || resource.header.resourceType!=ResourceTypeID("skelres") || resource.header.resourceId.type()!=ResourceTypeID("skelres") || resource.header.resourceId.isSubResource() ||
           resource.header.compilerVersion!=compiler.versionFor(ResourceTypeID("skelres")) ||
           !resource.header.installDependencies.empty() || !resource.header.installArtifacts.empty() || !resource.dependencies.empty() || resource.payload.empty() || resource.payload.size()>maximumBytes)
        { error="Unsupported typed skeleton identity/version/dependencies"; return nullptr; }
        if(resource.header.payloadSize!=resource.payload.size() || hashBytes(resource.payload.data(),resource.payload.size())!=resource.header.payloadHash)
        { error="Typed skeleton payload size/hash mismatch"; return nullptr; }
        auto budget=scratch(resource.payload.size(),error); if(!budget) return nullptr;
        try {
            const String text(reinterpret_cast<const char*>(resource.payload.data()),resource.payload.size()); validate(text);
            auto candidate=make_ptr<MeshSkeleton>();
            if(!XMLSkeletonSerializer{}.importSkeletonData(text,candidate.get(),error)) return nullptr;
            return candidate;
        } catch(const std::exception &e) { error=e.what(); return nullptr; }
    }
}
