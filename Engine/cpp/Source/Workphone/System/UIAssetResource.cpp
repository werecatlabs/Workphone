#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/UIAssetResource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <cstring>
#include <cmath>

namespace workphone::ui
{
    namespace
    {
        constexpr size_t maximumBytes=1ull<<20;
        void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
        void fields(const rapidjson::Value &value,std::initializer_list<const char *> names)
        {
            need(value.IsObject(),"UI asset record must be an object");std::set<String> seen;
            for(auto it=value.MemberBegin();it!=value.MemberEnd();++it){String key(it->name.GetString(),it->name.GetStringLength());
                bool known=false;for(auto name:names)known|=key==name;need(known&&seen.insert(key).second,"Unknown or duplicate UI asset field");}
            for(auto name:names)need(value.HasMember(name),"Missing UI asset field");
        }
        String string(const rapidjson::Value &value,size_t limit)
        {need(value.IsString()&&value.GetStringLength()&&value.GetStringLength()<=limit&&!std::memchr(value.GetString(),0,value.GetStringLength()),"Invalid UI asset string");return String(value.GetString(),value.GetStringLength());}
        bool tag(const String &value)
        {return !value.empty()&&value.size()<=35&&value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==String::npos;}
        bool utf8(const String &value)
        {
            for(size_t at=0;at<value.size();){const auto first=static_cast<u8>(value[at++]);if(first<128){if(!first)return false;continue;}
                const unsigned count=first>=0xc2&&first<=0xdf?1:first>=0xe0&&first<=0xef?2:first>=0xf0&&first<=0xf4?3:0;
                if(!count||count>value.size()-at)return false;u32 code=first&((1u<<(6-count))-1u);
                for(unsigned i=0;i<count;++i){const auto byte=static_cast<u8>(value[at++]);if((byte&0xc0)!=0x80)return false;code=(code<<6)|(byte&63);}
                if((count==2&&code<0x800)||(count==3&&code<0x10000)||code>0x10ffff||(code>=0xd800&&code<=0xdfff))return false;
            }return true;
        }
        void word(std::ostream &out,u32 value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));}
        void text(std::ostream &out,const String &value){word(out,u32(value.size()));out.write(value.data(),value.size());}
        void number(std::ostream &out,f32 value){u32 bits;std::memcpy(&bits,&value,4);word(out,bits);}
        struct Reader
        {
            const Array<u8> &bytes;size_t at=8;
            u32 word(){need(at<=bytes.size()&&bytes.size()-at>=4,"Truncated UI asset");u32 value=0;for(unsigned i=0;i<4;++i)value|=u32(bytes[at++])<<(8*i);return value;}
            String text(size_t limit){const auto size=word();need(size&&size<=limit&&at<=bytes.size()&&size<=bytes.size()-at&&!std::memchr(bytes.data()+at,0,size),"Invalid UI asset string span");
                String value(reinterpret_cast<const char*>(bytes.data()+at),size);at+=size;
                need(utf8(value),"Invalid UI asset UTF-8 text");return value;}
            f32 number(){const auto bits=word();f32 value;std::memcpy(&value,&bits,4);need(std::isfinite(value),"Nonfinite UI style value");return value;}
        };
        void header(const resource::RuntimeResource &value,const char *type,const char *magic)
        {
            const resource::ResourceTypeID id(type);UIAssetResourceCompiler compiler;
            need(value.header.isValid()&&value.header.resourceType==id&&value.header.resourceId.type()==id&&!value.header.resourceId.isSubResource()&&
                value.header.compilerVersion==compiler.versionFor(id),"Unsupported UI asset identity or version");
            need(value.header.installDependencies.empty()&&value.dependencies.empty()&&value.header.installArtifacts.empty(),"UI style/locale assets cannot contain undeclared dependencies");
            need(value.payload.size()>=12&&value.payload.size()<=maximumBytes&&value.header.payloadSize==value.payload.size()&&
                resource::hashBytes(value.payload.data(),value.payload.size())==value.header.payloadHash,"UI asset integrity mismatch");
            need(!std::memcmp(value.payload.data(),magic,4)&&value.payload[4]==1&&!value.payload[5]&&!value.payload[6]&&!value.payload[7],"Unsupported UI asset envelope");
        }
    }
    String UIAssetResourceCompiler::name() const{return "WorkphoneUIAssetData";}
    Array<resource::CompilerOutput> UIAssetResourceCompiler::outputs() const{return {{resource::ResourceTypeID("uistyle"),1},{resource::ResourceTypeID("locres"),1}};}
    resource::CompilationStatus UIAssetResourceCompiler::compile(const resource::CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try{
            need(!context.resourceId.isSubResource(),"UI asset selectors are unsupported");String error;
            auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,32*maximumBytes,error);need(bool(budget),"UI asset parser budget denied");
            std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);const auto size=input?input.tellg():std::streampos(-1);
            need(size>0&&size<=maximumBytes,"UI asset source exceeds its one-MiB bound");String source(size_t(size),'\0');input.seekg(0);input.read(source.data(),source.size());need(bool(input),"Cannot read UI asset source");
            rapidjson::Document doc;doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(source.data(),source.size());need(!doc.HasParseError()&&source.find('\0')==String::npos,"Invalid UTF-8 UI asset source");
            if(context.resourceId.type()==resource::ResourceTypeID("uistyle")){
                fields(doc,{"version","styles"});need(doc["version"].IsUint()&&doc["version"].GetUint()==1&&doc["styles"].IsArray()&&!doc["styles"].Empty()&&doc["styles"].Size()<=256,"Invalid UI style version/count");
                output.write("WPUS",4);word(output,1);word(output,doc["styles"].Size());std::set<String> names;
                for(const auto &item:doc["styles"].GetArray()){
                    fields(item,{"name","colour","textSize"});const auto name=string(item["name"],128);need(names.insert(name).second,"Duplicate UI style name");text(output,name);
                    need(item["colour"].IsArray()&&item["colour"].Size()==4,"UI style colour requires RGBA");
                    for(const auto &channel:item["colour"].GetArray()){need(channel.IsNumber()&&std::isfinite(channel.GetDouble())&&channel.GetDouble()>=0&&channel.GetDouble()<=1,"Invalid UI style colour");number(output,f32(channel.GetDouble()));}
                    need(item["textSize"].IsNumber()&&std::isfinite(item["textSize"].GetDouble())&&item["textSize"].GetDouble()>0&&item["textSize"].GetDouble()<=256,"Invalid UI style text size");number(output,f32(item["textSize"].GetDouble()));
                }
            }else{
                need(context.resourceId.type()==resource::ResourceTypeID("locres"),"Unsupported UI asset type");fields(doc,{"version","fallback","locales"});
                need(doc["version"].IsUint()&&doc["version"].GetUint()==1&&doc["locales"].IsObject()&&doc["locales"].MemberCount()&&doc["locales"].MemberCount()<=32,"Invalid locale version/count");
                const auto fallback=string(doc["fallback"],35);need(tag(fallback),"Invalid fallback language tag");output.write("WPUL",4);word(output,1);text(output,fallback);word(output,doc["locales"].MemberCount());
                std::map<String,std::map<String,String>> locales;size_t total=0;
                for(auto it=doc["locales"].MemberBegin();it!=doc["locales"].MemberEnd();++it){const auto language=string(it->name,35);need(tag(language)&&it->value.IsObject()&&it->value.MemberCount()<=1024,"Invalid UI locale");
                    std::map<String,String> entries;for(auto entry=it->value.MemberBegin();entry!=it->value.MemberEnd();++entry){need(++total<=4096,"UI locale entry bound exceeded");const auto key=string(entry->name,256);need(entries.emplace(key,string(entry->value,4096)).second,"Duplicate translation key");}
                    need(locales.emplace(language,std::move(entries)).second,"Duplicate UI locale");}
                need(locales.count(fallback)&&!locales.at(fallback).empty(),"Missing UI fallback dictionary");
                for(const auto &locale:locales){text(output,locale.first);word(output,u32(locale.second.size()));for(const auto &entry:locale.second){text(output,entry.first);text(output,entry.second);}}
            }
            need(bool(output)&&output.tellp()<=std::streampos(maximumBytes),"UI asset output exceeds bound");return resource::CompilationStatus::Success;
        }catch(const std::exception &failure){messages.push_back(failure.what());return resource::CompilationStatus::Failure;}
    }
    bool decodeUIStyles(const resource::RuntimeResource &value,Array<UIStyleRecord> &output,String &error)
    {
        error.clear();try{header(value,"uistyle","WPUS");auto admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,value.payload.size()*16+65536,error);need(bool(admission),"UI style retained budget denied");Reader reader{value.payload};const auto count=reader.word();need(count&&count<=256,"Invalid UI style count");Array<UIStyleRecord> staged;std::set<String> names;
            for(u32 i=0;i<count;++i){UIStyleRecord item;item.admission=admission;item.name=reader.text(128);need(names.insert(item.name).second,"Duplicate UI style name");f32 c[4];for(auto &channel:c){channel=reader.number();need(channel>=0&&channel<=1,"Invalid UI colour channel");}item.colour={c[0],c[1],c[2],c[3]};item.textSize=reader.number();need(item.textSize>0&&item.textSize<=256,"Invalid UI style size");staged.push_back(std::move(item));}
            need(reader.at==value.payload.size(),"Trailing UI style bytes");output.swap(staged);return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
    bool decodeUILocalization(const resource::RuntimeResource &value,UILocalizationData &output,String &error)
    {
        error.clear();try{header(value,"locres","WPUL");UILocalizationData staged;staged.admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,value.payload.size()*16+65536,error);need(bool(staged.admission),"UI localization retained budget denied");Reader reader{value.payload};staged.fallback=reader.text(35);need(tag(staged.fallback),"Invalid fallback tag");const auto count=reader.word();need(count&&count<=32,"Invalid locale count");size_t total=0;
            for(u32 i=0;i<count;++i){const auto locale=reader.text(35);need(tag(locale)&&!staged.locales.count(locale),"Duplicate or invalid locale");const auto entries=reader.word();need(entries<=1024&&total+entries<=4096,"UI translation bound exceeded");total+=entries;std::map<String,String> dictionary;
                for(u32 j=0;j<entries;++j){const auto key=reader.text(256),text=reader.text(4096);need(dictionary.emplace(key,text).second,"Duplicate translation key");}staged.locales.emplace(locale,std::move(dictionary));}
            need(reader.at==value.payload.size()&&staged.locales.count(staged.fallback)&&!staged.locales.at(staged.fallback).empty(),"Missing fallback or trailing locale bytes");output=std::move(staged);return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
    bool resolveUILocalization(const UILocalizationData &data,const String &locale,const String &key,String &text,String &error)
    {
        error.clear();for(const auto &language:{locale,data.fallback}){auto found=data.locales.find(language);if(found!=data.locales.end()){auto entry=found->second.find(key);if(entry!=found->second.end()){text=entry->second;return true;}}}
        error="Missing translation in the selected locale and declared fallback: "+key;return false;
    }
}
