#pragma once
#include <Workphone/System/Rttr.hpp>
#include "GenericEngineCatalogBudgetContracts.hpp"

namespace reflection_payload_budget_contracts
{
    using namespace workphone;
    using namespace workphone::reflection;
    using generic_engine_catalog_budget_contracts::Capacity;
    using generic_engine_catalog_budget_contracts::require;

    struct alignas(64) Large { std::array<unsigned char, 512> bytes{}; };
    static_assert(!Storage<std::any>::supported, "opaque std::any must cross a checked typed import");
    static_assert(!std::is_constructible_v<Any, std::any>, "no unchecked erased payload import");
    static_assert(!std::is_constructible_v<Callback<int()>, std::function<int()>>,
                  "opaque function heaps must not bypass capture admission");

    inline void values()
    {
        Capacity budget;
        const auto baseline = budget.used();
        {
            Large input; input.bytes[3] = 41;
            Any value(input);
            const auto one = sourceBudgetStorageBytes(sizeof(Large), alignof(Large));
            require(budget.used() == baseline + one &&
                        reinterpret_cast<uintptr_t>(any_cast<Large>(&value)) % alignof(Large) == 0,
                    "erased payload uses exact aligned SourceCpu storage");
            Any copied(value);
            any_cast<Large &>(copied).bytes[3] = 42;
            require(budget.used() == baseline + 2 * one && any_cast<const Large &>(value).bytes[3] == 41,
                    "erased payload copies own independent admitted storage");
            const auto held = budget.used();
            budget.set(held);
            Any moved(std::move(copied));
            require(!copied.has_value() && moved.has_value() && budget.used() == held,
                    "erased payload moves transfer storage without admission");
            bool denied = false;
            try { value = moved; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && any_cast<const Large &>(value).bytes[3] == 41 && budget.used() == held,
                    "denied erased payload copy preserves the previous value");
            denied = false;
            try { value.emplace<Large>(); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && any_cast<const Large &>(value).bytes[3] == 41 && budget.used() == held,
                    "denied erased payload emplace preserves the previous value");
            budget.set(held + 64);
            denied = false;
            try { value.emplace<Stored<std::string>>(4000, 'x'); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && any_cast<const Large &>(value).bytes[3] == 41 && budget.used() == held,
                    "nested emplace denial unwinds its admitted shell without replacing the old payload");
            budget.set(held);
            require(any_cast<int>(&value) == nullptr && any_cast<int>(static_cast<Any *>(nullptr)) == nullptr,
                    "pointer casts reject type mismatch and null erased values");
            bool badCast = false;
            try { (void)any_cast<int>(value); } catch(const std::bad_any_cast &) { badCast = true; }
            require(badCast, "value casts retain bad_any_cast semantics");
            value.reset();
            std::atomic<bool> transferred{false};
            std::thread worker([received = std::move(moved), &transferred]() mutable {
                transferred = any_cast<const Large &>(received).bytes[3] == 42;
                received.reset();
            });
            worker.join();
            require(transferred && !moved.has_value(), "erased payload ownership transfers across threads at the cap");
            require(budget.used() == baseline, "full-budget reset retires erased payload storage");
        }
        budget.restore();
        {
            std::vector<std::string> input;
            input.reserve(40);
            input.emplace_back(2000, 'a'); input.back().reserve(6000);
            input.emplace_back(3000, 'b');
            std::any legacy(input);
            Any value = Any::fromStdAny<std::vector<std::string>>(legacy);
            using Native = Stored<std::vector<std::string>>;
            auto &native = any_cast<Native &>(value);
            require(native.size() == 2 && native.capacity() >= std::any_cast<const std::vector<std::string> &>(legacy).capacity() &&
                        budget.used() > baseline + 5000,
                    "checked std::any import admits nested container/name buffers and retained capacity");
            const auto held = budget.used();
            budget.set(held);
            bool denied = false;
            try { native[0].reserve(12000); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && native[0].size() == 2000 && native[0][1999] == 'a' && budget.used() == held,
                    "reflected nested buffers enforce admission during later mutation");
            denied = false;
            try { (void)Any::fromStdAny<std::vector<std::string>>(std::move(legacy)); }
            catch(const std::bad_alloc &) { denied = true; }
            require(denied && legacy.has_value() && native.size() == 2 && budget.used() == held,
                    "denied consuming std::any import preserves the source and destination ownership");
            bool badCast = false;
            try { (void)Any::fromStdAny<int>(legacy); } catch(const std::bad_any_cast &) { badCast = true; }
            require(badCast && budget.used() == held, "wrong typed std::any import allocates no payload");
            budget.restore();
            Any copy(value);
            any_cast<Native &>(copy)[0][0] = 'z';
            require(native[0][0] == 'a', "nested reflected copies retain value semantics");
            auto imported = Any::fromStdAny<std::vector<std::string>>(std::move(legacy));
            require(!legacy.has_value() && any_cast<const Native &>(imported)[1].size() == 3000,
                    "consuming std::any import retries and transfers only after complete admission");
        }
        require(budget.used() == baseline, "last erased nested payload owner retires all physical buffers");
        {
            Array<std::vector<std::string>> input;
            input.reserve(32);
            input.emplace_back(2, std::string(1500, 'e'));
            Any value(input);
            auto &native = any_cast<Stored<decltype(input)> &>(value);
            require(native.capacity() >= 32 && native[0][1].size() == 1500,
                    "engine Array payloads canonicalize nested containers and preserve admitted spare capacity");
            std::array<std::wstring, 2> wide{{std::wstring(2000, L'w'), std::wstring(1000, L'z')}};
            Any wideValue(wide);
            require(any_cast<const Stored<decltype(wide)> &>(wideValue)[0][1999] == L'w',
                    "fixed-array payloads admit nested wide string storage");
        }
        require(budget.used() == baseline, "engine and fixed-array payloads release all canonical buffers");
    }

