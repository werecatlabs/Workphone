#include <WPProcedural/PlanarGraph.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <cstdlib>
#define CHECK(condition) do { if( !(condition) ) std::abort(); } while( false )
#include <memory>

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
    Graph graph;
    CHECK( extract( graph ).empty() );
    graph.InsertVertex( Point( 0, 0 ), 42 );
    CHECK( extract( graph )[0]->Sequence[0].second == 42 );
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
        double area = 0;
        for( size_t i = 0; i < 4; ++i )
        {
            const auto &a = cell->Sequence[i].first;
            const auto &b = cell->Sequence[( i + 1 ) % 4].first;
            area += a.X() * b.Y() - a.Y() * b.X();
        }
        CHECK( area == 2 );
    }

    Graph branches;
    for( unsigned i = 0; i < 4; ++i ) branches.InsertVertex( Point( i, i * i ), i );
    branches.InsertEdge( 0, 1 ); branches.InsertEdge( 0, 2 ); branches.InsertEdge( 0, 3 );
    CHECK( extract( branches ).size() == 3 );
}

