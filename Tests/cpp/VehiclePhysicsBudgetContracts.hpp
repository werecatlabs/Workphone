#pragma once
#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>
#include <array>
#include <memory>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace vehicle_physics_budget_contracts
{
    using namespace workphone;
    using namespace workphone::procedural;
    inline void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error( message ); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "vehicle physics Source budget applies" );
        }
        void restore() { set( original ); }
        ~Budget() { restore(); }
    };
    template<class T> inline u64 bytes( size_t count )
    { return count ? sourceBudgetStorageBytes( count * sizeof(T), alignof(T) ) : 0; }
    inline u64 characters( const SourceBudgetStdString &text )
    {
        // Every string exercised here has explicitly reserved non-inline space.
        return bytes<char>( text.capacity() + 1 );
    }
    template<class F> inline void denied( F operation, const char *message )
    {
        bool rejected = false;
        try { operation(); } catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected, message );
    }
    template<class T> inline void heapBody( Budget &budget )
    {
        const auto baseline = budget.used();
        const auto body = sourceBudgetStorageBytes( sizeof(T), alignof(std::max_align_t) );
        budget.set( baseline + body - 1 );
        denied( [] { auto value = std::make_unique<T>(); }, "owning physics heap body requires admission" );
        require( budget.used() == baseline, "denied physics heap body leaves no charge" );
        budget.set( baseline + body );
        auto value = std::make_unique<T>();
        require( budget.used() == baseline + body, "owning physics heap body has an exact physical charge" );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        value.reset();
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "physics body can retire at a full Source cap" );
    }

    inline void nestedOwnership()
    {
        Budget budget;
        const auto baseline = budget.used();
        VehiclePhysicsConfig input;
        input.seed = 73;
        input.preset = VehiclePhysicsPreset::GT;
        input.wheelbaseM = 2.78;
        input.massElements.reserve( 5 );
        input.massElements.emplace_back();
        input.massElements.front().massKg = 350;
        input.massElements.front().centre = { 1, 2, 3 };
        std::string foreignName;
        foreignName.reserve( 4097 ); foreignName.assign( 65, 'm' );
        input.massElements.front().name = std::move( foreignName );
        std::vector<double> foreignGears;
        foreignGears.reserve( 29 ); foreignGears = { 3.1, 2.2, 1.4 };
        input.drivetrain.forwardGearRatios = std::move( foreignGears );
        input.drivetrain.redlineRpm = 9000;
        const auto massSlots = bytes<VehicleMassElement>( input.massElements.capacity() );
        const auto nameBytes = characters( input.massElements.front().name );
        const auto gears = bytes<double>( input.drivetrain.forwardGearRatios.capacity() );
        const auto incoming = massSlots + nameBytes + gears;
        require( budget.used() == baseline + incoming && foreignName.size() == 65 &&
                     foreignGears.size() == 3 && input.massElements.front().name.data() != foreignName.data() &&
                     input.drivetrain.forwardGearRatios.data() != foreignGears.data(),
                 "mass slots, nested name spare capacity and foreign gear capacity have independent Source ownership" );

        VehiclePhysicsConfig output;
        output.seed = 11; output.preset = VehiclePhysicsPreset::RoadSport; output.wheelbaseM = 2.1;
        output.massElements.reserve( 3 ); output.massElements.emplace_back();
        output.massElements.front().name = "old"; output.massElements.front().massKg = 71;
        output.drivetrain.forwardGearRatios.reserve( 2 ); output.drivetrain.forwardGearRatios.push_back( 7 );
        const auto oldSlots = output.massElements.data();
        const auto oldGears = output.drivetrain.forwardGearRatios.data();
        const auto resident = budget.used();
        for( const auto headroom : { u64(0), massSlots, massSlots + nameBytes, incoming - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { output = input; }, "whole physics replacement stages every nested owner before publication" );
            require( budget.used() == resident && output.seed == 11 &&
                         output.preset == VehiclePhysicsPreset::RoadSport && output.wheelbaseM == 2.1 &&
                         output.massElements.data() == oldSlots && output.massElements.front().name == "old" &&
                         output.massElements.front().massKg == 71 &&
                         output.drivetrain.forwardGearRatios.data() == oldGears &&
                         output.drivetrain.forwardGearRatios.front() == 7,
                     "partial physics copy denial preserves scalars, named mass inventory and drivetrain" );
        }
        budget.set( resident + incoming ); output = input;
        require( output.seed == 73 && output.preset == VehiclePhysicsPreset::GT &&
                     output.massElements.capacity() == 5 && output.massElements.front().name.capacity() == foreignName.capacity() &&
                     output.drivetrain.forwardGearRatios.capacity() == 29 &&
                     output.massElements.data() != input.massElements.data() &&
                     output.massElements.front().name.data() != input.massElements.front().name.data(),
                 "admitted physics retry retains complete spare capacity with independent nested names" );
        budget.restore();
        std::optional<SourceBudgetVector<VehicleMassElement>> retained( output.massElements );
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
        output = {}; input = {};
        require( budget.used() == baseline + massSlots + nameBytes && retained->front().massKg == 350 &&
                     retained->front().name.front() == 'm',
                 "escaped mass inventory retains names beyond both config generations" );
        std::thread finalOwner( [owner = std::move( retained )]() mutable { owner.reset(); } );
        finalOwner.join(); retained.reset();
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "cross-thread final physics snapshot retirement admits no new storage" );

        heapBody<VehicleMassElement>( budget );
        heapBody<VehicleDrivetrainConfig>( budget );
        heapBody<VehiclePhysicsConfig>( budget );
        heapBody<VehiclePhysicsValidation>( budget );
    }

    inline void diagnostics()
    {
        Budget budget;
        const auto baseline = budget.used();
        VehiclePhysicsValidation input;
        input.errors.reserve( 13 ); input.warnings.reserve( 17 );
        input.errors.emplace_back( 129, 'e' ); input.errors.front().reserve( 1025 );
        input.warnings.emplace_back( 257, 'w' ); input.warnings.front().reserve( 2049 );
        const auto errors = bytes<SourceBudgetStdString>( input.errors.capacity() );
        const auto errorText = characters( input.errors.front() );
        const auto warnings = bytes<SourceBudgetStdString>( input.warnings.capacity() );
        const auto warningText = characters( input.warnings.front() );
        const auto incoming = errors + errorText + warnings + warningText;
        require( budget.used() == baseline + incoming, "physics diagnostics charge arrays and nested character spare capacity" );
        VehiclePhysicsValidation output;
        output.errors.emplace_back( "old error" ); output.warnings.emplace_back( "old warning" );
        const auto oldErrors = output.errors.data(), oldWarnings = output.warnings.data();
        const auto resident = budget.used();
        for( const auto headroom : { u64(0), errors, errors + errorText, incoming - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { output = input; }, "diagnostic replacement stages both complete string generations" );
            require( budget.used() == resident && output.errors.data() == oldErrors && output.warnings.data() == oldWarnings &&
                         output.errors.front() == "old error" && output.warnings.front() == "old warning",
                     "diagnostic copy denial preserves both previous inventories and messages" );
        }
        budget.set( resident + incoming ); output = input;
        require( !output.isValid() && output.errors.capacity() == 13 && output.warnings.capacity() == 17 &&
                     output.errors.front().capacity() == input.errors.front().capacity() &&
                     output.warnings.front().data() != input.warnings.front().data(),
                 "physics diagnostic retry owns independent complete spare capacity" );
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
        input = {}; output = {};
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "diagnostic generations retire at the cap without replacement admission" );
    }

    inline void generation()
    {
        Budget budget;
        const auto baseline = budget.used();
        const VehiclePhysicsPreset presets[] = { VehiclePhysicsPreset::GrandPrix, VehiclePhysicsPreset::GT,
            VehiclePhysicsPreset::RoadSport, VehiclePhysicsPreset::HelicopterLittleBird, VehiclePhysicsPreset::Warbird };
        const double masses[] = { 798, 1325, 1480, 900, 3400 };
        for( size_t i = 0; i < std::size( presets ); ++i )
        {
            budget.restore();
            auto config = WPVehiclePhysics::generate( presets[i], 73 );
            auto repeat = WPVehiclePhysics::generate( presets[i], 73 );
            require( config.massProperties.massKg == masses[i] && config.massElements.size() == repeat.massElements.size() &&
                         config.massProperties.inertia.yy == repeat.massProperties.inertia.yy &&
                         config.wheels[0].suspension.springRateNPerM == repeat.wheels[0].suspension.springRateNPerM,
                     "physics Source ownership preserves all five preset masses, numerical setup and determinism" );
            auto validation = WPVehiclePhysics::validate( config );
            require( validation.isValid(), "all five physics presets retain their existing validation semantics" );
            validation = {}; repeat = {};
            const auto resident = budget.used(); const auto denials = sourceBudgetDenialCount();
            budget.set( resident );
            if( i < 3 )
            {
                auto readOnly = WPVehiclePhysics::validate( config );
                require( readOnly.isValid() && readOnly.warnings.empty() && budget.used() == resident &&
                             sourceBudgetDenialCount() == denials,
                         "valid road-vehicle checks need no deep config snapshot or Source allocation at the cap" );
            }
            else
            {
                denied( [&] { auto warning = WPVehiclePhysics::validate( config ); },
                        "aircraft warning strings and inventories require Source admission" );
                require( budget.used() == resident && config.massProperties.massKg == masses[i],
                         "denied aircraft warning generation unwinds without mutating physics" );
                budget.restore(); auto retry = WPVehiclePhysics::validate( config );
                require( retry.isValid() && retry.warnings.size() == 1,
                         "aircraft metadata warning retries when Source headroom returns" );
            }
            config = {};
            require( budget.used() == baseline, "physics preset retirement releases all named mass and ratio storage" );
            size_t rejected = 0, admitted = 0;
            for( const auto headroom : { u64(0), u64(128), u64(512), u64(1024), u64(4096), u64(16384) } )
            {
                budget.set( baseline + headroom );
                std::optional<VehiclePhysicsConfig> candidate;
                bool failure = false;
                try { candidate.emplace( WPVehiclePhysics::generate( presets[i], 73 ) ); }
                catch( const std::bad_alloc & ) { failure = true; }
                require( budget.used() <= baseline + headroom,
                         "physics generator cannot exceed admitted physical capacity during constructor staging" );
                if( failure ) ++rejected; else ++admitted;
                budget.set( budget.used() ); const auto beforeRetire = sourceBudgetDenialCount();
                candidate.reset();
                require( budget.used() == baseline && sourceBudgetDenialCount() == beforeRetire,
                         "denied and admitted physics producer attempts both retire complete physical capacity" );
            }
            require( rejected && admitted, "each physics preset exercises denied and admitted generation" );
        }
    }

    template<class T> inline void errorInventory( Budget &budget )
    {
        const auto baseline = budget.used();
        T input;
        input.errors.reserve( 11 ); input.errors.emplace_back( 65, 'e' );
        input.errors.front().reserve( 1025 );
        const auto slots = bytes<SourceBudgetStdString>( input.errors.capacity() );
        const auto text = characters( input.errors.front() );
        require( budget.used() == baseline + slots + text,
                 "vehicle validation inventory charges its array and nested character spare capacity" );
        T output; output.errors.emplace_back( "old" );
        const auto old = output.errors.data(); const auto resident = budget.used();
        for( const auto headroom : { u64(0), slots, slots + text - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { output = input; }, "validation copy stages nested errors before publication" );
            require( budget.used() == resident && output.errors.data() == old && output.errors.front() == "old",
                     "validation denial preserves the complete old diagnostic generation" );
        }
        budget.set( resident + slots + text ); output = input;
        require( output.errors.capacity() == 11 && output.errors.front().capacity() == input.errors.front().capacity() &&
                     output.errors.front().data() != input.errors.front().data(),
                 "validation retry retains independent character and array spare capacity" );
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
        input = {}; output = {};
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "vehicle validation retirement requires no replacement admission" );
        heapBody<T>( budget );
        budget.restore();
    }

    inline void relatedValidation()
    {
        Budget budget;
        const auto baseline = budget.used();
        errorInventory<VehicleDynamicsValidation>( budget );
        errorInventory<VehicleDamageValidation>( budget );
        auto physics = WPVehiclePhysics::generate( VehiclePhysicsPreset::GrandPrix, 73 );
        const auto resident = budget.used();
        budget.set( resident ); const auto denials = sourceBudgetDenialCount();
        auto dynamics = WPVehicleDynamics::validate( physics, {} );
        auto damage = WPVehicleDamage::validate( {} );
        auto state = WPVehicleDamage::reset( 73 );
        auto stateValidation = WPVehicleDamage::validateState( state );
        require( dynamics.isValid() && damage.isValid() && stateValidation.isValid() &&
                     budget.used() == resident && sourceBudgetDenialCount() == denials,
                 "valid dynamics, damage config and damage state checks work at the full Source cap" );
        VehicleDynamicsTuning invalidTuning; invalidTuning.shiftDurationSeconds = -1;
        denied( [&] { auto result = WPVehicleDynamics::validate( physics, invalidTuning ); },
                "dynamics diagnostic storage requires Source admission" );
        VehicleDamageConfig invalidDamage; invalidDamage.minimumSeverity = -1;
        const auto oldRandom = state.randomState;
        denied( [&] { WPVehicleDamage::update( invalidDamage, .01, state ); },
                "damage diagnostic storage requires Source admission before state consumption" );
        require( state.damage == 0 && state.randomState == oldRandom && budget.used() == resident,
                 "denied damage diagnostic leaves simulation state and all previous Source owners unchanged" );
        state.damage = 2;
        denied( [&] { auto result = WPVehicleDamage::validateState( state ); },
                "invalid damage-state diagnostics cannot bypass Source quota" );
        budget.restore();
        dynamics = WPVehicleDynamics::validate( physics, invalidTuning );
        damage = WPVehicleDamage::validate( invalidDamage );
        stateValidation = WPVehicleDamage::validateState( state );
        require( !dynamics.isValid() && !damage.isValid() && !stateValidation.isValid() &&
                     dynamics.errors.front().find( "shift duration" ) != SourceBudgetStdString::npos &&
                     damage.errors.front().find( "thresholds" ) != SourceBudgetStdString::npos,
                 "dynamics and damage errors retry with their original diagnostic text" );
        physics.massProperties.massKg = 0;
        auto propagated = WPVehicleDynamics::validate( physics, {} );
        require( !propagated.isValid() && propagated.errors.front().find( "physics: " ) == 0,
                 "physics-to-dynamics error decoration remains Source owned" );
        budget.set( budget.used() ); const auto beforeRetire = sourceBudgetDenialCount();
        dynamics = {}; damage = {}; stateValidation = {}; propagated = {}; physics = {};
        require( budget.used() == baseline && sourceBudgetDenialCount() == beforeRetire,
                 "all physics, dynamics and damage diagnostic producers retire at the cap" );
    }
    inline void run() { nestedOwnership(); diagnostics(); generation(); relatedValidation(); }
}
