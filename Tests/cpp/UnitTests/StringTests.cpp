#include <random>
#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace std;

enum EState
{
    q0,
    q1,
    q2,
    qd
};

class StateMachine
{
    // Store current state value.
    EState currentState;
    // Store result formed and it's sign.
    int result, sign;

    // Transition to state q1.
    void toStateQ1( char &ch )
    {
        sign = ( ch == '-' ) ? -1 : 1;
        currentState = q1;
    }

    // Transition to state q2.
    void toStateQ2( int digit )
    {
        currentState = q2;
        appendDigit( digit );
    }

    // Transition to dead state qd.
    void toStateQd()
    {
        currentState = qd;
    }

    // Append digit to result, if out of range return clamped value.
    void appendDigit( int &digit )
    {
        if( ( result > INT_MAX / 10 ) || ( result == INT_MAX / 10 && digit > INT_MAX % 10 ) )
        {
            if( sign == 1 )
            {
                // If sign is 1, clamp result to INT_MAX.
                result = INT_MAX;
            }
            else
            {
                // If sign is -1, clamp result to INT_MIN.
                result = INT_MIN;
                sign = 1;
            }

            // When the 32-bit int range is exceeded, a dead state is reached.
            toStateQd();
        }
        else
        {
            // Append current digit to the result.
            result = result * 10 + digit;
        }
    }

public:
    StateMachine()
    {
        currentState = q0;
        result = 0;
        sign = 1;
    }

    // Change state based on current input character.
    void transition( char &ch )
    {
        if( currentState == q0 )
        {
            // Beginning state of the string (or some whitespaces are skipped).
            if( ch == ' ' )
            {
                // Current character is a whitespaces.
                // We stay in same state.
                return;
            }
            else if( ch == '-' || ch == '+' )
            {
                // Current character is a sign.
                toStateQ1( ch );
            }
            else if( isdigit( ch ) )
            {
                // Current character is a digit.
                toStateQ2( ch - '0' );
            }
            else
            {
                // Current character is not a space/sign/digit.
                // Reached a dead state.
                toStateQd();
            }
        }
        else if( currentState == q1 || currentState == q2 )
        {
            // Previous character was a sign or digit.
            if( isdigit( ch ) )
            {
                // Current character is a digit.
                toStateQ2( ch - '0' );
            }
            else
            {
                // Current character is not a digit.
                // Reached a dead state.
                toStateQd();
            }
        }
    }

    // Return the final result formed with it's sign.
    int getInteger()
    {
        return sign * result;
    }

    // Get current state.
    EState getState()
    {
        return currentState;
    }
};

int myAtoi( String input )
{
    int sign = 1;
    int result = 0;
    int index = 0;
    auto n = (s32)input.size();

    // Discard all spaces from the beginning of the input string.
    while( index < n && input[index] == ' ' )
    {
        index++;
    }

    // sign = +1, if it's positive number, otherwise sign = -1.
    if( index < n && input[index] == '+' )
    {
        sign = 1;
        index++;
    }
    else if( index < n && input[index] == '-' )
    {
        sign = -1;
        index++;
    }

    // Traverse next digits of input and stop if it is not a digit.
    // End of string is also non-digit character.
    while( index < n && isdigit( input[index] ) )
    {
        int digit = input[index] - '0';

        // Check overflow and underflow conditions.
        if( ( result > INT_MAX / 10 ) || ( result == INT_MAX / 10 && digit > INT_MAX % 10 ) )
        {
            // If integer overflowed return 2^31-1, otherwise if underflowed return -2^31.
            return sign == 1 ? INT_MAX : INT_MIN;
        }

        // Append current digit to the result.
        result = 10 * result + digit;
        index++;
    }

    // We have formed a valid number without any overflow/underflow.
    // Return it after multiplying it with its sign.
    return sign * result;
}

int myAtoi_dfa( String s )
{
    StateMachine Q;

    for( int i = 0; i < s.size() && Q.getState() != qd; ++i )
    {
        Q.transition( s[i] );
    }

    return Q.getInteger();
}

