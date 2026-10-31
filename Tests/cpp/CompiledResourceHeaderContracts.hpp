#pragma once
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace compiled_resource_header_contracts
{
    using namespace workphone;
    using namespace workphone::resource;
    inline void require(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
    inline void integer(std::string &bytes,u64 value,unsigned width)
    { for(unsigned i=0;i<width;++i) bytes.push_back(char(value>>(8*i))); }
    inline void string(std::string &bytes,const std::string &value)
    { integer(bytes,value.size(),4); bytes+=value; }
    inline std::string fixed(const char *type,u32 dependencies,u16 version=1)
    {
        std::string bytes="WPRS"; integer(bytes,version,2); integer(bytes,0,2);
        integer(bytes,1,8); integer(bytes,1,8); integer(bytes,14695981039346656037ull,8);
        integer(bytes,0,8); integer(bytes,ResourceTypeID(type).value(),8); integer(bytes,dependencies,4);
        return bytes;
    }
    inline void run(const std::filesystem::path &root)
    {
        const auto file=root/"header-bounds.wprs";
        const auto path=String(file.u8string().c_str()); String error; CompiledResourceHeader header;
        auto write=[&](const std::string &bytes) { std::ofstream output(file,std::ios::binary|std::ios::trunc);
            output.write(bytes.data(),bytes.size()); require(bool(output),"Header bounds fixture write"); };
        // Preserve the old single-identity limit; only aggregate metadata is tightened.
        const std::string large="data://"+std::string((1u<<20)-11,'a')+".bin";
        auto bytes=fixed("bin",0); string(bytes,large); write(bytes);
        require(CompiledResourceIO::readHeader(path,header,error)&&header.resourceId.str().size()==(1u<<20),
            "A valid one-MiB compiled identity remains readable");
        const std::string unicode="data://caf" "\xc3\xa9" "/" "\xe6\xa8\xb9" ".bin";
        ResourceID unicodeID;require(unicodeID.set(unicode.c_str(),&error)&&unicodeID.str()==unicode.c_str()&&
            ResourceID::isValidString(unicode.c_str()),"ResourceID normalization preserves valid Unicode");
        require(ResourceID::isValidString(large.c_str()),"ResourceID retains valid one-MiB identities");
        bytes=fixed("bin",0);string(bytes,unicode);write(bytes);
        require(CompiledResourceIO::readHeader(path,header,error)&&header.resourceId.str()==unicode.c_str(),
            "Valid multibyte UTF-8 identities remain readable");
        for(const auto &bad : {std::string("\xff",1),std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),
            std::string("\xf4\x90\x80\x80",4),std::string("\xe2\x28\xa1",3),std::string("\0",1)}) {
            const auto invalid=std::string("data://bad")+bad+".bin";const String invalidID(invalid.data(),invalid.size());
            ResourceID direct("data://previous.bin");
            require(!direct.set(invalidID,&error)&&!direct.isValid()&&direct.str().empty()&&!error.empty()&&
                !ResourceID::isValidString(invalidID),"ResourceID rejects malformed UTF-8/NUL before normalization and clears stale identity");
            const auto badArtifact=std::string("data://artifacts/v2/bad")+bad+".bin";
            require(!ResourceID::isValidString(String(badArtifact.data(),badArtifact.size())),"Manifest artifact validation rejects malformed UTF-8 paths");
            bytes=fixed("bin",0);string(bytes,invalid);write(bytes);
            require(!CompiledResourceIO::readHeader(path,header,error)&&!error.empty(),
                "Malformed UTF-8/NUL compiled identities reject at the header boundary");
        }
        bytes=fixed("bin",0); integer(bytes,1u<<20,4); write(bytes);
        require(!CompiledResourceIO::readHeader(path,header,error)&&!error.empty(),
            "Declared string exceeding remaining file bytes must reject before allocation");
        bytes=fixed("bin",4096); string(bytes,"data://root.bin"); write(bytes);
        require(!CompiledResourceIO::readHeader(path,header,error)&&header.installDependencies.capacity()==0,
            "Truncated dependency arrays must reject before reserving their vectors");
        // Individually legal one-MiB IDs must not accumulate an unbounded header.
        bytes=fixed("bin",8); string(bytes,"data://root.bin");
        for(unsigned index=0;index<8;++index) { auto id=large; id[7]=char('a'+index); string(bytes,id); }
        write(bytes); require(!CompiledResourceIO::readHeader(path,header,error)&&header.installDependencies.size()<8,
            "Aggregate encoded headers over eight MiB must reject before the next string allocation");
        CompiledResourceHeader oversized; oversized.formatVersion=1; oversized.resourceId=ResourceID("data://root.bin");
        oversized.resourceType=ResourceTypeID("bin"); oversized.compilerVersion=1;
        oversized.sourceHash=1; oversized.payloadHash=14695981039346656037ull;
        for(unsigned index=0;index<8;++index) { auto id=large; id[7]=char('a'+index); oversized.installDependencies.emplace_back(id.c_str()); }
        require(oversized.isValid(),"Aggregate writer fixture must be valid before its encoded-size check");
        const auto empty=root/"header-empty.payload",destination=root/"header-overlimit.wprs";
        { std::ofstream output(empty,std::ios::binary); require(bool(output),"Header empty payload fixture"); }
        require(!CompiledResourceIO::writeAtomic(destination.u8string().c_str(),oversized,empty.u8string().c_str(),error) &&
            !std::filesystem::exists(destination),"Writer must not publish an envelope rejected by the aggregate reader bound");
        CompiledResourceHeader invalidText;invalidText.resourceId=ResourceID("data://bad\xff.bin");
        invalidText.resourceType=ResourceTypeID("bin");invalidText.compilerVersion=1;invalidText.sourceHash=1;invalidText.payloadHash=14695981039346656037ull;
        const auto invalidDestination=root/"header-invalid-utf8.wprs",validDestination=root/"header-valid-utf8.wprs";
        require(!CompiledResourceIO::writeAtomic(invalidDestination.u8string().c_str(),invalidText,empty.u8string().c_str(),error)&&
            !std::filesystem::exists(invalidDestination),"Writer rejects invalid UTF-8 identities without publication");
        invalidText.resourceId=ResourceID(unicode.c_str());
        error.clear();const bool wroteUnicode=CompiledResourceIO::writeAtomic(validDestination.u8string().c_str(),invalidText,empty.u8string().c_str(),error);
        require(wroteUnicode,(std::string("Writer preserves valid UTF-8 identities with an empty payload: ")+error.c_str()).c_str());
        require(CompiledResourceIO::readHeader(validDestination.u8string().c_str(),header,error)&&header.resourceId==invalidText.resourceId&&header.payloadSize==0,
            "Published UTF-8 identity and empty payload round-trip through the bounded reader");
        std::filesystem::remove(validDestination);
        bytes=fixed("bin",1,2); string(bytes,"data://root.bin"); string(bytes,"data://child.bin");
        string(bytes,std::string(4097,'a')); integer(bytes,1,8); integer(bytes,1,8); write(bytes);
        require(!CompiledResourceIO::readHeader(path,header,error),"Artifact paths above their existing 4096-byte limit must reject early");
        bytes=fixed("bin",1,2); string(bytes,"data://root.bin"); string(bytes,"data://child.bin");
        string(bytes,"artifacts/v2/child.bin"); integer(bytes,1,8); integer(bytes,1,8); write(bytes);
        require(CompiledResourceIO::readHeader(path,header,error)&&header.installArtifacts.size()==1,
            "Valid bounded V2 dependency headers remain readable");
        bytes=fixed("bin",1,2);string(bytes,"data://root.bin");string(bytes,"data://child.bin");
        string(bytes,"artifacts/v2/bad\xff.bin");integer(bytes,1,8);integer(bytes,1,8);write(bytes);
        require(!CompiledResourceIO::readHeader(path,header,error),"Malformed UTF-8 artifact pins reject before filesystem resolution");
        // Mounting a manifest uses this reader before checking its container type/deps.
        bytes=fixed("wprm",4096); string(bytes,"data://runtime.wprm"); write(bytes);
        ResourceSystem runtime(nullptr,nullptr); RuntimeResourceConfig config;
        config.compiledRoot=root.u8string().c_str(); config.manifestPath=path;
        require(!runtime.initializeRuntime(config,error),"Malformed manifest headers must reject during mount admission");
        std::error_code ignored; std::filesystem::remove(file,ignored); std::filesystem::remove(empty,ignored);
    }
}
