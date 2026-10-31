#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/UIResource.hpp>
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
#include <stdexcept>

namespace workphone::ui
{
    namespace
    {
        constexpr size_t maximumBytes=65536, maximumNodes=256;
        enum Type : u8 { Window=1,Container=2,Text=3,Button=4 };
        struct Node { u8 type=0; u32 id=0; s32 parent=-1; String label; Vector2F position,size; f32 textSize=16; bool visible=true,enabled=true; };
        void require(bool valid,const char *message) { if(!valid) throw std::runtime_error(message); }
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
                const auto &node=nodes[i]; require(node.type>=Window&&node.type<=Button,"Unsupported UI type");
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
            }
        }
        void integer(std::ostream &stream,u32 value) { for(int i=0;i<4;++i)stream.put(static_cast<char>((value>>(8*i))&255)); }
        void scalar(std::ostream &stream,f32 value) { u32 bits;std::memcpy(&bits,&value,4);integer(stream,bits); }
        Array<Node> parse(const String &source)
        {
            require(source.find('\0')==String::npos,"UI source contains embedded NUL bytes");
            rapidjson::Document doc;doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(source.data(),source.size());
            require(!doc.HasParseError(),"Invalid UTF-8 UI JSON");fields(doc,{"version","nodes"});
            require(doc["version"].IsUint()&&doc["version"].GetUint()==1,"Unsupported UI source version");
            require(doc["nodes"].IsArray()&&!doc["nodes"].Empty()&&doc["nodes"].Size()<=maximumNodes,"UI tree requires 1-256 nodes");
            Array<Node> nodes;
            for(const auto &value:doc["nodes"].GetArray()) {
                fields(value,{"type","id","parent","label","position","size","visible","enabled"},{"textSize"}); Node node;
                require(value["type"].IsString(),"UI type requires a string");
                require(value["type"].GetStringLength()<=16&&std::memchr(value["type"].GetString(),0,value["type"].GetStringLength())==nullptr,"Invalid UI type string");
                String type(value["type"].GetString(),value["type"].GetStringLength());
                node.type=type=="window"?Window:type=="container"?Container:type=="text"?Text:type=="button"?Button:0;
                require(value["id"].IsUint()&&value["parent"].IsInt(),"UI IDs and parents require integers");node.id=value["id"].GetUint();node.parent=value["parent"].GetInt();
                require(value["label"].IsString(),"UI label requires a string");
                require(value["label"].GetStringLength()<=4096&&std::memchr(value["label"].GetString(),0,value["label"].GetStringLength())==nullptr,"Invalid UI label string");
                node.label=String(value["label"].GetString(),value["label"].GetStringLength());
                node.position=vector(value["position"]);node.size=vector(value["size"]);
                require(value["visible"].IsBool()&&value["enabled"].IsBool(),"UI visibility/enabled require booleans");node.visible=value["visible"].GetBool();node.enabled=value["enabled"].GetBool();
                if(value.HasMember("textSize")) {require(node.type==Text||node.type==Button,"Text size is supported only for text/buttons");node.textSize=number(value["textSize"]);}
                nodes.push_back(std::move(node));
            }
            validate(nodes);return nodes;
        }
        struct Reader {
            const Array<u8> &bytes;size_t at=0;
            u32 integer(){require(bytes.size()-at>=4,"Truncated UI payload");u32 value=0;for(int i=0;i<4;++i)value|=u32(bytes[at++])<<(8*i);return value;}
            f32 scalar(){u32 bits=integer();f32 value;std::memcpy(&value,&bits,4);return value;}
            u8 byte(){require(at<bytes.size(),"Truncated UI payload");return bytes[at++];}
        };
        Array<Node> decode(const resource::RuntimeResource &resource)
        {
            require(!resource.header.resourceId.isSubResource(),"UI subasset selection is unsupported");
            require(resource.header.installDependencies.empty()&&resource.header.installArtifacts.empty()&&resource.dependencies.empty(),
                "UI external dependencies are unsupported by this format version");
            UIResourceCompiler compiler; require(resource.header.isValid()&&resource.header.resourceType==resource::ResourceTypeID("uires")&&resource.header.resourceId.type()==resource::ResourceTypeID("uires")&&
                resource.header.compilerVersion==compiler.versionFor(resource::ResourceTypeID("uires")),"Unsupported UI resource identity or compiler version");
            const auto &bytes=resource.payload;require(bytes.size()>=12&&bytes.size()<=maximumBytes,"Invalid UI payload size");
            require(resource.header.payloadSize==bytes.size()&&resource::hashBytes(bytes.data(),bytes.size())==resource.header.payloadHash,
                "UI resource payload differs from its sealed header");
            require(std::memcmp(bytes.data(),"WPUI",4)==0,"Invalid UI payload magic");Reader reader{bytes,4};require(reader.integer()==1,"Unsupported UI payload version");
            const auto count=reader.integer();require(count&&count<=maximumNodes,"Invalid UI node count");Array<Node> nodes;
            for(u32 i=0;i<count;++i) {
                Node node;node.type=reader.byte();node.id=reader.integer();const auto parent=reader.integer();
                require(parent==0xffffffffu||parent<maximumNodes,"Invalid UI parent encoding");node.parent=parent==0xffffffffu?-1:static_cast<s32>(parent);
                const auto visible=reader.byte(),enabled=reader.byte();require(visible<=1&&enabled<=1,"Invalid UI flags");node.visible=visible!=0;node.enabled=enabled!=0;
                const auto size=reader.integer();require(size<=4096&&size<=bytes.size()-reader.at,"Invalid UI label length");
                require(std::memchr(bytes.data()+reader.at,0,size)==nullptr,"UI label contains embedded NUL");node.label=String(reinterpret_cast<const char*>(bytes.data()+reader.at),size);reader.at+=size;
                node.position={reader.scalar(),reader.scalar()};node.size={reader.scalar(),reader.scalar()};node.textSize=reader.scalar();nodes.push_back(std::move(node));
            }
            require(reader.at==bytes.size(),"Trailing UI payload content");validate(nodes);return nodes;
        }
        hash64 nativeType(u8 type)
        {switch(type){case Window:return IUIWindow::typeInfo();case Container:return IUILayoutContainer::typeInfo();case Text:return IUIText::typeInfo();case Button:return IUIButton::typeInfo();default:return 0;}}
    }
    String UIResourceCompiler::name() const{return "WorkphoneTypedUI";}
    Array<resource::CompilerOutput> UIResourceCompiler::outputs() const{return {{resource::ResourceTypeID("uires"),1}};}
    resource::CompilationStatus UIResourceCompiler::compile(const resource::CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try {
            require(!context.resourceId.isSubResource(),"UI subasset selection is unsupported");
            std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);const auto size=input?input.tellg():std::streampos(-1);
            require(size>0&&size<=maximumBytes,"UI source requires 1-65536 bytes");String error;auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*maximumBytes,error);require(budget!=nullptr,"UI parser working budget exhausted");
            String source(static_cast<size_t>(size),'\0');input.seekg(0);input.read(source.data(),source.size());require(bool(input),"Cannot read UI source");const auto nodes=parse(source);
            size_t encoded=12;for(const auto &node:nodes)encoded+=35+node.label.size();require(encoded<=maximumBytes,"Compiled UI payload exceeds 64 KiB");
            output.write("WPUI",4);integer(output,1);integer(output,static_cast<u32>(nodes.size()));
            for(const auto &node:nodes) {output.put(node.type);integer(output,node.id);integer(output,static_cast<u32>(node.parent));output.put(node.visible);output.put(node.enabled);integer(output,static_cast<u32>(node.label.size()));output.write(node.label.data(),node.label.size());scalar(output,node.position.X());scalar(output,node.position.Y());scalar(output,node.size.X());scalar(output,node.size.Y());scalar(output,node.textSize);}
            require(bool(output),"Cannot write UI payload");return resource::CompilationStatus::Success;
        } catch(const std::exception &error) {messages.push_back(error.what());return resource::CompilationStatus::Failure;}
    }
    UIResourceInstance::~UIResourceInstance()
    {for(size_t i=m_elements.size();i>0;--i)if(m_manager&&m_elements[i-1])m_manager->removeElement(m_elements[i-1]);}
    SmartPtr<IUIElement> UIResourceInstance::root() const{return m_elements.empty()?nullptr:m_elements[0];}
    bool UIResourceInstance::publish(SmartPtr<IUIElement> parent,String &error)
    {
        error.clear();if(std::this_thread::get_id()!=m_owner){error="UI publication requires its creating owner thread";return false;}
        if(!parent||m_published){error="UI publication requires a parent and an unpublished instance";return false;}
        try {
            require(m_manager&&m_manager->isLoaded(),"UI publication manager is unloaded");
            const auto snapshot=m_manager->getChildObjects();std::set<const IUIElement*> registered,own;
            for(const auto &object:snapshot)if(auto element=dynamic_cast<IUIElement*>(object.get()))registered.insert(element);
            require(registered.count(parent.get())!=0,"UI publication parent is not registered in the captured manager");
            for(const auto &element:m_elements) {
                require(registered.count(element.get())!=0,"UI staged control was removed from the captured manager");own.insert(element.get());
            }
            // Existing ImGui windows identify native state by their labels. Until
            // an explicit backend ID namespace exists, prevent two visible trees
            // from aliasing that global state instead of silently mixing them.
            for(const auto &object:snapshot)if(auto window=dynamic_cast<IUIWindow*>(object.get()))
                if(!own.count(window)&&window->isVisible())for(const auto &element:m_elements)
                    if(auto staged=dynamic_cast<IUIWindow*>(element.get()))
                        require(staged->getLabel()!=window->getLabel(),"UI window label is already visible in the captured manager");
            size_t depth=0;for(auto ancestor=parent;ancestor;ancestor=ancestor->getParent()) {
                require(ancestor!=root(),"UI publication cannot create an ownership cycle");require(++depth<=4096,"UI publication parent hierarchy is cyclic or too deep");
            }
            parent->addChild(root());for(size_t i=0;i<m_elements.size();++i)m_elements[i]->setVisible(m_visible[i]!=0,false);m_published=true;return true;
        }
        catch(const std::exception &failure){for(auto &element:m_elements)element->setVisible(false,false);if(root())root()->remove();error=failure.what();return false;}
    }
    std::shared_ptr<UIResourceInstance> stageUIResource(SmartPtr<IUIManager> manager,std::shared_ptr<const resource::RuntimeResource> resource,String &error)
    {
        error.clear();try {
            require(manager&&resource&&manager->isLoaded(),"UI staging requires a loaded manager and pinned runtime resource");
            auto budget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*maximumBytes,error);require(budget!=nullptr,"UI decode working budget exhausted");const auto nodes=decode(*resource);
            auto result=std::shared_ptr<UIResourceInstance>(new UIResourceInstance);result->m_manager=manager;result->m_resource=std::move(resource);result->m_owner=std::this_thread::get_id();
            // Participation estimate, retained through unregister/unload and member
            // destruction: 16 KiB per bounded control plus labels and fixed state.
            // Backend object pools/renderer storage keep their existing budgets.
            u64 metadataBytes=65536+16384*nodes.size();for(const auto &node:nodes)metadataBytes+=8*node.label.size();
            result->m_metadataBudget=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,metadataBytes,error);
            require(result->m_metadataBudget!=nullptr,"UI retained metadata budget exhausted");
            result->m_elements.reserve(nodes.size());result->m_visible.reserve(nodes.size());
            for(const auto &node:nodes) {
                auto element=manager->addElement(nativeType(node.type));require(element!=nullptr,"UI backend does not support a required cooked control type");
                result->m_elements.push_back(element);result->m_visible.push_back(node.visible);element->setVisible(false,false);
                auto properties=make_ptr<Properties>();properties->setProperty("position",node.position);properties->setProperty("size",node.size);properties->setProperty("visible",false);properties->setProperty("enabled",node.enabled);element->setProperties(properties);
                element->setElementId(node.id);element->setLabel(node.label);element->setPosition(node.position);element->setSize(node.size);element->setEnabled(node.enabled,false);element->setVisible(false,false);
                if(node.type==Text){auto text=workphone::dynamic_pointer_cast<IUIText>(element);require(text!=nullptr,"UI backend returned incompatible text control");text->setText(node.label);text->setTextSize(node.textSize);}
                if(node.type==Button){auto button=workphone::dynamic_pointer_cast<IUIButton>(element);require(button!=nullptr,"UI backend returned incompatible button control");button->setTextSize(node.textSize);}
                if(node.parent>=0)result->m_elements[node.parent]->addChild(element);
            }
            return result;
        }catch(const std::exception &failure){error=failure.what();return {};}
    }
}
