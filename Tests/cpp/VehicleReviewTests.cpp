#include "ReviewEvidence.h"
#include <filesystem>
#include <iostream>

int main()
{
    using namespace workphone::advanced;
    const auto path = std::filesystem::temp_directory_path() /
        ( "workphone-review-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) + ".csv" );
    try
    {
        if( !reviewPathsAlias( path.string(), ( path.parent_path() / "." / path.filename() ).string() ) ||
            reviewPathsAlias( "", path.string() ) ) throw std::runtime_error( "Review path alias protection failed." );
        if( jsonString( "a\"\\\n\t\x01" ) != "\"a\\\"\\\\\\n\\t\\u0001\"" )
            throw std::runtime_error( "JSON escaping failed." );
        auto check = [&]( const std::string &csv, bool valid ) {
            if( !writeReviewEvidence( path.string(), csv ) ) throw std::runtime_error( "Fixture write failed." );
            bool accepted = false;
            try { accepted = !loadReviewReplay( path.string() ).empty(); }
            catch( const std::runtime_error & ) {}
            if( accepted != valid ) throw std::runtime_error( "Replay acceptance mismatch: " + csv );
        };
        check( "tick,throttle,brake,steering\r\n0,1,0,-1\r\n1,0,.5,1\r\n", true );
        const auto controls = loadReviewReplay( path.string() );
        if( controls.size() != 2 || controls[0].steering != -1 || controls[1].brake != .5f )
            throw std::runtime_error( "Replay values changed." );
        check( "tick,throttle,brake,steering\n", false );
        check( "tick,throttle,brake,steering\n1,0,0,0\n", false );
        check( "tick,throttle,brake,steering\n0,1.1,0,0\n", false );
        check( "tick,throttle,brake,steering\n0,0,0,nan\n", false );
        check( "tick,throttle,brake,steering\n0,0,0,0,extra\n", false );
        check( "tick,throttle,brake,steering\n0,0,0,0\n0,0,0,0\n", false );
        check( "tick,throttle,brake,steering\n0;0;0;0\n", false );
        if( writeReviewEvidence( ( path / "missing.json" ).string(), "{}" ) )
            throw std::runtime_error( "Invalid output path accepted." );
        std::filesystem::remove( path );
        std::cout << "Vehicle review replay and evidence contracts passed\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::filesystem::remove( path );
        std::cerr << error.what() << '\n';
        return 1;
    }
}
