#pragma once
#include <Workphone/Interface/Procedural/VehicleGeneratorTypes.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <memory>
#include <stdexcept>

namespace vehicle_appearance_budget_contracts
{
    using namespace workphone;
    using namespace workphone::procedural;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
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
                     "vehicle appearance Source budget applies" );
        }
        void restore() { set( original ); }
        ~Budget() { restore(); }
    };
    template<class T> inline u64 bytes( size_t capacity )
    { return capacity ? sourceBudgetStorageBytes( capacity * sizeof(T), alignof(T) ) : 0; }
    inline u64 textBytes( const SourceBudgetStdString &text )
    {
        return text.capacity() > SourceBudgetStdString{}.capacity() ?
            bytes<char>( text.capacity() + 1 ) : 0;
    }
    template<class F> inline void denied( F operation, const char *message )
    {
        bool failed = false;
        try { operation(); } catch( const std::bad_alloc & ) { failed = true; }
        require( failed, message );
    }
    template<class T> inline void body( Budget &budget )
    {
        const auto baseline = budget.used();
        const auto cost = sourceBudgetStorageBytes( sizeof(T), alignof(std::max_align_t) );
        budget.set( baseline + cost - 1 );
        denied( [] { auto candidate = std::make_unique<T>(); }, "vehicle owning body requires complete admission" );
        require( budget.used() == baseline, "denied vehicle owning body leaves no lease" );
        budget.set( baseline + cost );
        auto owner = std::make_unique<T>();
        require( budget.used() == baseline + cost, "empty vehicle body uses exact physical admission" );
        budget.set( budget.used() ); owner.reset();
        require( budget.used() == baseline, "vehicle body can retire at the cap" );
    }

    inline void validation()
    {
        Budget budget;
        const auto baseline = budget.used();
        VehicleAppearanceValidation source;
        source.valid = false;
        source.issues.reserve( 7 );
        source.issues.emplace_back( VehicleAppearanceIssueSeverity::Error, "appearance.contract", "invalid material metadata" );
        source.issues[0].code.reserve( 261 );
        source.issues[0].message.reserve( 2027 );
        const auto outer = bytes<VehicleAppearanceIssue>( source.issues.capacity() );
        const auto total = outer + textBytes( source.issues[0].code ) + textBytes( source.issues[0].message );
        require( budget.used() == baseline + total,
                 "appearance issue inventory and both nested text capacities charge physical Source storage" );

        VehicleAppearanceValidation previous;
        previous.issues.reserve( 3 );
        previous.issues.emplace_back( VehicleAppearanceIssueSeverity::Warning, "old", "previous" );
        const auto resident = budget.used();
        const auto old = previous.issues.data();
        for( const auto headroom : { u64(0), outer, total - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { previous = source; }, "appearance validation replacement stages outer and nested storage" );
            require( previous.valid && previous.issues.data() == old && previous.issues[0].code == "old" &&
                         previous.issues[0].severity == VehicleAppearanceIssueSeverity::Warning && budget.used() == resident,
                     "partial issue-copy denial leaves previous validity, severity and text generation unchanged" );
        }
        budget.set( resident + total ); previous = source;
        require( !previous.valid && previous.issues.capacity() == source.issues.capacity() &&
                     previous.issues[0].code.capacity() == source.issues[0].code.capacity() &&
                     previous.issues[0].message.data() != source.issues[0].message.data(),
                 "appearance validation retry retains independent nested spare capacity" );
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
        source = {}; previous = {};
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "validation retirement frees every admitted nested allocation without replacement admission" );
    }

    inline void appearance()
    {
        Budget budget;
        const auto baseline = budget.used();
        VehicleAppearanceBundle source;
        source.quality = VehicleAppearanceQuality::Cinematic; source.seed = 73; source.contentHash = 997;
        source.materials.front().name = "first"; source.materials.front().name.reserve( 257 );
        source.materials.back().name = "last"; source.materials.back().name.reserve( 1025 );
        source.materials.front().roughness = .83f;
        source.textures.bodyLivery = TextureBuffer( 17, 19 ); source.textures.bodyLivery.pixels[0] = 113;
        source.textures.lightEmission = TextureBuffer( 23, 11 ); source.textures.lightEmission.pixels[0] = 77;
        const auto firstName = textBytes( source.materials.front().name );
        const auto pixelCost = bytes<u8>( source.textures.bodyLivery.pixels.capacity() ) +
                               bytes<u8>( source.textures.lightEmission.pixels.capacity() );
        const auto total = firstName + textBytes( source.materials.back().name ) + pixelCost;
        require( budget.used() == baseline + total, "appearance descriptor names and texture payloads share Source admission" );

        VehicleAppearanceBundle previous;
        previous.quality = VehicleAppearanceQuality::Preview; previous.seed = 7; previous.contentHash = 9;
        previous.materials.front().name = "old"; previous.materials.front().roughness = .22f;
        previous.textures.bodyLivery = TextureBuffer( 1, 1 ); previous.textures.bodyLivery.pixels[0] = 42;
        const auto resident = budget.used(); const auto old = previous.textures.bodyLivery.data();
        for( const auto headroom : { u64(0), firstName, total - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { previous = source; }, "appearance bundle replacement requires complete descriptor and texture admission" );
            require( previous.quality == VehicleAppearanceQuality::Preview && previous.seed == 7 && previous.contentHash == 9 &&
                         previous.materials.front().name == "old" && previous.materials.front().roughness == .22f &&
                         previous.textures.bodyLivery.data() == old && previous.textures.bodyLivery.width == 1 &&
                         previous.textures.bodyLivery.pixels[0] == 42 && budget.used() == resident,
                     "late appearance copy denial preserves complete previous descriptors, dimensions, pixels and hash" );
        }
        budget.set( resident + total ); previous = source;
        require( previous.contentHash == 997 && previous.seed == 73 && previous.materials.back().name.capacity() ==
                     source.materials.back().name.capacity() && previous.textures.lightEmission.pixels[0] == 77 &&
                     previous.textures.bodyLivery.data() != source.textures.bodyLivery.data(),
                 "complete appearance retry preserves metadata and independent pixel owners" );
        budget.set( budget.used() ); source = {}; previous = {};
        require( budget.used() == baseline, "appearance bundle reset retires names and all texture capacities at the cap" );

        budget.restore();
        VehicleTextureAssets textures;
        textures.bodyLivery = TextureBuffer( 17, 19 ); textures.lightEmission = TextureBuffer( 23, 11 );
        VehicleTextureAssets oldTextures; oldTextures.bodyLivery = TextureBuffer( 1, 1 );
        oldTextures.bodyLivery.pixels[0] = 39;
        const auto beforeTextures = budget.used(); const auto oldPixels = oldTextures.bodyLivery.data();
        budget.set( beforeTextures + pixelCost - 1 );
        denied( [&] { oldTextures = textures; }, "texture-set replacement stages the last map before publishing the first" );
        require( oldTextures.bodyLivery.data() == oldPixels && oldTextures.bodyLivery.width == 1 &&
                     oldTextures.bodyLivery.pixels[0] == 39 && oldTextures.lightEmission.pixels.empty() && budget.used() == beforeTextures,
                 "partial texture-set copy leaves the whole previous set intact" );
        budget.set( budget.used() ); textures = {}; oldTextures = {};
        require( budget.used() == baseline, "texture sets retire all admitted buffers at a full Source limit" );
        body<VehicleMaterialDescriptor>( budget ); body<VehicleTextureAssets>( budget );
        body<VehicleAppearanceIssue>( budget ); body<VehicleAppearanceValidation>( budget ); body<VehicleAppearanceBundle>( budget );
    }

    inline void generatedVehicle()
    {
        Budget budget;
        const auto baseline = budget.used();
        GeneratedVehicle source;
        source.geometry.lods.reserve( 3 ); source.appearance.textures.bodyLivery = TextureBuffer( 17, 19 );
        source.appearance.textures.bodyLivery.pixels[0] = 99;
        source.physics.massElements.reserve( 5 ); source.physics.drivetrain.forwardGearRatios.reserve( 9 );
        source.physics.seed = 73; source.contentHash = 997;
        source.issues.reserve( 13 ); source.issues.push_back( { true, "geometry", "generated metadata contract" } );
        source.issues[0].component.reserve( 87 ); source.issues[0].message.reserve( 257 );
        const auto geometryCost = bytes<VehicleLOD>( source.geometry.lods.capacity() );
        const auto appearanceCost = bytes<u8>( source.appearance.textures.bodyLivery.pixels.capacity() );
        const auto total = geometryCost + appearanceCost + bytes<VehicleMassElement>( source.physics.massElements.capacity() ) +
            bytes<double>( source.physics.drivetrain.forwardGearRatios.capacity() ) +
            bytes<VehicleGenerationIssue>( source.issues.capacity() ) + textBytes( source.issues[0].component ) +
            textBytes( source.issues[0].message );
        require( budget.used() == baseline + total, "generated vehicle owns admitted geometry, pixels, physics and nested issue capacities" );

        GeneratedVehicle previous; previous.geometry.lods.reserve( 1 ); previous.physics.seed = 7; previous.contentHash = 9;
        previous.appearance.textures.bodyLivery = TextureBuffer( 1, 1 ); previous.appearance.textures.bodyLivery.pixels[0] = 39;
        previous.issues.reserve( 2 ); previous.issues.push_back( { false, "old", "previous" } );
        const auto resident = budget.used(); const auto oldGeometry = previous.geometry.lods.data();
        const auto oldPixels = previous.appearance.textures.bodyLivery.data(); const auto oldIssues = previous.issues.data();
        for( const auto headroom : { u64(0), geometryCost, geometryCost + appearanceCost, total - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { previous = source; }, "generated replacement must admit all nested families before publication" );
            require( previous.geometry.lods.data() == oldGeometry && previous.physics.seed == 7 && previous.contentHash == 9 &&
                         previous.appearance.textures.bodyLivery.data() == oldPixels && previous.appearance.textures.bodyLivery.pixels[0] == 39 &&
                         previous.issues.data() == oldIssues && previous.issues[0].component == "old" && budget.used() == resident,
                     "multi-family generation copy denial leaves the complete previous vehicle unchanged" );
        }
        budget.set( resident + total ); previous = source;
        require( previous.contentHash == 997 && previous.physics.seed == 73 && previous.issues.capacity() == 13 &&
                     previous.issues.get_allocator().isSourceBudgeted() && previous.appearance.textures.bodyLivery.data() !=
                         source.appearance.textures.bodyLivery.data(),
                 "generated retry retains independently admitted nested owner generations" );
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount(); source = {};
        require( previous.appearance.textures.bodyLivery.pixels[0] == 99 && budget.used() == baseline + total,
                 "generated snapshot remains valid after producer retirement" );
        previous = {};
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "full-cap generated retirement admits no replacement storage" );

        budget.restore(); Array<VehicleGenerationIssue> foreign; foreign.reserve( 17 ); foreign.push_back( { false, "x", "y" } );
        source.issues = foreign; const auto external = budget.used();
        const auto imported = bytes<VehicleGenerationIssue>( foreign.capacity() );
        budget.set( external + imported - 1 );
        denied( [&] { GeneratedVehicle candidate( source ); }, "foreign issue spare capacity cannot bypass generated copy admission" );
        require( budget.used() == external, "foreign issue copy denial unwinds all candidate ownership" );
        budget.set( external + imported ); GeneratedVehicle canonical( source );
        require( canonical.issues.get_allocator().isSourceBudgeted() && canonical.issues.capacity() == foreign.capacity() &&
                     canonical.issues[0].component == "x" && budget.used() == external + imported,
                 "generated copy canonicalizes foreign issue Arrays without changing diagnostic values" );
        budget.set( budget.used() ); source = {}; canonical = {};
        require( budget.used() == baseline, "foreign import owner retires its admitted physical capacity" );
        body<VehicleGenerationIssue>( budget ); body<GeneratedVehicle>( budget );
    }

    inline void run() { validation(); appearance(); generatedVehicle(); }
}