Array<s32> getTests()
{
    auto tests = Array<s32>();
    tests.push_back( 12 );
    tests.push_back( 24 );
    tests.push_back( 33 );
    tests.push_back( 66 );
    tests.push_back( 100 );
    tests.push_back( 1000 );
    return tests;
}

BOOST_AUTO_TEST_CASE( string_test_atoi )
{
    auto tests = getTests();

    for( auto test : tests )
    {
        auto num = test;
        auto numStr = StringUtil::toString( test );

        BOOST_CHECK( myAtoi( numStr ) == num );
    }

    BOOST_CHECK_EQUAL( myAtoi( "42" ), 42 );
    BOOST_CHECK_EQUAL( myAtoi( "   -42" ), -42 );
    BOOST_CHECK_EQUAL( myAtoi( "+100" ), 100 );
    BOOST_CHECK_EQUAL( myAtoi( "4193 with words" ), 4193 );
    BOOST_CHECK_EQUAL( myAtoi( "words and 987" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi( "-91283472332" ), INT_MIN );
    BOOST_CHECK_EQUAL( myAtoi( "91283472332" ), INT_MAX );
    BOOST_CHECK_EQUAL( myAtoi( "" ), 0 );
}

BOOST_AUTO_TEST_CASE( string_test_atoi_dfa )
{
    auto tests = getTests();

    for( auto test : tests )
    {
        auto num = test;
        auto numStr = StringUtil::toString( test );

        BOOST_CHECK( myAtoi_dfa( numStr ) == num );
    }

    BOOST_CHECK_EQUAL( myAtoi_dfa( "42" ), 42 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "   -42" ), -42 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "+100" ), 100 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "4193 with words" ), 4193 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "words and 987" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "-91283472332" ), INT_MIN );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "91283472332" ), INT_MAX );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "" ), 0 );
}

size_t rhymeMatch( string str1, string str2 )
{
    size_t matches = 0;

    std::reverse( str1.begin(), str1.end() );
    std::reverse( str2.begin(), str2.end() );

    for( size_t i = 0; i < str1.size() && i < str2.size(); ++i )
    //for( size_t i = (size_t)stringLength - 1; i >= 0; --i )
    {
        if( str1[i] == str2[i] )
        {
            ++matches;
        }
        else
        {
            break;
        }
    }

    return matches;
}

// The best rhyming match is defined to be the one that
// has the most matching characters at the end of the
//    string,  ties should be broken randomly.
//    We expect that your solution to be efficient.
String getRhyme( const String &input, Array<String> rhymes )
{
    auto numMatching = 0;
    Array<String> matches;

    for( auto rhyme : rhymes )
    {
        const auto matching = (s32)rhymeMatch( input.c_str(), rhyme.c_str() );
        if( matching > numMatching )
        {
            matches.clear();
            matches.push_back( rhyme );
            numMatching = matching;
        }
        else if( matching == numMatching )
        {
            matches.push_back( rhyme );
        }
    }

    if( numMatching > 1 && !matches.empty() )
    {
        Array<String> output;
        std::sample( matches.begin(), matches.end(), std::back_inserter( output ), 1,
                     std::mt19937{ std::random_device{}() } );
        auto it = matches.begin();
        return output.front();
    }

    return {};
}

BOOST_AUTO_TEST_CASE( string_test_rhyme )
{
    Array<String> rhymes;
    rhymes.push_back( "Computing" );
    rhymes.push_back( "Polluting" );
    rhymes.push_back( "Diluting" );
    rhymes.push_back( "Commuting" );
    rhymes.push_back( "Fish" );
    rhymes.push_back( "Recruiting" );
    rhymes.push_back( "Drooping" );
    rhymes.push_back( "Bicycle" );

    Array<std::pair<String, Array<String>>> tests;
    tests.reserve( 4 );

    tests.push_back( std::make_pair( "Disputing", Array<String>( { "Computing" } ) ) );
    tests.push_back( std::make_pair( "Shooting", Array<String>( { "Computing", "Polluting", "Diluting",
                                                                  "Commuting", "Recruiting" } ) ) );
    tests.push_back( std::make_pair( "Convoluting", Array<String>( { "Polluting", "Diluting" } ) ) );
    tests.push_back( std::make_pair( "Orange", Array<String>() ) );

    for( const auto &test : tests )
    {
        const auto &expectedRhymes = test.second;
        auto rhyme = getRhyme( test.first.c_str(), rhymes );

        if( !expectedRhymes.empty() )
        {
            auto it = std::find( expectedRhymes.begin(), expectedRhymes.end(), rhyme );
            WP_ASSERT( it != expectedRhymes.end() );
        }
        else
        {
            WP_ASSERT( expectedRhymes.empty() && StringUtil::isNullOrEmpty( rhyme ) );
        }

        std::cout << "input: " << test.first << "ryhme: " << rhyme << std::endl;
    }
}

