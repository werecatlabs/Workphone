#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/SystemBind.hpp>
#include <WPLuabind/ScriptObjectFunctions.hpp>
#include <WPLuabind/Helpers/EngineHelper.hpp>
#include <WPLuabind/Helpers/FileSystemHelper.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>
#include <boost/core/noncopyable.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <filesystem>
#include <set>
#include <cmath>
#include <Workphone/System/ResourceID.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/System/PackageManager.hpp>
#include <rapidjson/document.h>
#include <fstream>
#include <map>

namespace workphone
{
    namespace
    {
        namespace profile_fs = std::filesystem;
        void profileRequire(bool valid, const char *message)
        {
            if(!valid) throw std::runtime_error(message);
        }
        String profileString(const luabind::object &value)
        {
            profileRequire(luabind::type(value) == LUA_TSTRING, "Profile fields must be strings");
            auto text = luabind::object_cast<String>(value);
            profileRequire(!text.empty() && text.size() <= 4096 && text.find('\0') == String::npos,
                           "Profile strings must contain 1 to 4096 bytes without NULs");
            return text;
        }
        bool profileUUID(const String &text)
        {
            if(text.size() != 36) return false;
            bool nonzero = false;
            for(size_t i = 0; i < text.size(); ++i)
            {
                const auto c = text[i];
                if(i == 8 || i == 13 || i == 18 || i == 23) { if(c != '-') return false; }
                else
                {
                    if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
                    nonzero = nonzero || c != '0';
                }
            }
            return nonzero;
        }
        size_t profileArraySize(const luabind::object &value, size_t maximum)
        {
            profileRequire(luabind::type(value) == LUA_TTABLE, "Profile lists must be arrays");
            size_t count = 0, largest = 0;
            for(luabind::iterator i(value), end; i != end; ++i)
            {
                profileRequire(luabind::type(i.key()) == LUA_TNUMBER, "Profile array has a nonnumeric key");
                const auto key = luabind::object_cast<double>(i.key());
                profileRequire(key >= 1 && key <= maximum && key == std::floor(key), "Invalid profile array index");
                largest = std::max(largest, static_cast<size_t>(key));
                ++count;
            }
            profileRequire(count > 0 && count == largest && count <= maximum, "Profile arrays must be nonempty, dense and bounded");
            return count;
        }
        void profileFields(const luabind::object &value, const std::set<String> &allowed)
        {
            profileRequire(luabind::type(value) == LUA_TTABLE, "Profile settings must be a table");
            std::set<String> seen;
            for(luabind::iterator i(value), end; i != end; ++i)
            {
                const auto key = profileString(i.key());
                profileRequire(allowed.count(key) && seen.insert(key).second, "Unknown profile field");
            }
            profileRequire(seen.size() == allowed.size(), "Missing profile field");
        }
        profile_fs::path profileProject(IPackageManager *manager)
        {
            profileRequire(manager && !manager->isResourcePackageBusy(), "Finish or cancel the active package job first");
            const auto root = core::IApplicationManager::instance()->getProjectPath();
            profileRequire(!root.empty(), "Open a project before editing a package profile");
            return profile_fs::canonical(profile_fs::u8path(root.c_str()));
        }
        profile_fs::path profilePath(const String &text, const profile_fs::path &project)
        {
            profileRequire(!text.empty() && text.size() <= 4096 && text.find('\0') == String::npos, "Invalid profile path");
            auto path = profile_fs::u8path(text.c_str());
            return (path.is_absolute() ? path : project / path).lexically_normal();
        }
        SmartPtr<AssetDatabaseManager> packageCatalog(IPackageManager *manager)
        {
            profileProject(manager);
            auto app = core::IApplicationManager::instance();
            auto resources = app->getResourceDatabase();
            auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>(resources->getDatabaseManager()) : nullptr;
            String root, error;
            profileRequire(catalog && canonicalAssetCatalogRoot(app->getProjectPath(), root, error) &&
                root == catalog->getProjectRoot(), "Active project catalog is unavailable; reopen the project.");
            return catalog;
        }

        luabind::object packageRootQueryImpl(IPackageManager *manager, const String &search,
            const String &type, lua_State *state, const luabind::object *continuation = nullptr)
        {
            auto result = luabind::newtable(state);
            result["ok"] = false;
            try
            {
                profileRequire(search.size() <= 4096 && type.size() <= 4096 &&
                    search.find('\0') == String::npos && type.find('\0') == String::npos, "Invalid catalog filter.");
                auto catalog = packageCatalog(manager);
                AssetDatabaseManager::EntryQuery query;
                query.search = search; query.type = type; query.includeScenes = false; query.pageSize = 25;
                AssetDatabaseManager::EntryCursor cursor;
                const bool resumed = continuation && luabind::type(*continuation) != LUA_TNIL;
                if(resumed)
                {
                    const auto &value = *continuation;
                    profileFields(value, {"afterUuid", "search", "type", "generation", "catalogInstance"});
                    cursor.afterUuid = profileString(value["afterUuid"]);
                    profileRequire(luabind::type(value["search"]) == LUA_TSTRING &&
                        luabind::type(value["type"]) == LUA_TSTRING, "Invalid catalog cursor filters.");
                    cursor.search = luabind::object_cast<String>(value["search"]);
                    cursor.type = luabind::object_cast<String>(value["type"]);
                    profileRequire(cursor.search == search && cursor.type == type, "Catalog filters changed; search again.");
                    auto identity = [&](const char *key) -> u64 {
                        const auto text = profileString(value[key]);
                        size_t used = 0;
                        const auto number = std::stoull(text.c_str(), &used);
                        profileRequire(used == text.size() && StringUtil::toString(static_cast<u64>(number)) == text,
                            "Invalid catalog cursor identity.");
                        return static_cast<u64>(number);
                    };
                    cursor.generation = identity("generation");
                    cursor.catalogInstance = identity("catalogInstance");
                    cursor.includeScenes = false;
                }
                auto page = catalog->queryEntries(query, resumed ? &cursor : nullptr);
                if(!page.succeeded) throw std::runtime_error(page.error.c_str());
                auto rows = luabind::newtable(state);
                int index = 1;
                for(const auto &entry : page.entries)
                {
                    auto row = luabind::newtable(state);
                    row["uuid"] = entry.uuid; row["path"] = entry.path; row["type"] = entry.type;
                    // Keep 64-bit identities out of Lua's floating-point number representation.
                    row["generation"] = StringUtil::toString(entry.generation);
                    row["catalogInstance"] = StringUtil::toString(entry.catalogInstance);
                    rows[index++] = row;
                }
                result["entries"] = rows; result["hasMore"] = page.hasMore; result["ok"] = true;
                if(page.hasMore)
                {
                    auto next = luabind::newtable(state);
                    next["afterUuid"] = page.next.afterUuid; next["search"] = page.next.search;
                    next["type"] = page.next.type;
                    next["generation"] = StringUtil::toString(page.next.generation);
                    next["catalogInstance"] = StringUtil::toString(page.next.catalogInstance);
                    result["next"] = next;
                }
            }
            catch(const std::exception &error) { result["error"] = String(error.what()); }
            return result;
        }

        luabind::object packageRootQuery(IPackageManager *manager, const String &search,
            const String &type, lua_State *state)
        { return packageRootQueryImpl(manager, search, type, state); }

        luabind::object packageRootQueryPage(IPackageManager *manager, const String &search,
            const String &type, const luabind::object &cursor, lua_State *state)
        { return packageRootQueryImpl(manager, search, type, state, &cursor); }

        luabind::object packageRootSelect(IPackageManager *manager, const luabind::object &row,
                                          lua_State *state)
        {
            auto result = luabind::newtable(state);
            result["ok"] = false;
            try
            {
                profileFields(row, {"uuid", "path", "type", "generation", "catalogInstance"});
                auto catalog = packageCatalog(manager);
                AssetDatabaseManager::EntrySnapshot entry;
                profileRequire(catalog->tryGetEntry(profileString(row["uuid"]), entry) &&
                    entry.kind == AssetDatabaseManager::EntryKind::File &&
                    entry.path == profileString(row["path"]) && entry.type == profileString(row["type"]) &&
                    StringUtil::toString(entry.generation) == profileString(row["generation"]) &&
                    StringUtil::toString(entry.catalogInstance) == profileString(row["catalogInstance"]) &&
                    catalog->isEntryCurrent(entry), "Catalog changed; search again before selecting a root.");
                profileRequire(profileUUID(entry.uuid), "Catalog entry does not have a package UUID.");
                result["uuid"] = entry.uuid; result["ok"] = true;
            }
            catch(const std::exception &error) { result["error"] = String(error.what()); }
            return result;
        }

        bool packagePreviewStart(IPackageManager *manager)
        {
            auto concrete = dynamic_cast<PackageManager *>(manager);
            return concrete && concrete->startResourcePackagePreview();
        }

        luabind::object packagePreviewReadImpl(IPackageManager *manager, lua_State *state, int page)
        {
            auto result = luabind::newtable(state);
            result["ok"] = false;
            try
            {
                profileRequire(page >= -1 && page <= 163, "Invalid closure preview page.");
                profileProject(manager);
                auto concrete = dynamic_cast<PackageManager *>(manager);
                profileRequire(concrete != nullptr, "Package manager does not support closure previews.");
                const auto path = concrete->getResourcePackagePreviewPath();
                profileRequire(!path.empty(), "No successful preview for the active project.");
                std::ifstream stream(profile_fs::u8path(path.c_str()), std::ios::binary | std::ios::ate);
                const auto size = stream.tellg();
                profileRequire(size > 0 && size <= 8 * 1024 * 1024, "Invalid closure preview report size.");
                std::string bytes(static_cast<size_t>(size), '\0');
                stream.seekg(0); stream.read(bytes.data(), bytes.size());
                profileRequire(static_cast<bool>(stream) && bytes.find('\0') == std::string::npos, "Cannot read closure preview report.");
                rapidjson::Document doc;
                doc.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag>(bytes.data(), bytes.size());
                profileRequire(!doc.HasParseError() && doc.IsObject() && doc.HasMember("version") &&
                    doc["version"].IsUint() && doc["version"].GetUint() == 1 && doc.HasMember("target") &&
                    doc["target"].IsString() && doc.HasMember("entries") && doc["entries"].IsArray() &&
                    doc["entries"].Size() > 0 && doc["entries"].Size() <= 4096, "Invalid closure preview report.");
                auto jsonString = [](const rapidjson::Value &object, const char *key) -> String {
                    profileRequire(object.IsObject() && object.HasMember(key) && object[key].IsString() &&
                        object[key].GetStringLength() > 0 && object[key].GetStringLength() <= 4096,
                        "Invalid closure preview string.");
                    const String value(object[key].GetString(), object[key].GetStringLength());
                    profileRequire(value.find('\0') == String::npos, "Invalid closure preview string.");
                    return value;
                };
                std::map<String, Array<String>> parents;
                std::set<String> ids;
                u64 totalBytes = 0;
                for(const auto &entry : doc["entries"].GetArray())
                {
                    const auto id = jsonString(entry, "resourceId");
                    profileRequire(ids.insert(id).second && profileUUID(jsonString(entry, "uuid")) &&
                        entry.HasMember("root") && entry["root"].IsBool() && entry.HasMember("payloadBytes") &&
                        entry["payloadBytes"].IsUint64() && entry["payloadBytes"].GetUint64() <= (1024ull << 30) &&
                        entry.HasMember("installDependencies") && entry["installDependencies"].IsArray() &&
                        entry["installDependencies"].Size() <= 4096, "Invalid closure preview entry.");
                    totalBytes += entry["payloadBytes"].GetUint64();
                    for(const auto &child : entry["installDependencies"].GetArray())
                    {
                        profileRequire(child.IsString() && child.GetStringLength() > 0 && child.GetStringLength() <= 4096,
                            "Invalid closure dependency.");
                        parents[String(child.GetString(), child.GetStringLength())].push_back(id);
                    }
                }
                for(const auto &item : parents) profileRequire(ids.count(item.first) != 0, "Incomplete closure preview.");
                auto rows = luabind::newtable(state);
                const int start = page < 0 ? 0 : page * 25;
                const int limit = page < 0 ? 100 : 25;
                profileRequire(start < static_cast<int>(doc["entries"].Size()), "Closure preview page is out of range.");
                int index = 1;
                int position = 0;
                for(const auto &entry : doc["entries"].GetArray())
                {
                    if(position++ < start) continue;
                    if(index > limit) break;
                    auto row = luabind::newtable(state);
                    const auto id = jsonString(entry, "resourceId");
                    row["resourceId"] = id; row["uuid"] = jsonString(entry, "uuid");
                    row["root"] = entry["root"].GetBool();
                    row["payloadBytes"] = StringUtil::toString(static_cast<u64>(entry["payloadBytes"].GetUint64()));
                    String reason;
                    size_t count = 0;
                    for(const auto &parent : parents[id])
                    {
                        if(count++ == 3) { reason += ", ..."; break; }
                        if(!reason.empty()) reason += ", ";
                        reason += parent;
                    }
                    row["includedBy"] = reason;
                    rows[index++] = row;
                }
                result["entries"] = rows; result["total"] = static_cast<int>(doc["entries"].Size());
                result["page"] = page < 0 ? 0 : page;
                result["hasPrevious"] = start > 0;
                result["hasMore"] = start + index - 1 < static_cast<int>(doc["entries"].Size());
                result["totalPayloadBytes"] = StringUtil::toString(totalBytes);
                result["target"] = jsonString(doc, "target"); result["path"] = path; result["ok"] = true;
            }
            catch(const std::exception &error) { result["error"] = String(error.what()); }
            return result;
        }

