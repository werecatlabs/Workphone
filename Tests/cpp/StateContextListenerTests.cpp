#include <Workphone/Workphone.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/State/States/State.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <functional>

using namespace workphone;
namespace
{
    void require(bool value, const char *message)
    {
        if(!value) throw std::runtime_error(message);
    }
    class Listener : public IStateListener
    {
    public:
        bool handleStateMessage(const SmartPtr<IStateMessage> &) override { return true; }
        bool handleStateChanged(SmartPtr<IState> &) override { ++notifications; return true; }
        unsigned notifications = 0;
    };
    class EventListener : public IEventListener
    {
    public:
        Parameter handleEvent( EventType, hash_type, const Array<Parameter> &,
                               SmartPtr<ISharedObject>, SmartPtr<ISharedObject>,
                               SmartPtr<IEvent> ) override { return {}; }
    };
    class DirtyState : public State
    {
    public:
        bool isDirty() const override { return dirty; }
        void setDirty(bool value) override { dirty = value; }
        bool dirty = true;
    };
    class InventoryListener : public IStateListener
    {
    public:
        bool handleStateMessage(const SmartPtr<IStateMessage> &) override { return true; }
        bool handleStateChanged(SmartPtr<IState> &) override
        {
            ++notifications;
            if(onFirst) { auto callback = std::move(onFirst); callback(); }
            return true;
        }
        unsigned notifications = 0;
        std::function<void()> onFirst;
    };
}
int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance(&types);
    int result = 0;
    try
    {
        StateContext context;
        context.setLoadingState(LoadingState::Loaded);
        std::vector<SmartPtr<Listener>> listeners;
        for(unsigned i = 0; i < 256; ++i) listeners.push_back(make_ptr<Listener>());
        // Exercise material-style fan-out beyond the old 32-listener bound,
        // with concurrent registration of the same listeners.
        std::vector<std::thread> threads;
        for(unsigned worker = 0; worker < 4; ++worker)
            threads.emplace_back([&]() {
                for(const auto &listener : listeners) context.addStateListener(listener);
            });
        for(auto &thread : threads) thread.join();
        auto snapshot = context.getStateListeners();
        require(snapshot.size() == listeners.size(), "Listeners must grow and remain unique");
        SmartPtr<IState> state = make_ptr<State>();
        context._processStateUpdate(state);
        for(const auto &listener : listeners)
            require(listener->notifications == 1, "Each listener must receive the state update");
        require(context.removeStateListener(listeners[100]), "Registered listener must be removable");
        require(snapshot.size() == 256 && context.getStateListeners().size() == 255,
                "Dispatch snapshots must remain independent of later removals");
        context._processStateUpdate(state);
        require(listeners[100]->notifications == 1 && listeners[101]->notifications == 2,
                "Removed listeners must stop receiving updates");
        std::vector<SmartPtr<EventListener>> events;
        for(unsigned i = 0; i < 256; ++i) events.push_back(make_ptr<EventListener>());
        threads.clear();
        // Duplicate attempts race with growth unless search and insertion share a lock.
        for(unsigned worker = 0; worker < 4; ++worker)
            threads.emplace_back([&]() {
                for(const auto &listener : events) context.addEventListener(listener);
            });
        for(auto &thread : threads) thread.join();
        const auto eventSnapshot = context.getEventListeners();
        require(eventSnapshot.size() == events.size(), "Event listeners must grow and remain unique");
        threads.clear();
        for(unsigned worker = 0; worker < 4; ++worker)
            threads.emplace_back([&, worker]() {
                for(unsigned i = worker; i < events.size(); i += 4)
                    context.removeEventListener(events[i]);
            });
        for(auto &thread : threads) thread.join();
        require(context.getEventListeners().empty() && eventSnapshot.size() == events.size(),
                "Concurrent removal must preserve event snapshots");
        for(const auto &listener : events) context.addEventListener(listener);
        context.setLoadingState(LoadingState::Unloaded);
        context.unload(nullptr);
        require(context.getStateListeners().empty() && context.getEventListeners().empty(),
                "Teardown must release all registrations");
        {
            auto inventory = make_ptr<StateContext>();
            auto owner = make_ptr<State>();
            owner->setLoadingState(LoadingState::Loaded);
            inventory->setLoadingState(LoadingState::Loaded);
            inventory->setOwner(owner);
            std::vector<SmartPtr<DirtyState>> states;
            for(unsigned i = 0; i < 128; ++i) states.push_back(make_ptr<DirtyState>());
            threads.clear();
            for(unsigned worker = 0; worker < 4; ++worker)
                threads.emplace_back([&]() {
                    for(const auto &entry : states) inventory->addState(entry);
                });
            for(auto &thread : threads) thread.join();
            require(inventory->getStates().size() == states.size(),
                    "Concurrent state registration must not publish duplicates");
            auto retained = inventory->getStates();
            auto dispatch = make_ptr<InventoryListener>();
            std::promise<void> beginRemoval, removed;
            auto begin = beginRemoval.get_future();
            auto complete = removed.get_future();
            std::thread remover([&]() {
                begin.wait();
                for(const auto &entry : states) inventory->removeState(entry);
                removed.set_value();
            });
            bool removedInsideCallback = false;
            dispatch->onFirst = [&]() {
                beginRemoval.set_value();
                removedInsideCallback = complete.wait_for(std::chrono::seconds(2)) ==
                                        std::future_status::ready;
            };
            inventory->addStateListener(dispatch);
            inventory->update();
            remover.join();
            require(removedInsideCallback, "State callbacks must not hold the inventory lock");
            require(dispatch->notifications == states.size(),
                    "A dispatch snapshot must survive concurrent removal of every state");
            require(inventory->getStates().empty() && retained.size() == states.size(),
                    "State removal must preserve previously retained snapshots");
            inventory->update();
            require(dispatch->notifications == states.size(),
                    "Removed states must not be dispatched in a later update");
            for(auto &entry : states) entry->setStateContext(nullptr);
            inventory->setOwner(nullptr);
            inventory->unload(nullptr);
        }
        std::cout << "State inventory/listeners: concurrent registration, retained dispatch and cleanup PASS\n";
    }
    catch(const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    TypeManager::setInstance(nullptr);
    types.unload();
    return result;
}
