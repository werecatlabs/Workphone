#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/LSystem/LSystem.hpp>
#include <Workphone/Core/CharacterPoolAllocator.hpp>
#include <limits>
#include <memory>
#include <type_traits>

namespace lsystem_storage_budget_contracts
{
    using namespace city_inventory_budget_contracts;

    class System : public LSystem
    {
    public:
        const String &current() const { return curString; }
        const String &axiom() const { return start; }
        s32 currentLevel() const { return level; }
        const Array<SmartPtr<ILSystemRule>> &ruleInventory() const { return rules; }
        const Array<String> &variableInventory() const { return variables; }
        const Array<String> &constantInventory() const { return constants; }
        void reserveRules( size_t count ) { rules.reserve( count ); }
        void reserveVariables( size_t count ) { variables.reserve( count ); }
        void reserveConstants( size_t count ) { constants.reserve( count ); }
        void setCurrent( const String &value, s32 count )
        {
            String candidate( value ); curString.swap( candidate ); level = count;
        }
        bool detached() const
        {
            return rules.empty() && !rules.capacity() && variables.empty() && !variables.capacity() &&
                   constants.empty() && !constants.capacity() && start.empty() && curString.empty() && level == 0 &&
                   rules.get_allocator().isSourceBudgeted() && variables.get_allocator().isSourceBudgeted() &&
                   constants.get_allocator().isSourceBudgeted();
        }
    };

    class Rule : public LRule
    {
    public:
        const String &predecessor() const { return m_predecessor; }
        const String &successor() const { return m_successor; }
    };

    template <class T> inline u64 storageBytes( size_t capacity )
    {
        return capacity ? sourceBudgetStorageBytes( capacity * sizeof( T ), alignof( T ) ) : 0;
    }

    template <class Exception, class F> inline void throws( F action, const char *message )
    {
        bool caught = false;
        try { action(); } catch( const Exception & ) { caught = true; }
        check( caught, message );
    }

