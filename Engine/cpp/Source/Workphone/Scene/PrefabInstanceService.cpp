#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/PrefabInstanceService.hpp>
#include <Workphone/Scene/Components/PrefabInstance.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

namespace workphone::scene
{
    namespace
    {
        using Data = SmartPtr<Properties>;
        using IdMap = std::unordered_map<String, String>;
        using Objects = std::unordered_map<String, SmartPtr<ISharedObject>>;
        const String linkType = "workphone::scene::PrefabInstance";
        const String stageTag = "__WorkphonePrefabEditing";
        std::atomic_bool transactionActive{ false };

        String id( const SmartPtr<ISharedObject> &object )
        { return object->getHandle()->getUUIDAsString(); }
        String value( Data data, const String &name )
        { return data->hasProperty( name ) ? data->getPropertyObject( name ).getValue() : String(); }
        bool isLink( Data data )
        { auto type=value(data,"componentType");return type==linkType||type=="PrefabInstance"; }
        bool actorNode( Data data )
        { return data->getNumProperties()>0||data->hasChild("localTransform")||data->hasChild("component")||data->hasChild("components"); }
        void gather( Data data, bool component, Array<Data> &result )
        {
            if( component ? data->hasProperty("componentType") : actorNode(data) ) { result.push_back(data);return; }
            for( auto child:data->getChildren() )
                if( child->getName()==(component?"component":"child")||child->getName()==(component?"components":"children") ) gather(child,component,result);
        }
        Array<Data> nodes( Data data, bool component )
        {
            Array<Data> result;
            for( auto child:data->getChildren() )
                if( child->getName()==(component?"component":"child")||child->getName()==(component?"components":"children") ) gather(child,component,result);
            return result;
        }
        Data copy( Data source, const IdMap &mapping = {}, bool *changed = nullptr )
        {
            auto result=make_ptr<Properties>();result->setName(source->getName());
            for( auto property:source->getPropertiesAsArray() )
            {
                if(source->getName()=="identity"&&property.getName()=="sourceUuid")
                {result->setProperty(property);continue;}
                auto text=property.getValue();auto found=mapping.find(text);
                if(found!=mapping.end()) { property.setValue(found->second);if(changed)*changed=true; }
                else if(text.find(';')!=String::npos)
                {
                    Array<String> items;StringUtil::parseArray(text,items);bool edit=false;
                    for(auto &item:items)if(auto it=mapping.find(item);it!=mapping.end()){item=it->second;edit=true;}
                    if(edit){items.erase(std::remove(items.begin(),items.end(),String()),items.end());property.setValue(StringUtil::toString(items));if(changed)*changed=true;}
                }
                result->setProperty(property);
            }
            for(auto child:source->getChildren())result->addChild(copy(child,mapping,changed));
            return result;
        }
        String normalizedPath(const String &source)
        {
            std::string text(source.c_str());std::replace(text.begin(),text.end(),'\\','/');
            namespace fs=std::filesystem;auto path=fs::u8path(text);
            auto app=core::IApplicationManager::instancePtr();
            const auto root=app?app->getProjectPath():String();
            if(!root.empty())
            {
                auto base=fs::absolute(fs::u8path(root.c_str())).lexically_normal();
                if(!path.is_absolute())path=base/path;
                path=path.lexically_normal();auto full=path.generic_u8string();auto prefix=base.generic_u8string();
                if(!prefix.empty()&&prefix.back()!='/')prefix+='/';auto compared=full,comparedPrefix=prefix;
#ifdef _WIN32
                auto lower=[](std::string &text){for(auto &c:text)if(c>='A'&&c<='Z')c=char(c+('a'-'A'));};lower(compared);lower(comparedPrefix);
#endif
                if(compared.compare(0,comparedPrefix.size(),comparedPrefix)==0)return full.substr(prefix.size()).c_str();
            }
            return path.lexically_normal().generic_u8string().c_str();
        }
        String pathKey(const String &path)
        {
            auto result=normalizedPath(path);
#ifdef _WIN32
            result=StringUtil::make_lower(result);
#endif
            return result;
        }
        void requireEdit()
        {
            auto app=core::IApplicationManager::instancePtr();
            if(!app||!app->isEditor()||app->isPlaying())throw std::runtime_error("Prefab updates require Editor Edit mode. Stop Play first.");
        }
        SmartPtr<ITask> taskToPause(TaskId task)
        {
            if(Thread::getCurrentTask()==task)return nullptr;
            auto app=core::IApplicationManager::instancePtr();auto tasks=app?app->getTaskManager():nullptr;
            auto result=tasks?tasks->getTask(task):nullptr;
            return result&&result->isLoaded()?result:nullptr;
        }
        template<class F> auto mutation(F &&function)->decltype(function())
        {
            TaskLock render(taskToPause(TaskId::Render));
            TaskLock physics(taskToPause(TaskId::Physics));
            TaskLock application(taskToPause(TaskId::Application));
            auto app=core::IApplicationManager::instancePtr();auto manager=app->getGameManager();
            ScopedLock managerLock(manager);auto graphics=app->getGraphicsSystem();ScopedLock graphicsLock(graphics);
            return function();
        }
        void walkData(Data root,const std::function<void(Data,bool)> &visit)
        {
            visit(root,false);for(auto component:nodes(root,true))visit(component,true);
            for(auto child:nodes(root,false))walkData(child,visit);
        }
        void walkActors(SmartPtr<IGameActor> root,const std::function<void(SmartPtr<IGameActor>)> &visit)
        {visit(root);for(auto child:root->getChildren())walkActors(child,visit);}
        Data canonical(Data source,const String &path)
        {
            if(!source)throw std::invalid_argument("Prefab document is missing");
            auto result=copy(source);std::unordered_set<String> seen;size_t ordinal=0;
            const auto seed=pathKey(path);
            walkData(result,[&](Data node,bool){auto uuid=value(node,"uuid");if(uuid.empty())uuid=StringUtil::toString(StringUtil::getUUID(seed+"#"+StringUtil::toString(++ordinal)));else ++ordinal;
                uuid=StringUtil::toString(StringUtil::parseUUID(uuid));if(!seen.insert(uuid).second)throw std::runtime_error("Duplicate prefab UUID: "+uuid);
                node->setProperty("uuid",uuid);});
            return result;
        }
        Data actorOnly(Data source)
        {
            auto result=make_ptr<Properties>();
            for(auto property:source->getPropertiesAsArray())result->setProperty(property);
            for(auto child:source->getChildren())if(child->getName()=="localTransform")result->addChild(copy(child));
            return result;
        }
        Data removeRootLink(Data source)
        {
            if(!source)throw std::invalid_argument("Prefab document is missing");
            auto result=make_ptr<Properties>();result->setName(source->getName());
            for(auto property:source->getPropertiesAsArray())result->setProperty(property);
            for(auto child:source->getChildren())
            {
                if(child->getName()=="component"||child->getName()=="components")
                {Array<Data> list;gather(child,true,list);for(auto component:list)if(!isLink(component)){auto entry=copy(component);entry->setName("component");result->addChild(entry);}}
                else result->addChild(copy(child));
            }
            Array<String> tags;result->getPropertyValue("tags",tags);
            tags.erase(std::remove(tags.begin(),tags.end(),stageTag),tags.end());result->setProperty("tags",tags);
            return result;
        }
        IdMap readMap(SmartPtr<PrefabInstance> link)
        {
            IdMap mapping;std::unordered_set<String> targets;
            for(auto entry:link->getIdentityMap()->getChildren())
            {
                auto source=value(entry,"sourceUuid"),instance=value(entry,"instanceUuid");
                if(source.empty()||instance.empty()||!mapping.emplace(source,instance).second||!targets.insert(instance).second)
                    throw std::runtime_error("Invalid prefab instance identity map");
            }
            return mapping;
        }
        Data linkData(const String &path,const String &asset,const IdMap &mapping,const String &uuid)
        {
            auto data=make_ptr<Properties>();data->setName("component");data->setProperty("componentType",linkType);
            data->setProperty("uuid",uuid);data->setProperty("state",s32(IComponent::State::Edit));data->setProperty("componentFlags",u32(2));
            data->setProperty("sourcePath",normalizedPath(path));data->setProperty("sourceAssetUuid",asset);
            Array<String> sources;for(const auto &entry:mapping)sources.push_back(entry.first);std::sort(sources.begin(),sources.end());
            for(const auto &source:sources){auto entry=make_ptr<Properties>();entry->setName("identity");entry->setProperty("sourceUuid",source);entry->setProperty("instanceUuid",mapping.at(source));data->addChild(entry);}
            return data;
        }
        void parallelMap(Data source,Data instance,IdMap &mapping)
        {
            mapping.emplace(value(source,"uuid"),value(instance,"uuid"));
            auto a=nodes(source,true),b=nodes(instance,true);if(a.size()!=b.size())throw std::runtime_error("Prefab component mapping mismatch");
            for(size_t i=0;i<a.size();++i)mapping.emplace(value(a[i],"uuid"),value(b[i],"uuid"));
            a=nodes(source,false);b=nodes(instance,false);if(a.size()!=b.size())throw std::runtime_error("Prefab child mapping mismatch");
            for(size_t i=0;i<a.size();++i)parallelMap(a[i],b[i],mapping);
        }
        struct ActorState
        {
            SmartPtr<IGameActor> actor;Data properties;
            Array<SmartPtr<IGameActor>> children;
            Array<SmartPtr<IComponent>> components;
            Array<Data> componentProperties;
        };
        struct Plan
        {
            SmartPtr<IGameActor> root;SmartPtr<IGameActor> parent;Vector3<real_Num> position;
            Array<ActorState> before,after;
            Array<SmartPtr<IGameActor>> newActors;
            Array<SmartPtr<IComponent>> newComponents;
            std::unordered_set<String> deleted,retyped;
        };
        ActorState snapshot(SmartPtr<IGameActor> actor)
        {
            ActorState result;result.actor=actor;result.properties=actorOnly(static_pointer_cast<Properties>(actor->toData()));
            result.children=actor->getChildren();result.components=actor->getComponents();
            for(auto component:result.components)result.componentProperties.push_back(copy(static_pointer_cast<Properties>(component->toData())));
            return result;
        }
        void detachComponents(SmartPtr<IGameActor> actor)
        {
            auto manager=core::IApplicationManager::instancePtr()->getGameManager();
            for(auto component:actor->getComponents())
            {manager->unregisterAllComponent(component);actor->removeComponentInstance(component);}
        }
        void detach(Array<ActorState> &actors)
        {
            for(auto &entry:actors)for(auto child:entry.actor->getChildren())entry.actor->removeChild(child);
            for(auto &entry:actors)detachComponents(entry.actor);
        }
        void install(Array<ActorState> &actors)
        {
            auto manager=core::IApplicationManager::instancePtr()->getGameManager();
            for(auto &entry:actors)entry.actor->setProperties(entry.properties);
            for(auto &entry:actors)for(auto child:entry.children)entry.actor->addChild(child);
            Array<Pair<SmartPtr<IComponent>,Data>> loading;
            for(auto &entry:actors)for(size_t i=0;i<entry.components.size();++i)
            {
                auto component=entry.components[i];component->getHandle()->setUUID(value(entry.componentProperties[i],"uuid"));
                entry.actor->addComponentInstance(component);
                if(component->getActor()!=entry.actor)throw std::runtime_error("Could not attach prefab component");
                loading.emplace_back(component,entry.componentProperties[i]);
            }
            std::stable_sort(loading.begin(),loading.end(),[](const auto &a,const auto &b){return ApplicationUtil::getCreationOrder(a.first)>ApplicationUtil::getCreationOrder(b.first);});
            for(auto &entry:loading)
            {
                manager->loadObject(entry.first,entry.second,false);
                if(!entry.first->isLoaded())throw std::runtime_error("Prefab component failed to load");
                entry.first->setProperties(entry.second);
                if(auto actor=entry.first->getActor())actor->componentLoaded(entry.first);
            }
            for(auto &entry:actors){entry.actor->updateTransform();manager->addDirtyActor(entry.actor);}
        }
        void indexActors(SmartPtr<IGameActor> root,Objects &objects)
        {walkActors(root,[&](auto actor){objects[id(actor)]=actor;for(auto component:actor->getComponents())objects[id(component)]=component;});}
        struct Resolver
        {
            Properties::ObjectResolver previous;
            explicit Resolver(const Objects &objects)
            {
                previous=Properties::exchangeObjectResolver({});
                Properties::exchangeObjectResolver([&objects,this](const String &uuid)->SmartPtr<ISharedObject>{auto found=objects.find(uuid);return found!=objects.end()?found->second:previous?previous(uuid):nullptr;});
            }
            ~Resolver(){Properties::exchangeObjectResolver(std::move(previous));}
        };
        void destroyNew(Plan &plan)
        {
            auto manager=core::IApplicationManager::instancePtr()->getGameManager();
            for(auto actor:plan.newActors){if(auto parent=actor->getParent())parent->removeChild(actor);for(auto child:actor->getChildren())actor->removeChild(child);}
            for(auto actor:plan.newActors)manager->destroyActor(actor,true);
            plan.newActors.clear();plan.newComponents.clear();
        }
        Plan preparePlan(SmartPtr<IGameActor> root,Data source,const String &path,const String &asset,IdMap mapping,SmartPtr<PrefabInstance> link)
        {
            Plan plan;plan.root=root;plan.parent=root->getParent();root->updateTransform();plan.position=root->getPosition();
            Objects originals;walkActors(root,[&](auto actor){plan.before.push_back(snapshot(actor));originals[id(actor)]=actor;for(auto c:actor->getComponents())originals[id(c)]=c;});
            source=canonical(removeRootLink(source),path);mapping[value(source,"uuid")]=id(root);IdMap next;
            walkData(source,[&](Data node,bool){auto sourceId=value(node,"uuid");auto found=mapping.find(sourceId);next.emplace(sourceId,found!=mapping.end()?found->second:StringUtil::getUUID());});
            auto desired=copy(source,next);desired->addChild(linkData(path,asset,next,link?id(link):StringUtil::getUUID()));
            auto temporary=GameActorUtil::createInstanceData(desired);IdMap temporaryIds;parallelMap(desired,temporary,temporaryIds);
            walkData(temporary,[](Data node,bool component){if(component)node->setProperty("state",s32(IComponent::State::Edit));});
            IdMap restoreIds;for(const auto &entry:temporaryIds)restoreIds.emplace(entry.second,entry.first);
            Array<String> tags;temporary->getPropertyValue("tags",tags);tags.push_back(stageTag);temporary->setProperty("tags",tags);
            Array<SmartPtr<IGameActor>> stageActors;SmartPtr<IGameActor> stage;
            try
            {
                auto loaded=GameActorUtil::loadSceneActors({temporary});if(loaded.empty())throw std::runtime_error("Prefab validation produced no actor");stage=loaded.front();
                walkActors(stage,[&](auto actor){stageActors.push_back(actor);});
                Objects future;std::unordered_set<ISharedObject*> retained;
                for(auto actor:stageActors)
                {
                    auto destinationId=restoreIds.at(id(actor));auto found=originals.find(destinationId);
                    auto destination=found==originals.end()?nullptr:dynamic_pointer_cast<IGameActor>(found->second);
                    if(!destination){destination=actor;plan.newActors.push_back(actor);}else retained.insert(destination.get());
                    future[destinationId]=destination;
                    for(auto component:actor->getComponents())
                    {
                        auto componentId=restoreIds.at(id(component));auto existing=originals.find(componentId);
                        auto replacement=existing==originals.end()?nullptr:dynamic_pointer_cast<IComponent>(existing->second);
                        if(replacement&&replacement->getTypeInfo()!=component->getTypeInfo()){plan.retyped.insert(componentId);replacement=nullptr;}
                        if(!replacement){replacement=component;plan.newComponents.push_back(component);}else retained.insert(replacement.get());
                        future[componentId]=replacement;
                    }
                }
                for(auto actor:stageActors)
                {
                    auto destinationId=restoreIds.at(id(actor));ActorState state;state.actor=dynamic_pointer_cast<IGameActor>(future.at(destinationId));
                    state.properties=copy(actorOnly(static_pointer_cast<Properties>(actor->toData())),restoreIds);
                    if(actor==stage){Array<String> clean;state.properties->getPropertyValue("tags",clean);clean.erase(std::remove(clean.begin(),clean.end(),stageTag),clean.end());state.properties->setProperty("tags",clean);}
                    for(auto child:actor->getChildren())state.children.push_back(dynamic_pointer_cast<IGameActor>(future.at(restoreIds.at(id(child)))));
                    for(auto component:actor->getComponents())
                    {
                        state.components.push_back(dynamic_pointer_cast<IComponent>(future.at(restoreIds.at(id(component)))));
                        auto properties=copy(static_pointer_cast<Properties>(component->toData()),restoreIds);
                        properties->setProperty("state",s32(IComponent::State::Edit));state.componentProperties.push_back(properties);
                    }
                    plan.after.push_back(std::move(state));
                }
                for(const auto &entry:originals)if(!future.count(entry.first))plan.deleted.insert(entry.first);
                // Extract validated shells without publishing any of them in a scene.
                for(auto actor:stageActors)for(auto child:actor->getChildren())actor->removeChild(child);
                for(auto actor:stageActors)detachComponents(actor);
                auto manager=core::IApplicationManager::instancePtr()->getGameManager();
                for(auto actor:stageActors)
                    if(std::find(plan.newActors.begin(),plan.newActors.end(),actor)==plan.newActors.end())manager->destroyActor(actor,true);
                for(auto actor:plan.newActors){actor->setEnabled(false,false);actor->setVisible(false,false);actor->addTag(stageTag);}
                return plan;
            }
            catch(...)
            {
                auto manager=core::IApplicationManager::instancePtr()->getGameManager();
                for(auto actor:stageActors)for(auto child:actor->getChildren())actor->removeChild(child);
                for(auto actor:stageActors)manager->destroyActor(actor,true);
                throw;
            }
        }
    }