        luabind::object packagePreviewRead(IPackageManager *manager, lua_State *state)
        { return packagePreviewReadImpl(manager, state, -1); }

        luabind::object packagePreviewReadPage(IPackageManager *manager, int page, lua_State *state)
        {
            // Negative values are reserved for the legacy bounded reader.
            return packagePreviewReadImpl(manager, state, page < 0 ? 164 : page);
        }

        luabind::object packageProfileRead(IPackageManager *manager, const String &path, lua_State *state)
        {
            auto result = luabind::newtable(state);
            result["ok"] = false;
            try
            {
                const auto project = profileProject(manager);
                const auto file = profilePath(path, project);
                const auto args = loadPackageProfile(file.u8string().c_str());
                profileRequire(profile_fs::equivalent(project, profile_fs::u8path(args[1].c_str())), "Profile belongs to a different project");
                auto values = luabind::newtable(state);
                const char *fields[] = {"sourceRoot", "catalog", "cookedRoot", "output", "target"};
                for(size_t i = 0; i < 5; ++i) values[fields[i]] = args[i + 1];
                auto mappings = luabind::newtable(state), roots = luabind::newtable(state);
                size_t mappingIndex = 1, rootIndex = 1;
                std::set<String> types, identities;
                for(size_t i = 6; i < args.size(); i += 2)
                    if(args[i] == "--root")
                    {
                        profileRequire(profileUUID(args[i + 1]) && identities.insert(StringUtil::make_lower(args[i + 1])).second,
                                       "Invalid or duplicate root UUID");
                        roots[rootIndex++] = args[i + 1];
                    }
                    else
                    {
                        const auto split = args[i + 1].find('=');
                        const auto type = args[i + 1].substr(0, split), extension = args[i + 1].substr(split + 1);
                        profileRequire(resource::ResourceTypeID(extension).isValid() && types.insert(type).second,
                                       "Invalid or duplicate type mapping");
                        auto mapping = luabind::newtable(state);
                        mapping["catalogType"] = type;
                        mapping["extension"] = extension;
                        mappings[mappingIndex++] = mapping;
                    }
                values["mappings"] = mappings; values["roots"] = roots;
                result["profile"] = values;
                result["path"] = String(file.u8string().c_str());
                result["ok"] = true;
                manager->setResourcePackageProfile(file.u8string().c_str());
            }
            catch(const std::exception &error) { result["error"] = String(error.what()); }
            return result;
        }
        luabind::object packageProfileSave(IPackageManager *manager, const String &path,
                                           const luabind::object &values, lua_State *state)
        {
            auto result = luabind::newtable(state);
            result["ok"] = false;
            try
            {
                const auto project = profileProject(manager);
                const auto file = profilePath(path, project);
                profileFields(values, {"sourceRoot", "catalog", "cookedRoot", "output", "target", "mappings", "roots"});
                Array<String> args = {"WorkphoneResourcePackage"};
                for(const char *field : {"sourceRoot", "catalog", "cookedRoot", "output"})
                    args.push_back(profilePath(profileString(values[field]), project).u8string().c_str());
                profileRequire(profile_fs::equivalent(project, profile_fs::u8path(args[1].c_str())), "Profile source root must match the active project");
                args.push_back(profileString(values["target"]));
                const luabind::object mappings = values["mappings"];
                const luabind::object roots = values["roots"];
                const auto mappingCount = profileArraySize(mappings, 128), rootCount = profileArraySize(roots, 512);
                std::set<String> types, identities;
                for(size_t i = 1; i <= mappingCount; ++i)
                {
                    const luabind::object mapping = mappings[i];
                    profileFields(mapping, {"catalogType", "extension"});
                    const auto type = profileString(mapping["catalogType"]), extension = profileString(mapping["extension"]);
                    profileRequire(type.find('=') == String::npos && resource::ResourceTypeID(extension).isValid() &&
                                   types.insert(type).second, "Invalid or duplicate type mapping");
                    args.push_back("--map"); args.push_back(type + "=" + extension);
                }
                for(size_t i = 1; i <= rootCount; ++i)
                {
                    const auto uuid = profileString(roots[i]);
                    profileRequire(profileUUID(uuid) && identities.insert(StringUtil::make_lower(uuid)).second,
                                   "Invalid or duplicate root UUID");
                    args.push_back("--root"); args.push_back(uuid);
                }
                savePackageProfile(file.u8string().c_str(), args);
                manager->setResourcePackageProfile(file.u8string().c_str());
                result["path"] = String(file.u8string().c_str());
                result["ok"] = true;
            }
            catch(const std::exception &error) { result["error"] = String(error.what()); }
            return result;
        }
    }
    SmartPtr<render::ITexture> loadEditorTexture( IResourceDatabase *resources, const String &path )
    {
        return resources ? dynamic_pointer_cast<render::ITexture>( resources->loadResource( path ) )
                         : nullptr;
    }

    SmartPtr<render::ITexture> loadEditorMediaTexture( IResourceDatabase *resources, const String &path )
    {
        auto database = dynamic_cast<ResourceDatabase *>( resources );
        return database ? dynamic_pointer_cast<render::ITexture>( database->loadMediaResource( path ) )
                        : nullptr;
    }

    luabind::object _IProfiler_getProfiles( IProfiler *profiler, lua_State *L )
    {
        auto result = luabind::newtable( L );
        size_t index = 1;
        for( const auto &profile : profiler->getProfiles() )
        {
            result[index++] = profile;
        }
        return result;
    }

    Parameter Application_triggerEvent( core::IApplicationManager *applicationManager,
                                        lua_Integer eventType, lua_Integer eventValue,
                                        const Array<Parameter> &arguments,
                                        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                        SmartPtr<IEvent> event )
    {
        return applicationManager->triggerEvent( static_cast<EventType>( eventType ), eventValue,
                                                 arguments, sender, object, event );
    }

    Parameter Application_triggerEventString( core::IApplicationManager *applicationManager,
                                              const String &eventType, const String &eventValue,
                                              const Array<Parameter> &arguments,
                                              SmartPtr<ISharedObject> sender,
                                              SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto eEventType = EventType::Object;
        auto iEventValue = StringUtil::getHash( eventValue );
        return applicationManager->triggerEvent( eEventType, iEventValue, arguments, sender, object,
                                                 event );
    }

    lua_Integer _IStateMessage_getType( IStateMessage *message )
    {
        auto hash = message->getType();
        return *&hash;
    }

    void _IStateMessage_setType( IStateMessage *message, lua_Integer type )
    {
        auto hash = *&type;
        message->setType( hash );
    }

    lua_Integer _ITask_getTask( ITask *task )
    {
        return static_cast<lua_Integer>( task->getTask() );
    }

    void _ITask_setTask( ITask *task, lua_Integer taskId )
    {
        task->setTask( static_cast<TaskId>( taskId ) );
    }

    lua_Integer _IApplicationManager_getStateTask( core::IApplicationManager *applicationManager )
    {
        return applicationManager ? static_cast<lua_Integer>( applicationManager->getStateTask() ) : 0;
    }

    lua_Integer _IApplicationManager_getApplicationTask( core::IApplicationManager *applicationManager )
    {
        return applicationManager ? static_cast<lua_Integer>( applicationManager->getApplicationTask() )
                                  : 0;
    }

    // Helper to cast INativeFileDialog::Result to int for Lua
    int _INativeFileDialog_openDialog( SmartPtr<INativeFileDialog> dialog )
    {
        if( dialog )
        {
            return static_cast<int>( dialog->openDialog() );
        }
        WP_LOG_ERROR( "INativeFileDialog is null" );

        return static_cast<int>( INativeFileDialog::Result::Dialog_Error );
    }

    // Helper to cast INativeFileDialog::DialogMode to int for Lua
    int _INativeFileDialog_getDialogMode( SmartPtr<INativeFileDialog> dialog )
    {
        if( dialog )
        {
            return static_cast<int>( dialog->getDialogMode() );
        }
        WP_LOG_ERROR( "INativeFileDialog is null" );

        return static_cast<int>( INativeFileDialog::DialogMode::Open );
    }

    // Helper to cast INativeFileDialog::FilterMode to int for Lua
    int _INativeFileDialog_getFilterMode( SmartPtr<INativeFileDialog> dialog )
    {
        if( dialog )
        {
            return static_cast<int>( dialog->getFilterMode() );
        }
        WP_LOG_ERROR( "INativeFileDialog is null" );

        return static_cast<int>( INativeFileDialog::FilterMode::FilterMode_Files );
    }

    SmartPtr<ISharedObject> _IFactoryManager_make_object( SmartPtr<IFactoryManager> factoryManager,
                                                          hash64 typeId )
    {
        if( factoryManager )
        {
            return factoryManager->createById( static_cast<u32>( typeId ) );
        }
        WP_LOG_ERROR( "IFactoryManager is null" );
        return nullptr;
    }

    SmartPtr<ISharedObject> _IFactoryManager_make_object_with_hint(
        SmartPtr<IFactoryManager> factoryManager, hash64 typeId, const String &hint )
    {
        if( factoryManager )
        {
            return factoryManager->createById( static_cast<u32>( typeId ), hint );
        }
        WP_LOG_ERROR( "IFactoryManager is null" );
        return nullptr;
    }

    SmartPtr<ISharedObject> _IFactoryManager_make_object_by_string(
        SmartPtr<IFactoryManager> factoryManager, const String &typeName )
    {
        if( factoryManager )
        {
            return factoryManager->createById( ISharedObject::typeInfo(), typeName );
        }

        WP_LOG_ERROR( "IFactoryManager is null" );
        return nullptr;
    }

