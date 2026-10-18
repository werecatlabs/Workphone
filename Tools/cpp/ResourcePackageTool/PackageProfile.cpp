#include "PackageProfile.hpp"
#include <Workphone/Core/StringUtil.hpp>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>

namespace
{
    using namespace workphone;
    namespace fs = std::filesystem;
    constexpr size_t maximumBytes = 65536;
    const char *pathFields[] = {"sourceRoot", "catalog", "cookedRoot", "output"};

    void require(bool valid, const char *message)
    {
        if(!valid) throw std::runtime_error(message);
    }
    void fields(const rapidjson::Value &value, const std::set<std::string> &allowed)
    {
        require(value.IsObject(), "Package profile requires an object");
        std::set<std::string> seen;
        for(auto i = value.MemberBegin(); i != value.MemberEnd(); ++i)
        {
            const std::string key(i->name.GetString(), i->name.GetStringLength());
            require(allowed.count(key) && seen.insert(key).second,
                    "Unknown or duplicate package profile field");
        }
        require(seen.size() == allowed.size(), "Missing package profile field");
    }
    String string(const rapidjson::Value &value)
    {
        require(value.IsString() && value.GetStringLength() > 0 && value.GetStringLength() <= 4096,
                "Package profile strings must contain 1 to 4096 bytes");
        const std::string text(value.GetString(), value.GetStringLength());
        require(text.find('\0') == std::string::npos, "Package profile contains embedded NUL");
        return text.c_str();
    }
}

workphone::Array<workphone::String> loadPackageProfile(const workphone::String &path)
{
    const auto file = fs::absolute(fs::u8path(path.c_str()));
    std::ifstream input(file, std::ios::binary);
    require(input.good(), "Cannot open package profile");
    std::string bytes(maximumBytes + 1, '\0');
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    const auto count = input.gcount();
    require(!input.bad() && count <= maximumBytes, "Package profile exceeds 64 KiB or cannot be read");
    bytes.resize(static_cast<size_t>(count));
    require(bytes.find('\0') == std::string::npos, "Package profile contains embedded NUL bytes");
    rapidjson::Document document;
    document.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag>(bytes.data(), bytes.size());
    require(!document.HasParseError(), "Invalid UTF-8 JSON package profile");
    fields(document, {"version", "sourceRoot", "catalog", "cookedRoot", "output", "target", "mappings", "roots"});
    require(document["version"].IsUint() && document["version"].GetUint() == 1,
            "Unsupported package profile version");
    Array<String> args = {"WorkphoneResourcePackage"};
    for(const auto field : pathFields)
    {
        const auto value = fs::u8path(string(document[field]).c_str());
        args.push_back((value.is_absolute() ? value : file.parent_path() / value).lexically_normal().u8string().c_str());
    }
    args.push_back(string(document["target"]));
    const auto &mappings = document["mappings"];
    require(mappings.IsArray() && !mappings.Empty() && mappings.Size() <= 128,
            "Package profile requires 1 to 128 mappings");
    for(const auto &mapping : mappings.GetArray())
    {
        fields(mapping, {"catalogType", "extension"});
        const auto type = string(mapping["catalogType"]);
        const auto extension = string(mapping["extension"]);
        require(type.find('=') == String::npos && extension.find('=') == String::npos,
                "Package profile mappings cannot contain equals signs");
        args.push_back("--map");
        args.push_back(type + "=" + extension);
    }
    const auto &roots = document["roots"];
    require(roots.IsArray() && !roots.Empty() && roots.Size() <= 512,
            "Package profile requires 1 to 512 roots");
    for(const auto &root : roots.GetArray())
    {
        args.push_back("--root");
        args.push_back(string(root));
    }
    return args;
}

void savePackageProfile(const workphone::String &path, const workphone::Array<workphone::String> &args)
{
    const auto file = fs::absolute(fs::u8path(path.c_str()));
    require(fs::is_directory(file.parent_path()), "Package profile parent directory must exist");
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("version"); writer.Uint(1);
    for(size_t i = 0; i < 4; ++i)
    {
        writer.Key(pathFields[i]);
        const auto absolute = fs::absolute(fs::u8path(args[i + 1].c_str())).lexically_normal();
        const auto relative = absolute.lexically_relative(file.parent_path());
        const auto text = (relative.empty() ? absolute : relative).generic_u8string();
        writer.String(text.c_str());
    }
    writer.Key("target"); writer.String(args[5].c_str());
    writer.Key("mappings"); writer.StartArray();
    for(size_t i = 6; i < args.size(); i += 2)
        if(args[i] == "--map")
        {
            const auto split = args[i + 1].find('=');
            writer.StartObject();
            writer.Key("catalogType"); writer.String(args[i + 1].substr(0, split).c_str());
            writer.Key("extension"); writer.String(args[i + 1].substr(split + 1).c_str());
            writer.EndObject();
        }
    writer.EndArray(); writer.Key("roots"); writer.StartArray();
    for(size_t i = 6; i < args.size(); i += 2)
        if(args[i] == "--root") writer.String(args[i + 1].c_str());
    writer.EndArray(); writer.EndObject();
    require(buffer.GetSize() <= maximumBytes, "Package profile exceeds 64 KiB");
    const auto staging = file.parent_path() / fs::u8path(".wp-profile-" + std::string(StringUtil::getUUID().c_str()));
    try
    {
        std::ofstream output(staging, std::ios::binary | std::ios::trunc);
        output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
        output.close();
        require(!output.fail(), "Cannot write package profile");
        // Validate the serialized request using the reader before publication.
        loadPackageProfile(staging.u8string().c_str());
        // Hard-link publication is exclusive on Windows and POSIX; it cannot
        // replace an existing profile. Unsupported filesystems fail closed.
        fs::create_hard_link(staging, file);
        std::error_code ignored;
        fs::remove(staging, ignored);
    }
    catch(...)
    {
        std::error_code ignored;
        fs::remove(staging, ignored);
        throw;
    }
}