    struct PrefabUpdateTransaction::Impl
    {
        Array<Plan> plans;
        SmartPtr<IGameScene> scene;
        struct External{SmartPtr<IComponent> component;Data before,after;};
        Array<External> external;
        bool committed=false,finished=false,started=false,ownsLock=true;
        ~Impl()
        {
            if(!finished)try{for(auto &plan:plans)destroyNew(plan);}catch(const std::exception &e){WP_LOG_EXCEPTION(e);}
            if(ownsLock)transactionActive=false;
        }
        Objects objects(bool after)
        {
            Objects result;if(scene)for(auto root:scene->getActors())indexActors(root,result);
            for(const auto &plan:plans)
            {
                for(const auto &uuid:plan.deleted)result.erase(uuid);
                for(const auto &entry:(after?plan.after:plan.before))
                {
                    result[value(entry.properties,"uuid")]=entry.actor;
                    for(size_t i=0;i<entry.components.size();++i)result[value(entry.componentProperties[i],"uuid")]=entry.components[i];
                }
            }
            return result;
        }
        void prepareExternal()
        {
            if(!scene)return;std::unordered_set<IComponent*> internal;IdMap touched;
            for(const auto &plan:plans){for(const auto &entry:plan.before)for(auto c:entry.components)internal.insert(c.get());for(const auto &uuid:plan.deleted)touched[uuid]="";for(const auto &uuid:plan.retyped)touched[uuid]=uuid;}
            for(auto root:scene->getActors())walkActors(root,[&](auto actor){for(auto c:actor->getComponents())if(!internal.count(c.get())){auto before=copy(static_pointer_cast<Properties>(c->toData()));bool changed=false;auto after=copy(before,touched,&changed);if(changed)external.push_back({c,before,after});}});
        }
    };

