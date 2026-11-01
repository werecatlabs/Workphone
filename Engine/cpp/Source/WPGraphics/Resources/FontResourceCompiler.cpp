#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/FontResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/Resources/ClawTextureStream.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <rapidjson/document.h>
#include <fstream>
#include <set>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>

namespace workphone::render
{
    namespace
    {
        constexpr u64 maximumBytes=256ull<<10;constexpr u32 maximumGlyphs=4096;
        static_assert(sizeof(f32)==4&&std::numeric_limits<f32>::is_iec559,"Bitmap fonts require IEEE-754 binary32");
        void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
        void report(String &out,const char *message) noexcept{try{out=message;}catch(...){try{out="Font resource failed";}catch(...){out.clear();}}}
        bool scalar(u32 value){return value>=32&&value<=0x10ffff&&!(value>=0xd800&&value<=0xdfff);}
        bool utf8(const String &text)
        {for(size_t i=0;i<text.size();){const auto first=static_cast<unsigned char>(text[i++]);if(!first)return false;if(first<0x80)continue;
            unsigned count=0;u32 value=0,minimum=0;if(first>=0xc2&&first<=0xdf){count=1;value=first&31;minimum=0x80;}else if(first>=0xe0&&first<=0xef){count=2;value=first&15;minimum=0x800;}else if(first>=0xf0&&first<=0xf4){count=3;value=first&7;minimum=0x10000;}else return false;
            if(count>text.size()-i)return false;while(count--){const auto next=static_cast<unsigned char>(text[i++]);if((next&0xc0)!=0x80)return false;value=(value<<6)|(next&63);}if(value<minimum||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return false;}return true;}
        void fields(const rapidjson::Value &value,std::initializer_list<const char *> expected)
        {
            require(value.IsObject()&&value.MemberCount()==expected.size(),"Invalid bitmap-font object fields");std::set<String> names;
            for(auto i=value.MemberBegin();i!=value.MemberEnd();++i){require(i->name.IsString()&&names.insert(String(i->name.GetString(),i->name.GetStringLength())).second,"Duplicate bitmap-font field");
                bool found=false;for(const auto name:expected)found|=std::strcmp(i->name.GetString(),name)==0;require(found,"Unknown bitmap-font field");}
            for(const auto name:expected)require(value.HasMember(name),"Missing bitmap-font field");
        }
        f32 number(const rapidjson::Value &value)
        {require(value.IsNumber()&&std::isfinite(value.GetDouble())&&std::abs(value.GetDouble())<=4096,"Nonfinite or out-of-range bitmap-font metric");return f32(value.GetDouble());}
        void validate(const CookedFontData &data,bool atlasKnown)
        {
            require(!data.target.empty()&&data.target.size()<=128&&utf8(data.target),"Bitmap-font target must be bounded valid UTF-8");
            require(data.atlas.isValid()&&!data.atlas.isSubResource()&&data.atlas.type()==resource::ResourceTypeID("texres")&&data.atlas.str().size()<=1024,"Bitmap font requires one typed atlas ResourceID");
            require(data.height>=1&&data.height<=1024&&std::isfinite(data.height)&&std::isfinite(data.ascent)&&std::isfinite(data.descent)&&data.ascent>=0&&data.descent<=0&&data.ascent-data.descent>0&&data.ascent-data.descent<=4*data.height,"Invalid bitmap-font line metrics");
            require(!data.glyphs.empty()&&data.glyphs.size()<=maximumGlyphs&&scalar(data.fallback),"Invalid bitmap-font glyph count or fallback");
            if(atlasKnown)require(data.atlasWidth&&data.atlasHeight&&data.atlasWidth<=1024&&data.atlasHeight<=1024&&data.whiteX<data.atlasWidth&&data.whiteY<data.atlasHeight&&data.atlasCompilerVersion,"Bitmap-font atlas/white texel exceeds its supported dimensions");
            u32 previous=0;bool fallback=false;
            for(const auto &g:data.glyphs){
                const f32 values[]={g.xadvance,g.x0,g.y0,g.x1,g.y1,g.width,g.height,g.u0,g.v0,g.u1,g.v1};for(const auto value:values)require(std::isfinite(value)&&std::abs(value)<=4096,"Invalid bitmap-font glyph metric");
                require(scalar(g.codepoint)&&g.codepoint>previous&&g.xadvance>=0&&g.x1>=g.x0&&g.y1>=g.y0&&g.x1-g.x0<=2048&&g.y1-g.y0<=2048,"Glyphs must be sorted unique Unicode scalars with bounded metrics");previous=g.codepoint;
                require(g.width==g.x1-g.x0&&g.height==g.y1-g.y0,"Bitmap-font width/height differ from glyph bounds");
                require(g.u0>=0&&g.v0>=0&&g.u1<=1&&g.v1<=1&&g.u1>=g.u0&&g.v1>=g.v0,"Bitmap-font glyph UVs exceed the pinned atlas");
                if(atlasKnown)require(std::abs((g.u1-g.u0)*data.atlasWidth-(g.x1-g.x0))<=.01f&&std::abs((g.v1-g.v0)*data.atlasHeight-(g.y1-g.y0))<=.01f,"Bitmap-font glyph bounds differ from atlas texel extents");
                if(g.codepoint==data.fallback){fallback=true;require(g.x1>g.x0&&g.y1>g.y0&&g.xadvance>0,"Bitmap-font fallback must have visible glyph geometry");}
            }
            require(fallback,"Bitmap-font fallback glyph is absent");
        }
        void atlasMetadata(const resource::RuntimeResource &atlas,CookedFontData &font)
        {
            require(atlas.header.resourceId==font.atlas&&atlas.header.resourceType==resource::ResourceTypeID("texres")&&atlas.header.installDependencies.empty()&&atlas.dependencies.empty(),"Font atlas identity/dependencies differ from its descriptor");
            CookedTextureData data;String error;
            const bool full=atlas.payload.size()==atlas.header.payloadSize&&!atlas.payload.empty();
            require(full?decodeCookedTexture(atlas,data,error):describeRuntimeTextureSnapshot(atlas,data,error),error.empty()?"Invalid typed bitmap-font atlas":error.c_str());
            require(data.storageFormat==0&&data.ddsBytes.empty()&&data.levels.size()==1&&data.mipSettings.filter==TextureMipFilter::None&&data.mipSettings.atlasColumns==1,"Bitmap fonts require a portable single-level atlas without mip/normal/data transforms");
            require(data.target==font.target&&data.packagedBuild==font.packagedBuild,"Font atlas target/build mode differs from its descriptor");
            font.atlasWidth=data.levels[0].width;font.atlasHeight=data.levels[0].height;
            require(font.atlasWidth&&font.atlasWidth<=1024&&font.atlasHeight&&font.atlasHeight<=1024,"Bitmap-font atlas dimensions exceed 1024 pixels");
            require(font.whiteX<font.atlasWidth&&font.whiteY<font.atlasHeight,"Bitmap-font utility texel is outside the atlas");std::array<u8,4> pixel{};
            if(full){const auto &bytes=data.levels[0].bgra;const auto offset=(size_t(font.whiteY)*font.atlasWidth+font.whiteX)*4;require(offset+4<=bytes.size(),"Font atlas utility texel is truncated");for(size_t c=0;c<4;++c)pixel[c]=bytes[offset+c];}
            else require(sampleRuntimeTextureSnapshotBGRA(atlas,font.whiteX,font.whiteY,pixel,error),error.empty()?"Cannot validate private font utility texel":error.c_str());
            for(const auto channel:pixel)require(channel==255,"Bitmap-font utility texel must be opaque white");
        }
        CookedFontData source(const resource::CompileContext &context)
        {
            require(context.resourceId.type()==resource::ResourceTypeID("fontres")&&!context.resourceId.isSubResource(),"Unsupported bitmap-font identity/selector");
            std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);require(bool(input),"Cannot open bitmap-font recipe");const auto size=input.tellg();
            require(size>0&&u64(size)<=maximumBytes,"Bitmap-font recipe exceeds its byte bound");String error;
            auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,65536+16*u64(size),error);require(bool(scratch),"Bitmap-font parsing budget exhausted");
            std::string text(size_t(size),'\0');input.seekg(0);input.read(text.data(),std::streamsize(size));require(bool(input)&&text.find('\0')==std::string::npos,"Truncated bitmap-font recipe or embedded NUL");
            rapidjson::Document doc;doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(text.data(),text.size());require(!doc.HasParseError(),"Invalid bitmap-font UTF-8 JSON");
            fields(doc,{"version","atlas","whitePixel","height","ascent","descent","fallback","glyphs"});require(doc["version"].IsUint()&&doc["version"].GetUint()==1&&doc["atlas"].IsString()&&doc["fallback"].IsUint(),"Unsupported bitmap-font recipe version/identity");
            CookedFontData data;require(data.atlas.set(String(doc["atlas"].GetString(),doc["atlas"].GetStringLength())),"Invalid font atlas ResourceID");
            const auto &white=doc["whitePixel"];require(white.IsArray()&&white.Size()==2&&white[0].IsUint()&&white[1].IsUint()&&white[0].GetUint()<1024&&white[1].GetUint()<1024,"Font whitePixel requires two bounded integer texel coordinates");data.whiteX=white[0].GetUint();data.whiteY=white[1].GetUint();
            data.target=context.target;data.packagedBuild=context.packagedBuild;data.height=number(doc["height"]);data.ascent=number(doc["ascent"]);data.descent=number(doc["descent"]);data.fallback=doc["fallback"].GetUint();
            const auto &glyphs=doc["glyphs"];require(glyphs.IsArray()&&!glyphs.Empty()&&glyphs.Size()<=maximumGlyphs,"Invalid bitmap-font glyph array");
            data.admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,8192+128*glyphs.Size(),error);require(bool(data.admission),"Bitmap-font descriptor admission failed");data.glyphs.reserve(glyphs.Size());
            for(const auto &item:glyphs.GetArray()){
                fields(item,{"codepoint","advance","bounds","uv"});require(item["codepoint"].IsUint(),"Invalid font glyph codepoint");const auto &b=item["bounds"],&uv=item["uv"];
                require(b.IsArray()&&b.Size()==4&&uv.IsArray()&&uv.Size()==4,"Font glyph bounds/UV require four scalars");FontGlyphInfo g{};g.codepoint=item["codepoint"].GetUint();g.xadvance=number(item["advance"]);
                g.x0=number(b[0]);g.y0=number(b[1]);g.x1=number(b[2]);g.y1=number(b[3]);g.width=g.x1-g.x0;g.height=g.y1-g.y0;g.u0=number(uv[0]);g.v0=number(uv[1]);g.u1=number(uv[2]);g.v1=number(uv[3]);data.glyphs.push_back(g);
            }validate(data,false);return data;
        }
        void word(std::ostream &out,u32 value){for(unsigned i=0;i<4;++i)out.put(char(value>>(i*8)));}
        void wide(std::ostream &out,u64 value){word(out,u32(value));word(out,u32(value>>32));}
        void real(std::ostream &out,f32 value){u32 bits;std::memcpy(&bits,&value,4);word(out,bits);}
        void string(std::ostream &out,const String &value){word(out,u32(value.size()));out.write(value.data(),value.size());}
        struct Reader
        {
            const Array<u8> &bytes;size_t at=0;
            u32 word(){require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated bitmap-font payload");u32 value=0;for(unsigned i=0;i<4;++i)value|=u32(bytes[at++])<<(i*8);return value;}
            u64 wide(){const auto low=word();return u64(low)|(u64(word())<<32);}
            f32 real(){const auto bits=word();f32 value;std::memcpy(&value,&bits,4);return value;}
            String string(u32 maximum){const auto count=word();require(count&&count<=maximum&&count<=bytes.size()-at,"Invalid bitmap-font string bound");String value(reinterpret_cast<const char *>(bytes.data()+at),count);at+=count;require(utf8(value),"Invalid UTF-8 bitmap-font identity");return value;}
        };
    }
    String FontResourceCompiler::name() const{return "BakedBitmapFont";}
    bool validateCookedFontData(const CookedFontData &data,String &error)
    {error.clear();try{validate(data,true);return true;}catch(const std::exception &failure){report(error,failure.what());return false;}}
    Array<resource::CompilerOutput> FontResourceCompiler::outputs() const{return {{resource::ResourceTypeID("fontres"),1}};}
    bool FontResourceCompiler::getDependencies(const resource::CompileContext &context,resource::DependencySet &dependencies,String &error) const
    {error.clear();try {const auto data=source(context);dependencies.compileDependencies.push_back(resource::CompileDependency::resource(data.atlas));dependencies.installDependencies.push_back(data.atlas);return true;}catch(const std::exception &failure){report(error,failure.what());return false;}}
    resource::CompilationStatus FontResourceCompiler::compile(const resource::CompileContext &context,std::ostream &out,Array<String> &messages) const
    {
        try {auto data=source(context);const auto found=context.dependencyArtifacts.find(data.atlas.str());require(found!=context.dependencyArtifacts.end(),"Missing pinned font atlas compile dependency");String error;
            auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16ull<<20,error);require(bool(scratch),"Font atlas validation budget exhausted");resource::RuntimeResource atlas;
            require(resource::CompiledResourceIO::read(found->second,atlas,error,(4ull<<20)+4096,16ull<<10),error.empty()?"Cannot read typed font atlas":error.c_str());
            atlasMetadata(atlas,data);data.atlasSourceHash=atlas.header.sourceHash;data.atlasPayloadHash=atlas.header.payloadHash;data.atlasCompilerVersion=atlas.header.compilerVersion;validate(data,true);
            out.write("WPFN",4);word(out,1);string(out,data.target);word(out,data.packagedBuild?1:0);string(out,data.atlas.str());wide(out,data.atlasSourceHash);wide(out,data.atlasPayloadHash);wide(out,data.atlasCompilerVersion);
            word(out,data.atlasWidth);word(out,data.atlasHeight);word(out,data.whiteX);word(out,data.whiteY);real(out,data.height);real(out,data.ascent);real(out,data.descent);word(out,data.fallback);word(out,u32(data.glyphs.size()));
            for(const auto &g:data.glyphs){word(out,g.codepoint);for(const auto v:{g.xadvance,g.x0,g.y0,g.x1,g.y1,g.u0,g.v0,g.u1,g.v1})real(out,v);}require(bool(out),"Cannot write bitmap-font payload");return resource::CompilationStatus::Success;
        }catch(const std::exception &failure){messages.push_back(failure.what());return resource::CompilationStatus::Failure;}
    }
    bool decodeCookedFont(const resource::RuntimeResource &value,CookedFontData &output,String &error)
    {
        error.clear();try {
            require(value.header.isValid()&&value.header.resourceType==resource::ResourceTypeID("fontres")&&value.header.compilerVersion==FontResourceCompiler().versionFor(resource::ResourceTypeID("fontres"))&&!value.header.resourceId.isSubResource()&&value.header.payloadSize==value.payload.size()&&value.payload.size()<=maximumBytes&&resource::hashBytes(value.payload.data(),value.payload.size())==value.header.payloadHash,"Bitmap-font payload identity/integrity mismatch");
            require(value.header.formatVersion==2&&value.header.installDependencies.size()==1&&value.header.installArtifacts.size()==1&&value.dependencies.size()==1&&value.dependencies[0],"Bitmap font requires its exact installed atlas");
            auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16ull<<20,error);require(bool(scratch),"Font atlas decode budget exhausted");
            Reader in{value.payload};require(in.word()==0x4e465057&&in.word()==1,"Unsupported bitmap-font payload version");CookedFontData data;data.target=in.string(128);const auto mode=in.word();require(mode<=1,"Invalid bitmap-font build mode");data.packagedBuild=mode!=0;
            require(data.atlas.set(in.string(1024)),"Invalid cooked bitmap-font atlas identity");data.atlasSourceHash=in.wide();data.atlasPayloadHash=in.wide();data.atlasCompilerVersion=in.wide();data.atlasWidth=in.word();data.atlasHeight=in.word();data.whiteX=in.word();data.whiteY=in.word();data.height=in.real();data.ascent=in.real();data.descent=in.real();data.fallback=in.word();const auto count=in.word();
            require(count&&count<=maximumGlyphs&&count<=((in.bytes.size()-in.at)/40)&&in.bytes.size()-in.at==40ull*count,"Invalid or truncated bitmap-font glyph table");
            data.admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,8192+128*count,error);require(bool(data.admission),"Bitmap-font retained descriptor budget exhausted");data.glyphs.reserve(count);
            for(u32 i=0;i<count;++i){FontGlyphInfo g{};g.codepoint=in.word();g.xadvance=in.real();g.x0=in.real();g.y0=in.real();g.x1=in.real();g.y1=in.real();g.width=g.x1-g.x0;g.height=g.y1-g.y0;g.u0=in.real();g.v0=in.real();g.u1=in.real();g.v1=in.real();data.glyphs.push_back(g);}validate(data,true);
            const auto &atlas=*value.dependencies[0];require(value.header.installDependencies[0]==data.atlas&&atlas.header.resourceId==data.atlas&&atlas.header.sourceHash==data.atlasSourceHash&&atlas.header.payloadHash==data.atlasPayloadHash&&atlas.header.compilerVersion==data.atlasCompilerVersion,"Bitmap-font installed atlas differs from its sealed generation");
            require(value.header.installArtifacts[0].sourceHash==data.atlasSourceHash&&value.header.installArtifacts[0].payloadHash==data.atlasPayloadHash,"Bitmap-font container dependency pin differs from its descriptor");
            const auto width=data.atlasWidth,height=data.atlasHeight;atlasMetadata(atlas,data);require(data.atlasWidth==width&&data.atlasHeight==height,"Bitmap-font atlas dimensions changed");output=std::move(data);return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
}
