#include <WPVehiclePhysics/VehicleHandling.hpp>
#include <cstdlib>
#include <iostream>

namespace
{
    void check( bool condition, const char *message )
    {
        if( !condition )
        {
            std::cerr << message << '\n';
            std::exit( 1 );
        }
    }
}  // namespace

int main()
{
    using namespace workphone::handling;
    // A key held for 0.1 s produces the same angle at 30, 60 and 120 Hz.
    for( const int hz : { 30, 60, 120 } )
    {
        double angle = 0;
        for( int tick = 0; tick < hz / 10; ++tick )
            angle = steering( 22, angle, 0, 3.6, 14, 90, 1.0 / hz );
        check( std::abs( angle - 9 ) < 1e-9, "Steering rate varies with update frequency" );
        check( steering( -22, angle, 0, 3.6, 14, 90, 1.0 / hz ) > -1,
               "Direction reversal snaps across full lock" );
    }
    check( steering( 22, 0, 80, 3.6, 14, 90, 1 ) < .5, "High-speed steering is excessive" );
    check( steering( 22, 0, 80, 3.6, 14, 0, 1 ) == 22, "Disabled assist alters legacy steering" );
    const auto straight = tractionTorque( 4000, 2000, 1.8, 1.25, 0, .36 );
    const auto corner = tractionTorque( 4000, 2000, 1.8, 1.25, 3500, .36 );
    check( straight > corner && corner > 0, "Cornering leaves no reduced drive budget" );
    check( tractionTorque( 4000, 0, 1.8, 1.25, 0, .36 ) == 0, "Unloaded tyre transmits torque" );
    check( tractionTorque( 4000, 2000, 1.8, .35, 3500, .36 ) == 0, "Surface grip ignored" );
    check( tractionTorque( -4000, 2000, 1.8, 1.25, 0, .36 ) == -straight, "Reverse torque sign lost" );
    check( std::abs( rollingLimit( 1000, 36, .36, .08 ) - 108 ) < 1e-9, "Wheelspin not limited" );
    check( std::abs( rollingLimit( -1000, -36, .36, .08 ) + 108 ) < 1e-9, "Reverse slip not limited" );
    check( std::abs( brushMagnitude( 3000 - 1e-3, 1000, 800 ) -
                     brushMagnitude( 3000 + 1e-3, 1000, 800 ) ) < .001,
           "Tyre loses grip abruptly at saturation" );
    check( brushMagnitude( 4000, 1000, 800 ) > brushMagnitude( 8000, 1000, 800 ),
           "Sliding grip does not transition progressively" );
    check(std::abs(slipLimitedTorque(4000, 113, 36, .36, 2, .02)) < 1e-9,
          "Traction control drives an already spinning tyre");
    check(slipLimitedTorque(4000, 100, 36, .36, 2, .02) > 0,
          "Traction control prevents acceleration at rolling speed");
    check(slipLimitedTorque(4000, 200, 36, .36, 2, .02) == 0,
          "Traction control invents braking torque");
    check(std::abs(brakeTorque(3000, 100, 36, .36, 2, .02) - 1200) < 1e-9,
          "ABS permits service brake lockup");
    check(std::abs(brakeTorque(3000, -100, -36, .36, 2, .02) - 1200) < 1e-9,
          "ABS changes with travel direction");
    check(brakeTorque(3000, 80, 36, .36, 2, .02) == 0,
          "ABS does not release a locking tyre");
    check(brakeTorque(3000, 2, .5, .36, 2, .02) == 3000,
          "ABS prevents stopping at walking speed");
    check( brushMagnitude( 100, 0, 0 ) == 0, "Zero load produces force" );
    std::cout << "Vehicle handling regression checks passed.\n";
}