    PrefabUpdateTransaction::PrefabUpdateTransaction(std::unique_ptr<Impl> implementation):m_impl(std::move(implementation)){}
    PrefabUpdateTransaction::~PrefabUpdateTransaction(){if(m_impl&&!m_impl->finished)try{rollback();}catch(const std::exception &e){WP_LOG_EXCEPTION(e);}}
    size_t PrefabUpdateTransaction::changedCount() const{return m_impl->plans.size();}
    void PrefabUpdateTransaction::commit()
    {
        requireEdit();if(m_impl->finished)throw std::logic_error("Prefab transaction already finished");if(m_impl->committed)return;
        mutation([&]{try{
            for(const auto &plan:m_impl->plans)if(plan.root->getParent()!=plan.parent)throw std::runtime_error("Prefab parent changed while preparing update");
            m_impl->started=true;auto objects=m_impl->objects(true);Resolver resolver(objects);
            for(auto &plan:m_impl->plans){detach(plan.before);detach(plan.after);install(plan.after);plan.root->setPosition(plan.position);plan.root->updateTransform();}
            for(auto &entry:m_impl->external)entry.component->setProperties(entry.after);
            m_impl->committed=true;
        }catch(...){auto failure=std::current_exception();rollback();std::rethrow_exception(failure);}});
    }
    void PrefabUpdateTransaction::rollback()
    {
        if(m_impl->finished)return;
        mutation([&]{
            if(m_impl->started)
            {
                auto objects=m_impl->objects(false);Resolver resolver(objects);
                for(auto &plan:m_impl->plans){detach(plan.after);detach(plan.before);install(plan.before);plan.root->setPosition(plan.position);plan.root->updateTransform();}
                for(auto &entry:m_impl->external)entry.component->setProperties(entry.before);
            }
            for(auto &plan:m_impl->plans)destroyNew(plan);
            m_impl->finished=true;m_impl->committed=false;m_impl->ownsLock=false;transactionActive=false;
        });
    }
    void PrefabUpdateTransaction::finish()
    {
        if(m_impl->finished)return;if(!m_impl->committed)throw std::logic_error("Commit prefab updates before finishing");
        mutation([&]{m_impl->finished=true;m_impl->ownsLock=false;transactionActive=false;auto manager=core::IApplicationManager::instancePtr()->getGameManager();
            for(auto &plan:m_impl->plans){std::unordered_set<IGameActor*> retained;for(const auto &entry:plan.after)retained.insert(entry.actor.get());for(const auto &entry:plan.before)if(!retained.count(entry.actor.get()))manager->destroyActor(entry.actor,true);}
            m_impl->plans.clear();m_impl->external.clear();});
    }

