#pragma once
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <stdexcept>
#include <cstdio>

namespace claw_queued_material_node_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    class Owner final : public Material
    {
    public:
        explicit Owner(bool &destroyed):m_destroyed(destroyed){}
        ~Owner() override {m_destroyed=true;}
    private:
        bool &m_destroyed;
    };
    class Unit final : public MaterialTexture
    {
    public:
        explicit Unit(bool &destroyed):m_destroyed(destroyed){}
        ~Unit() override {m_destroyed=true;}
    private:
        bool &m_destroyed;
    };
    class Pass final : public MaterialPass
    {
    public:
        using MaterialPass::setTextureUnits;
    };
    inline void run(SmartPtr<IGraphicsSystem> graphics)
    {
        require(graphics!=nullptr,"Queued material retirement requires the graphics queue");
        for(const bool evicted:{false,true})
        for(const auto state:{LoadingState::Allocated,LoadingState::Loading,LoadingState::Unloaded})
        {
            bool ownerDestroyed=false,unitDestroyed=false;
            auto owner=make_ptr<Owner>(ownerDestroyed);
            auto technique=make_ptr<MaterialTechnique>();auto pass=make_ptr<Pass>();
            owner->addTechnique(technique);technique->addPass(pass);pass->load(nullptr);
            pass->reload(nullptr);
            require(pass->getMaterial()==owner&&pass->getParent()==technique,"Pass reload must restore the material and parent it temporarily retired");
            auto unit=make_ptr<Unit>(unitDestroyed);unit->setMaterial(owner);pass->setTextureUnits({unit});
            require(unit->getMaterial()==owner&&unit->getParent()==pass,"Queued unit must begin attached to the real graph");
            // The force-load queue owns the detached facades beyond the graph,
            // exactly as an upload accepted just before Editor teardown can do.
            graphics->loadObject(unit,true);graphics->loadObject(pass,true);graphics->loadObject(technique,true);
            if(evicted)
            {
                pass->setTextureUnits({});
                require(!unit->getMaterial()&&!unit->getParent(),"Evicting a queued unit must retire both links before the old graph unloads");
            }
            unit->setLoadingState(state);pass->setLoadingState(state);technique->setLoadingState(state);
            owner->unload(nullptr);
            require(!unit->getMaterial()&&!unit->getParent()&&!pass->getMaterial()&&!pass->getParent()&&
                !technique->getMaterial()&&!technique->getParent(),"Every queued material facade must retire both owner links during unload");
            owner=nullptr;require(ownerDestroyed,"Pending material nodes must not retain their retired material");
            unit=nullptr;pass=nullptr;technique=nullptr;
            require(!unitDestroyed,"Pending upload queue must still own the detached unit");
            graphics->clearObjectQueues();
            require(unitDestroyed,"Clearing a future queue must safely destroy the unit after its material owner");
        }
        for(unsigned route=0;route<12;++route)
        {
            bool ownerDestroyed=false,otherDestroyed=false,unitDestroyed=false;
            auto owner=make_ptr<Owner>(ownerDestroyed),other=make_ptr<Owner>(otherDestroyed);
            auto technique=make_ptr<MaterialTechnique>(),destination=make_ptr<MaterialTechnique>();auto pass=make_ptr<Pass>();
            owner->addTechnique(technique);technique->addPass(pass);pass->load(nullptr);
            auto unit=make_ptr<Unit>(unitDestroyed);pass->setTextureUnits({unit});
            {auto found=technique->getChildById(pass->getId());require(found==pass&&pass->getParent()==technique&&pass->getMaterial()==owner,"Child lookup must preserve graph ownership");}
            graphics->loadObject(unit,true);graphics->loadObject(pass,true);graphics->loadObject(technique,true);
            switch(route)
            {
            case 0:technique->removePass(pass);break;
            case 1:technique->removePasses();break;
            case 2:technique->setPasses({});break;
            case 3:technique->removeChild(pass);break;
            case 4:technique->removeAllChildren();break;
            case 5:technique->setChildren({});break;
            case 6:technique->addChild(make_ptr<MaterialPass>(),0);break;
            case 7:owner->removeTechnique(technique);break;
            case 8:owner->removeAllTechniques();break;
            case 9:owner->setTechniques({});break;
            case 10:other->addTechnique(destination);destination->addPass(pass);break;
            case 11:other->addTechnique(technique);break;
            }
            if(route<10)require(!pass->getMaterial()&&!unit->getMaterial(),"Removing/replacing graph membership must retire descendant material links");
            owner->unload(nullptr);owner=nullptr;require(ownerDestroyed,"Detached graph must not retain its former material");
            if(route>=10)require(pass->getMaterial()==other&&unit->getMaterial()==other&&pass->isLoaded(),"Retiring an old owner must preserve a graph moved to another owner");
            other->unload(nullptr);other=nullptr;require(otherDestroyed,"New material owner must retire independently");
            technique=nullptr;destination=nullptr;pass=nullptr;unit=nullptr;graphics->clearObjectQueues();
            require(unitDestroyed,"Removed/reparented queued facade must retire after both owners");
        }
        {
            auto parent=make_ptr<MaterialTechnique>(),child=make_ptr<MaterialTechnique>(),leaf=make_ptr<MaterialTechnique>();parent->addChild(child);child->addChild(leaf);
            auto rejects=[](const auto &operation){try{operation();}catch(const std::invalid_argument &){return true;}return false;};
            require(rejects([&]{parent->addChild(parent);})&&rejects([&]{child->addChild(parent);})&&
                rejects([&]{child->addChild(parent,0);})&&rejects([&]{child->setChildren({parent});})&&
                rejects([&]{parent->setParent(child);}),"Self/ancestor links must reject before recursive material propagation");
            require(parent->getNumChildren()==1&&parent->getChildByIndex(0)==child&&child->getChildByIndex(0)==leaf&&
                !parent->getParent()&&child->getParent()==parent,"Rejected graph cycles must preserve the original tree");
            parent->removeAllChildren();child->removeAllChildren();
        }
        std::puts("PASS: queued material nodes survive owner-first teardown without stale material/parent links");
    }
}
