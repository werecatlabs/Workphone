#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CProceduralTexture.hpp>
#include <WPProcedural/ProceduralTexture.hpp>
#include <cmath>
#include <limits>

namespace procedural_texture_storage_contracts
{
    using namespace city_inventory_budget_contracts;
    class CountingRule : public TextureRule
    {
    public:
        mutable int calls = 0;
        mutable float lastU = 0, lastV = 0;
        Color4 sample( float u, float v ) const override
        {
            ++calls; lastU = u; lastV = v;
            check( std::isfinite( u ) && std::isfinite( v ), "Texture callbacks receive finite UVs" );
            return { 1, .5f, 0, 1 };
        }
    };
    class DenyingRule : public TextureRule
    {
    public:
        bool swallow = false;
        mutable int calls = 0;
        Color4 sample( float, float ) const override
        {
            if( ++calls == 2 )
            {
                try
                {
                    auto scratch = makeSourceBudgetSnapshot<char>();
                    scratch.reserve( 1 );
                }
                catch( const std::bad_alloc & ) { if( !swallow ) throw; }
            }
            return { 0, 0, 0, 1 };
        }
    };
    inline void run()
    {
        Budget budget;
        {
            CProceduralTexture texture;
            texture.setSize( 2, 2 );
            auto previous = texture.getData();
            Array<ColourF> foreign;
            foreign.reserve( 67 ); foreign.resize( 4 ); foreign[0] = ColourF::Red;
            const auto before = budget.used();
            const auto bytes = sourceBudgetStorageBytes( 67 * sizeof( ColourF ), alignof( ColourF ) );
            budget.set( before + bytes - 1 );
            denied( [&]{ texture.setData( foreign ); } );
            check( texture.getWidth() == 2 && texture.getHeight() == 2 && budget.used() == before,
                   "One-byte-short texture data import preserves dimensions and releases its candidate" );
            budget.set( before + bytes ); texture.setData( foreign );
            budget.set( budget.original );
            auto retained = texture.getData();
            check( retained.capacity() == 67 && retained.get_allocator().isSourceBudgeted() && retained[0] == ColourF::Red,
                   "Procedural texture imports and caller snapshots admit retained foreign capacity" );
            budget.set( budget.used() );
            denied( [&]{ texture.setSize( 4, 4 ); } );
            bool unsupported = false;
            try { texture.generate(); } catch( const std::logic_error & ) { unsupported = true; }
            check( unsupported && texture.getWidth() == 2 && texture.getHeight() == 2,
                   "Unconfigured generation reports unsupported operation without mutating CPU storage" );
            texture.unload( nullptr );
            auto empty = texture.getData();
            check( texture.getWidth() == 0 && texture.getHeight() == 0 && empty.empty() &&
                   empty.capacity() == 0 && empty.get_allocator().isSourceBudgeted() && retained[0] == ColourF::Red,
                   "Full-cap texture unload retires storage and preserves independently retained pixels" );
            budget.set( budget.original );
            Array<ColourF> spare;
            spare.reserve( 17 ); texture.setData( spare );
            budget.set( budget.used() ); texture.setSize( 0, 0 );
            check( texture.getData().capacity() == 0, "Zero-size reset retires even empty spare pixel storage" );
            budget.set( budget.original );
            bool invalid = false, overflow = false;
            try { texture.setSize( 0, 1 ); } catch( const std::invalid_argument & ) { invalid = true; }
            try { texture.setSize( std::numeric_limits<int>::max(), std::numeric_limits<int>::max() ); }
            catch( const std::length_error & ) { overflow = true; }
            check( invalid && overflow && texture.getWidth() == 0 && texture.getData().empty(),
                   "Invalid and overflowing texture dimensions are rejected before publication" );
        }
        {
            Array<Color4> foreign;
            foreign.reserve( 41 ); foreign.resize( 4 ); foreign[0] = { 0, 1, 0, 1 };
            Texture2D source{ 2, 2, foreign }, destination;
            const auto before = budget.used();
            const auto bytes = sourceBudgetStorageBytes( 41 * sizeof( Color4 ), alignof( Color4 ) );
            budget.set( before + bytes - 1 );
            denied( [&]{ destination = source; } );
            check( destination.width == 0 && destination.height == 0 && destination.pixels.empty() &&
                   budget.used() == before, "Denied complete texture assignment preserves descriptor and pixels" );
            budget.set( before + bytes ); destination = source;
            check( destination.width == 2 && destination.height == 2 && destination.pixels.capacity() == 41 &&
                   destination.pixels.get_allocator().isSourceBudgeted(), "Texture snapshots admit exact spare capacity" );
            budget.set( budget.used() );
            denied( []{ auto body = std::make_unique<Texture2D>(); } );
            denied( []{ auto body = std::make_unique<SolidColorRule>(); } );
            destination.clear();
            check( destination.width == 0 && destination.height == 0 && destination.pixels.capacity() == 0 &&
                   source.at( 0, 0 ) == Color4{ 0, 1, 0, 1 }, "Full-cap baked texture retirement leaves source pixels intact" );
            budget.set( budget.original );
        }
        {
            CountingRule rule;
            budget.set( budget.used() );
            denied( [&]{ TextureBaker::bake( rule, 16, 16 ); } );
            check( rule.calls == 0, "Texture pixel admission occurs before invoking any sampling callback" );
            auto tooLarge = TextureBaker::bake( rule, std::numeric_limits<int>::max(), std::numeric_limits<int>::max() );
            check( !tooLarge.isValid() && tooLarge.width == 0 && tooLarge.pixels.empty() && rule.calls == 0,
                   "Oversized baker requests return a coherent empty result before sampling" );
            budget.set( budget.original );
            auto previous = TextureBaker::bake( rule, 1, 1 );
            check( previous.isValid() && rule.calls == 1 && rule.lastU == .5f && rule.lastV == .5f &&
                   previous.pixels.get_allocator().isSourceBudgeted(), "Single-pixel baking keeps existing finite center-UV semantics" );
            const auto before = budget.used();
            const auto pixels = sourceBudgetStorageBytes( 4 * sizeof( Color4 ), alignof( Color4 ) );
            DenyingRule callback;
            budget.set( before + pixels );
            denied( [&]{ previous = TextureBaker::bake( callback, 2, 2 ); } );
            check( callback.calls == 2 && previous.isValid() && previous.width == 1 && budget.used() == before,
                   "Mid-bake callback denial retires candidate pixels and retains old output" );
            callback.calls = 0; callback.swallow = true;
            denied( [&]{ previous = TextureBaker::bake( callback, 2, 2 ); } );
            check( callback.calls == 4 && previous.width == 1 && budget.used() == before,
                   "Swallowed callback admission failure cannot publish a partially defaulted texture" );
            budget.set( budget.original );
            callback.calls = 0;
            auto complete = TextureBaker::bake( callback, 2, 2 );
            check( complete.isValid() && complete.pixels.size() == 4, "Baking retries after complete admission succeeds" );
        }
    }
}