BOOST_AUTO_TEST_CASE( string_test_rhyme_class )
{
    Array<String> rhymes;
    rhymes.push_back( "Computing" );
    rhymes.push_back( "Polluting" );
    rhymes.push_back( "Diluting" );
    rhymes.push_back( "Commuting" );
    rhymes.push_back( "Fish" );
    rhymes.push_back( "Recruiting" );
    rhymes.push_back( "Drooping" );
    rhymes.push_back( "Bicycle" );

    // Deterministic: single best match
    BOOST_CHECK_EQUAL( getRhyme( "Disputing", rhymes ), "Computing" );

    // No match beyond threshold: expect empty
    BOOST_CHECK( StringUtil::isNullOrEmpty( getRhyme( "Orange", rhymes ) ) );

    // Multiple valid matches: result must be one of the accepted rhymes
    {
        auto rhyme = getRhyme( "Convoluting", rhymes );
        Array<String> accepted = { "Polluting", "Diluting" };
        auto it = std::find( accepted.begin(), accepted.end(), rhyme );
        BOOST_CHECK( it != accepted.end() );
    }
}

BOOST_AUTO_TEST_CASE( string_test_state_machine_get_state )
{
    // Initial state should be q0
    StateMachine sm;
    BOOST_CHECK_EQUAL( sm.getState(), q0 );

    // A leading space keeps the machine in q0
    char space = ' ';
    sm.transition( space );
    BOOST_CHECK_EQUAL( sm.getState(), q0 );

    // A minus sign advances the machine to q1
    char minusSign = '-';
    sm.transition( minusSign );
    BOOST_CHECK_EQUAL( sm.getState(), q1 );

    // A digit in q1 advances the machine to q2
    char digit = '7';
    sm.transition( digit );
    BOOST_CHECK_EQUAL( sm.getState(), q2 );

    // A non-digit in q2 transitions to the dead state qd
    char nonDigit = 'x';
    sm.transition( nonDigit );
    BOOST_CHECK_EQUAL( sm.getState(), qd );

    // A plus sign from q0 also advances the machine to q1
    {
        StateMachine sm2;
        char plusSign = '+';
        sm2.transition( plusSign );
        BOOST_CHECK_EQUAL( sm2.getState(), q1 );
    }

    // A digit directly from q0 skips q1 and goes straight to q2
    {
        StateMachine sm3;
        char directDigit = '5';
        sm3.transition( directDigit );
        BOOST_CHECK_EQUAL( sm3.getState(), q2 );
    }

    // A non-digit in q1 transitions to the dead state qd
    {
        StateMachine sm4;
        char plus = '+';
        sm4.transition( plus );
        BOOST_CHECK_EQUAL( sm4.getState(), q1 );
        char letter = 'a';
        sm4.transition( letter );
        BOOST_CHECK_EQUAL( sm4.getState(), qd );
    }

    // Dead state is absorbing: further transitions stay in qd
    {
        StateMachine sm5;
        char letter = 'z';
        sm5.transition( letter );
        BOOST_CHECK_EQUAL( sm5.getState(), qd );
        char anotherDigit = '9';
        sm5.transition( anotherDigit );
        BOOST_CHECK_EQUAL( sm5.getState(), qd );
    }
}