    template <class T> inline void bodyAdmission( Budget &budget )
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, T> );
        static_assert( !std::is_copy_constructible_v<T> && !std::is_copy_assignable_v<T> );
        const auto baseline = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) );
        budget.set( baseline + bytes - 1 );
        denied( []{ auto memory = T::operator new( sizeof( T ) ); T::operator delete( memory ); } );
        check( budget.used() == baseline, "Denied L-system body owns no partial lease" );
        budget.set( baseline + bytes );
        auto release = []( void *memory ) { T::operator delete( memory ); };
        std::unique_ptr<void, decltype( release )> memory( T::operator new( sizeof( T ) ), release );
        check( budget.used() == baseline + bytes, "L-system body requires exact physical SourceCpu admission" );
        budget.set( budget.used() ); memory.reset();
        check( budget.used() == baseline, "L-system body storage retires at the full cap" );
        budget.set( budget.original );
    }

    class LateRule : public LRule
    {
    public:
        mutable int calls = 0;
        int failAt = 0;
        String getSuccessor() const override
        {
            if( ++calls == failAt )
            {
                Budget denial; denial.set( denial.used() );
                auto scratch = makeSourceBudgetSnapshot<u8>(); scratch.reserve( 1 );
            }
            return "FF";
        }
    };

    class CappedRule : public LRule
    {
    public:
        Budget *budget = nullptr;
        bool restrictOutput = false;
        String getSuccessor() const override
        {
            String result( m_successor );
            if( restrictOutput ) budget->set( budget->used() );
            return result;
        }
    };

    class CallbackRule : public LRule
    {
    public:
        enum class Action { Restart, Reset, Remove, Reenter, SwallowDenial };
        System *system = nullptr;
        Action action = Action::Restart;
        mutable bool fired = false;
        mutable bool nestedRejected = false;
        String getSuccessor() const override
        {
            if( !fired )
            {
                fired = true;
                switch( action )
                {
                case Action::Restart: system->setStart( "R" ); break;
                case Action::Reset: system->reset(); break;
                case Action::Remove:
                    system->removeRule( SmartPtr<ILSystemRule>( const_cast<CallbackRule *>( this ) ) );
                    break;
                case Action::Reenter:
                    try { system->getNextLevel(); }
                    catch( const std::logic_error & ) { nestedRejected = true; }
                    break;
                case Action::SwallowDenial:
                    {
                        Budget denial; denial.set( denial.used() );
                        try { auto scratch = makeSourceBudgetSnapshot<u8>(); scratch.reserve( 1 ); }
                        catch( const std::bad_alloc & ) {}
                    }
                    break;
                }
            }
            return "FF";
        }
    };

    class ObservingRule : public LRule
    {
    public:
        System *system = nullptr;
        int *destructions = nullptr;
        int *unloads = nullptr;
        bool *detached = nullptr;
        bool *failed = nullptr;
        ~ObservingRule() override
        {
            ++*destructions;
            try
            {
                *detached = system->detached();
                system->unload( nullptr );
            }
            catch( ... ) { *failed = true; }
        }
        void unload( SmartPtr<ISharedObject> data ) override
        {
            ++*unloads; LRule::unload( data );
        }
    };

    class ClearingRule : public ObservingRule
    {
    public:
        bool *inside = nullptr;
        bool *retiredInCallback = nullptr;
        ~ClearingRule() override { *retiredInCallback = *inside; }
        String getPredecessor() const override
        {
            *inside = true;
            system->unload( nullptr );
            *inside = false;
            return "F";
        }
    };

    inline void run()
    {
        Budget budget;
        bodyAdmission<LSystem>( budget ); bodyAdmission<LRule>( budget );
        {
            System system;
            auto rule = make_ptr<LRule>( "F", "FF" );
            const auto baseline = budget.used(); budget.set( baseline );
            denied( [&]{ system.addRule( rule ); } );
            denied( [&]{ system.addVariable( "F" ); } );
            denied( [&]{ system.addConstant( "+" ); } );
            check( system.ruleInventory().empty() && system.variableInventory().empty() &&
                   system.constantInventory().empty() && budget.used() == baseline,
                   "All three L-system inventories require admission before their first entry" );
            const auto ruleBytes = storageBytes<SmartPtr<ILSystemRule>>( 41 );
            budget.set( baseline + ruleBytes - 1 ); denied( [&]{ system.reserveRules( 41 ); } );
            budget.set( baseline + ruleBytes ); system.reserveRules( 41 ); system.addRule( rule );
            check( budget.used() == baseline + ruleBytes && system.ruleInventory().capacity() == 41,
                   "Rule inventory admits exact spare capacity and non-growing append at the cap" );
            const auto beforeVariables = budget.used();
            const auto variableBytes = storageBytes<String>( 47 );
            budget.set( beforeVariables + variableBytes - 1 ); denied( [&]{ system.reserveVariables( 47 ); } );
            budget.set( beforeVariables + variableBytes ); system.reserveVariables( 47 ); system.addVariable( "F" );
            const auto beforeConstants = budget.used();
            const auto constantBytes = storageBytes<String>( 53 );
            budget.set( beforeConstants + constantBytes - 1 ); denied( [&]{ system.reserveConstants( 53 ); } );
            budget.set( beforeConstants + constantBytes ); system.reserveConstants( 53 ); system.addConstant( "+" );
            const auto resident = budget.used(); budget.set( resident );
            denied( [&]{ auto snapshot = system.ruleInventory(); } );
            denied( [&]{ auto snapshot = system.variableInventory(); } );
            denied( [&]{ auto snapshot = system.constantInventory(); } );
            budget.set( resident + ruleBytes + variableBytes + constantBytes );
            auto rules = system.ruleInventory();
            auto variables = system.variableInventory();
            auto constants = system.constantInventory();
            check( rules.capacity() == 41 && variables.capacity() == 47 && constants.capacity() == 53 &&
                   rules.get_allocator().isSourceBudgeted() && variables.get_allocator().isSourceBudgeted() &&
                   constants.get_allocator().isSourceBudgeted(),
                   "Escaped inventory snapshots retain complete SourceCpu spare capacity" );
            const auto withSnapshots = budget.used(); budget.set( withSnapshots );
            const auto epoch = sourceBudgetDenialCount(); system.unload( nullptr );
            check( system.detached() && rules.size() == 1 && rules[0] == rule && variables[0] == "F" &&
                   constants[0] == "+" && budget.used() == withSnapshots - ruleBytes - variableBytes - constantBytes &&
                   sourceBudgetDenialCount() == epoch,
                   "Full-cap L-system unload retires physical inventories while caller snapshots survive" );
            budget.set( budget.original );
        }
        {
            System system;
            auto rule = make_ptr<LRule>( "F", "FF" );
            system.addRule( rule ); system.addRule( rule );
            system.addVariable( "F" ); system.addVariable( "F" );
            system.addConstant( "+" ); system.addConstant( "+" );
            const auto ruleCapacity = system.ruleInventory().capacity();
            const auto variableCapacity = system.variableInventory().capacity();
            const auto constantCapacity = system.constantInventory().capacity();
            const auto baseline = budget.used(); budget.set( baseline );
            system.removeRule( nullptr ); system.removeVariable( "absent" ); system.removeConstant( "absent" );
            denied( [&]{ system.removeRule( rule ); } );
            denied( [&]{ system.removeVariable( "F" ); } );
            denied( [&]{ system.removeConstant( "+" ); } );
            check( system.ruleInventory().size() == 2 && system.variableInventory().size() == 2 &&
                   system.constantInventory().size() == 2 && budget.used() == baseline,
                   "Denied inventory removal preserves every duplicate and its storage" );
            budget.set( budget.original );
            system.removeRule( rule ); system.removeVariable( "F" ); system.removeConstant( "+" );
            check( system.ruleInventory().empty() && system.variableInventory().empty() && system.constantInventory().empty() &&
                   system.ruleInventory().capacity() == ruleCapacity && system.variableInventory().capacity() == variableCapacity &&
                   system.constantInventory().capacity() == constantCapacity,
                   "Removal retires all matching grammar entries while retaining paid spare capacity" );
            budget.set( budget.used() ); system.unload( nullptr );
            check( system.detached(), "Empty grammar spare buffers retire at the full cap" );
            budget.set( budget.original );
        }
        {
            System system;
            system.setStart( "F" );
            system.addRule( nullptr );
            system.addRule( make_ptr<LRule>( "FX", "wrong" ) );
            system.addRule( make_ptr<LRule>( "F", "F+F" ) );
            system.addRule( make_ptr<LRule>( "F", "wrong" ) );
            system.addConstant( "F" );
            check( system.getNextLevel() == "F+F" && system.currentLevel() == 1 &&
                   system.getLevel( 2 ) == "F+F+F+F" && system.currentLevel() == 2,
                   "Rewriting preserves original one-byte first-rule semantics and informational alphabets" );
            const auto before = budget.used(); budget.set( before );
            check( system.getLevel( 0 ) == "F" && system.current() == "F" && system.currentLevel() == 0,
                   "Level zero returns the axiom without rule callbacks or inventory snapshots" );
            throws<std::invalid_argument>( [&]{ system.getLevel( -1 ); }, "Negative level rejects before mutation" );
            system.setCurrent( "last", std::numeric_limits<s32>::max() );
            const auto epoch = sourceBudgetDenialCount();
            throws<std::overflow_error>( [&]{ system.getNextLevel(); }, "Next-level counter overflow rejects before work" );
            check( system.current() == "last" && system.currentLevel() == std::numeric_limits<s32>::max() &&
                   sourceBudgetDenialCount() == epoch && budget.used() == before,
                   "Invalid level requests preserve prior state without allocation or signed overflow" );
            budget.set( budget.original );
            system.setStart( "new" );
            check( system.currentLevel() == 0 && system.current() == "new", "Changing the axiom restarts the current level" );
            system.getNextLevel(); system.reset();
            check( system.currentLevel() == 0 && system.current() == "new", "Reset restores the retained axiom and level zero" );
            const char embedded[] = { 'F', '\0', 'X' };
            system.setStart( String( embedded, sizeof( embedded ) ) );
            const auto result = system.getLevel( 1 );
            check( result.size() == 5 && result[0] == 'F' && result[1] == '+' && result[2] == 'F' &&
                   result[3] == '\0' && result[4] == 'X', "Whole-level return preserves embedded zero symbols" );
        }
        {
            System system;
            auto rule = make_ptr<LateRule>(); rule->setPredecessor( "F" );
            system.setStart( "F" ); system.addRule( rule );
            const auto baseline = budget.used();
            budget.set( baseline ); denied( [&]{ system.getNextLevel(); } );
            check( system.current() == "F" && system.currentLevel() == 0 && budget.used() == baseline,
                   "Denied owner snapshot leaves the prior generation intact" );
            budget.set( baseline + storageBytes<SmartPtr<ILSystemRule>>( system.ruleInventory().capacity() ) );
            denied( [&]{ system.getNextLevel(); } );
            check( !rule->calls && system.current() == "F" && !system.currentLevel() && budget.used() == baseline,
                   "Per-symbol rewrite scratch requires SourceCpu admission before rule callbacks" );
            budget.set( budget.original ); check( system.getNextLevel() == "FF", "Rewrite admission retries normally" );
            const auto completed = budget.used(); rule->calls = 0; rule->failAt = 2;
            denied( [&]{ system.getNextLevel(); } );
            check( system.current() == "FF" && system.currentLevel() == 1 && budget.used() == completed,
                   "Late successor denial retires earlier replacements without advancing the live level" );
            rule->calls = 0; rule->failAt = 3;
            denied( [&]{ system.getLevel( 3 ); } );
            check( system.current() == "FF" && system.currentLevel() == 1 && budget.used() == completed,
                   "Whole getLevel keeps earlier private generations unpublished when a later rewrite is denied" );
            rule->calls = 0; rule->failAt = 0;
            check( system.getLevel( 3 ) == "FFFFFFFF" && system.currentLevel() == 3,
                   "Multi-level rewrite retries from the axiom after late admission failure" );
        }
        {
            System system;
            auto rule = make_ptr<CappedRule>(); rule->setPredecessor( "F" ); rule->budget = &budget;
            // Oversized buffers bypass the retained character pool, making each
            // candidate/return allocation observable without trimming global pools.
            const String large( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 4096, 'X' );
            rule->setSuccessor( large ); system.setStart( "F" ); system.addRule( rule );
            const auto baseline = budget.used(); rule->restrictOutput = true;
            denied( [&]{ system.getNextLevel(); } );
            check( system.current() == "F" && !system.currentLevel() && budget.used() == baseline,
                   "Output-buffer denial after successful successor callbacks preserves the previous generation" );
            budget.set( budget.original ); rule->restrictOutput = false;
            auto result = system.getNextLevel();
            check( result == large && system.current() == large && system.currentLevel() == 1,
                   "Oversized rewritten output retries with independent caller and live strings" );
        }
        {
            System system;
            const String large( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 4096, 'A' );
            const auto beforeProbe = budget.used();
            u64 copyBytes = 0;
            { String probe( large ); copyBytes = budget.used() - beforeProbe; }
            check( copyBytes > 0 && budget.used() == beforeProbe, "Oversized string copies own independent physical buffers" );
            system.setStart( "old" ); system.setCurrent( "completed", 7 );
            auto baseline = budget.used(); budget.set( baseline + copyBytes );
            denied( [&]{ system.setStart( large ); } );
            check( system.axiom() == "old" && system.current() == "completed" && system.currentLevel() == 7 && budget.used() == baseline,
                   "Late second-axiom-copy denial preserves both stored strings and the completed counter" );
            budget.set( budget.original ); system.setStart( large ); system.setCurrent( "completed", 7 );
            baseline = budget.used(); budget.set( baseline );
            denied( [&]{ system.reset(); } );
            check( system.current() == "completed" && system.currentLevel() == 7, "Denied reset retains the previous generation" );
            budget.set( baseline + copyBytes ); denied( [&]{ system.getLevel( 0 ); } );
            check( system.current() == "completed" && system.currentLevel() == 7 && system.axiom() == large && budget.used() == baseline,
                   "Denied independent caller return copy precedes publication even for level zero" );
            budget.set( budget.original ); auto result = system.getLevel( 0 );
            check( result == large && system.current() == large && !system.currentLevel(), "Level-zero return-copy admission retries atomically" );
            Rule rule; rule.setPredecessor( "F" ); rule.setSuccessor( "old" );
            baseline = budget.used(); budget.set( baseline );
            denied( [&]{ rule.setPredecessor( large ); } ); denied( [&]{ rule.setSuccessor( large ); } );
            check( rule.predecessor() == "F" && rule.successor() == "old" && budget.used() == baseline,
                   "Rule text setters stage their complete character storage before publication" );
            budget.set( budget.original ); rule.setPredecessor( large ); rule.setSuccessor( large );
            auto snapshot = rule.getSuccessor();
            const auto beforeUnload = budget.used(); budget.set( beforeUnload );
            rule.unload( nullptr ); system.unload( nullptr );
            check( rule.predecessor().empty() && rule.successor().empty() && system.detached() && snapshot == large &&
                   budget.used() < beforeUnload, "Full-cap grammar teardown releases oversized strings while retained string snapshots survive" );
            budget.set( budget.original );
        }
        {
            for( const auto action : { CallbackRule::Action::Restart, CallbackRule::Action::Reset,
                                      CallbackRule::Action::Remove, CallbackRule::Action::Reenter,
                                      CallbackRule::Action::SwallowDenial } )
            {
                System system; system.setStart( "F" ); system.setCurrent( "FF", 1 );
                auto rule = make_ptr<CallbackRule>(); rule->setPredecessor( "F" ); rule->system = &system; rule->action = action;
                system.addRule( rule );
                if( action == CallbackRule::Action::SwallowDenial ) denied( [&]{ system.getNextLevel(); } );
                else throws<std::logic_error>( [&]{ system.getNextLevel(); }, "Callback mutation or reentry rejects stale publication" );
                if( action == CallbackRule::Action::Restart )
                    check( system.current() == "R" && system.axiom() == "R" && !system.currentLevel(),
                           "Callback-owned restart survives rejection of the old generation" );
                else if( action == CallbackRule::Action::Reset )
                    check( system.current() == "F" && !system.currentLevel(), "Callback-owned reset survives stale generation rejection" );
                else
                    check( system.current() == "FF" && system.currentLevel() == 1,
                           "Removed rules, swallowed denial and swallowed recursive evaluation cannot publish partial output" );
                if( action == CallbackRule::Action::Remove ) check( system.ruleInventory().empty(), "Callback removal remains committed" );
                if( action == CallbackRule::Action::Reenter ) check( rule->nestedRejected, "Recursive callback evaluation was rejected" );
                check( system.getNextLevel() == ( action == CallbackRule::Action::Restart ? "R" :
                        action == CallbackRule::Action::Reset ? "FF" : action == CallbackRule::Action::Remove ? "FF" : "FFFF" ),
                       "Evaluation guard permits a fresh retry after callback rejection" );
            }
        }
        {
            int destructions = 0, unloads = 0; bool detached = false, failed = false;
            System system; system.setStart( "F" ); system.addVariable( "F" ); system.addConstant( "+" );
            auto rule = make_ptr<ObservingRule>(); rule->system = &system; rule->destructions = &destructions;
            rule->unloads = &unloads; rule->detached = &detached; rule->failed = &failed;
            system.addRule( rule ); rule = nullptr;
            const auto baseline = budget.used(); budget.set( baseline );
            const auto epoch = sourceBudgetDenialCount(); system.unload( nullptr );
            check( destructions == 1 && !unloads && detached && !failed && system.detached() &&
                   sourceBudgetDenialCount() == epoch && budget.used() < baseline,
                   "Last-owner rule destruction observes complete full-cap detachment without child unload or admission" );
            budget.set( budget.original );
        }
        {
            int destructions = 0, unloads = 0; bool detached = false, failed = false, inside = false, retiredInside = false;
            System system; system.setStart( "F" );
            auto rule = make_ptr<ClearingRule>(); rule->system = &system; rule->destructions = &destructions;
            rule->unloads = &unloads; rule->detached = &detached; rule->failed = &failed;
            rule->inside = &inside; rule->retiredInCallback = &retiredInside;
            system.addRule( rule ); rule = nullptr;
            throws<std::logic_error>( [&]{ system.getNextLevel(); }, "Callback clear invalidates the pending rewrite" );
            check( system.detached() && destructions == 1 && !unloads && detached && !failed && !retiredInside,
                   "Immutable owner snapshot pins a clearing rule until its callback and stale rewrite unwind" );
            check( system.getNextLevel().empty() && system.currentLevel() == 1, "A cleared system can start a fresh empty generation" );
        }
        budget.set( budget.original );
    }
}
