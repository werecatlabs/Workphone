#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <memory>
#include <vector>

struct BoundingBox
{
    float minx, miny, minz;
    float maxx, maxy, maxz;
};

struct Object
{
    BoundingBox bbox;
    // additional data for the object
};

struct Node
{
    BoundingBox bbox;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    std::vector<Object *> objects;
};

class BVH
{
public:
    BVH( std::vector<Object> &objects )
    {
        root_ = Build( objects, 0, objects.size() );
    }

    ~BVH() = default;

    Node *GetRoot() const
    {
        return root_.get();
    }

private:
    std::unique_ptr<Node> Build( std::vector<Object> &objects, int start, int end )
    {
        if( start == end )
            return nullptr;
        if( end - start == 1 )
        {
            auto node = std::make_unique<Node>();
            node->bbox = objects[start].bbox;
            node->objects.push_back( &objects[start] );
            return node;
        }
        int mid = ( start + end ) / 2;
        auto left = Build( objects, start, mid );
        auto right = Build( objects, mid, end );
        auto node = std::make_unique<Node>();
        node->bbox = CombineBoundingBoxes( left->bbox, right->bbox );
        node->left = std::move( left );
        node->right = std::move( right );
        return node;
    }

    BoundingBox CombineBoundingBoxes( const BoundingBox &b1, const BoundingBox &b2 )
    {
        BoundingBox bbox;
        bbox.minx = std::min( b1.minx, b2.minx );
        bbox.miny = std::min( b1.miny, b2.miny );
        bbox.minz = std::min( b1.minz, b2.minz );
        bbox.maxx = std::max( b1.maxx, b2.maxx );
        bbox.maxy = std::max( b1.maxy, b2.maxy );
        bbox.maxz = std::max( b1.maxz, b2.maxz );
        return bbox;
    }

    std::unique_ptr<Node> root_;
};
