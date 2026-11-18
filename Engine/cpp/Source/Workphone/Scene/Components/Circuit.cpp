#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Circuit.hpp>

namespace workphone::scene::race
{

    void Circuit::swap( Circuit &other ) noexcept
    {
        samples.swap( other.samples );
        std::swap( length, other.length );
    }

    workphone::scene::race::Circuit &Circuit::operator=( const Circuit &other )
    {
        if( this != &other )
        {
            Circuit candidate( other );
            swap( candidate );
        }
        return *this;
    }

    Circuit::Circuit() = default;

    Circuit::Circuit( const Circuit & ) = default;

    Circuit::Circuit( Circuit && ) noexcept = default;

    workphone::scene::race::Circuit &Circuit::operator=( Circuit && ) noexcept = default;

}  // namespace workphone::scene::race