    inline void callbacks()
    {
        auto opaque = [value = std::string(1000, 'x')] { return int(value.size()); };
        static_assert(!std::is_constructible_v<Callback<int()>, decltype(opaque)>,
                      "owning opaque lambda captures require managed binding");
        auto erasedReturn = []() -> std::any { return 3; };
        static_assert(!std::is_constructible_v<Callback<Any()>, decltype(erasedReturn)>,
                      "legacy erased callback results require a typed import adapter");
        Capacity budget;
        const auto baseline = budget.used();
        {
            Large state; state.bytes[3] = 17;
            auto function = [state](int value) { return state.bytes[3] + value; };
            Callback<int(int)> callback(function);
            const auto one = sourceBudgetStorageBytes(sizeof(function), alignof(decltype(function)));
            require(budget.used() == baseline + one && callback(2) == 19,
                    "large inline capture state uses exact aligned SourceCpu storage");
            Callback<int(int)> copy(callback);
            const auto held = budget.used();
            budget.set(held);
            bool denied = false;
            try { callback = [](int) { return -1; }; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && callback(2) == 19 && budget.used() == held,
                    "denied callback replacement preserves the previous callable");
            Callback<int(int)> moved(std::move(copy));
            require(!copy && moved(3) == 20 && budget.used() == held,
                    "callback moves and warm invocation require no admission");
            callback.reset(); moved.reset();
            require(budget.used() == baseline, "full-budget callback reset frees capture storage");
            Callback<int()> empty;
            bool badCall = false;
            try { empty(); } catch(const std::bad_function_call &) { badCall = true; }
            require(badCall, "empty callbacks retain bad_function_call behavior");
        }
        budget.restore();
        {
            std::vector<std::string> input(3, std::string(3000, 'c'));
            input.reserve(50);
            auto callback = Callback<size_t()>::bind([](auto &values) { return values.capacity() + values[0].size(); }, input);
            const auto held = budget.used();
            require(callback() >= 3050 && held > baseline + 9000,
                    "explicit managed binding admits capture containers, spare capacity and nested strings");
            budget.set(held);
            bool denied = false;
            try { auto rejected = callback; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && callback() >= 3050 && budget.used() == held,
                    "denied managed callback copy leaves its nested captures callable");
            budget.restore();
            // Permit the capture shell but reject a later nested buffer. All
            // earlier candidate storage must unwind without changing the owner.
            budget.set(held + 256);
            denied = false;
            try { auto rejected = callback; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && callback() >= 3050 && budget.used() == held,
                    "partial managed capture admission unwinds shell and nested allocations");
            budget.restore();
            auto copy = callback;
            require(copy() >= 3003 && budget.used() > held, "managed capture copying retries with independent physical storage");
        }
        require(budget.used() == baseline, "last callback capture owner retires every nested allocation");
        budget.restore();
        {
            Callback<Any()> failing = []() -> Any {
                Any temporary(std::vector<int>(1024, 7));
                throw std::runtime_error("callback failure");
            };
            const auto held = budget.used();
            bool threw = false;
            try { (void)failing(); } catch(const std::runtime_error &) { threw = true; }
            require(threw && budget.used() == held,
                    "callback exceptions propagate and retire unpublished erased payloads");
        }
        require(budget.used() == baseline, "throwing callback releases its capture shell");
    }

