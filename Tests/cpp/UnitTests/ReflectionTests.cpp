#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <type_traits>
#include <tuple>

struct Person
{
    std::string name;
    int age;
};

template <typename T>
constexpr auto reflect()
{
    if constexpr( std::is_same_v<T, Person> )
    {
        return std::tuple{ std::make_pair( std::string_view( "name" ), &Person::name ),
                           std::make_pair( std::string_view( "age" ), &Person::age ) };
    }
}

template <typename T>
void print( const T &obj )
{
    /*
    auto reflection = reflect<T>();
    for( const auto &[name, member_ptr] : reflection )
    {
        std::cout << name << ": ";
        if constexpr( std::is_same_v<decltype( *member_ptr ), std::string> )
        {
            std::cout << ( obj.*member_ptr ) << std::endl;
        }
        else if constexpr( std::is_same_v<decltype( *member_ptr ), int> )
        {
            std::cout << ( obj.*member_ptr ) << std::endl;
        }
    }
    */
}

//int main()
//{
//    Person p{ "John Doe", 30 };
//    print( p );
//    return 0;
//}

BOOST_AUTO_TEST_CASE( reflection )
{
    using namespace workphone;

    //data::actor_data data;
    //data.base_id = 1;
    //data.actorId = 123456;

    //auto dataStr = DataUtil::toString( &data );
    //BOOST_CHECK( !StringUtil::isNullOrEmpty( dataStr ) );

    //auto properties = PropertiesUtil::getProperties( &data );
    //BOOST_CHECK( properties->getPropertyAsInt( "actorId" ) == 123456 );

    //properties->setProperty( "actorId", 654321 );
    //BOOST_CHECK( properties->getPropertyAsInt( "actorId" ) == 654321 );

    //PropertiesUtil::fromProperties( properties, &data );
    //BOOST_CHECK( data.actorId == 654321 );
}
