#pragma once
#include <Workphone/System/ResourceBuildProfile.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <set>

namespace workphone
{
    inline std::map<String,String> parsePackageBuildSettings(const String &text)
    {
        if(text.size()>65536 || text.find('\0')!=String::npos)
            throw std::invalid_argument("Build settings exceed 64 KiB or contain NUL");
        rapidjson::Document doc;
        doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(text.data(),text.size());
        if(doc.HasParseError() || !doc.IsObject() || doc.MemberCount()>64)
            throw std::invalid_argument("Build settings require a JSON object with at most 64 string values");
        std::map<String,String> result;
        for(auto i=doc.MemberBegin();i!=doc.MemberEnd();++i)
        {
            if(!i->value.IsString()) throw std::invalid_argument("Build setting values must be strings");
            String key(i->name.GetString(),i->name.GetStringLength());
            String value(i->value.GetString(),i->value.GetStringLength());
            if(key.empty() || key.size()>4096 || value.size()>4096 || key.find('\0')!=String::npos ||
                value.find('\0')!=String::npos || !result.emplace(key,value).second)
                throw std::invalid_argument("Invalid or duplicate build setting");
        }
        return result;
    }
    inline String packageBuildSettingsJSON(const resource::ResourceBuildProfile &profile)
    {
        rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        for(const auto &entry:profile.settings) { writer.Key(entry.first.c_str()); writer.String(entry.second.c_str()); }
        writer.EndObject(); return String(buffer.GetString(),buffer.GetSize());
    }
    inline resource::ResourceBuildProfile packageBuildProfile(const Array<String> &args)
    {
        if(args.size()<6 || (args.size()-6)%2) throw std::invalid_argument("Invalid package argument pairs");
        resource::ResourceBuildProfile result; std::set<String> seen;
        for(size_t i=6;i<args.size();i+=2)
        {
            const auto &flag=args[i];
            if(flag=="--root" || flag=="--map") continue;
            if(!seen.insert(flag).second) throw std::invalid_argument("Duplicate build profile option");
            if(flag=="--build-profile") result.profile=args[i+1];
            else if(flag=="--toolchain") result.toolchain=args[i+1];
            else if(flag=="--build-settings") result.settings=parsePackageBuildSettings(args[i+1]);
            else throw std::invalid_argument("Unknown package option");
        }
        if(!seen.empty() && (result.profile.empty() || result.toolchain.empty()))
            throw std::invalid_argument("Explicit build profiles require profile and toolchain identities");
        if(result.toolchain.size()>4000) throw std::invalid_argument("Package toolchain labels are limited to 4000 bytes");
        (void)result.key();
        return result;
    }
}
