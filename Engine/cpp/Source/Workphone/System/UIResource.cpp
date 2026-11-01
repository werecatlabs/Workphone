#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/UIResource.hpp>
#include <Workphone/System/UIAssetResource.hpp>
#include <Workphone/System/ProjectLocalization.hpp>
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstring>
#include <set>
#include <algorithm>
#include <stdexcept>

namespace workphone::ui
{
    namespace
    {
        constexpr size_t maximumBytes=65536, maximumNodes=256;
        enum Type : u8 { Window=1,Container=2,Text=3,Button=4,Image=5 };
        struct Node { u8 type=0; u32 id=0; s32 parent=-1; String label,texture,font,style,key; Vector2F position,size; f32 textSize=16; bool visible=true,enabled=true;ColourF colour=ColourF::White; };
        struct Document {u32 version=1;String styles,localization,locale;Array<Node> nodes;};
        void require(bool valid,const char *message) { if(!valid) throw std::runtime_error(message); }
        StreamBudget reserveUIBudget(const UIResourceInstallOptions &options,StreamMemoryClass category,u64 bytes,String &error)
        {
            auto shared=WorldStreamingScheduler::shared();auto physical=shared->reserve(category,bytes,error);if(!physical)return {};
            if(!options.mount)return physical;
            auto probe=options.mount->reserveOwner(StreamMemoryClass::SourceCpu,0,error);if(!probe)return {};
            if(shared->ownsBudget(probe))return physical;
            struct Admissions {StreamBudget physical,captured;};
            auto owner=std::allocate_shared<Admissions>(SourceBudgetAllocator<Admissions>{});owner->physical=std::move(physical);
            owner->captured=options.mount->reserveOwner(category,bytes,error);if(!owner->captured)return {};
            return StreamBudget(owner,owner->captured.get());
        }
        bool utf8(const String &text)
        {
            for(size_t i=0;i<text.size();) {
                const auto c=static_cast<unsigned char>(text[i++]); if(c<0x80) { if(!c)return false; continue; }
                unsigned count=0,code=0,minimum=0;
                if(c>=0xc2&&c<=0xdf) {count=1;code=c&31;minimum=0x80;}
                else if(c>=0xe0&&c<=0xef) {count=2;code=c&15;minimum=0x800;}
                else if(c>=0xf0&&c<=0xf4) {count=3;code=c&7;minimum=0x10000;}
                else return false;
                if(count>text.size()-i)return false;
                while(count--) { const auto next=static_cast<unsigned char>(text[i++]); if((next&0xc0)!=0x80)return false; code=(code<<6)|(next&63); }
                if(code<minimum||code>0x10ffff||(code>=0xd800&&code<=0xdfff))return false;
            }
            return true;
        }
        void fields(const rapidjson::Value &value,const std::set<String> &required,const std::set<String> &optional={})
        {
            require(value.IsObject(),"UI record must be an object"); std::set<String> seen;
            for(auto i=value.MemberBegin();i!=value.MemberEnd();++i) {
                require(std::memchr(i->name.GetString(),0,i->name.GetStringLength())==nullptr,"UI field contains embedded NUL");
                String name(i->name.GetString(),i->name.GetStringLength());
                require((required.count(name)||optional.count(name))&&seen.insert(name).second,"Unknown or duplicate UI field");
            }
            for(const auto &name:required)require(seen.count(name)!=0,"Missing UI field");
        }
        f32 number(const rapidjson::Value &value)
        { require(value.IsNumber()&&std::isfinite(value.GetDouble())&&std::abs(value.GetDouble())<=1000000,"Invalid UI numeric property"); return static_cast<f32>(value.GetDouble()); }
        Vector2F vector(const rapidjson::Value &value)
        { require(value.IsArray()&&value.Size()==2,"UI position/size requires two numbers"); return {number(value[0]),number(value[1])}; }
        void validate(const Array<Node> &nodes)
        {
            require(!nodes.empty()&&nodes.size()<=maximumNodes,"UI tree requires 1-256 nodes"); std::set<u32> ids;
            std::set<std::pair<s32,String>> labels;std::set<String> windowLabels;Array<u32> depth;
            for(size_t i=0;i<nodes.size();++i) {
                const auto &node=nodes[i]; require(node.type>=Window&&node.type<=Image,"Unsupported UI type");
                require(node.id&&ids.insert(node.id).second,"Invalid or duplicate UI element ID");
                require(node.label.size()<=4096&&utf8(node.label),"Invalid UI UTF-8 label");
                if(node.type==Window||node.type==Button)
                    require(!node.label.empty()&&node.label.find("##")==String::npos&&labels.emplace(node.parent,node.label).second,
                        "UI interactive sibling labels must be nonempty, unique and omit backend ID escapes");
                if(node.type==Window)
                    require(windowLabels.insert(node.label).second,"UI window labels must be unique across the tree");
                require(i?node.parent>=0&&static_cast<size_t>(node.parent)<i:node.parent==-1,"UI parent must precede its child; root parent must be -1");
                if(i)require(nodes[node.parent].type==Window||nodes[node.parent].type==Container,"Only UI windows/containers may have children");
                else require(node.type==Window||node.type==Container,"UI root must be a window or container");
                const u32 d=i?depth[node.parent]+1:1; require(d<=32,"UI hierarchy exceeds 32 levels"); depth.push_back(d);
                for(auto value : {node.position.X(),node.position.Y(),node.size.X(),node.size.Y(),node.textSize})
                    require(std::isfinite(value)&&std::abs(value)<=1000000,"Invalid UI numeric value");
                require(node.size.X()>=0&&node.size.Y()>=0&&node.textSize>0&&node.textSize<=256,"Invalid UI size/text size");
                if(node.type==Window||node.type==Container)require(node.textSize==16,"UI container/window has unsupported text-size data");
                require(node.texture.empty()||node.type==Image,"Only UI images may bind a texture");
                if(node.type==Image)require(!node.texture.empty()&&node.textSize==16,"UI image requires a texture and cannot set text size");
                require(node.key.empty()||node.type==Text||node.type==Button,"Only UI text/buttons may bind a locale key");
                require(node.font.empty()||node.type==Text||node.type==Button,"Only UI text/buttons may bind a font");
                require(node.style.size()<=128&&node.key.size()<=256&&utf8(node.style)&&utf8(node.key),"Invalid UI style/localization key");
            }
        }
        void integer(std::ostream &stream,u32 value) { for(int i=0;i<4;++i)stream.put(static_cast<char>((value>>(8*i))&255)); }
        void scalar(std::ostream &stream,f32 value) { u32 bits;std::memcpy(&bits,&value,4);integer(stream,bits); }
        void string(std::ostream &stream,const String &value){integer(stream,u32(value.size()));stream.write(value.data(),value.size());}
        Document parse(const String &source)
        {
            require(source.find('\0')==String::npos,"UI source contains embedded NUL bytes");
            rapidjson::Document doc;doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(source.data(),source.size());
            require(!doc.HasParseError(),"Invalid UTF-8 UI JSON");fields(doc,{"version","nodes"},{"styleAssetUuid","localizationAssetUuid","locale"});
            require(doc["version"].IsUint()&&(doc["version"].GetUint()==1||doc["version"].GetUint()==2),"Unsupported UI source version");
            Document result;result.version=doc["version"].GetUint();
            auto optional=[&](const rapidjson::Value &value,const char *name,size_t limit){if(!value.HasMember(name))return String();
                const auto &field=value[name];require(field.IsString()&&field.GetStringLength()<=limit&&!std::memchr(field.GetString(),0,field.GetStringLength()),"Invalid UI asset field");return String(field.GetString(),field.GetStringLength());};
            result.styles=optional(doc,"styleAssetUuid",36);result.localization=optional(doc,"localizationAssetUuid",36);result.locale=optional(doc,"locale",35);
            require(result.version==2||(result.styles.empty()&&result.localization.empty()&&result.locale.empty()),"UI asset dependencies require version two");
            require(result.locale.empty()||result.locale.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==String::npos,"Invalid UI locale tag");
            require(doc["nodes"].IsArray()&&!doc["nodes"].Empty()&&doc["nodes"].Size()<=maximumNodes,"UI tree requires 1-256 nodes");
            auto &nodes=result.nodes;
            for(const auto &value:doc["nodes"].GetArray()) {
                fields(value,{"type","id","parent","label","position","size","visible","enabled"},{"textSize","textureAssetUuid","fontAssetUuid","styleClass","localeKey"}); Node node;
                require(value["type"].IsString(),"UI type requires a string");
                require(value["type"].GetStringLength()<=16&&std::memchr(value["type"].GetString(),0,value["type"].GetStringLength())==nullptr,"Invalid UI type string");
                String type(value["type"].GetString(),value["type"].GetStringLength());
                node.type=type=="window"?Window:type=="container"?Container:type=="text"?Text:type=="button"?Button:type=="image"?Image:0;
                node.texture=optional(value,"textureAssetUuid",36);node.font=optional(value,"fontAssetUuid",36);node.style=optional(value,"styleClass",128);node.key=optional(value,"localeKey",256);
                require(result.version==2||(node.type!=Image&&node.texture.empty()&&node.font.empty()&&node.style.empty()&&node.key.empty()),"UI extended controls require version two");
                require(node.style.empty()||!result.styles.empty(),"UI style class requires styleAssetUuid");require(node.key.empty()||!result.localization.empty(),"UI translation requires localizationAssetUuid");
                require(value["id"].IsUint()&&value["parent"].IsInt(),"UI IDs and parents require integers");node.id=value["id"].GetUint();node.parent=value["parent"].GetInt();
                require(value["label"].IsString(),"UI label requires a string");
                require(value["label"].GetStringLength()<=4096&&std::memchr(value["label"].GetString(),0,value["label"].GetStringLength())==nullptr,"Invalid UI label string");
                node.label=String(value["label"].GetString(),value["label"].GetStringLength());
                node.position=vector(value["position"]);node.size=vector(value["size"]);
                require(value["visible"].IsBool()&&value["enabled"].IsBool(),"UI visibility/enabled require booleans");node.visible=value["visible"].GetBool();node.enabled=value["enabled"].GetBool();
                if(value.HasMember("textSize")) {require(node.type==Text||node.type==Button,"Text size is supported only for text/buttons");node.textSize=number(value["textSize"]);}
                nodes.push_back(std::move(node));
            }
            validate(nodes);return result;
        }
        String canonicalReferenceUUID(const String &identity)
        {
            require(identity.size()==36,"UI asset reference requires a UUID");
            const auto key=StringUtil::make_lower(identity);bool nonzero=false;for(size_t i=0;i<key.size();++i){const auto c=key[i];if(i==8||i==13||i==18||i==23)require(c=='-',"Invalid UI UUID separator");else{require((c>='0'&&c<='9')||(c>='a'&&c<='f'),"Invalid UI UUID");nonzero|=c!='0';}}require(nonzero,"Nil UI UUID");
            return key;
        }
        resource::ResourceID resolveAssetBinding(const resource::CompileContext &context,const String &identity,const char *type)
        {
            require(bool(context.assetBindings),"UI asset reference requires a captured UUID map");const auto key=canonicalReferenceUUID(identity);
            const auto found=context.assetBindings->find(key);require(found!=context.assetBindings->end()&&found->second.isValid()&&!found->second.isSubResource()&&found->second.type()==resource::ResourceTypeID(type),"UI asset UUID is missing or has the wrong type");return found->second;
        }
        void bindDocument(Document &data,const resource::CompileContext &context)
        {if(!data.styles.empty())data.styles=resolveAssetBinding(context,data.styles,"uistyle").str();if(!data.localization.empty())data.localization=resolveAssetBinding(context,data.localization,"locres").str();for(auto &node:data.nodes){if(!node.texture.empty())node.texture=resolveAssetBinding(context,node.texture,"texres").str();if(!node.font.empty())node.font=resolveAssetBinding(context,node.font,"fontres").str();}}
        struct Reader {
            const Array<u8> &bytes;size_t at=0;
            u32 integer(){require(bytes.size()-at>=4,"Truncated UI payload");u32 value=0;for(int i=0;i<4;++i)value|=u32(bytes[at++])<<(8*i);return value;}
            f32 scalar(){u32 bits=integer();f32 value;std::memcpy(&value,&bits,4);return value;}
            u8 byte(){require(at<bytes.size(),"Truncated UI payload");return bytes[at++];}
            String text(size_t limit){const auto size=integer();require(size<=limit&&at<=bytes.size()&&size<=bytes.size()-at&&!std::memchr(bytes.data()+at,0,size),"Invalid UI string span");String value(reinterpret_cast<const char*>(bytes.data()+at),size);at+=size;require(utf8(value),"Invalid UI UTF-8 text");return value;}
        };
        Document decode(const resource::RuntimeResource &resource)
        {
            require(!resource.header.resourceId.isSubResource(),"UI subasset selection is unsupported");
            const auto legacyVersion=resource::combineHash(resource::combineHash(resource::combineHash(14695981039346656037ull,1),1),resource::ResourceTypeID("uires").value());
            UIResourceCompiler compiler; require(resource.header.isValid()&&resource.header.resourceType==resource::ResourceTypeID("uires")&&resource.header.resourceId.type()==resource::ResourceTypeID("uires")&&
                (resource.header.compilerVersion==compiler.versionFor(resource::ResourceTypeID("uires"))||resource.header.compilerVersion==legacyVersion),"Unsupported UI resource identity or compiler version");
            const auto &bytes=resource.payload;require(bytes.size()>=12&&bytes.size()<=maximumBytes,"Invalid UI payload size");
            require(resource.header.payloadSize==bytes.size()&&resource::hashBytes(bytes.data(),bytes.size())==resource.header.payloadHash,
                "UI resource payload differs from its sealed header");
            require(std::memcmp(bytes.data(),"WPUI",4)==0,"Invalid UI payload magic");Reader reader{bytes,4};Document result;result.version=reader.integer();require(result.version==1||result.version==2,"Unsupported UI payload version");
            require(result.version==1||resource.header.compilerVersion!=legacyVersion,"Legacy UI ABI cannot contain extended controls");
            if(result.version==2){result.styles=reader.text(1024);result.localization=reader.text(1024);result.locale=reader.text(35);}
            require(result.locale.empty()||result.locale.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==String::npos,"Invalid captured locale tag");
            const auto count=reader.integer();require(count&&count<=maximumNodes,"Invalid UI node count");auto &nodes=result.nodes;
            for(u32 i=0;i<count;++i) {
                Node node;node.type=reader.byte();node.id=reader.integer();const auto parent=reader.integer();
                require(parent==0xffffffffu||parent<maximumNodes,"Invalid UI parent encoding");node.parent=parent==0xffffffffu?-1:static_cast<s32>(parent);
                const auto visible=reader.byte(),enabled=reader.byte();require(visible<=1&&enabled<=1,"Invalid UI flags");node.visible=visible!=0;node.enabled=enabled!=0;
                const auto size=reader.integer();require(size<=4096&&size<=bytes.size()-reader.at,"Invalid UI label length");
                require(std::memchr(bytes.data()+reader.at,0,size)==nullptr,"UI label contains embedded NUL");node.label=String(reinterpret_cast<const char*>(bytes.data()+reader.at),size);reader.at+=size;
                node.position={reader.scalar(),reader.scalar()};node.size={reader.scalar(),reader.scalar()};node.textSize=reader.scalar();
                if(result.version==2){node.texture=reader.text(1024);node.font=reader.text(1024);node.style=reader.text(128);node.key=reader.text(256);}else require(node.type!=Image,"Legacy UI cannot contain images");nodes.push_back(std::move(node));
            }
            require(reader.at==bytes.size(),"Trailing UI payload content");validate(nodes);
            std::set<resource::ResourceID> expected;auto dependency=[&](const String &id,const char *type){if(id.empty())return;resource::ResourceID selected(id);require(selected.isValid()&&!selected.isSubResource()&&selected.type()==resource::ResourceTypeID(type),"Invalid typed UI dependency");expected.insert(selected);};
            dependency(result.styles,"uistyle");dependency(result.localization,"locres");for(const auto &node:nodes){dependency(node.texture,"texres");dependency(node.font,"fontres");require(node.style.empty()||!result.styles.empty(),"UI class has no styles");require(node.key.empty()||!result.localization.empty(),"UI key has no dictionary");}
            require(expected.size()==resource.header.installDependencies.size()&&resource.dependencies.size()==expected.size(),"UI dependency set differs from sealed fields");
            std::set<resource::ResourceID> seen;for(size_t i=0;i<resource.dependencies.size();++i){const auto &child=resource.dependencies[i];const auto &id=resource.header.installDependencies[i];require(expected.count(id)&&seen.insert(id).second&&child&&child->header.isValid()&&child->header.resourceId==id&&child->header.resourceType==id.type(),"UI dependency identity mismatch");
                if(resource.header.formatVersion>=2){const auto &pin=resource.header.installArtifacts[i];require(pin.sourceHash==child->header.sourceHash&&pin.payloadHash==child->header.payloadHash,"UI dependency generation mismatch");}}
            return result;
        }
        hash64 nativeType(u8 type)
        {switch(type){case Window:return IUIWindow::typeInfo();case Container:return IUILayoutContainer::typeInfo();case Text:return IUIText::typeInfo();case Button:return IUIButton::typeInfo();case Image:return IUIImage::typeInfo();default:return 0;}}
    }
    String UIResourceCompiler::name() const{return "WorkphoneTypedUI";}
    Array<resource::CompilerOutput> UIResourceCompiler::outputs() const{return {{resource::ResourceTypeID("uires"),2}};}
    SmartPtr<Properties> UIResourceCompiler::sourceReferenceProperties(const Array<u8> &bytes,String &error)
    {
        error.clear();try{
            require(!bytes.empty()&&bytes.size()<=maximumBytes,"Invalid authored UI descriptor size");
            auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*maximumBytes,error);require(bool(budget),"UI reference parsing budget exhausted");
            const auto data=parse(String(reinterpret_cast<const char*>(bytes.data()),bytes.size()));auto result=make_ptr<Properties>();
            auto field=[](SmartPtr<Properties> properties,const char *name,const String &uuid){if(uuid.empty())return;properties->setProperty(name,canonicalReferenceUUID(uuid),Properties::resourceStr);properties->getPropertyObject(name).setAttribute("resourceType","workphone::IResource");};
            field(result,"styleAssetUuid",data.styles);field(result,"localizationAssetUuid",data.localization);
            for(const auto &node:data.nodes){auto properties=make_ptr<Properties>();field(properties,"textureAssetUuid",node.texture);field(properties,"fontAssetUuid",node.font);result->addChild(properties);}
            return result;
        }catch(const std::bad_alloc &){throw;}catch(const std::exception &failure){error=failure.what();return {};}
    }
    bool UIResourceCompiler::getDependencies(const resource::CompileContext &context,resource::DependencySet &dependencies,String &error) const
    {
        error.clear();try{std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);const auto size=input?input.tellg():std::streampos(-1);require(size>0&&size<=maximumBytes,"Invalid UI source size");
            auto working=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*maximumBytes,error);require(bool(working),"UI dependency scan budget denied");String source(size_t(size),'\0');input.seekg(0);input.read(source.data(),source.size());require(bool(input),"Cannot read UI source");auto data=parse(source);bindDocument(data,context);std::set<resource::ResourceID> ids;
            if(!data.styles.empty())ids.emplace(data.styles);if(!data.localization.empty())ids.emplace(data.localization);for(const auto &node:data.nodes){if(!node.texture.empty())ids.emplace(node.texture);if(!node.font.empty())ids.emplace(node.font);}
            for(const auto &id:ids){dependencies.compileDependencies.push_back(resource::CompileDependency::resource(id));dependencies.installDependencies.push_back(id);}return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
    resource::CompilationStatus UIResourceCompiler::compile(const resource::CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try {
            require(!context.resourceId.isSubResource(),"UI subasset selection is unsupported");
            std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);const auto size=input?input.tellg():std::streampos(-1);
            require(size>0&&size<=maximumBytes,"UI source requires 1-65536 bytes");String error;auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*maximumBytes,error);require(budget!=nullptr,"UI parser working budget exhausted");
            String source(static_cast<size_t>(size),'\0');input.seekg(0);input.read(source.data(),source.size());require(bool(input),"Cannot read UI source");auto data=parse(source);bindDocument(data,context);const auto &nodes=data.nodes;
            size_t encoded=12;if(data.version==2)encoded+=12+data.styles.size()+data.localization.size()+data.locale.size();for(const auto &node:nodes){encoded+=35+node.label.size();if(data.version==2)encoded+=16+node.texture.size()+node.font.size()+node.style.size()+node.key.size();}require(encoded<=maximumBytes,"Compiled UI payload exceeds 64 KiB");
            output.write("WPUI",4);integer(output,data.version);if(data.version==2){string(output,data.styles);string(output,data.localization);string(output,data.locale);}integer(output,static_cast<u32>(nodes.size()));
            for(const auto &node:nodes) {output.put(node.type);integer(output,node.id);integer(output,static_cast<u32>(node.parent));output.put(node.visible);output.put(node.enabled);integer(output,static_cast<u32>(node.label.size()));output.write(node.label.data(),node.label.size());scalar(output,node.position.X());scalar(output,node.position.Y());scalar(output,node.size.X());scalar(output,node.size.Y());scalar(output,node.textSize);if(data.version==2){string(output,node.texture);string(output,node.font);string(output,node.style);string(output,node.key);}}
            require(bool(output),"Cannot write UI payload");return resource::CompilationStatus::Success;
        } catch(const std::exception &error) {messages.push_back(error.what());return resource::CompilationStatus::Failure;}
    }
    UIResourceInstance::~UIResourceInstance()
    {
        auto app=core::IApplicationManager::instancePtr();SmartPtr<ITask> task;if(app)if(auto tasks=app->getTaskManager())if(auto render=tasks->getTask(TaskId::Render);render&&render->isLoaded())task=render;
        TaskLock pause(task);auto graphics=app?app->getGraphicsSystem():nullptr;ScopedLock lock(graphics);
        for(size_t i=m_elements.size();i>0;--i)if(m_manager&&m_elements[i-1])m_manager->removeElement(m_elements[i-1]);
        m_fonts.clear();
        m_textures.clear();
    }
    SmartPtr<IUIElement> UIResourceInstance::root() const{return m_elements.empty()?nullptr:m_elements[0];}
    bool UIResourceInstance::setActive(bool visible,bool enabled,String &error)
    {
        error.clear();if(std::this_thread::get_id()!=m_owner||!m_published||!m_manager||!m_manager->isLoaded()){
            error="UI activation requires its published generation and loaded owner manager";return false;}
        for(size_t i=0;i<m_elements.size();++i){m_elements[i]->setVisible(visible&&m_visible[i]!=0,false);m_elements[i]->setEnabled(enabled&&m_enabled[i]!=0,false);}m_activeVisible=visible;m_activeEnabled=enabled;return true;
    }
    bool UIResourceInstance::publish(SmartPtr<IUIElement> parent,String &error,const UIResourceInstance *replacing)
    {
        error.clear();if(std::this_thread::get_id()!=m_owner){error="UI publication requires its creating owner thread";return false;}
        if(!parent||m_published){error="UI publication requires a parent and an unpublished instance";return false;}
        try {
            require(m_manager&&m_manager->isLoaded(),"UI publication manager is unloaded");
            const auto snapshot=m_manager->getChildObjects();
            using ElementSet=std::set<const IUIElement*,std::less<const IUIElement*>,SourceBudgetAllocator<const IUIElement*>>;
            ElementSet registered,own;
            for(const auto &object:snapshot)if(auto element=dynamic_cast<IUIElement*>(object.get()))registered.insert(element);
            require(registered.count(parent.get())!=0,"UI publication parent is not registered in the captured manager");
            for(const auto &element:m_elements) {
                require(registered.count(element.get())!=0,"UI staged control was removed from the captured manager");own.insert(element.get());
            }
            ElementSet retiring;
            if(replacing){
                require(replacing!=this&&replacing->m_manager==m_manager&&replacing->m_owner==m_owner&&
                    replacing->m_published&&replacing->root()&&replacing->root()->getParent()==parent,
                    "UI replacement requires the same live manager, owner and publication parent");
                for(const auto &element:replacing->m_elements){require(registered.count(element.get())!=0,"UI replacement generation was already retired");retiring.insert(element.get());}
            }
            // Existing ImGui windows identify native state by their labels. Until
            // an explicit backend ID namespace exists, prevent two visible trees
            // from aliasing that global state instead of silently mixing them.
            for(const auto &object:snapshot)if(auto window=dynamic_cast<IUIWindow*>(object.get()))
                if(!own.count(window)&&!retiring.count(window)&&window->isVisible())for(const auto &element:m_elements)
                    if(auto staged=dynamic_cast<IUIWindow*>(element.get()))
                        require(staged->getLabel()!=window->getLabel(),"UI window label is already visible in the captured manager");
            size_t depth=0;for(auto ancestor=parent;ancestor;ancestor=ancestor->getParent()) {
                require(ancestor!=root(),"UI publication cannot create an ownership cycle");require(++depth<=4096,"UI publication parent hierarchy is cyclic or too deep");
            }
            parent->addChild(root());for(size_t i=0;i<m_elements.size();++i)m_elements[i]->setVisible(m_visible[i]!=0,false);m_published=true;return true;
        }
        catch(const std::exception &failure){for(auto &element:m_elements)element->setVisible(false,false);if(root())root()->remove();error=failure.what();return false;}
    }
    bool UIResourceInstance::setLocale(const String &locale,String &error)
    {
        error.clear();try{
            require(std::this_thread::get_id()==m_owner&&m_published&&root()&&root()->getParent(),"UI language replacement requires its published owner tree");
            require(!locale.empty()&&locale.size()<=35&&locale.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==String::npos,"Invalid UI locale tag");
            if(locale==m_locale)return true;
            auto app=core::IApplicationManager::instancePtr();SmartPtr<ITask> task;if(app)if(auto tasks=app->getTaskManager())if(auto render=tasks->getTask(TaskId::Render);render&&render->isLoaded())task=render;
            TaskLock pause(task);auto graphics=app?app->getGraphicsSystem():nullptr;ScopedLock lock(graphics);
            auto options=m_options;options.locale=locale;
            auto candidate=stageUIResource(m_manager,m_resource,error,options);if(!candidate)return false;
            if(!candidate->publish(root()->getParent(),error,this))return false;
            if(!candidate->setActive(m_activeVisible,m_activeEnabled,error))return false;
            m_metadataBudget.swap(candidate->m_metadataBudget);m_fonts.swap(candidate->m_fonts);m_textures.swap(candidate->m_textures);m_elements.swap(candidate->m_elements);m_visible.swap(candidate->m_visible);m_enabled.swap(candidate->m_enabled);m_locale.swap(candidate->m_locale);
            m_options.mount.swap(candidate->m_options.mount);m_options.target.swap(candidate->m_options.target);m_options.locale.swap(candidate->m_options.locale);std::swap(m_options.packagedBuild,candidate->m_options.packagedBuild);
            return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
    std::shared_ptr<UIResourceInstance> stageUIResource(SmartPtr<IUIManager> manager,std::shared_ptr<const resource::RuntimeResource> resource,String &error,
        const UIResourceInstallOptions &options)
    {
        error.clear();try {
            require(manager&&resource&&manager->isLoaded(),"UI staging requires a loaded manager and pinned runtime resource");
            require(!options.target.empty()&&options.target.size()<=128&&options.locale.size()<=35&&options.locale.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==String::npos,"Invalid UI installation target/locale");
            require(!options.mount||options.mount->runtimeLimits().target==options.target,"UI install target differs from its captured mount");
            auto app=core::IApplicationManager::instancePtr();SmartPtr<ITask> task;if(app)if(auto tasks=app->getTaskManager())if(auto render=tasks->getTask(TaskId::Render);render&&render->isLoaded())task=render;
            TaskLock pause(task);auto graphics=app?app->getGraphicsSystem():nullptr;ScopedLock graphicsLock(graphics);
            u64 workingBytes=16*maximumBytes;for(const auto &child:resource->dependencies)if(child&&(child->header.resourceType==resource::ResourceTypeID("uistyle")||child->header.resourceType==resource::ResourceTypeID("locres"))){require(child->payload.size()<=(1u<<20),"UI data dependency exceeds its descriptor bound");workingBytes+=32*child->payload.size();}
            auto budget=reserveUIBudget(options,StreamMemoryClass::WorkingCpu,workingBytes,error);require(budget!=nullptr,"UI decode working budget exhausted");auto document=decode(*resource);auto &nodes=document.nodes;
            std::map<resource::ResourceID,std::shared_ptr<const resource::RuntimeResource>> dependencies;for(const auto &child:resource->dependencies)dependencies.emplace(child->header.resourceId,child);
            if(!document.styles.empty()){Array<UIStyleRecord> styles;const bool decoded=decodeUIStyles(*dependencies.at(resource::ResourceID(document.styles)),styles,error);if(!decoded)throw std::runtime_error(error.c_str());
                for(auto &node:nodes)if(!node.style.empty()){const auto found=std::find_if(styles.begin(),styles.end(),[&](const UIStyleRecord &style){return style.name==node.style;});require(found!=styles.end(),"UI style class is absent from its captured style sheet");node.colour=found->colour;if(node.type==Text||node.type==Button)node.textSize=found->textSize;}}
            const auto language=options.locale.empty()?(document.locale.empty()?getProjectLanguageTag():document.locale):options.locale;
            if(!document.localization.empty()){UILocalizationData data;if(!decodeUILocalization(*dependencies.at(resource::ResourceID(document.localization)),data,error))throw std::runtime_error(error.c_str());
                for(auto &node:nodes)if(!node.key.empty()){String label;if(!resolveUILocalization(data,language,node.key,label,error))throw std::runtime_error(error.c_str());require(node.type!=Button||(!label.empty()&&label.find("##")==String::npos),"Translated UI button cannot contain backend ID escapes");node.label=std::move(label);}}
            validate(nodes);
            auto result=std::shared_ptr<UIResourceInstance>(new UIResourceInstance);result->m_manager=manager;result->m_resource=std::move(resource);result->m_options=options;result->m_locale=language;result->m_owner=std::this_thread::get_id();
            // Participation estimate, retained through unregister/unload and member
            // destruction: 16 KiB per bounded control plus labels and fixed state.
            // Backend object pools/renderer storage keep their existing budgets.
            u64 metadataBytes=65536+16384*nodes.size();for(const auto &node:nodes)metadataBytes+=8*node.label.size();
            result->m_metadataBudget=reserveUIBudget(options,StreamMemoryClass::SourceCpu,metadataBytes,error);
            require(result->m_metadataBudget!=nullptr,"UI retained metadata budget exhausted");
            std::map<resource::ResourceID,SmartPtr<render::ITexture>> textures;
            std::map<resource::ResourceID,SmartPtr<render::IFont>> fonts;
            for(const auto &node:nodes)if(!node.texture.empty()){const resource::ResourceID id(node.texture);if(textures.count(id))continue;require(graphics!=nullptr,"UI image requires an installed graphics backend");
                auto texture=graphics->createRuntimeTexture(dependencies.at(id),error,options.target,options.packagedBuild,options.mount);
                if(!texture)throw std::runtime_error(error.c_str());require(texture->isLoaded(),"UI image texture did not install");textures.emplace(id,texture);result->m_textures.push_back(std::move(texture));}
            for(const auto &node:nodes)if(!node.font.empty()){const resource::ResourceID id(node.font);if(fonts.count(id))continue;require(graphics!=nullptr,"UI font requires an installed graphics backend");
                auto font=graphics->createRuntimeFont(dependencies.at(id),error,options.target,options.packagedBuild,options.mount);if(!font)throw std::runtime_error(error.c_str());require(font->isLoaded(),"UI font did not install");fonts.emplace(id,font);result->m_fonts.push_back(std::move(font));}
            result->m_elements.reserve(nodes.size());result->m_visible.reserve(nodes.size());result->m_enabled.reserve(nodes.size());
            for(const auto &node:nodes) {
                auto element=manager->addElement(nativeType(node.type));require(element!=nullptr,"UI backend does not support a required cooked control type");
                result->m_elements.push_back(element);result->m_visible.push_back(node.visible);result->m_enabled.push_back(node.enabled);element->setVisible(false,false);
                auto properties=make_ptr<Properties>();properties->setProperty("position",node.position);properties->setProperty("size",node.size);properties->setProperty("visible",false);properties->setProperty("enabled",node.enabled);element->setProperties(properties);
                element->setElementId(node.id);element->setLabel(node.label);element->setPosition(node.position);element->setSize(node.size);element->setEnabled(node.enabled,false);element->setVisible(false,false);
                if(!node.style.empty())element->setColour(node.colour);
                if(node.type==Text){auto text=workphone::dynamic_pointer_cast<IUIText>(element);require(text!=nullptr,"UI backend returned incompatible text control");if(document.version==2)require(text->setRuntimeLiteralText(true),"UI backend cannot isolate captured text from authoring localization");text->setText(node.label);text->setTextSize(node.textSize);}
                if(node.type==Button){auto button=workphone::dynamic_pointer_cast<IUIButton>(element);require(button!=nullptr,"UI backend returned incompatible button control");if(document.version==2)require(button->setRuntimeLiteralText(true),"UI backend cannot isolate captured button text");button->setTextSize(node.textSize);}
                if(node.type==Image){auto image=workphone::dynamic_pointer_cast<IUIImage>(element);require(image!=nullptr,"UI backend returned incompatible image control");image->setTexture(textures.at(resource::ResourceID(node.texture)));}
                if(!node.font.empty()){const auto font=fonts.at(resource::ResourceID(node.font));const bool installed=node.type==Text?dynamic_pointer_cast<IUIText>(element)->setRuntimeFont(font):dynamic_pointer_cast<IUIButton>(element)->setRuntimeFont(font);require(installed,"UI backend cannot install the captured font");}
                if(node.parent>=0)result->m_elements[node.parent]->addChild(element);
            }
            return result;
        }catch(const std::exception &failure){error=failure.what();return {};}
    }
}