    inline void reflection()
    {
        Capacity budget;
        const auto baseline = budget.used();
        Any retained;
        {
            Type descriptor;
            std::vector<std::string> source(2, std::string(2200, 'r'));
            workphone::reflection::Property property;
            property.name = "data";
            property.getter = Callback<Any(void *)>::bind([](auto &values, void *) -> Any { return values; }, source);
            descriptor.addProperty(property);
            const auto held = budget.used();
            budget.set(held);
            bool denied = false;
            try { (void)descriptor.properties.at("data").getter(nullptr); }
            catch(const std::bad_alloc &) { denied = true; }
            require(denied && budget.used() == held && descriptor.properties.size() == 1,
                    "denied getter result admission preserves the stored callback and descriptor");
            budget.restore();
            workphone::reflection::Property replacement;
            replacement.name = "data";
            replacement.getter = Callback<Any(void *)>::bind([](auto &values, void *) -> Any { return values; },
                std::vector<std::string>(2, std::string(2200, 'x')));
            const auto beforeReplacement = budget.used();
            budget.set(beforeReplacement + 128);
            denied = false;
            try { descriptor.addProperty(replacement); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && descriptor.properties.size() == 1 && budget.used() == beforeReplacement,
                    "partial callback replacement admission preserves the existing published property");
            budget.restore();
            retained = descriptor.properties.at("data").getter(nullptr);
            require(any_cast<const Stored<std::vector<std::string>> &>(retained)[0].size() == 2200 &&
                        any_cast<const Stored<std::vector<std::string>> &>(retained)[0][0] == 'r',
                    "getter return payload owns admitted storage through normal reflection invocation");
            Arguments arguments;
            arguments.reserve(20);
            arguments.emplace_back(std::vector<int>(1000, 4));
            Method method;
            method.name = "size";
            method.invoker = [](void *, const Arguments &values) -> Any {
                return any_cast<const Stored<std::vector<int>> &>(values.at(0)).size(); };
            descriptor.addMethod(method);
            require(any_cast<size_t>(descriptor.methods.at("size").invoker(nullptr, arguments)) == 1000,
                    "method argument containers and nested erased values use admitted owners");
            const auto argumentBytes = budget.used();
            budget.set(argumentBytes);
            bool argumentDenied = false;
            try { arguments.reserve(100); } catch(const std::bad_alloc &) { argumentDenied = true; }
            require(argumentDenied && arguments.size() == 1 && budget.used() == argumentBytes,
                    "denied argument inventory growth preserves admitted nested payloads");
            budget.restore();
        }
        require(budget.used() > baseline && any_cast<const Stored<std::vector<std::string>> &>(retained)[1][0] == 'r',
                "escaped getter values pin physical payloads after descriptor destruction");
        retained.reset();
        require(budget.used() == baseline, "last escaped reflection value releases its source storage");
    }
    struct MacroObject
    {
        int number = 0;
        std::vector<std::string> names;
        int twice() { return number * 2; }
    };
    inline void macros()
    {
        Capacity budget;
        const auto baseline = budget.used();
        {
            SharedPtr<Type> descriptor;
            REFLECT_TYPE(MacroObject)
                REFLECT_PROPERTY(MacroObject, number)
                REFLECT_PROPERTY(MacroObject, names)
                REFLECT_METHOD(MacroObject, twice)
                descriptor = t;
            }
            auto instance = std::unique_ptr<MacroObject>(static_cast<MacroObject *>(descriptor->constructor()));
            Any number(21);
            descriptor->properties.at("number").setter(instance.get(), std::move(number));
            require(!number.has_value() && any_cast<int>(descriptor->properties.at("number").getter(instance.get())) == 21 &&
                        any_cast<int>(descriptor->methods.at("twice").invoker(instance.get(), {})) == 42,
                    "reflection macros route constructor, property and method calls through admitted callbacks/values");
            Any names(std::vector<std::string>(2, std::string(2000, 'n')));
            descriptor->properties.at("names").setter(instance.get(), std::move(names));
            auto read = descriptor->properties.at("names").getter(instance.get());
            require(instance->names.size() == 2 && any_cast<const Stored<std::vector<std::string>> &>(read)[1].size() == 2000,
                    "macro setter exports to caller fields while getters retain budget-native nested buffers");
        }
        require(budget.used() == baseline, "macro descriptor, captures and returned values retire completely");
    }
    inline void run()
    {
        values(); callbacks(); reflection(); macros();
        std::cout << "PASS: reflection capture, argument and erased-payload SourceCpu admission\n";
    }
}