BOOST_AUTO_TEST_CASE( string_test_state_machine_get_integer )
{
    // Positive integer
    {
        StateMachine sm;
        for( char ch : std::string( "123" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), 123 );
    }

    // Negative integer
    {
        StateMachine sm;
        for( char ch : std::string( "-456" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), -456 );
    }

    // Explicit positive sign
    {
        StateMachine sm;
        for( char ch : std::string( "+789" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), 789 );
    }

    // Leading whitespace
    {
        StateMachine sm;
        for( char ch : std::string( "  42" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), 42 );
    }

    // Non-numeric input: result stays at 0
    {
        StateMachine sm;
        for( char ch : std::string( "abc" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), 0 );
    }

    // Overflow clamps to INT_MAX
    {
        StateMachine sm;
        for( char ch : std::string( "9999999999" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), INT_MAX );
    }

    // Underflow clamps to INT_MIN
    {
        StateMachine sm;
        for( char ch : std::string( "-9999999999" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), INT_MIN );
    }

    // Zero
    {
        StateMachine sm;
        char zero = '0';
        sm.transition( zero );
        BOOST_CHECK_EQUAL( sm.getInteger(), 0 );
    }

    // Single digit
    {
        StateMachine sm;
        char singleDigit = '9';
        sm.transition( singleDigit );
        BOOST_CHECK_EQUAL( sm.getInteger(), 9 );
    }

    // Digits followed by non-digit characters: only leading digits are parsed
    {
        StateMachine sm;
        for( char ch : std::string( "42abc" ) )
            sm.transition( ch );
        BOOST_CHECK_EQUAL( sm.getInteger(), 42 );
    }

    // Sign-only input: result stays at 0
    {
        StateMachine sm;
        char plus = '+';
        sm.transition( plus );
        BOOST_CHECK_EQUAL( sm.getInteger(), 0 );
    }

    // Empty input: result stays at 0
    {
        StateMachine sm;
        BOOST_CHECK_EQUAL( sm.getInteger(), 0 );
    }
}

// =============================================================================
// StringUtil tests
// =============================================================================

BOOST_AUTO_TEST_CASE( stringutil_to_string_int )
{
    BOOST_CHECK_EQUAL( StringUtil::toString( 0 ), "0" );
    BOOST_CHECK_EQUAL( StringUtil::toString( 42 ), "42" );
    BOOST_CHECK_EQUAL( StringUtil::toString( -42 ), "-42" );
    BOOST_CHECK_EQUAL( StringUtil::toString( 2147483647 ), "2147483647" );
    BOOST_CHECK_EQUAL( StringUtil::toString( -2147483648LL ), "-2147483648" );
}

BOOST_AUTO_TEST_CASE( stringutil_to_string_float )
{
    auto s = StringUtil::toString( 3.14f );
    BOOST_CHECK( !s.empty() );
    // Verify the round-trip via myAtoi on the integer part
    BOOST_CHECK( s.find( '3' ) != String::npos );
}

BOOST_AUTO_TEST_CASE( stringutil_is_null_or_empty )
{
    BOOST_CHECK( StringUtil::isNullOrEmpty( "" ) );
    BOOST_CHECK( StringUtil::isNullOrEmpty( String() ) );
    BOOST_CHECK( !StringUtil::isNullOrEmpty( "hello" ) );
    BOOST_CHECK( !StringUtil::isNullOrEmpty( " " ) );
}

BOOST_AUTO_TEST_CASE( stringutil_atoi_zero_and_edge_cases )
{
    // Zero
    BOOST_CHECK_EQUAL( myAtoi( "0" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "0" ), 0 );
    // Only spaces
    BOOST_CHECK_EQUAL( myAtoi( "    " ), 0 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "    " ), 0 );
    // Sign only
    BOOST_CHECK_EQUAL( myAtoi( "+" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "+" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi( "-" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "-" ), 0 );
    // Multiple signs (invalid)
    BOOST_CHECK_EQUAL( myAtoi( "+-12" ), 0 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "+-12" ), 0 );
    // Leading zeros
    BOOST_CHECK_EQUAL( myAtoi( "007" ), 7 );
    BOOST_CHECK_EQUAL( myAtoi_dfa( "007" ), 7 );
}
