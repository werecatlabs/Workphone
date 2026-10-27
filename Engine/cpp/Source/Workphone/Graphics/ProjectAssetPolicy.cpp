#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/ResourceID.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <zlib.h>
#if defined( _WIN32 )
#include <windows.h>
#endif

namespace workphone
{
    namespace
    {
        std::mutex encoderMutex;
        std::map<String,TextureCompressionEncoder> encoders;
        void put64( Array<u8> &bytes, u64 value ) { for( u32 i=0;i<8;++i ) bytes.push_back( static_cast<u8>(value>>(i*8)) ); }
        u64 get64( const Array<u8> &bytes, size_t offset )
        { u64 value=0; for( u32 i=0;i<8;++i ) value|=u64(bytes[offset+i])<<(i*8); return value; }
    }
    void registerTextureCompressionEncoder( const String &format, TextureCompressionEncoder encoder )
    {
        if( format != "bc7" && format != "astc" && format != "pvrtc" && format != "etc2" )
            throw std::invalid_argument( "Unknown texture compression provider format" );
        std::lock_guard<std::mutex> lock( encoderMutex );
        if( encoder ) encoders[format]=std::move(encoder); else encoders.erase(format);
    }
    bool encodeTextureCompression( const TextureCompressionRequest &request, TextureCompressionOutput &output, String &error )
    {
        TextureCompressionEncoder encoder;
        { std::lock_guard<std::mutex> lock(encoderMutex); const auto found=encoders.find(request.format); if(found!=encoders.end()) encoder=found->second; }
        if(!encoder) { error="Install a texture encoder provider for "+request.format; return false; }
        TextureCompressionOutput candidate;
        try
        {
            if(!encoder(request,candidate,error)) return false;
            if(candidate.bytes.empty() || candidate.bytes.size()>256ull*1024*1024 ||
               (candidate.container!="dds" && candidate.container!="ktx2" && candidate.container!="pvr"))
            { error="Texture encoder returned an invalid container"; return false; }
            output=std::move(candidate); error.clear(); return true;
        }
        catch(const std::exception &e) { error=e.what(); return false; }
    }
    bool writeProjectAssetPolicy( const String &root, SmartPtr<Properties> settings, String &error )
    {
        namespace fs=std::filesystem; fs::path temporary;
        try
        {
            if(root.empty() || !settings) throw std::invalid_argument("Asset policy requires a project directory/profile");
            const auto path=fs::u8path(root.c_str())/".workphone/ProjectAssetPolicy.settings";
            fs::create_directories(path.parent_path()); temporary=path;
            temporary+=std::string(("."+StringUtil::getUUID()+".tmp").c_str());
            auto policy=make_ptr<Properties>(); policy->setProperty("policyVersion",s32(1));
            for(const char *key:{"assets.compression","assets.textureCompression.pc","assets.textureCompression.mac",
                "assets.textureCompression.ios","assets.textureCompression.android","platform.active","quality.texture","quality.maxTextureSize"})
                if(settings->hasProperty(key)) policy->setProperty(settings->getPropertyObject(key));
            std::ofstream stream(temporary,std::ios::binary|std::ios::trunc); stream.exceptions(std::ios::badbit|std::ios::failbit);
            stream<<DataUtil::toString(policy.get(),true); stream.close();
#if defined( _WIN32 )
            if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
                throw std::runtime_error("Unable to publish project asset policy");
#else
            fs::rename(temporary,path);
#endif
            error.clear();return true;
        }
        catch(const std::exception &e) {std::error_code ignored;if(!temporary.empty()) fs::remove(temporary,ignored);error=e.what();return false;}
    }
    bool readProjectAssetPolicy( const String &path, ProjectAssetPolicy &policy, String &error )
    {
        try
        {
            const auto file=std::filesystem::u8path(path.c_str());
            if(!std::filesystem::is_regular_file(file)) {policy={};error.clear();return true;}
            if(std::filesystem::file_size(file)>65536) throw std::runtime_error("Project asset policy exceeds its byte limit");
            std::ifstream stream(file,std::ios::binary);std::ostringstream text;text<<stream.rdbuf();
            if(!stream || text.str().size()>65536) throw std::runtime_error("Invalid project asset policy file");
            auto data=make_ptr<Properties>();DataUtil::parse(text.str().c_str(),data.get());
            if(data->getPropertyAsInt("policyVersion")!=1) throw std::runtime_error("Unsupported project asset policy version");
            ProjectAssetPolicy result;result.present=true;result.platform=data->getPropertyAsInt("platform.active",0);
            if(result.platform<0 || result.platform>3) throw std::runtime_error("Invalid asset target platform");
            const char *keys[]={"assets.textureCompression.pc","assets.textureCompression.mac","assets.textureCompression.ios","assets.textureCompression.android"};
            result.textureCompression=data->getPropertyAsInt(keys[result.platform],0);result.compression=data->getPropertyAsInt("assets.compression",0);
            result.textureQuality=data->getPropertyAsInt("quality.texture",2);result.maximumTextureSize=data->getPropertyAsInt("quality.maxTextureSize",16384);
            if(result.textureCompression<0 || result.textureCompression>(result.platform==3?4:3) || result.compression<0 || result.compression>3 ||
               result.textureQuality<0 || result.textureQuality>3 || result.maximumTextureSize<1 || result.maximumTextureSize>16384)
                throw std::runtime_error("Project asset policy is out of range");
            policy=result;error.clear();return true;
        }
        catch(const std::exception &e){error=e.what();return false;}
    }
    String projectTextureCompressionName( const ProjectAssetPolicy &p, bool alpha )
    {
        if(p.textureCompression==0) return "none";
        if(p.platform<2)
        {if(p.textureCompression==1)return alpha?"bc3":"bc1";return p.textureCompression==2?"bc7":"astc";}
        if(p.platform==2) {const char *names[]={"none","pvrtc","astc","etc2"};return names[p.textureCompression];}
        const char *names[]={"none","etc2","astc","bc7","etc2"};return names[p.textureCompression];
    }
    Array<u8> compressProjectAssetPayload( const Array<u8> &payload, s32 effort )
    {
        if(effort==0)return payload;
        if(effort<0 || effort>3 || payload.empty() || payload.size()>256ull*1024*1024)
            throw std::invalid_argument("Invalid asset compression request");
        uLongf size=compressBound(static_cast<uLong>(payload.size()));Array<u8> compressed(size);
        constexpr int levels[]={0,1,6,9};
        if(compress2(compressed.data(),&size,payload.data(),static_cast<uLong>(payload.size()),levels[effort])!=Z_OK)
            throw std::runtime_error("Asset deflate encoder failed");
        compressed.resize(size);Array<u8> output={'W','P','Z','1'};put64(output,payload.size());
        put64(output,resource::hashBytes(payload.data(),payload.size()));output.insert(output.end(),compressed.begin(),compressed.end());return output;
    }
    Array<u8> decompressProjectAssetPayload( const Array<u8> &payload, u64 maximumBytes )
    {
        if(payload.size()<4 || payload[0]!='W' || payload[1]!='P' || payload[2]!='Z' || payload[3]!='1')return payload;
        if(payload.size()<20)throw std::runtime_error("Truncated compressed asset wrapper");
        const auto size=get64(payload,4);if(!size || size>maximumBytes || size>256ull*1024*1024)
            throw std::runtime_error("Compressed asset exceeds its decode limit");
        Array<u8> output(static_cast<size_t>(size));uLongf bytes=static_cast<uLongf>(size);
        uLong sourceBytes=static_cast<uLong>(payload.size()-20);
        if(uncompress2(output.data(),&bytes,payload.data()+20,&sourceBytes)!=Z_OK || bytes!=size || sourceBytes!=payload.size()-20 ||
           resource::hashBytes(output.data(),output.size())!=get64(payload,12))throw std::runtime_error("Compressed asset integrity failure");
        return output;
    }
}
