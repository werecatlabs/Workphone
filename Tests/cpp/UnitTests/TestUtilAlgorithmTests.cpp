#include "UnitTests.hpp"
#include <Workphone/Test/TestUtil.hpp>
#include <boost/test/unit_test.hpp>
#include <set>
#include <string>

using namespace workphone;

namespace
{
    using Matrix = Array<Array<s32>>;
    using TripletSet = std::set<std::string>;

    void checkArrayEqual( const Array<s32> &actual, const Array<s32> &expected )
    {
        BOOST_REQUIRE_EQUAL( actual.size(), expected.size() );
        BOOST_CHECK_EQUAL_COLLECTIONS( actual.begin(), actual.end(), expected.begin(), expected.end() );
    }

    void checkMatrixEqual( const Matrix &actual, const Matrix &expected )
    {
        BOOST_REQUIRE_EQUAL( actual.size(), expected.size() );
        for( size_t row = 0; row < expected.size(); ++row )
        {
            BOOST_REQUIRE_EQUAL( actual[row].size(), expected[row].size() );
            BOOST_CHECK_EQUAL_COLLECTIONS( actual[row].begin(), actual[row].end(), expected[row].begin(),
                                           expected[row].end() );
        }
    }

    TripletSet toTripletSet( const Array<Array<s32>> &triplets )
    {
        TripletSet values;
        for( const auto &triplet : triplets )
        {
            BOOST_REQUIRE_EQUAL( triplet.size(), 3 );
            values.insert( std::to_string( triplet[0] ) + "," + std::to_string( triplet[1] ) + "," +
                           std::to_string( triplet[2] ) );
        }

        return values;
    }

    Matrix multiplyNaive( Matrix mat1, Matrix mat2 )
    {
        return test::TestUtil::multiply_naive_iteration( mat1, mat2 );
    }

    Matrix multiplySparse( Matrix mat1, Matrix mat2 )
    {
        return test::TestUtil::multiply( mat1, mat2 );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( testutil_two_sum_returns_matching_indices )
{
    {
        Array<s32> values{ 2, 7, 11, 15 };
        checkArrayEqual( test::TestUtil::twoSum( values, 9 ), { 0, 1 } );
    }

    {
        Array<s32> values{ 3, 3 };
        checkArrayEqual( test::TestUtil::twoSum( values, 6 ), { 0, 1 } );
    }

    {
        Array<s32> values{ -3, 4, 3, 90 };
        checkArrayEqual( test::TestUtil::twoSum( values, 0 ), { 0, 2 } );
    }

    {
        Array<s32> values{ 1, 2, 3 };
        BOOST_CHECK( test::TestUtil::twoSum( values, 100 ).empty() );
    }
}

BOOST_AUTO_TEST_CASE( testutil_three_sum_returns_unique_zero_sum_triplets )
{
    {
        Array<s32> values{ -1, 0, 1, 2, -1, -4 };
        const auto triplets = test::TestUtil::threeSum( values );
        const TripletSet expected{ "-1,-1,2", "-1,0,1" };
        BOOST_CHECK( toTripletSet( triplets ) == expected );
    }

    {
        Array<s32> values{ 0, 0, 0, 0 };
        const auto triplets = test::TestUtil::threeSum( values );
        const TripletSet expected{ "0,0,0" };
        BOOST_CHECK( toTripletSet( triplets ) == expected );
    }

    {
        Array<s32> values{ 1, 2, -2, -1 };
        BOOST_CHECK( test::TestUtil::threeSum( values ).empty() );
    }
}

BOOST_AUTO_TEST_CASE( testutil_trap_handles_empty_edges_and_known_examples )
{
    const Array<Array<s32>> inputs{
        {},
        { 1 },
        { 1, 2 },
        { 1, 2, 3, 4 },
        { 4, 3, 2, 1 },
        { 2, 2, 2 },
        { 0, 1, 0, 2, 1, 0, 1, 3, 2, 1, 2, 1 },
        { 4, 2, 0, 3, 2, 5 },
    };
    const Array<s32> expected{ 0, 0, 0, 0, 0, 0, 6, 9 };

    for( size_t i = 0; i < inputs.size(); ++i )
    {
        auto inputForTrap = inputs[i];
        auto inputForTrapOn = inputs[i];

        BOOST_CHECK_EQUAL( test::TestUtil::trap( inputForTrap ), expected[i] );
        BOOST_CHECK_EQUAL( test::TestUtil::trap_On( inputForTrapOn ), expected[i] );
    }
}

BOOST_AUTO_TEST_CASE( testutil_compress_matrix_preserves_non_zero_values_and_columns )
{
    Matrix matrix{
        { 0, 0, 3 },
        { 4, 0, 0 },
        { 0, -2, 5 },
    };

    const auto compressed = test::TestUtil::compressMatrix( matrix );

    BOOST_REQUIRE_EQUAL( compressed.size(), 3 );
    BOOST_REQUIRE_EQUAL( compressed[0].size(), 1 );
    BOOST_CHECK_EQUAL( compressed[0][0].first, 3 );
    BOOST_CHECK_EQUAL( compressed[0][0].second, 2 );

    BOOST_REQUIRE_EQUAL( compressed[1].size(), 1 );
    BOOST_CHECK_EQUAL( compressed[1][0].first, 4 );
    BOOST_CHECK_EQUAL( compressed[1][0].second, 0 );

    BOOST_REQUIRE_EQUAL( compressed[2].size(), 2 );
    BOOST_CHECK_EQUAL( compressed[2][0].first, -2 );
    BOOST_CHECK_EQUAL( compressed[2][0].second, 1 );
    BOOST_CHECK_EQUAL( compressed[2][1].first, 5 );
    BOOST_CHECK_EQUAL( compressed[2][1].second, 2 );
}

BOOST_AUTO_TEST_CASE( testutil_sparse_matrix_multiply_matches_naive_results )
{
    {
        Matrix mat1{
            { 1, 0, 0 },
            { -1, 0, 3 },
        };
        Matrix mat2{
            { 7, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 1 },
        };

        checkMatrixEqual( multiplySparse( mat1, mat2 ), Matrix( {
                                                            { 7, 0, 0 },
                                                            { -7, 0, 3 },
                                                        } ) );
        checkMatrixEqual( multiplySparse( mat1, mat2 ), multiplyNaive( mat1, mat2 ) );
    }

    {
        Matrix mat1{
            { 0, 2, 0, 4 },
            { 3, 0, 0, 0 },
            { 0, 0, -1, 0 },
        };
        Matrix mat2{
            { 5, 0 },
            { 0, 6 },
            { 7, 0 },
            { 0, -2 },
        };

        checkMatrixEqual( multiplySparse( mat1, mat2 ), multiplyNaive( mat1, mat2 ) );
    }

    {
        Matrix mat1{
            { 0, 0 },
            { 0, 0 },
        };
        Matrix mat2{
            { 1, 2 },
            { 3, 4 },
        };

        checkMatrixEqual( multiplySparse( mat1, mat2 ), Matrix( {
                                                            { 0, 0 },
                                                            { 0, 0 },
                                                        } ) );
    }
}
