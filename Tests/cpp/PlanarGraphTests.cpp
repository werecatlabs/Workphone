#include <WPProcedural/PlanarGraph.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <set>
#include <vector>
#define CHECK(condition) do { if( !(condition) ) std::abort(); } while( false )

// Standalone regression executable; no engine initialization is required.
int main()
{
    using Graph = workphone::procedural::PlanarGraph<workphone::Vector2<double>>;
    using Point = workphone::Vector2<double>;
    auto extract = []( const Graph &graph ) {
        workphone::Array<Graph::Primitive *> raw;
        graph.ExtractPrimitives( raw );
        std::vector<std::unique_ptr<Graph::Primitive>> result;
        for( auto primitive : raw ) result.emplace_back( primitive );
        return result;
    };
    using Triangle = std::array<unsigned, 3>;
    auto canonicalTriangle = []( Triangle ids ) {
        std::rotate( ids.begin(), std::min_element( ids.begin(), ids.end() ), ids.end() );
        return ids;
    };
    auto twiceArea = []( const auto &sequence ) {
        double area = 0;
        for( size_t i = 0; i < sequence.size(); ++i )
        {
            const auto &a = sequence[i].first;
            const auto &b = sequence[( i + 1 ) % sequence.size()].first;
            area += a.X() * b.Y() - a.Y() * b.X();
        }
        return area;
    };
    Graph graph;
    CHECK( extract( graph ).empty() );
    graph.InsertVertex( Point( 0, 0 ), 42 );
    CHECK( extract( graph )[0]->Sequence[0].second == 42 );
    const auto originId = graph.FindVertex( Point( -0.0, 0 ) );
    CHECK( originId && *originId == 42 );
    CHECK( !graph.FindVertex( Point( 1, 0 ) ) );
    graph.InsertVertex( Point( 1, 0 ), 7 );
    graph.InsertVertex( Point( 1, 1 ), 99 );
    graph.InsertVertex( Point( 0, 1 ), 5 );
    graph.InsertEdge( 42, 7 );
    graph.InsertEdge( 7, 99 );
    graph.InsertEdge( 99, 5 );
    CHECK( extract( graph ).size() == 1 );
    CHECK( extract( graph )[0]->Sequence.size() == 4 );
    graph.InsertEdge( 5, 42 );
    graph.InsertEdge( 7, 42 ); // Repeated reverse edge.
    CHECK( extract( graph ).size() == 1 );
    CHECK( extract( graph )[0]->Sequence.size() == 4 );
    graph.InsertEdge( 42, 99 );
    auto triangles = extract( graph );
    CHECK( triangles.size() == 2 );
    for( const auto &triangle : triangles ) CHECK( triangle->Sequence.size() == 3 );
    graph.InsertVertex( Point( 2, 1 ), 100 );
    graph.InsertVertex( Point( 3, 1 ), 101 );
    graph.InsertEdge( 99, 100 );
    graph.InsertEdge( 100, 101 );
    auto withTail = extract( graph );
    CHECK( withTail.size() == 3 );
    CHECK( withTail[0]->Sequence.size() == 3 );
    graph.InsertVertex( Point( 5, 5 ), 200 );
    CHECK( extract( graph ).size() == 4 );
    std::multiset<Triangle> visitedFaces;
    graph.VisitBoundedFaces( [&]( const auto &ids ) {
        CHECK( ids.size() == 3 );
        visitedFaces.insert( canonicalTriangle( { ids[0], ids[1], ids[2] } ) );
    } );
    const std::multiset<Triangle> expectedFaces{ { 5, 42, 99 }, { 7, 99, 42 } };
    CHECK( visitedFaces == expectedFaces );
    int rejected = 0;
    try { graph.InsertVertex( Point( 4, 4 ), 42 ); } catch( const std::invalid_argument & ) { ++rejected; }
    try { graph.InsertVertex( Point( 0, 0 ), 300 ); } catch( const std::invalid_argument & ) { ++rejected; }
    try { graph.InsertEdge( 42, 42 ); } catch( const std::invalid_argument & ) { ++rejected; }
    try { graph.InsertEdge( 42, 12345 ); } catch( const std::invalid_argument & ) { ++rejected; }
    CHECK( rejected == 4 );
    workphone::Array<Graph::Primitive *> appended;
    graph.ExtractPrimitives( appended );
    graph.ExtractPrimitives( appended );
    CHECK( appended.size() == 8 );
    for( auto primitive : appended ) delete primitive;

    // Two loops sharing an articulation vertex must remain separate faces.
    Graph touching;
    touching.InsertVertex( Point( 0, 0 ), 0 );
    touching.InsertVertex( Point( 1, 0 ), 1 );
    touching.InsertVertex( Point( 0, 1 ), 2 );
    touching.InsertVertex( Point( -1, 0 ), 3 );
    touching.InsertVertex( Point( 0, -1 ), 4 );
    touching.InsertEdge( 0, 1 ); touching.InsertEdge( 1, 2 ); touching.InsertEdge( 2, 0 );
    touching.InsertEdge( 0, 3 ); touching.InsertEdge( 3, 4 ); touching.InsertEdge( 4, 0 );
    auto loops = extract( touching );
    CHECK( loops.size() == 2 );
    for( const auto &loop : loops ) CHECK( loop->Sequence.size() == 3 );

    // An inner triangle touches the outer square at its leftmost corner. The face
    // between them visits that junction twice and includes the inner loop clockwise.
    Graph nested;
    unsigned id = 0;
    for( const auto &p : { Point( 0, 0 ), Point( 4, 0 ), Point( 4, 4 ), Point( 0, 4 ),
                           Point( 2, 1 ), Point( 1, 2 ) } )
    {
        nested.InsertVertex( p, id++ );
    }
    for( unsigned i = 0; i < 4; ++i ) nested.InsertEdge( i, ( i + 1 ) % 4 );
    nested.InsertEdge( 0, 4 ); nested.InsertEdge( 4, 5 ); nested.InsertEdge( 5, 0 );
    std::multiset<double> nestedAreas;
    for( const auto &face : extract( nested ) )
    {
        nestedAreas.insert( twiceArea( face->Sequence ) );
    }
    const std::multiset<double> expectedAreas{ 3, 29 };
    CHECK( nestedAreas == expectedAreas );

    auto checkFan = [&]( double length, double spacing ) {
        Graph fan;
        fan.InsertVertex( Point( 0, 0 ), 0 );
        for( unsigned tip = 1; tip <= 3; ++tip )
        {
            fan.InsertVertex( Point( -length, tip * spacing ), tip );
            fan.InsertEdge( 0, tip );
        }
        fan.InsertEdge( 1, 2 );
        fan.InsertEdge( 2, 3 );
        auto faces = extract( fan );
        CHECK( faces.size() == 2 );
        std::set<Triangle> boundaries;
        for( const auto &face : faces )
        {
            const auto &sequence = face->Sequence;
            CHECK( sequence.size() == 3 );
            boundaries.insert( canonicalTriangle(
                { sequence[0].second, sequence[1].second, sequence[2].second } ) );
        }
        const std::set<Triangle> expected{ { 0, 2, 1 }, { 0, 3, 2 } };
        CHECK( boundaries == expected );
    };
    checkFan( 1, 0x1p-80 );  // All three atan2 angles round to pi on MSVC.
    checkFan( 1e-200, 1e-200 );  // Double-precision cross products underflow.
    checkFan( 1e200, 1e200 );    // Double-precision cross products overflow.

    Graph grid;
    for( unsigned y = 0; y < 4; ++y )
    {
        for( unsigned x = 0; x < 4; ++x ) grid.InsertVertex( Point( x, y ), y * 4 + x );
    }
    for( unsigned y = 0; y < 4; ++y )
    {
        for( unsigned x = 0; x < 4; ++x )
        {
            if( x < 3 ) grid.InsertEdge( y * 4 + x, y * 4 + x + 1 );
            if( y < 3 ) grid.InsertEdge( y * 4 + x, ( y + 1 ) * 4 + x );
        }
    }
    auto cells = extract( grid );
    CHECK( cells.size() == 9 );
    for( const auto &cell : cells )
    {
        CHECK( cell->Sequence.size() == 4 );
        CHECK( twiceArea( cell->Sequence ) == 2 );
    }

    Graph branches;
    for( unsigned i = 0; i < 4; ++i ) branches.InsertVertex( Point( i, i * i ), i );
    branches.InsertEdge( 0, 1 ); branches.InsertEdge( 0, 2 ); branches.InsertEdge( 0, 3 );
    CHECK( extract( branches ).size() == 3 );
}