    void bindSystem( lua_State *L )
    {
        using namespace core;
        using namespace editor;
        using namespace luabind;

        module( L )[class_<FileInfo>( "FileInfo" )
                        .def_readwrite( "filePath", &FileInfo::filePath )
                        .def_readwrite( "filePathLowerCase", &FileInfo::filePathLowerCase )
                        .def_readwrite( "path", &FileInfo::path )
                        .def_readwrite( "absolutePath", &FileInfo::absolutePath )
                        .def_readwrite( "fileName", &FileInfo::fileName )
                        .def_readwrite( "fileNameLowerCase", &FileInfo::fileNameLowerCase )
                        .def_readwrite( "compressedSize", &FileInfo::compressedSize )
                        .def_readwrite( "uncompressedSize", &FileInfo::uncompressedSize )
                        .def_readwrite( "fileId", &FileInfo::fileId )
                        .def_readwrite( "archiveId", &FileInfo::archiveId )
                        .def_readwrite( "offset", &FileInfo::offset )
                        .def_readwrite( "isDirectory", &FileInfo::isDirectory )];

        module( L )[class_<FileSelection, ISharedObject>( "FileSelection" )
                        .def( "getFilePath", &FileSelection::getFilePath )
                        .def( "setFilePath", &FileSelection::setFilePath )
                        .def( "getFileInfo", &FileSelection::getFileInfo )
                        .def( "setFileInfo", &FileSelection::setFileInfo )
                        .scope[def( "typeInfo", FileSelection::typeInfo )]];

        module( L )[class_<IFactory, ISharedObject>( "IFactory" )
                        .def( "isObjectDerivedFromByInfo", &IFactory::isObjectDerivedFromByInfo )
                        .def( "getTypeName", &IFactory::getTypeName )
                        .def( "setTypeName", &IFactory::setTypeName )
                        .def( "getGrowSize", &IFactory::getGrowSize )
                        .def( "setGrowSize", &IFactory::setGrowSize )
                        .def( "allocatePoolData", &IFactory::allocatePoolData )
                        .def( "freePoolData", &IFactory::freePoolData )
                        .def( "allocateMemory", &IFactory::allocateMemory )
                        .def( "freeMemory", &IFactory::freeMemory )
                        .def( "createObject", &IFactory::createObject )
                        .def( "freeObject", &IFactory::freeObject )
                        .def( "createArray", &IFactory::createArray )
                        .def( "getObjectTypeName", &IFactory::getObjectTypeName )
                        .def( "setObjectTypeName", &IFactory::setObjectTypeName )
                        .def( "getObjectTypeHash", &IFactory::getObjectTypeHash )
                        .def( "setObjectTypeHash", &IFactory::setObjectTypeHash )
                        .def( "getObjectSize", &IFactory::getObjectSize )
                        .def( "setObjectSize", &IFactory::setObjectSize )
                        .def( "getMemoryUsed", &IFactory::getMemoryUsed )
                        .def( "getListener", &IFactory::getListener )
                        .def( "getInstanceObjects", &IFactory::getInstanceObjects )
                        .def( "getTags", &IFactory::getTags )
                        .def( "setTags", &IFactory::setTags )];

        module( L )[class_<IFactoryManager, ISharedObject>( "IFactoryManager" )
                        .def( "allocateData", &IFactoryManager::allocateData )
                        .def( "freeData", &IFactoryManager::freeData )
                        .def( "addFactory", &IFactoryManager::addFactory )
                        .def( "removeFactory", &IFactoryManager::removeFactory )
                        .def( "removeAllFactories", &IFactoryManager::removeAllFactories )
                        .def( "getFactoryByName", &IFactoryManager::getFactoryByName )
                        .def( "getFactoryById", &IFactoryManager::getFactoryById )
                        .def( "hasFactoryByName", &IFactoryManager::hasFactoryByName )
                        .def( "hasFactoryById", &IFactoryManager::hasFactoryById )
                        .def( "getFactories", &IFactoryManager::getFactories )
                        .def( "createById", static_cast<SmartPtr<ISharedObject> ( IFactoryManager::* )(
                                                u32 ) const>( &IFactoryManager::createById ) )
                        .def( "createByIdWithHint",
                              static_cast<SmartPtr<ISharedObject> ( IFactoryManager::* )(
                                  u32, const String & ) const>( &IFactoryManager::createById ) )
                        .def( "setPoolSize", &IFactoryManager::setPoolSize )
                        .def( "compareTags", &IFactoryManager::compareTags )
                        .def( "hasTag", &IFactoryManager::hasTag )
                        .def( "addTag", &IFactoryManager::addTag )
                        .def( "removeTag", &IFactoryManager::removeTag )
                        .def( "getFactoriesWithTag", &IFactoryManager::getFactoriesWithTag )
                        .def( "make_object", _IFactoryManager_make_object )
                        .def( "make_object_with_hint", _IFactoryManager_make_object_with_hint )
                        .def( "make_object", _IFactoryManager_make_object_by_string )
                        .scope[def( "typeInfo", IFactoryManager::typeInfo )]];

        module( L )[class_<IPackageManager, ISharedObject, SmartPtr<ISharedObject>>( "IPackageManager" )
                        .def( "createPackage", &IPackageManager::createPackage )
                        .def( "setResourcePackageProfile", &IPackageManager::setResourcePackageProfile )
                        .def( "readResourcePackageProfile", packageProfileRead )
                        .def( "startResourcePackagePreview", packagePreviewStart )
                        .def( "readResourcePackagePreview", packagePreviewRead )
                        .def( "readResourcePackagePreviewPage", packagePreviewReadPage )
                        .def( "queryResourcePackageRoots", packageRootQuery )
                        .def( "queryResourcePackageRootsPage", packageRootQueryPage )
                        .def( "selectResourcePackageRoot", packageRootSelect )
                        .def( "saveResourcePackageProfile", packageProfileSave )
                        .def( "getResourcePackageProfile", &IPackageManager::getResourcePackageProfile )
                        .def( "startResourcePackage", &IPackageManager::startResourcePackage )
                        .def( "cancelResourcePackage", &IPackageManager::cancelResourcePackage )
                        .def( "isResourcePackageBusy", &IPackageManager::isResourcePackageBusy )
                        .def( "getResourcePackageStatus", &IPackageManager::getResourcePackageStatus )
                        .def( "getResourcePackageDiagnostic", &IPackageManager::getResourcePackageDiagnostic )
                        .def( "getResourcePackageLogPath", &IPackageManager::getResourcePackageLogPath )
                        .def( "getPackageTextures", &IPackageManager::getPackageTextures )
                        .def( "setPackageTextures", &IPackageManager::setPackageTextures )
                        .scope[def( "typeInfo", IPackageManager::typeInfo )]];

        module( L )[class_<IPrototype, ISharedObject, SmartPtr<ISharedObject>>( "IPrototype" )
                        .def( "getParentPrototype", &IPrototype::getParentPrototype )
                        .def( "setParentPrototype", &IPrototype::setParentPrototype )
                        .def( "getProperties", &IPrototype::getProperties )
                        .def( "setProperties", &IPrototype::setProperties )
                        .scope[def( "typeInfo", IPrototype::typeInfo )]];

        module( L )[class_<IResource, IPrototype, SmartPtr<ISharedObject>>( "IResource" )
                        .def( "saveToFile", &IResource::saveToFile )
                        .def( "loadFromFile", &IResource::loadFromFile )
                        .def( "save", &IResource::save )
                        .def( "import", &IResource::import )
                        .def( "reimport", &IResource::reimport )
                        .def( "getFileSystemId", &IResource::getFileSystemId )
                        .def( "setFileSystemId", &IResource::setFileSystemId )
                        .def( "getFilePath", &IResource::getFilePath )
                        .def( "setFilePath", &IResource::setFilePath )
                        .def( "getSettingsFileSystemId", &IResource::getSettingsFileSystemId )
                        .def( "setSettingsFileSystemId", &IResource::setSettingsFileSystemId )
                        .def( "getDependencies", &IResource::getDependencies )
                        .def( "getResourceManager", &IResource::getResourceManager )
                        .def( "setResourceManager", &IResource::setResourceManager )
                        .def( "getStateContext", &IResource::getStateContext )
                        .def( "handleStateMessage", &IResource::handleStateMessage )
                        .def( "handleStateChanged", &IResource::handleStateChanged )
                        .scope[def( "typeInfo", IResource::typeInfo )]];

        module( L )
            [class_<IResourceDatabase, ISharedObject, SmartPtr<ISharedObject>>( "IResourceDatabase" )
                 .def( "importCache", &IResourceDatabase::importCache )
                 .def( "calculateDependencies", &IResourceDatabase::calculateDependencies )
                 .def( "refresh", &IResourceDatabase::refresh )
                 .def( "optimise", &IResourceDatabase::optimise )
                 .def( "clean", &IResourceDatabase::clean )
                 .def( "deleteCache", &IResourceDatabase::deleteCache )
                 .def( "importAssets", &IResourceDatabase::importAssets )
                 .def( "reimportAssets", &IResourceDatabase::reimportAssets )
                 .def( "build", &IResourceDatabase::build )
                 .def( "hasResource", &IResourceDatabase::hasResource )
                 .def( "addResource", &IResourceDatabase::addResource )
                 .def( "removeResource", &IResourceDatabase::removeResource )
                 .def( "removeResourceFromPath", &IResourceDatabase::removeResourceFromPath )
                 .def( "findResource", &IResourceDatabase::findResource )
                 .def( "cloneResource", &IResourceDatabase::cloneResource )
                 .def( "getResourceData", &IResourceDatabase::getResourceData )
                 .def( "getResources", &IResourceDatabase::getResources )
                 .def( "importFolder",
                       static_cast<void ( IResourceDatabase::* )( SmartPtr<IFolderExplorer>, bool )>(
                           &IResourceDatabase::importFolder ) )
                 .def( "importFolder", static_cast<void ( IResourceDatabase::* )(
                                           const String &, bool )>( &IResourceDatabase::importFolder ) )
                 .def( "importFile", &IResourceDatabase::importFile )
                 //.def( "loadResource", ( SmartPtr<IResource>( IResourceDatabase::* )( hash64 ) ) &
                 //                          IResourceDatabase::loadResource )
                 .def( "loadResource", static_cast<SmartPtr<IResource> ( IResourceDatabase::* )(
                                           const String & )>( &IResourceDatabase::loadResource ) )
                 .def( "loadResource", static_cast<SmartPtr<IResource> ( IResourceDatabase::* )(
                                           const UUID & )>( &IResourceDatabase::loadResource ) )
                 .def( "loadTexture", &loadEditorTexture )
                 .def( "loadMediaTexture", &loadEditorMediaTexture )
                 .def( "loadDirector", static_cast<SmartPtr<IBuildDirector> ( IResourceDatabase::* )(
                                           const String & )>( &IResourceDatabase::loadDirector ) )
                 .def( "loadDirectorFromResourcePath",
                       static_cast<SmartPtr<IBuildDirector> ( IResourceDatabase::* )( const String & )>(
                           &IResourceDatabase::loadDirectorFromResourcePath ) )
                 .def( "loadDirectorFromResourcePath",
                       static_cast<SmartPtr<IBuildDirector> ( IResourceDatabase::* )(
                           const String &, u32 )>( &IResourceDatabase::loadDirectorFromResourcePath ) )
                 .def( "loadDirector", static_cast<SmartPtr<IBuildDirector> ( IResourceDatabase::* )(
                                           SmartPtr<IResource> )>( &IResourceDatabase::loadDirector ) )
                 //.def( "loadResourceById", &IResourceDatabase::loadResourceById )
                 .def( "getDatabaseManager", &IResourceDatabase::getDatabaseManager )
                 .def( "setDatabaseManager", &IResourceDatabase::setDatabaseManager )
                 .def( "createOrRetrieve",
                       static_cast<Pair<SmartPtr<IResource>, bool> ( IResourceDatabase::* )(
                           hash_type, const String & )>( &IResourceDatabase::createOrRetrieve ) )
                 .def( "createOrRetrieve",
                       static_cast<Pair<SmartPtr<IResource>, bool> ( IResourceDatabase::* )(
                           const String & )>( &IResourceDatabase::createOrRetrieve ) )
                 .def( "getObject", &IResourceDatabase::getObject )
                 .scope[def( "typeInfo", IResourceDatabase::typeInfo )]];

        module( L )[class_<IEventListener, ISharedObject>( "IEventListener" )
                        .def( "handleEvent", &IEventListener::handleEvent )];

        module( L )[class_<TypeManager>( "TypeManager" )
                        .def( "getName", &TypeManager::getName )
                        .def( "getLabel", &TypeManager::getLabel )
                        .def( "setLabel", &TypeManager::setLabel )
                        .def( "getHash", &TypeManager::getHash )
                        .def( "getBaseType", &TypeManager::getBaseType )
                        .def( "isExactly", &TypeManager::isExactly )
                        .def( "isDerived", &TypeManager::isDerived )
                        .def( "getClassHierarchy", &TypeManager::getClassHierarchy )
                        .def( "getClassHierarchyId", &TypeManager::getClassHierarchyId )
                        .def( "getTypeIndex", &TypeManager::getTypeIndex )
                        .def( "getNumInstances", &TypeManager::getNumInstances )
                        .def( "getTotalNumTypes", &TypeManager::getTotalNumTypes )
                        .def( "getDataType", &TypeManager::getDataType )
                        .def( "setDataType", &TypeManager::setDataType )
                        .def( "getBaseTypes", &TypeManager::getBaseTypes )
                        .def( "getBaseTypeNames", &TypeManager::getBaseTypeNames )
                        //.def( "getDerivedTypes", &TypeManager::getDerivedTypes )
                        .def( "getDerivedTypeNames", &TypeManager::getDerivedTypeNames )
                        .def( "getTypeGroup", &TypeManager::getTypeGroup )
                        .def( "getIdFromName", &TypeManager::getIdFromName )
                        .def( "getIdFromHash", &TypeManager::getIdFromHash )
                        .def( "getObjectId", &TypeManager::getObjectId )
                        .def( "resizeObjectData", &TypeManager::resizeObjectData )
                    //.def( "getType", &TypeManager::getType )
                    //.def( "registerType", &TypeManager::registerType )
        ];

        module(
            L )[class_<INativeFileDialog, ISharedObject, SmartPtr<ISharedObject>>( "INativeFileDialog" )
                    .def( "openDialog", _INativeFileDialog_openDialog )
                    .def( "getFilePath", &INativeFileDialog::getFilePath )
                    .def( "setFilePath", &INativeFileDialog::setFilePath )
                    .def( "getFileExtension", &INativeFileDialog::getFileExtension )
                    .def( "setFileExtension", &INativeFileDialog::setFileExtension )
                    .def( "getDialogMode", &_INativeFileDialog_getDialogMode )
                    .def( "setDialogMode", &INativeFileDialog::setDialogMode )
                    .def( "getFilterMode", &_INativeFileDialog_getFilterMode )
                    .def( "setFilterMode", &INativeFileDialog::setFilterMode )
                    .enum_( "DialogMode" )
                        [value( "Select", static_cast<int>( INativeFileDialog::DialogMode::Select ) ),
                         value( "Open", static_cast<int>( INativeFileDialog::DialogMode::Open ) ),
                         value( "Save", static_cast<int>( INativeFileDialog::DialogMode::Save ) )]
                    .enum_( "FilterMode" )
                        [value( "Files",
                                static_cast<int>( INativeFileDialog::FilterMode::FilterMode_Files ) ),
                         value( "Dirs",
                                static_cast<int>( INativeFileDialog::FilterMode::FilterMode_Dirs ) )]
                    .enum_( "Result" )
                        [value( "Dialog_Error",
                                static_cast<int>( INativeFileDialog::Result::Dialog_Error ) ),
                         value( "Dialog_Okay",
                                static_cast<int>( INativeFileDialog::Result::Dialog_Okay ) ),
                         value( "Dialog_Cancel",
                                static_cast<int>( INativeFileDialog::Result::Dialog_Cancel ) ),
                         value( "Count", static_cast<int>( INativeFileDialog::Result::Count ) )]];

        module( L )
            [class_<IFileSystem, ISharedObject, SmartPtr<IFileSystem>>( "FileSystem" )
                 .def( "openFileDialog", &IFileSystem::openFileDialog )
                 .def( "open", static_cast<SmartPtr<IStream> ( IFileSystem::* )( const String & )>(
                                   &IFileSystem::open ) )
                 .def( "openFull",
                       static_cast<SmartPtr<IStream> ( IFileSystem::* )(
                           const String &, bool, bool, bool, bool, bool )>( &IFileSystem::open ) )
                 .def( "addFileArchive", &IFileSystem::addFileArchive )
                 .def( "addFolder", &IFileSystem::addFolder )
                 .def( "addArchive", static_cast<void ( IFileSystem::* )( const String & )>(
                                         &IFileSystem::addArchive ) )
                 .def( "addArchiveTypeName",
                       static_cast<void ( IFileSystem::* )( const String &, const String & )>(
                           &IFileSystem::addArchive ) )
                 .def( "addArchiveType",
                       static_cast<void ( IFileSystem::* )( const String &, IFileSystem::ArchiveType )>(
                           &IFileSystem::addArchive ) )
                 .def( "getFileArchiveCount", &IFileSystem::getFileArchiveCount )
                 .def( "removeFileArchive",
                       static_cast<bool ( IFileSystem::* )( u32 )>( &IFileSystem::removeFileArchive ) )
                 .def( "removeFileArchiveByName", static_cast<bool ( IFileSystem::* )( const String & )>(
                                                      &IFileSystem::removeFileArchive ) )
                 .def( "moveFileArchive", &IFileSystem::moveFileArchive )
                 .def( "getFileArchive", &IFileSystem::getFileArchive )
                 .def( "getWorkingDirectory", &IFileSystem::getWorkingDirectory )
                 .def( "setWorkingDirectory", &IFileSystem::setWorkingDirectory )
                 .def( "getAbsolutePath", &IFileSystem::getAbsolutePath )
                 .def( "isExistingFile",
                       static_cast<bool ( IFileSystem::* )( const String &, bool, bool ) const>(
                           &IFileSystem::isExistingFile ) )
                 .def( "isExistingFilePath",
                       static_cast<bool ( IFileSystem::* )( const String &, const String &, bool, bool )
                                       const>( &IFileSystem::isExistingFile ) )
                 .def( "isExistingFolder", &IFileSystem::isExistingFolder )
                 .def( "getFileDir", &IFileSystem::getFileDir )
                 .def( "createDirectories", &IFileSystem::createDirectories )
                 .def( "deleteFilesFromPath", &IFileSystem::deleteFilesFromPath )
                 .def( "getFilesWithExtension",
                       static_cast<Array<FileInfo> ( IFileSystem::* )( const String & ) const>(
                           &IFileSystem::getFilesWithExtension ) )
                 .def( "getFilesWithExtensionPath",
                       static_cast<Array<FileInfo> ( IFileSystem::* )( const String &, const String & )
                                       const>( &IFileSystem::getFilesWithExtension ) )
                 .def( "getFileNamesWithExtension",
                       static_cast<Array<String> ( IFileSystem::* )( const String & ) const>(
                           &IFileSystem::getFileNamesWithExtension ) )
                 .def( "getFileNamesWithExtensionPath", static_cast<Array<String> ( IFileSystem::* )(
                                                            const String &, const String &, bool )>(
                                                            &IFileSystem::getFileNamesWithExtension ) )
                 .def( "getFileNamesInFolder", FileSystemHelper::_getFileNamesInFolder )
                 .def( "getSubFolders", &IFileSystem::getSubFolders )
                 .def( "isFolder", &IFileSystem::isFolder )
                 .def( "readAllBytes", &IFileSystem::readAllBytes )
                 .def( "readAllText", &IFileSystem::readAllText )
                 .def( "writeAllBytes", static_cast<void ( IFileSystem::* )(
                                            const String &, u8 *, u32 )>( &IFileSystem::writeAllBytes ) )
                 .def( "writeAllBytesArray",
                       static_cast<void ( IFileSystem::* )( const String &, Array<u8> )>(
                           &IFileSystem::writeAllBytes ) )
                 .def( "writeAllText", &IFileSystem::writeAllText )
                 .def( "getBase64String", &IFileSystem::getBase64String )
                 .def( "getBytesString", &IFileSystem::getBytesString )
                 .def( "copyFolder", &IFileSystem::copyFolder )
                 .def( "copyFile", &IFileSystem::copyFile )
                 .def( "deleteFile", &IFileSystem::deleteFile )
                 .def( "getFilePath", &IFileSystem::getFilePath )
                 .def( "getFileName", &IFileSystem::getFileName )
                 .def( "getFileHash", &IFileSystem::getFileHash )
                 .def( "getFolders", &IFileSystem::getFolders )
                 .def( "getFiles", &IFileSystem::getFiles )
                 .def( "getFilesAsAbsolutePaths", &IFileSystem::getFilesAsAbsolutePaths )
                 .def( "getFolderListing", static_cast<SmartPtr<IFolderExplorer> ( IFileSystem::* )(
                                               const String & )>( &IFileSystem::getFolderListing ) )
                 .def( "getFolderListingExt",
                       static_cast<SmartPtr<IFolderExplorer> ( IFileSystem::* )(
                           const String &, const String & )>( &IFileSystem::getFolderListing ) )
                 .def( "addFile", &IFileSystem::addFile )
                 .def( "addFiles", &IFileSystem::addFiles )
                 .def( "removeFile", &IFileSystem::removeFile )
                 .def( "removeFiles", &IFileSystem::removeFiles )
                 //.def( "findFileInfoById", &IFileSystem::findFileInfo )
                 //.def( "findFileInfoByPath", &IFileSystem::findFileInfo )
                 .def( "getSystemFiles", &IFileSystem::getSystemFiles )
                 .def( "refreshAll", &IFileSystem::refreshAll )
                 .def( "refreshPath", &IFileSystem::refreshPath )
                 .def( "getFileId", &IFileSystem::getFileId )
                 .def( "lock", &IFileSystem::lock )
                 .def( "unlock", &IFileSystem::unlock )
                 .def( "isValid", &IFileSystem::isValid )];

        module( L )[class_<IStateMessage, ISharedObject, SmartPtr<IStateMessage>>( "IStateMessage" )
                        .def( "getType", _IStateMessage_getType )
                        .def( "setType", _IStateMessage_setType )
                        .def( "getSender", &IStateMessage::getSender )
                        .def( "setSender", &IStateMessage::setSender )
                        .def( "getStateContext", &IStateMessage::getStateContext )
                        .def( "setStateContext", &IStateMessage::setStateContext )];

        module(
            L )[class_<IEvent, ISharedObject, SmartPtr<ISharedObject>>( "IEvent" )
                    .def( "getTarget", &IEvent::getTarget )
                    .def( "setTarget", &IEvent::setTarget )
                    .def( "isTarget", &IEvent::isTarget )
                    .enum_( "Type" )[value( "Loading", static_cast<lua_Integer>( EventType::Loading ) ),
                                     value( "Object", static_cast<lua_Integer>( EventType::Object ) ),
                                     value( "Input", static_cast<lua_Integer>( EventType::Input ) ),
                                     // value( "IO", (lua_Integer)EventType::IO ),
                                     value( "UI", static_cast<lua_Integer>( EventType::UI ) ),
                                     value( "Window", static_cast<lua_Integer>( EventType::Window ) ),
                                     value( "Scene", static_cast<lua_Integer>( EventType::Scene ) ),
                                     // value( "Actor", (lua_Integer)EventType::Actor ),
                                     // value( "Component", (lua_Integer)EventType::Component ),
                                     // value( "Application", (lua_Integer)EventType::Application ),
                                     // value( "Renderer", (lua_Integer)EventType::Renderer ),
                                     value( "Count", static_cast<lua_Integer>( EventType::Count ) )]
                    .enum_( "Events" )
                        [value( "loadingStateChanged", IEvent::loadingStateChanged ),
                         value( "loadScene", IEvent::loadScene ),
                         value( "unloadScene", IEvent::unloadScene ),
                         // value( "handlePropertyButtonClick",
                         // (lua_Integer)IEvent::handlePropertyButtonClick ), value( "addActor",
                         // (lua_Integer)IEvent::addActor ),
                         // value( "removeActor", (lua_Integer)IEvent::removeActor ),
                         // value( "enabled", (lua_Integer)IEvent::enabled ),
                         // value( "sceneChanged", (lua_Integer)IEvent::sceneChanged ),
                         // value( "createUI", (lua_Integer)IEvent::createUI ),
                         // value( "destroyUI", (lua_Integer)IEvent::destroyUI ),
                         // value( "handleWindowClose", (lua_Integer)IEvent::handleWindowClose ),
                         // value( "handleWindowResize", (lua_Integer)IEvent::handleWindowResize ),
                         // value( "handleWindowMove", (lua_Integer)IEvent::handleWindowMove ),
                         value( "handleTreeSelectionActivated", IEvent::handleTreeSelectionActivated ),
                         // value( "handleTreeSelectionRelease",
                         // (lua_Integer)IEvent::handleTreeSelectionRelease ),
                         value( "handleTreeNodeDoubleClicked", IEvent::handleTreeNodeDoubleClicked ),
                         value( "handlePropertyChanged", IEvent::handlePropertyChanged ),
                         value( "handleValueChanged", IEvent::handleValueChanged ),
                         value( "handleMouseClicked", IEvent::handleMouseClicked ),
                         value( "handleMouseReleased", IEvent::handleMouseReleased ),
                         value( "handleSelection", IEvent::handleSelection ),
                         value( "handleToggle", IEvent::handleToggle ),
                         value( "handleDrop", IEvent::handleDrop ),
                         value( "handleDrag", IEvent::handleDrag ),
                         value( "inputEvent", IEvent::inputEvent ),
                         value( "updateEvent", IEvent::updateEvent ),
                         value( "handleEnterFrame", IEvent::handleEnterFrame ),
                         value( "CLICK_HASH", IEvent::CLICK_HASH ),
                         value( "ACTIVATE_HASH", IEvent::ACTIVATE_HASH ),
                         value( "DEACTIVATE_HASH", IEvent::DEACTIVATE_HASH ),
                         value( "UPDATE_HASH", IEvent::UPDATE_HASH ),
                         value( "HANDLE_MESSAGE_HASH", IEvent::HANDLE_MESSAGE_HASH ),
                         value( "INITIALISE_START_HASH", IEvent::INITIALISE_START_HASH ),
                         value( "INITIALISE_END_HASH", IEvent::INITIALISE_END_HASH ),
                         value( "ADD_CHILD_HASH", IEvent::ADD_CHILD_HASH ),
                         value( "REMOVE_CHILD_HASH", IEvent::REMOVE_CHILD_HASH ),
                         value( "CHANGED_STATE_HASH", IEvent::CHANGED_STATE_HASH ),
                         value( "CHILD_CHANGED_STATE_HASH", IEvent::CHILD_CHANGED_STATE_HASH ),
                         value( "TOGGLE_ENABLED_HASH", IEvent::TOGGLE_ENABLED_HASH ),
                         value( "TOGGLE_HIGHLIGHT_HASH", IEvent::TOGGLE_HIGHLIGHT_HASH ),
                         value( "VISIBLE_HASH", IEvent::VISIBLE_HASH ),
                         value( "SHOW_HASH", IEvent::SHOW_HASH ),
                         value( "HIDE_HASH", IEvent::HIDE_HASH ),
                         value( "SELECT_HASH", IEvent::SELECT_HASH ),
                         value( "DESELECT_HASH", IEvent::DESELECT_HASH ),
                         value( "GAIN_FOCUS_HASH", IEvent::GAIN_FOCUS_HASH ),
                         value( "LOST_FOCUS_HASH", IEvent::LOST_FOCUS_HASH )
                         // value( "getPreviousCommand", (lua_Integer)IEvent::getPreviousCommand ),
                         // value( "refreshAll", (lua_Integer)IEvent::refreshAll ),
                         // value( "refreshPath", (lua_Integer)IEvent::refreshPath ),
                         // value( "addUIElement", (lua_Integer)IEvent::addUIElement ),
                         // value( "removeUIElement", (lua_Integer)IEvent::removeUIElement ),
                         // value( "transformUIElement", (lua_Integer)IEvent::transformUIElement ),
                         // value( "windowMovedOrResized", (lua_Integer)IEvent::windowMovedOrResized ),
                         // value( "renderTargetTextureLoaded",
                         // (lua_Integer)IEvent::renderTargetTextureLoaded ), value(
                         // "renderTargetTextureUnloaded",
                         // (lua_Integer)IEvent::renderTargetTextureUnloaded ), value( "meshLoaded",
                         // (lua_Integer)IEvent::meshLoaded ), value( "meshesImported",
                         // (lua_Integer)IEvent::meshesImported ), value( "cameraManagerReset",
                         // (lua_Integer)IEvent::cameraManagerReset )
        ]];

        // Luabind stores enum constants as 32-bit integers. Override the event hashes on the
        // class table so Lua receives the same 64-bit values emitted by the native event system.
        static_assert( sizeof( lua_Integer ) >= sizeof( hash_type ),
                       "Lua integers must be wide enough to store native event hashes." );
        lua_getglobal( L, "IEvent" );
        const auto setEventHash = [L]( const char *name, hash_type value ) {
            lua_pushinteger( L, static_cast<lua_Integer>( value ) );
            lua_setfield( L, -2, name );
        };

        setEventHash( "loadingStateChanged", IEvent::loadingStateChanged );
        setEventHash( "loadScene", IEvent::loadScene );
        setEventHash( "unloadScene", IEvent::unloadScene );
        setEventHash( "handleTreeSelectionActivated", IEvent::handleTreeSelectionActivated );
        setEventHash( "handleTreeNodeDoubleClicked", IEvent::handleTreeNodeDoubleClicked );
        setEventHash( "handlePropertyChanged", IEvent::handlePropertyChanged );
        setEventHash( "handleValueChanged", IEvent::handleValueChanged );
        setEventHash( "handleMouseClicked", IEvent::handleMouseClicked );
        setEventHash( "handleMouseReleased", IEvent::handleMouseReleased );
        setEventHash( "handleSelection", IEvent::handleSelection );
        setEventHash( "handleToggle", IEvent::handleToggle );
        setEventHash( "handleDrop", IEvent::handleDrop );
        setEventHash( "handleDrag", IEvent::handleDrag );
        setEventHash( "inputEvent", IEvent::inputEvent );
        setEventHash( "updateEvent", IEvent::updateEvent );
        setEventHash( "handleEnterFrame", IEvent::handleEnterFrame );
        setEventHash( "CLICK_HASH", IEvent::CLICK_HASH );
        setEventHash( "ACTIVATE_HASH", IEvent::ACTIVATE_HASH );
        setEventHash( "DEACTIVATE_HASH", IEvent::DEACTIVATE_HASH );
        setEventHash( "UPDATE_HASH", IEvent::UPDATE_HASH );
        setEventHash( "HANDLE_MESSAGE_HASH", IEvent::HANDLE_MESSAGE_HASH );
        setEventHash( "INITIALISE_START_HASH", IEvent::INITIALISE_START_HASH );
        setEventHash( "INITIALISE_END_HASH", IEvent::INITIALISE_END_HASH );
        setEventHash( "ADD_CHILD_HASH", IEvent::ADD_CHILD_HASH );
        setEventHash( "REMOVE_CHILD_HASH", IEvent::REMOVE_CHILD_HASH );
        setEventHash( "CHANGED_STATE_HASH", IEvent::CHANGED_STATE_HASH );
        setEventHash( "CHILD_CHANGED_STATE_HASH", IEvent::CHILD_CHANGED_STATE_HASH );
        setEventHash( "TOGGLE_ENABLED_HASH", IEvent::TOGGLE_ENABLED_HASH );
        setEventHash( "TOGGLE_HIGHLIGHT_HASH", IEvent::TOGGLE_HIGHLIGHT_HASH );
        setEventHash( "VISIBLE_HASH", IEvent::VISIBLE_HASH );
        setEventHash( "SHOW_HASH", IEvent::SHOW_HASH );
        setEventHash( "HIDE_HASH", IEvent::HIDE_HASH );
        setEventHash( "SELECT_HASH", IEvent::SELECT_HASH );
        setEventHash( "DESELECT_HASH", IEvent::DESELECT_HASH );
        setEventHash( "GAIN_FOCUS_HASH", IEvent::GAIN_FOCUS_HASH );
        setEventHash( "LOST_FOCUS_HASH", IEvent::LOST_FOCUS_HASH );
        lua_pop( L, 1 );

        module( L )[class_<ISelectionManager, ISharedObject, SmartPtr<ISelectionManager>>(
                        "ISelectionManager" )
                        .def( "addSelectedObject", &ISelectionManager::addSelectedObject )
                        .def( "removeSelectedObject", &ISelectionManager::removeSelectedObject )
                        .def( "clearSelection", &ISelectionManager::clearSelection )
                        .def( "getSelection", &ISelectionManager::getSelection )];

        module(
            L )[class_<IStateContext, ISharedObject, SmartPtr<IStateContext>>( "IStateContext" )
                    .def( "setOwner", &IStateContext::setOwner )
                    .def( "getOwner", &IStateContext::getOwner )
                    .def( "isDirty", &IStateContext::isDirty )
                    .def( "setDirty", &IStateContext::setDirty )
                    .def( "addStateListener", &IStateContext::addStateListener )
                    .def( "removeStateListener", &IStateContext::removeStateListener )
                    .def( "getStateListeners", &IStateContext::getStateListeners )
                    .def( "addEventListener", &IStateContext::addEventListener )
                    .def( "removeEventListener", &IStateContext::removeEventListener )
                    .def( "getEventListeners", &IStateContext::getEventListeners )
                    .def( "addState", &IStateContext::addState )
                    .def( "removeState", &IStateContext::removeState )
                    .def( "removeStatesById", &IStateContext::removeStatesById )
                    .def( "clear", &IStateContext::clear )
                    .def( "getStateById", ( SmartPtr<IState> ( IStateContext::* )( hash_type ) const ) &
                                              IStateContext::getStateById )
                    .def( "getStateByTypeId", &IStateContext::getStateByTypeId )
                    .def( "getStates", &IStateContext::getStates )
                    .def( "sendMessage", &IStateContext::sendMessage )
                    .def( "invalidateState", &IStateContext::invalidateState )];

        module( L )[class_<IStateManager, ISharedObject, SmartPtr<IStateManager>>( "StateManager" )
                        //.def( "addStateObject", &IStateManager::addStateObject )
                        //.def( "removeStateContext", _removeStateContext )
                        .def( "findStateObject", &IStateManager::findStateContext )];

        module( L )
            [class_<ITimer, ISharedObject, SmartPtr<ITimer>>( "ITimer" )
                 .def( "updateFixed", &ITimer::updateFixed )
                 .def( "getTimeSinceLevelLoad", &ITimer::getTimeSinceSceneLoad )
                 .def( "setTimeSinceLevelLoad", &ITimer::setSceneLoadTime )
                 .def( "getSmoothTime", &ITimer::getSmoothTime )
                 //.def( "getSmoothDeltaTime", &ITimer::getSmoothDeltaTime )
                 .def( "setSmoothDeltaTime", &ITimer::setSmoothDeltaTime )
                 .def( "getTimeMilliseconds", &ITimer::getTimeMilliseconds )
                 .def( "getRealTime", &ITimer::getRealTime )
                 .def( "getTimeIntervalMilliseconds", &ITimer::getTimeIntervalMilliseconds )
                 .def( "now", &ITimer::now )
                 .def( "getTime", static_cast<time_interval ( ITimer::* )() const>( &ITimer::getTime ) )
                 .def( "getTimeTask",
                       static_cast<f64 ( ITimer::* )( TaskId ) const>( &ITimer::getTime ) )
                 .def( "getDeltaTime",
                       static_cast<time_interval ( ITimer::* )() const>( &ITimer::getDeltaTime ) )
                 .def( "getDeltaTimeTask",
                       static_cast<f64 ( ITimer::* )( TaskId ) const>( &ITimer::getDeltaTime ) )
                 .def( "getSmoothDeltaTimeTask",
                       static_cast<f64 ( ITimer::* )( TaskId ) const>( &ITimer::getSmoothDeltaTime ) )
                 .def( "setFrameSmoothingPeriod", &ITimer::setFrameSmoothingPeriod )
                 .def( "getFrameSmoothingPeriod", &ITimer::getFrameSmoothingPeriod )
                 .def( "getFrameSmoothingTime", &ITimer::getFrameSmoothingTime )
                 .def( "setFrameSmoothingTime", &ITimer::setFrameSmoothingTime )
                 .def( "resetSmoothing", &ITimer::resetSmoothing )
                 .def( "getTickCount", static_cast<u32 ( ITimer::* )()>( &ITimer::getTickCount ) )
                 .def( "getTickCountTask",
                       static_cast<u32 ( ITimer::* )( TaskId )>( &ITimer::getTickCount ) )
                 .def( "getFixedTimeInterval",
                       static_cast<f64 ( ITimer::* )() const>( &ITimer::getFixedTimeInterval ) )
                 .def( "getFixedTimeIntervalTask",
                       static_cast<f64 ( ITimer::* )( TaskId ) const>( &ITimer::getFixedTimeInterval ) )
                 .def( "setFixedTimeInterval",
                       static_cast<void ( ITimer::* )( f64 )>( &ITimer::setFixedTimeInterval ) )
                 .def( "setFixedTimeIntervalTask",
                       static_cast<void ( ITimer::* )( TaskId, f64 )>( &ITimer::setFixedTimeInterval ) )
                 .def( "getPreviousTime", &ITimer::getPreviousTime )
                 .def( "getPreviousTimeTask", &ITimer::getPreviousTime )
                 //.def( "getStartOffset", &ITimer::getStartOffset )
                 .def( "setStartOffset",
                       static_cast<void ( ITimer::* )( f64 )>( &ITimer::setStartOffset ) )
                 .def( "getStartOffsetTask",
                       static_cast<f64 ( ITimer::* )( TaskId ) const>( &ITimer::getStartOffset ) )
                 .def( "setStartOffsetTask",
                       static_cast<void ( ITimer::* )( TaskId, f64 )>( &ITimer::setStartOffset ) )
                 .def( "reset", static_cast<void ( ITimer::* )()>( &ITimer::reset ) )
                 .def( "resetWithTime", static_cast<void ( ITimer::* )( f64 )>( &ITimer::reset ) )
                 .def( "getFixedTime", static_cast<f64 ( ITimer::* )() const>( &ITimer::getFixedTime ) )
                 .def( "getFixedTimeTask",
                       static_cast<f64 ( ITimer::* )( u32 ) const>( &ITimer::getFixedTime ) )
                 //.def( "getFixedTimeNow", ( f64 ( ITimer::* )( void ) const ) &ITimer::getFixedTimeNow
                 //) .def( "getFixedTimeNowTask", ( f64 ( ITimer::* )( u32 ) const )
                 //&ITimer::getFixedTimeNow ) .def( "setFixedTime", &ITimer::setFixedTime )
                 .def( "getFixedOffset", ( f64 ( ITimer::* )( void ) const ) & ITimer::getFixedOffset )
                 .def( "getFixedOffsetTask", &ITimer::getFixedOffset )
                 .def( "setFixedOffset", (void ( ITimer::* )( f64 ))&ITimer::setFixedOffset )
                 .def( "setFixedOffsetTask", &ITimer::setFixedOffset )
                 .def( "isSteady", &ITimer::isSteady )
             //.def( "getMinDeltaTime", &ITimer::getMinDeltaTime )
             //.def( "setMinDeltaTime", &ITimer::setMinDeltaTime )
             //.def( "getMaxDeltaTime", &ITimer::getMaxDeltaTime )
             //.def( "setMaxDeltaTime", &ITimer::setMaxDeltaTime )
             //.def( "getEnableSmoothing", &ITimer::getEnableSmoothing )
             //.def( "setEnableSmoothing", &ITimer::setEnableSmoothing )
             //.def( "setStartTime", &ITimer::setStartTime )
             //.def( "getDerivedFixedTime", &ITimer::getDerivedFixedTime )
        ];

        module( L )[class_<IApplication, ISharedObject, SmartPtr<ISharedObject>>( "IApplication" )
                        .def( "run", &IApplication::run )
                        .def( "iterate", &IApplication::iterate )
                        .def( "getFSMPtr", &IApplication::getFSMPtr )
                        .def( "getFSM", &IApplication::getFSM )
                        .def( "setFSM", &IApplication::setFSM )
                        .def( "getActiveThreads", &IApplication::getActiveThreads )
                        .def( "setActiveThreads", &IApplication::setActiveThreads )
                        .def( "createSceneObject", &IApplication::createSceneObject )
                        .def( "createPanel", &IApplication::createPanel )
                        .def( "createButton", &IApplication::createButton )
                        .def( "createText", &IApplication::createText )
                        .def( "createToggle", &IApplication::createToggle )
                        .def( "createSlider", &IApplication::createSlider )
                        .def( "createScrollbar", &IApplication::createScrollbar )
                        .def( "createDefaultCubemap", &IApplication::createDefaultCubemap )
                        .def( "createDefaultSky", &IApplication::createDefaultSky )
                        .def( "createDefaultCamera", &IApplication::createDefaultCamera )
                        .def( "createDefaultCube", &IApplication::createDefaultCube )
                        .def( "createDefaultCubeMesh", &IApplication::createDefaultCubeMesh )
                        .def( "createDefaultGround", &IApplication::createDefaultGround )
                        .def( "createDefaultTerrain", &IApplication::createDefaultTerrain )
                        .def( "createDefaultConstraint", &IApplication::createDefaultConstraint )
                        .def( "createDirectionalLight", &IApplication::createDirectionalLight )
                        .def( "createPointLight", &IApplication::createPointLight )
                        .def( "createDefaultPlane", &IApplication::createDefaultPlane )
                        .def( "createDefaultVehicle", &IApplication::createDefaultVehicle )
                        .def( "createDefaultCar", &IApplication::createDefaultCar )
                        .def( "createDefaultTruck", &IApplication::createDefaultTruck )
                        .def( "createDefaultParticleSystem", &IApplication::createDefaultParticleSystem )
                        .def( "createDefaultMaterialUI", &IApplication::createDefaultMaterialUI )
                        .def( "createDefaultMaterial", &IApplication::createDefaultMaterial )
                        .def( "createDefaultMaterials", &IApplication::createDefaultMaterials )
                        .def( "importScene", &IApplication::importScene )
                        .def( "getCreateFrameStatistics", &IApplication::getCreateFrameStatistics )
                        .def( "setCreateFrameStatistics", &IApplication::setCreateFrameStatistics )
                        .def( "getPluginsConfigFilePath", &IApplication::getPluginsConfigFilePath )
                        .def( "setPluginsConfigFilePath", &IApplication::setPluginsConfigFilePath )
                        .def( "isDebugMode", &IApplication::isDebugMode )
                        .def( "setDebugMode", &IApplication::setDebugMode )
                        .def( "getMediaPath", &IApplication::getMediaPath )
                        .def( "getApplicationFlags", &IApplication::getApplicationFlags )
                        .def( "setApplicationFlags", &IApplication::setApplicationFlags )];

        module( L )[class_<IApplicationManager, ISharedObject, SmartPtr<ISharedObject>>(
                        "IApplicationManager" )
                        .def( "getEnableRenderer", &IApplicationManager::getEnableRenderer )
                        .def( "setEnableRenderer", &IApplicationManager::setEnableRenderer )
                        .def( "isPauseMenuActive", &IApplicationManager::isPauseMenuActive )
                        .def( "setPauseMenuActive", &IApplicationManager::setPauseMenuActive )
                        .def( "isSceneLoading", &IApplicationManager::isSceneLoading )
                        .def( "setSceneLoading", &IApplicationManager::setSceneLoading )
                        .def( "isEditor", &IApplicationManager::isEditor )
                        .def( "setEditor", &IApplicationManager::setEditor )
                        .def( "isEditorCamera", &IApplicationManager::isEditorCamera )
                        .def( "setEditorCamera", &IApplicationManager::setEditorCamera )
                        .def( "isPlaying", &IApplicationManager::isPlaying )
                        .def( "setPlaying", &IApplicationManager::setPlaying )
                        .def( "isPaused", &IApplicationManager::isPaused )
                        .def( "setPaused", &IApplicationManager::setPaused )
                        .def( "isRunning", &IApplicationManager::isRunning )
                        .def( "setRunning", &IApplicationManager::setRunning )
                        .def( "getQuit", &IApplicationManager::getQuit )
                        .def( "setQuit", &IApplicationManager::setQuit )
                        .def( "hasTasks", &IApplicationManager::hasTasks )
                        .def( "getCachePath", &IApplicationManager::getCachePath )
                        .def( "setCachePath", &IApplicationManager::setCachePath )
                        .def( "getSettingsCachePath", &IApplicationManager::getSettingsPath )
                        .def( "setSettingsCachePath", &IApplicationManager::setSettingsPath )
                        .def( "getProjectPath", &IApplicationManager::getProjectPath )
                        .def( "setProjectPath", &IApplicationManager::setProjectPath )
                        .def( "getProjectLibraryName", &IApplicationManager::getProjectLibraryName )
                        .def( "setProjectLibraryName", &IApplicationManager::setProjectLibraryName )
                        .def( "getMediaPath", &IApplicationManager::getMediaPath )
                        .def( "setMediaPath", &IApplicationManager::setMediaPath )
                        .def( "getRenderMediaPath", &IApplicationManager::getRenderMediaPath )
                        .def( "setRenderMediaPath", &IApplicationManager::setRenderMediaPath )
                        .def( "getBuildConfig", &IApplicationManager::getBuildConfig )
                        .def( "getProjectLibraryExtension",
                              &IApplicationManager::getProjectLibraryExtension )
                        .def( "getProjectLibraryPath", &IApplicationManager::getProjectLibraryPath )
                        .def( "getStateTask", _IApplicationManager_getStateTask )
                        .def( "getApplicationTask", _IApplicationManager_getApplicationTask )
                        .def( "getFSM", &IApplicationManager::getFSM )
                        .def( "setFSM", &IApplicationManager::setFSM )
                        .def( "getEditorSettings", &IApplicationManager::getEditorSettings )
                        .def( "setEditorSettings", &IApplicationManager::setEditorSettings )
                        .def( "getPlayerSettings", &IApplicationManager::getPlayerSettings )
                        .def( "setPlayerSettings", &IApplicationManager::setPlayerSettings )
                        .def( "getAiManager", &IApplicationManager::getAiManager )
                        .def( "setAiManager", &IApplicationManager::setAiManager )
                        .def( "getLogManager", &IApplicationManager::getLogManager )
                        .def( "setLogManager", &IApplicationManager::setLogManager )
                        .def( "getFactoryManager", &IApplicationManager::getFactoryManager )
                        .def( "setFactoryManager", &IApplicationManager::setFactoryManager )
                        .def( "getProcessManager", &IApplicationManager::getProcessManager )
                        .def( "setProcessManager", &IApplicationManager::setProcessManager )
                        .def( "getParticleManager", &IApplicationManager::getParticleManager )
                        .def( "setParticleManager", &IApplicationManager::setParticleManager )
                        .def( "getProfiler", &IApplicationManager::getProfiler )
                        .def( "setProfiler", &IApplicationManager::setProfiler )
                        .def( "getApplication", &IApplicationManager::getApplication )
                        .def( "setApplication", &IApplicationManager::setApplication )
                        .def( "getJobQueue", &IApplicationManager::getJobQueue )
                        .def( "setJobQueue", &IApplicationManager::setJobQueue )
                        .def( "getGraphicsSystem", &IApplicationManager::getGraphicsSystem )
                        .def( "setGraphicsSystem", &IApplicationManager::setGraphicsSystem )
                        .def( "getVideoManager", &IApplicationManager::getVideoManager )
                        .def( "setVideoManager", &IApplicationManager::setVideoManager )
                        .def( "getTaskManager", &IApplicationManager::getTaskManager )
                        .def( "setTaskManager", &IApplicationManager::setTaskManager )
                        .def( "getEditorManager", &IApplicationManager::getEditorManager )
                        .def( "setEditorManager", &IApplicationManager::setEditorManager )
                        .def( "getFileSystem", &IApplicationManager::getFileSystem )
                        .def( "setFileSystem", &IApplicationManager::setFileSystem )
                        .def( "getTimer", &IApplicationManager::getTimer )
                        .def( "setTimer", &IApplicationManager::setTimer )
                        .def( "getFsmManager", &IApplicationManager::getFsmManager )
                        .def( "setFsmManager", &IApplicationManager::setFsmManager )
                        .def( "getProceduralManager", &IApplicationManager::getProceduralManager )
                        .def( "setProceduralManager", &IApplicationManager::setProceduralManager )
                        .def( "getPhysicsManager2D", &IApplicationManager::getPhysicsManager2D )
                        .def( "setPhysicsManager2D", &IApplicationManager::setPhysicsManager2D )
                        .def( "getPhysicsManager", &IApplicationManager::getPhysicsManager )
                        .def( "setPhysicsManager", &IApplicationManager::setPhysicsManager )
                        .def( "getScriptManager", &IApplicationManager::getScriptManager )
                        .def( "setScriptManager", &IApplicationManager::setScriptManager )
                        .def( "getInput", &IApplicationManager::getInput )
                        .def( "setInput", &IApplicationManager::setInput )
                        .def( "getInputDeviceManager", &IApplicationManager::getInputDeviceManager )
                        .def( "setInputDeviceManager", &IApplicationManager::setInputDeviceManager )
                        .def( "getThreadPool", &IApplicationManager::getThreadPool )
                        .def( "setThreadPool", &IApplicationManager::setThreadPool )
                        .def( "getConsole", &IApplicationManager::getConsole )
                        .def( "setConsole", &IApplicationManager::setConsole )
                        .def( "getCameraManager", &IApplicationManager::getCameraManager )
                        .def( "setCameraManager", &IApplicationManager::setCameraManager )
                        .def( "getStateManager", &IApplicationManager::getStateManager )
                        .def( "setStateManager", &IApplicationManager::setStateManager )
                        .def( "getCommandManager", &IApplicationManager::getCommandManager )
                        .def( "setCommandManager", &IApplicationManager::setCommandManager )
                        .def( "getResourceManager", &IApplicationManager::getResourceManager )
                        .def( "setResourceManager", &IApplicationManager::setResourceManager )
                        .def( "getPrefabManager", &IApplicationManager::getPrefabManager )
                        .def( "setPrefabManager", &IApplicationManager::setPrefabManager )
                        .def( "getMeshLoader", &IApplicationManager::getMeshLoader )
                        .def( "setMeshLoader", &IApplicationManager::setMeshLoader )
                        .def( "getResourceDatabase", &IApplicationManager::getResourceDatabase )
                        .def( "setResourceDatabase", &IApplicationManager::setResourceDatabase )
                        .def( "getGameManager", &IApplicationManager::getGameManager )
                        .def( "setGameManager", &IApplicationManager::setGameManager )
                        .def( "getSelectionManager", &IApplicationManager::getSelectionManager )
                        .def( "setSelectionManager", &IApplicationManager::setSelectionManager )
                        .def( "getSoundManager", &IApplicationManager::getSoundManager )
                        .def( "setSoundManager", &IApplicationManager::setSoundManager )
                        .def( "getUI", &IApplicationManager::getUI )
                        .def( "setUI", &IApplicationManager::setUI )
                        .def( "getRenderUI", &IApplicationManager::getRenderUI )
                        .def( "setRenderUI", &IApplicationManager::setRenderUI )
                        .def( "getDatabase", &IApplicationManager::getDatabase )
                        .def( "setDatabase", &IApplicationManager::setDatabase )
                        .def( "getMeshManager", &IApplicationManager::getMeshManager )
                        .def( "setMeshManager", &IApplicationManager::setMeshManager )
                        .def( "getPackageManager", &IApplicationManager::getPackageManager )
                        .def( "setPackageManager", &IApplicationManager::setPackageManager )
                        .def( "getVehicleManager", &IApplicationManager::getVehicleManager )
                        .def( "setVehicleManager", &IApplicationManager::setVehicleManager )
                        .def( "getLoadProgress", &IApplicationManager::getLoadProgress )
                        .def( "setLoadProgress", &IApplicationManager::setLoadProgress )
                        .def( "addLoadProgress", &IApplicationManager::addLoadProgress )
                        .def( "getActors", &IApplicationManager::getActors )
                        .def( "getPluginManager", &IApplicationManager::getPluginManager )
                        .def( "setPluginManager", &IApplicationManager::setPluginManager )
                        .def( "addPlugin", &IApplicationManager::addPlugin )
                        .def( "removePlugin", &IApplicationManager::removePlugin )
                        .def( "getWindow", &IApplicationManager::getWindow )
                        .def( "setWindow", &IApplicationManager::setWindow )
                        .def( "getSceneRenderWindow", &IApplicationManager::getSceneRenderWindow )
                        .def( "setSceneRenderWindow", &IApplicationManager::setSceneRenderWindow )
                        .def( "getNetworkManager", &IApplicationManager::getNetworkManager )
                        .def( "setNetworkManager", &IApplicationManager::setNetworkManager )
                        .def( "getStringPool", &IApplicationManager::getStringPool )
                        .def( "setStringPool", &IApplicationManager::setStringPool )
                        .def( "getStringPoolW", &IApplicationManager::getStringPoolW )
                        .def( "setStringPoolW", &IApplicationManager::setStringPoolW )
                        .def( "getPropertyNamePool", &IApplicationManager::getPropertyNamePool )
                        .def( "setPropertyNamePool", &IApplicationManager::setPropertyNamePool )
                        .def( "getPropertyValuePool", &IApplicationManager::getPropertyValuePool )
                        .def( "setPropertyValuePool", &IApplicationManager::setPropertyValuePool )
                        .def( "getComponentByType", &IApplicationManager::getComponentByType )
                        .def( "triggerEvent", Application_triggerEvent )
                        .def( "triggerEvent", Application_triggerEventString )
                        .def( "clearAllEvents", &IApplicationManager::clearAllEvents )
                        .scope[def( "instance", &IApplicationManager::instance ),
                               def( "setInstance", &IApplicationManager::setInstance )]

        ];

        module( L )[class_<IThreadPool, ISharedObject, SmartPtr<IThreadPool>>( "IThreadPool" )
                        .def( "addWorkerThread", &IThreadPool::addWorkerThread )
                        .def( "getNumThreads", &IThreadPool::getNumThreads )
                        .def( "setNumThreads", &IThreadPool::setNumThreads )
                        .def( "stop", &IThreadPool::stop )];

        module( L )[class_<IPlugin, ISharedObject, SmartPtr<IPlugin>>( "IPlugin" )
                        .def( "getFilePath", &IPlugin::getFilePath )
                        .def( "setFilePath", &IPlugin::setFilePath )];

        module( L )[class_<IPluginManager, ISharedObject, SmartPtr<IPluginManager>>( "IPluginManager" )
                        .def( "loadPlugin", (SmartPtr<IPlugin> ( IPluginManager::* )(
                                                const String & ))&IPluginManager::loadPlugin )
                        .def( "loadPluginInstance", (void ( IPluginManager::* )(
                                                        SmartPtr<IPlugin> ))&IPluginManager::loadPlugin )
                        .def( "unloadPlugin", &IPluginManager::unloadPlugin )
                        .def( "getPlugins", &IPluginManager::getPlugins )
                        .def( "setPlugins", &IPluginManager::setPlugins )];

        module(
            L )[class_<IProcessManager, ISharedObject, SmartPtr<IProcessManager>>( "IProcessManager" )
                    .def( "createProcess", (void ( IProcessManager::* )(
                                               const String & ))&IProcessManager::createProcess )
                    .def( "shellExecute",
                          (void ( IProcessManager::* )( const String & ))&IProcessManager::shellExecute )
                    .def( "isProcessRunning", (bool ( IProcessManager::* )(
                                                  const String & ))&IProcessManager::isProcessRunning )
                    .def( "terminateProcess", (bool ( IProcessManager::* )(
                                                  const String & ))&IProcessManager::terminateProcess )];

        module( L )[class_<IProfile, ISharedObject, SmartPtr<IProfile>>( "IProfile" )
                        .def( "getLabel", &IProfile::getLabel )
                        .def( "setLabel", &IProfile::setLabel )
                        .def( "getDescription", &IProfile::getDescription )
                        .def( "setDescription", &IProfile::setDescription )
                        .def( "getTotal", &IProfile::getTotal )
                        .def( "setTotal", &IProfile::setTotal )
                        .def( "start", &IProfile::start )
                        .def( "end", &IProfile::end )
                        .def( "getAverageTimeTaken", &IProfile::getAverageTimeTaken )
                        .def( "getAverageDeltaTime", &IProfile::getAverageDeltaTime )
                        .def( "clear", &IProfile::clear )];

        module( L )[class_<IProfiler, ISharedObject, SmartPtr<IProfiler>>( "IProfiler" )
                        .def( "addProfile", &IProfiler::addProfile )
                        .def( "removeProfile", &IProfiler::removeProfile )
                        .def( "getProfile", &IProfiler::getProfile )
                        .def( "getProfiles", &_IProfiler_getProfiles )
                        .def( "logResults", &IProfiler::logResults )];

        module( L )[class_<IProject, ISharedObject, SmartPtr<IProject>>( "IProject" )
                        .def( "getApplicationFilePath", &IProject::getApplicationFilePath )
                        .def( "setApplicationFilePath", &IProject::setApplicationFilePath )
                        .def( "getPath", &IProject::getPath )
                        .def( "setPath", &IProject::setPath )
                        .def( "getScriptFilePaths", &IProject::getScriptFilePaths )
                        .def( "setScriptFilePaths", &IProject::setScriptFilePaths )
                        .def( "getResourceFolders", &IProject::getResourceFolders )
                        .def( "setResourceFolders", &IProject::setResourceFolders )
                        .def( "getApplicationType", &IProject::getApplicationType )
                        .def( "setApplicationType", &IProject::setApplicationType )
                        .def( "isArchive", &IProject::isArchive )
                        .def( "setArchive", &IProject::setArchive )
                        .def( "isDirty", &IProject::isDirty )
                        .def( "setDirty", &IProject::setDirty )
                        .def( "getPlugin", &IProject::getPlugin )
                        .def( "setPlugin", &IProject::setPlugin )];

        module(
            L )[class_<IProjectManager, ISharedObject, SmartPtr<IProjectManager>>( "IProjectManager" )
                    .def( "generateProject", &IProjectManager::generateProject )
                    .def( "addIncludeFolder", &IProjectManager::addIncludeFolder )
                    .def( "removeIncludeFolder", &IProjectManager::removeIncludeFolder )
                    .def( "addLibraryFolder", &IProjectManager::addLibraryFolder )
                    .def( "removeLibraryFolder", &IProjectManager::removeLibraryFolder )];

        module( L )[class_<ICommand, ISharedObject, SmartPtr<ICommand>>( "ICommand" )
                        .def( "undo", &ICommand::undo )
                        .def( "redo", &ICommand::redo )
                        .def( "execute", &ICommand::execute )
                        .def( "isPrimary", &ICommand::isPrimary )
                        .def( "setPrimary", &ICommand::setPrimary )];

        module(
            L )[class_<ICommandManager, ISharedObject, SmartPtr<ICommandManager>>( "ICommandManager" )
                    .def( "addCommand", &ICommandManager::addCommand )
                    .def( "removeCommand", &ICommandManager::removeCommand )
                    .def( "hasCommand", &ICommandManager::hasCommand )
                    .def( "isCommandQueued", &ICommandManager::isCommandQueued )
                    .def( "getNextCommand", &ICommandManager::getNextCommand )
                    .def( "getPreviousCommand", &ICommandManager::getPreviousCommand )
                    .def( "clearAll", &ICommandManager::clearAll )];

        module( L )[class_<IJob, ISharedObject, SmartPtr<IJob>>( "IJob" )
                        .def( "getAffinity", &IJob::getAffinity )
                        .def( "setAffinity", &IJob::setAffinity )
                        .def( "execute", &IJob::execute )
                        .def( "coroutineExecute", &IJob::coroutine_execute )
                        .def( "getProgress", &IJob::getProgress )
                        .def( "setProgress", &IJob::setProgress )
                        .def( "getPriority", &IJob::getPriority )
                        .def( "setPriority", &IJob::setPriority )
                        .def( "isPrimary", &IJob::isPrimary )
                        .def( "setPrimary", &IJob::setPrimary )
                        .def( "isFinished", &IJob::isFinished )
                        .def( "setInterrupted", &IJob::setInterrupted )
                        .def( "isInterrupted", &IJob::isInterrupted )
                        .def( "stop", &IJob::stop )
                        .def( "wait", (bool ( IJob::* )())&IJob::wait )
                        .def( "waitFor", (bool ( IJob::* )( f64 ))&IJob::wait )
                        .def( "isCoroutine", &IJob::isCoroutine )
                        .def( "setCoroutine", &IJob::setCoroutine )];

        module( L )[class_<IJobGroup, ISharedObject, SmartPtr<IJobGroup>>( "IJobGroup" )
                        .def( "addJob", &IJobGroup::addJob )
                        .def( "removeJob", &IJobGroup::removeJob )
                        .def( "getJobs", &IJobGroup::getJobs )
                        .def( "getJobCount", &IJobGroup::getJobCount )
                        .def( "clearJobs", &IJobGroup::clearJobs )
                        .def( "addDependency", &IJobGroup::addDependency )
                        .def( "removeDependency", &IJobGroup::removeDependency )
                        .def( "getPrerequisites", &IJobGroup::getPrerequisites )
                        .def( "getDependents", &IJobGroup::getDependents )
                        .def( "arePrerequisitesSatisfied", &IJobGroup::arePrerequisitesSatisfied )
                        .def( "hasCircularDependencies", &IJobGroup::hasCircularDependencies )
                        .def( "getReadyJobs", &IJobGroup::getReadyJobs )
                        .def( "getExecutingJobs", &IJobGroup::getExecutingJobs )
                        .def( "getFinishedJobs", &IJobGroup::getFinishedJobs )
                        .def( "areAllJobsFinished", &IJobGroup::areAllJobsFinished )
                        .def( "setParallelExecution", &IJobGroup::setParallelExecution )
                        .def( "isParallelExecution", &IJobGroup::isParallelExecution )
                        .def( "setMaxConcurrentJobs", &IJobGroup::setMaxConcurrentJobs )
                        .def( "getMaxConcurrentJobs", &IJobGroup::getMaxConcurrentJobs )
                        .def( "cancelAllJobs", &IJobGroup::cancelAllJobs )
                        .def( "pauseAllJobs", &IJobGroup::pauseAllJobs )
                        .def( "resumeAllJobs", &IJobGroup::resumeAllJobs )
                        .def( "getGroupProgress", &IJobGroup::getGroupProgress )];

        module( L )[class_<IJobQueue, ISharedObject, SmartPtr<IJobQueue>>( "IJobQueue" )
                        .def( "hasJobs", &IJobQueue::hasJobs )
                        .def( "addJob", (void ( IJobQueue::* )( SmartPtr<IJob> ))&IJobQueue::addJob )
                        .def( "addJobAllTasks", &IJobQueue::addJobAllTasks )
                        .def( "clearEventJobs", &IJobQueue::clearEventJobs )
                        .def( "isRunning", &IJobQueue::isRunning )
                        .def( "setRunning", &IJobQueue::setRunning )
                        .def( "getRate", &IJobQueue::getRate )
                        .def( "setRate", &IJobQueue::setRate )
                        .def( "getUseAffinity", &IJobQueue::getUseAffinity )
                        .def( "setUseAffinity", &IJobQueue::setUseAffinity )
                        .def( "shutdown", &IJobQueue::shutdown )];

        module( L )[def( "getTimerSingleton", EngineHelper::getTimerSingleton )];
        module( L )[def( "getSingleton", EngineHelper::getSingleton )];
        module(
            L )[class_<IAsyncOperation, ISharedObject, SmartPtr<IAsyncOperation>>( "IAsyncOperation" )];

        module( L )[class_<IWorkerThread, ISharedObject, SmartPtr<IWorkerThread>>( "IWorkerThread" )
                        .def( "run", &IWorkerThread::run )
                        .def( "getTargetFPS", &IWorkerThread::getTargetFPS )
                        .def( "setTargetFPS", &IWorkerThread::setTargetFPS )
                        .def( "stop", &IWorkerThread::stop )
                        .def( "isUpdating", &IWorkerThread::isUpdating )
                        .def( "setUpdating", &IWorkerThread::setUpdating )];

        module(
            L )[class_<IResourceGroupManager, ISharedObject, SmartPtr<IResourceGroupManager>>(
                    "IResourceGroupManager" )
                    .def( "initialiseAllResourceGroups",
                          &IResourceGroupManager::initialiseAllResourceGroups )
                    .def( "initialiseResourceGroup", &IResourceGroupManager::initialiseResourceGroup )
                    .def( "unloadResourceGroup", &IResourceGroupManager::unloadResourceGroup )
                    .def( "clearResourceGroup", &IResourceGroupManager::clearResourceGroup )
                    .def( "destroyResourceGroup", &IResourceGroupManager::destroyResourceGroup )
                    .def( "reloadResources", &IResourceGroupManager::reloadResources )
                    .def( "parseScripts", &IResourceGroupManager::parseScripts )
                    .def( "getStateContext", &IResourceGroupManager::getStateContext )
                    .def( "setStateContext", &IResourceGroupManager::setStateContext )
                    .scope[def( "typeInfo", IResourceGroupManager::typeInfo )]];

        module(
            L )[class_<IResourceManager, ISharedObject, SmartPtr<IResourceManager>>( "IResourceManager" )
                    .def( "create", static_cast<SmartPtr<IResource> ( IResourceManager::* )(
                                        const String & )>( &IResourceManager::create ) )
                    .def( "create", static_cast<SmartPtr<IResource> ( IResourceManager::* )(
                                        const String &, const String & )>( &IResourceManager::create ) )
                    .def( "destroyResource", &IResourceManager::destroyResource )
                    .def( "destroyAll", &IResourceManager::destroyAll )
                    .def( "saveToFile", &IResourceManager::saveToFile )
                    .def( "loadFromFile", &IResourceManager::loadFromFile )
                    .def( "loadResource", &IResourceManager::loadResource )
                    .def( "getByName", &IResourceManager::getByName )
                    .def( "getById", &IResourceManager::getById )
                    .def( "getStateContext", &IResourceManager::getStateContext )
                    .def( "setStateContext", &IResourceManager::setStateContext )
                    .def( "cloneResource", static_cast<SmartPtr<IResource> ( IResourceManager::* )(
                                               SmartPtr<IResource>, const String & )>(
                                               &IResourceManager::cloneResource ) )
                    .def( "cloneResource",
                          static_cast<SmartPtr<IResource> ( IResourceManager::* )(
                              const String &, const String & )>( &IResourceManager::cloneResource ) )
                    .scope[def( "typeInfo", IResourceManager::typeInfo )]];

        module( L )[class_<IState, ISharedObject, SmartPtr<IState>>( "IState" )
                        .def( "getTime", &IState::getTime )
                        .def( "setTime", &IState::setTime )
                        .def( "isDirty", &IState::isDirty )
                        .def( "setDirty", &IState::setDirty )
                        .def( "clone", &IState::clone )
                        .def( "assign", &IState::assign )
                        .def( "getOwner", &IState::getOwner )
                        .def( "setOwner", &IState::setOwner )
                        .def( "getStateContext", &IState::getStateContext )
                        .def( "setStateContext", &IState::setStateContext )
                        .def( "getData", &IState::getData )
                        .def( "setData", &IState::setData )
                        .def( "addSendCount", &IState::addSendCount )
                        .def( "removeSendCount", &IState::removeSendCount )
                        .def( "getSendCount", &IState::getSendCount )
                        .def( "setSendCount", &IState::setSendCount )];

        module( L )[class_<IStateListener, ISharedObject, SmartPtr<IStateListener>>( "IStateListener" )
                        .def( "handleStateMessage", &IStateListener::handleStateMessage )
                        .def( "handleStateChanged", &IStateListener::handleStateChanged )];

        module( L )[class_<IStateQueue, ISharedObject, SmartPtr<IStateQueue>>( "IStateQueue" )
                        .def( "queueMessage", &IStateQueue::queueMessage )
                        .def( "clear", &IStateQueue::clear )
                        .def( "getTaskId", &IStateQueue::getTaskId )
                        .def( "setTaskId", &IStateQueue::setTaskId )
                        .def( "isEmpty", &IStateQueue::isEmpty )
                        .def( "getMessages", &IStateQueue::getMessages )
                        .def( "getMessagesAndClear", &IStateQueue::getMessagesAndClear )];

        module( L )[class_<ITask, ISharedObject, SmartPtr<ITask>>( "ITask" )
                        .def( "reset", &ITask::reset )
                        .def( "addJob", &ITask::addJob )
                        .def( "clearEventJobs", &ITask::clearEventJobs )
                        .def( "setPrimary", &ITask::setPrimary )
                        .def( "isPrimary", &ITask::isPrimary )
                        .def( "setRecycle", &ITask::setRecycle )
                        .def( "getRecycle", &ITask::getRecycle )
                        .def( "setAffinity", &ITask::setAffinity )
                        .def( "getAffinity", &ITask::getAffinity )
                        .def( "isParallel", &ITask::isParallel )
                        .def( "setParallel", &ITask::setParallel )
                        .def( "isEnabled", &ITask::isEnabled )
                        .def( "setEnabled", &ITask::setEnabled )
                        .def( "getTask", _ITask_getTask )
                        .def( "setTask", _ITask_setTask )
                        .def( "getThreadTaskFlags", &ITask::getThreadTaskFlags )
                        .def( "setThreadTaskFlags", &ITask::setThreadTaskFlags )
                        .def( "stop", &ITask::stop )
                        .def( "start", &ITask::start )
                        .def( "isStopped", &ITask::isStopped )
                        .def( "isUpdating", &ITask::isUpdating )
                        .def( "setUpdating", &ITask::setUpdating )
                        .def( "isExecuting", &ITask::isExecuting )
                        .def( "getTargetFPS", &ITask::getTargetFPS )
                        .def( "setTargetFPS", &ITask::setTargetFPS )
                        .def( "getUseFixedTime", &ITask::getUseFixedTime )
                        .def( "setUseFixedTime", &ITask::setUseFixedTime )
                        .def( "getNextUpdateTime", &ITask::getNextUpdateTime )
                        .def( "setNextUpdateTime", &ITask::setNextUpdateTime )
                        .def( "getTicks", &ITask::getTicks )
                        .def( "getOwner", &ITask::getOwner )
                        .def( "setOwner", &ITask::setOwner )
                        .def( "getProfile", &ITask::getProfile )
                        .def( "setProfile", &ITask::setProfile )];

        module( L )[class_<ITaskLock, ISharedObject, SmartPtr<ITaskLock>>( "ITaskLock" )
                        .def( "getTask", &ITaskLock::getTask )
                        .def( "setTask", &ITaskLock::setTask )];

        module( L )[class_<ITaskManager, ISharedObject, SmartPtr<ITaskManager>>( "ITaskManager" )
                        .def( "addJobAllTasks", &ITaskManager::addJobAllTasks )
                        .def( "clearEventJobs", &ITaskManager::clearEventJobs )
                        .def( "getTask", &ITaskManager::getTask )
                        .def( "getTasks", &ITaskManager::getTasks )
                        .def( "getNumTasks", &ITaskManager::getNumTasks )
                        .def( "wait", &ITaskManager::wait )
                        .def( "stop", &ITaskManager::stop )
                        .def( "reset", &ITaskManager::reset )
                        .def( "shutdown", &ITaskManager::shutdown )];

        module( L )[class_<IEditorManager, ISharedObject, SmartPtr<IEditorManager>>( "IEditorManager" )
                        .def( "importAssets", &IEditorManager::importAssets )
                        .def( "getProjectPath", &IEditorManager::getProjectPath )
                        .def( "setProjectPath", &IEditorManager::setProjectPath )
                        .def( "getCachePath", &IEditorManager::getCachePath )
                        .def( "setCachePath", &IEditorManager::setCachePath )
                        .def( "getShowDebug", &IEditorManager::getShowDebug )
                        .def( "setShowDebug", &IEditorManager::setShowDebug )
                        .def( "isTransformLocal", &IEditorManager::isTransformLocal )
                        .def( "setTransformLocal", &IEditorManager::setTransformLocal )
                        .def( "getDrawSceneDebug", &IEditorManager::getDrawSceneDebug )
                        .def( "setDrawSceneDebug", &IEditorManager::setDrawSceneDebug )
                        .def( "getDrawUiDebug", &IEditorManager::getDrawUiDebug )
                        .def( "setDrawUiDebug", &IEditorManager::setDrawUiDebug )];

        module( L )[class_<IFrameGrabber, ISharedObject, SmartPtr<IFrameGrabber>>( "IFrameGrabber" )
                        .def( "addFrame", &IFrameGrabber::addFrame )
                        .def( "popFrame", &IFrameGrabber::popFrame )];

        module(
            L )[class_<IFrameStatistics, ISharedObject, SmartPtr<IFrameStatistics>>( "IFrameStatistics" )
                    .def( "setVisible", &IFrameStatistics::setVisible )
                    .def( "isVisible", &IFrameStatistics::isVisible )];

        module( L )[class_<ILogManager, ISharedObject, SmartPtr<ILogManager>>( "ILogManager" )
                        .def( "getEnableQueue", &ILogManager::getEnableQueue )
                        .def( "setEnableQueue", &ILogManager::setEnableQueue )
                        .def( "close", &ILogManager::close )
                        .def( "flush", &ILogManager::flush )];

        module( L )[class_<IConfigFile, ISharedObject, SmartPtr<IConfigFile>>( "IConfigFile" )
                        .def( "loadFromFilePath", &IConfigFile::loadFromFilePath )
                        .def( "loadFromStream", &IConfigFile::loadFromStream )
                        .def( "getSetting", &IConfigFile::getSetting )
                        .def( "getSettings", &IConfigFile::getSettings )
                        .scope[def( "typeInfo", IConfigFile::typeInfo )]];

        module( L )[class_<IConsole, SmartPtr<IConsole>>( "IConsole" )];

        module( L )[class_<ICoroutineData, ISharedObject, SmartPtr<ICoroutineData>>( "ICoroutineData" )
                        .def( "getLineNumber", &ICoroutineData::getLineNumber )
                        .def( "setLineNumber", &ICoroutineData::setLineNumber )
                        .def( "yield", &ICoroutineData::yield )
                        .def( "stop", &ICoroutineData::stop )];
    }
}  // namespace workphone
