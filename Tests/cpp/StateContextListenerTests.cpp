#include <Workphone/Workphone.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/State/States/State.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

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
        context.setLoadingState(LoadingState::Unloaded);
        context.unload(nullptr);
        require(context.getStateListeners().empty(), "Teardown must release all registrations");
        std::cout << "State listeners: shared-material fan-out, concurrent deduplication, dispatch and cleanup PASS\n";
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