    Data PrefabInstanceService::createLinkedInstanceData(Data source,const String &path,const String &asset)
    {
        auto clean=canonical(removeRootLink(source),path);auto instance=GameActorUtil::createInstanceData(clean);IdMap mapping;parallelMap(clean,instance,mapping);
        walkData(instance,[](Data node,bool component){if(component)node->setProperty("state",s32(IComponent::State::Edit));});
        instance->addChild(linkData(path,asset,mapping,StringUtil::getUUID()));return instance;
    }
    Data PrefabInstanceService::makeTemplateData(SmartPtr<IGameActor> root)
    {
        if(!root)throw std::invalid_argument("Prefab editing root is missing");IdMap reverse;
        if(auto link=root->getComponent<PrefabInstance>())for(const auto &entry:readMap(link))reverse.emplace(entry.second,entry.first);
        return copy(removeRootLink(static_pointer_cast<Properties>(root->toData())),reverse);
    }
    void PrefabInstanceService::adopt(SmartPtr<IGameActor> root,Data source,const String &path,const String &asset)
    {
        requireEdit();if(!root||root->getComponent<PrefabInstance>())throw std::runtime_error("Choose an unlinked actor for prefab adoption");
        mutation([&]{auto clean=canonical(removeRootLink(source),path);IdMap mapping;
            auto match=[&](auto &&self,Data data,SmartPtr<IGameActor> actor)->void{
                mapping[value(data,"uuid")]=id(actor);auto components=nodes(data,true);auto actual=actor->getComponents();auto children=nodes(data,false);auto existing=actor->getChildren();
                if(components.size()!=actual.size()||children.size()!=existing.size())throw std::runtime_error("Existing actor does not match prefab structure; reset it explicitly before adoption");
                for(size_t i=0;i<components.size();++i){auto type=value(components[i],"componentType");auto actualType=String(TypeManager::instance()->getName(actual[i]->getTypeInfo()));if(type!=actualType&&actualType.substr(actualType.find_last_of(':')+1)!=type)throw std::runtime_error("Existing actor component types do not match prefab");mapping[value(components[i],"uuid")]=id(actual[i]);}
                for(size_t i=0;i<children.size();++i)self(self,children[i],existing[i]);};
            if(value(clean,"uuid")==id(root))
            {
                // Saving this very actor is exact provenance, including graphs
                // that intentionally omit resident streaming children.
                Objects objects;indexActors(root,objects);
                walkData(clean,[&](Data node,bool component){auto uuid=value(node,"uuid");auto found=objects.find(uuid);
                    if(found==objects.end()||bool(dynamic_pointer_cast<IComponent>(found->second))!=component)throw std::runtime_error("Saved prefab identity does not belong to this actor");mapping[uuid]=uuid;});
            }
            else match(match,clean,root);
            auto component=make_ptr<PrefabInstance>();
            auto manager=core::IApplicationManager::instancePtr()->getGameManager();
            try
            {
                root->addComponentInstance(component);
                if(component->getActor()!=root)throw std::runtime_error("Could not attach prefab source link");
                manager->loadObject(component,linkData(path,asset,mapping,id(component)),false);
                if(!component->isLoaded())throw std::runtime_error("Could not load prefab source link");
            }
            catch(...)
            {
                manager->unregisterAllComponent(component);
                root->removeComponentInstance(component);
                throw;
            }
        });
    }
    std::unique_ptr<PrefabUpdateTransaction> PrefabInstanceService::prepareUpdate(SmartPtr<IGameActor> root,Data source)
    {
        requireEdit();auto link=root?root->getComponent<PrefabInstance>():nullptr;if(!link)throw std::runtime_error("Actor has no prefab source link");
        bool expected=false;if(!transactionActive.compare_exchange_strong(expected,true))throw std::runtime_error("Another prefab update is still pending");
        auto implementation=std::make_unique<PrefabUpdateTransaction::Impl>();implementation->scene=root->getScene();
        mutation([&]{implementation->plans.push_back(preparePlan(root,source,link->getSourcePath(),link->getSourceAssetUuid(),readMap(link),link));implementation->prepareExternal();});
        return std::unique_ptr<PrefabUpdateTransaction>(new PrefabUpdateTransaction(std::move(implementation)));
    }
    std::unique_ptr<PrefabUpdateTransaction> PrefabInstanceService::prepareUpdates(SmartPtr<IGameScene> scene,const String &path,Data source,bool migrateWerecat)
    {
        requireEdit();if(!scene)throw std::invalid_argument("Authoring scene is missing");bool expected=false;
        if(!transactionActive.compare_exchange_strong(expected,true))throw std::runtime_error("Another prefab update is still pending");
        auto implementation=std::make_unique<PrefabUpdateTransaction::Impl>();implementation->scene=scene;
        mutation([&]{try{
            std::unordered_set<IGameActor*> selected;
            for(auto sceneRoot:scene->getActors())walkActors(sceneRoot,[&](auto actor){
                for(auto ancestor=actor->getParent();ancestor;ancestor=ancestor->getParent())
                    if(selected.count(ancestor.get()))return;
                if(auto link=actor->getComponent<PrefabInstance>();link&&pathKey(link->getSourcePath())==pathKey(path))
                {if(selected.insert(actor.get()).second)implementation->plans.push_back(preparePlan(actor,source,path,link->getSourceAssetUuid(),readMap(link),link));}
                if(migrateWerecat&&actor->hasTag("Werecat.ModelPrefab.Owned"))
                    for(const auto &tag:actor->getTags())
                    {
                        const String prefix="Werecat.ModelPrefab.Signature:";if(tag.compare(0,prefix.size(),prefix)!=0)continue;
                        auto start=tag.find('|');if(start==String::npos)continue;auto end=tag.find('|',start+1);if(end==String::npos||pathKey(tag.substr(start+1,end-start-1))!=pathKey(path))continue;
                        auto children=actor->getChildren();if(children.size()!=1)throw std::runtime_error("Legacy Werecat model wrapper needs exactly one imported model root");auto root=children.front();
                        if(root->getComponent<PrefabInstance>()||!selected.insert(root.get()).second)continue;
                        // Legacy visual instances have no reliable descendant IDs. Retain
                        // their root identity and explicitly rebuild their presentation.
                        implementation->plans.push_back(preparePlan(root,source,path,"",{},nullptr));
                    }
            });implementation->prepareExternal();
        }catch(...){for(auto &plan:implementation->plans)destroyNew(plan);throw;}});
        return std::unique_ptr<PrefabUpdateTransaction>(new PrefabUpdateTransaction(std::move(implementation)));
    }
}
