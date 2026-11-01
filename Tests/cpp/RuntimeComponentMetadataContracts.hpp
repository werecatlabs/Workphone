#pragma once
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace runtime_component_metadata_contracts
{
    using namespace workphone;
    inline void require(bool value,const String &message){if(!value)throw std::runtime_error(message.c_str());}
    /** Compile the actual serializer output, remove the temporary authored
     * document, and return validated cooked component data for native reload. */
    inline SmartPtr<Properties> cook(SmartPtr<scene::IComponent> component,const String &type,
        std::shared_ptr<const resource::RuntimeAssetContext> context,const Array<String> &assets)
    {
        auto data=dynamic_pointer_cast<Properties>(component->toData());require(data!=nullptr,"Runtime component toData must serialize Properties");
        bool enabled=true;require(data->getPropertyValue(scene::IComponent::enabledStr,enabled)&&enabled==component->isEnabled(),"Serialized component metadata must use and preserve the native Enabled key");
        for(const auto &key:Array<String>{"terrainAssetUuid","foliageWorldAssetUuid","particleAssetUuid"})if(data->hasProperty(key))
            require(data->getPropertyObject(key).getTypeName()==Properties::resourceStr,"Durable domain UUID fields must declare resource metadata for where-used discovery");
        data->setName("component");data->setProperty("componentType",type);
        auto actor=make_ptr<Properties>();actor->setName("actors");actor->setProperty("uuid",StringUtil::getUUID());actor->setProperty("name","MetadataRoundTrip");actor->addChild(data);
        namespace fs=std::filesystem;const auto folder=fs::temp_directory_path()/fs::u8path(("wp-meta-"+StringUtil::getUUID()).c_str());
        require(!fs::exists(folder),"Metadata roundtrip requires a fresh owned directory");fs::create_directories(folder);
        struct Cleanup {fs::path folder;~Cleanup(){std::error_code ignored;fs::remove_all(folder,ignored);}} cleanup{folder};
        const auto path=folder/"probe.prefab";const auto json=DataUtil::toString(actor.get(),true);
        {std::ofstream file(path,std::ios::binary);file.write(json.data(),json.size());require(bool(file),"Cannot write serialized component roundtrip source");}
        std::map<String,resource::ResourceID> bindings;String error;
        for(const auto &identity:assets)if(!identity.empty()){auto asset=context->resolve(identity,error);require(asset!=nullptr,error);bindings.emplace(identity,asset->header.resourceId);}
        resource::CompileContext compile;compile.resourceId=resource::ResourceID("data://probe.prefab");compile.sourceRoot=folder.generic_u8string().c_str();compile.sourcePath=path.generic_u8string().c_str();compile.target=context->target;compile.packagedBuild=context->packagedBuild;
        compile.assetBindings=std::make_shared<const std::map<String,resource::ResourceID>>(std::move(bindings));
        resource::SceneResourceCompiler compiler;resource::DependencySet dependencies;require(compiler.getDependencies(compile,dependencies,error),error);
        std::ostringstream output(std::ios::binary);Array<String> messages;const auto status=compiler.compile(compile,output,messages);
        if(status!=resource::CompilationStatus::Success){String diagnostic="Actual component metadata failed to compile";for(const auto &message:messages)diagnostic+="; "+message;throw std::runtime_error(diagnostic.c_str());}
        fs::remove_all(folder);require(!fs::exists(path),"Cooked component reload must not retain its authored roundtrip document");
        const auto bytes=output.str();Array<u8> binary(bytes.begin(),bytes.end());auto decoded=make_ptr<Properties>();require(PropertiesBinarySerializer::deserialize(binary,*decoded,&error),error);
        require(resource::validateCookedActorGraphData(decoded,false,context,error),error);auto children=decoded->getChildren();require(children.size()==1&&children.front()->getProperty("componentType")==type,"Roundtrip must preserve its actual component type and metadata");
        return children.front();
    }
}
