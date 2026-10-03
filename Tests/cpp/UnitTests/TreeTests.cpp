#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

class TreeNode : public Node<ISharedObject>
{
public:
    int val = 0;
    SmartPtr<TreeNode> left;
    SmartPtr<TreeNode> right;
};

bool isSameTreeBinary( SmartPtr<TreeNode> a, SmartPtr<TreeNode> b )
{
    if( a && b )
    {
        if( a->val == b->val )
        {
            bool result = true;

            if( a->left && b->left )
            {
                if( !isSameTreeBinary( a->left, b->left ) )
                {
                    result = false;
                }
            }
            else if( a->left != nullptr || b->left != nullptr )
            {
                result = false;
            }

            if( a->right && b->right )
            {
                if( !isSameTreeBinary( a->right, b->right ) )
                {
                    result = false;
                }
            }
            else if( a->right != nullptr || b->right != nullptr )
            {
                result = false;
            }

            return result;
        }

        return false;
    }

    return a == b;
}

bool isSameTree( SmartPtr<TreeNode> a, SmartPtr<TreeNode> b )
{
    if( a && b )
    {
        if( a->val == b->val )
        {
            auto aChildren = a->getChildren();
            auto bChildren = b->getChildren();

            if( aChildren.size() != bChildren.size() )
            {
                return false;
            }

            for( size_t i = 0; i < aChildren.size() && i < bChildren.size(); ++i )
            {
                auto childA = aChildren[i];
                auto childB = bChildren[i];

                if( !isSameTree( workphone::static_pointer_cast<TreeNode>( childA ),
                                 workphone::static_pointer_cast<TreeNode>( childB ) ) )
                {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}

bool isSameTree( SmartPtr<IGameActor> a, SmartPtr<IGameActor> b )
{
    if( a && b )
    {
        if( a->getName() == b->getName() )
        {
            auto aChildren = a->getChildren();
            auto bChildren = b->getChildren();

            if( aChildren.size() != bChildren.size() )
            {
                return false;
            }

            for( size_t i = 0; i < aChildren.size() && i < bChildren.size(); ++i )
            {
                auto childA = aChildren[i];
                auto childB = bChildren[i];

                if( !isSameTree( childA, childB ) )
                {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}

int maxDepth( SmartPtr<TreeNode> node, int nodeDepth )
{
    if( node )
    {
        int leftDepth = nodeDepth;
        int rightDepth = nodeDepth;

        if( node->left )
        {
            auto depth = maxDepth( node->left, leftDepth );
            leftDepth = leftDepth + depth;
        }

        if( node->right )
        {
            auto depth = maxDepth( node->right, rightDepth );
            rightDepth = rightDepth + depth;
        }

        return std::max( leftDepth, rightDepth );
    }

    return 0;
}

int maxDepth( SmartPtr<TreeNode> root )
{
    if( root )
    {
        int leftDepth = 1;
        int rightDepth = 1;

        if( root->left )
        {
            leftDepth += maxDepth( root->left );
        }

        if( root->right )
        {
            rightDepth += maxDepth( root->right );
        }

        return std::max( leftDepth, rightDepth );
    }

    return 0;
}

BOOST_AUTO_TEST_CASE( tree_equal_test )
{
    try
    {
        auto a = workphone::make_ptr<TreeNode>();
        auto b = workphone::make_ptr<TreeNode>();

        a->val = b->val = 1;

        auto aB = workphone::make_ptr<TreeNode>();
        auto aC = workphone::make_ptr<TreeNode>();

        aB->val = 2;
        aC->val = 3;

        auto bB = workphone::make_ptr<TreeNode>();
        auto bC = workphone::make_ptr<TreeNode>();

        bB->val = 2;
        bC->val = 3;

        a->addChild( aB );
        a->addChild( aC );

        b->addChild( bB );
        b->addChild( bC );

        a->left = aB;
        a->right = aC;

        b->left = bB;
        b->right = bC;

        BOOST_CHECK( isSameTreeBinary( a, b ) );
        BOOST_CHECK( isSameTree( a, b ) );

        a->removeAllChildren();
        b->removeAllChildren();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( tree_not_equal_test )
{
    try
    {
        auto a = workphone::make_ptr<TreeNode>();
        auto b = workphone::make_ptr<TreeNode>();

        a->val = b->val = 1;

        auto aB = workphone::make_ptr<TreeNode>();
        // auto aC = workphone::make_ptr<TreeNode>();

        aB->val = 2;
        // aC->val = 3;

        auto bB = workphone::make_ptr<TreeNode>();
        auto bC = workphone::make_ptr<TreeNode>();

        bB->val = 0;
        bC->val = 2;

        a->addChild( aB );
        // a->addChild( aC );

        b->addChild( bB );
        b->addChild( bC );

        a->left = aB;
        // a->right = aC;

        b->left = nullptr;
        b->right = bC;

        BOOST_CHECK( isSameTreeBinary( a, b ) == false );
        BOOST_CHECK( isSameTree( a, b ) == false );

        a->removeAllChildren();
        b->removeAllChildren();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( tree_maximum_depth )
{
    try
    {
        auto a = workphone::make_ptr<TreeNode>();
        auto b = workphone::make_ptr<TreeNode>();

        a->val = b->val = 1;

        auto aB = workphone::make_ptr<TreeNode>();
        auto aC = workphone::make_ptr<TreeNode>();

        aB->val = 2;
        aC->val = 3;

        auto bB = workphone::make_ptr<TreeNode>();
        auto bC = workphone::make_ptr<TreeNode>();

        bB->val = 2;
        bC->val = 3;

        a->addChild( aB );
        a->addChild( aC );

        b->addChild( bB );
        b->addChild( bC );

        a->left = aB;
        a->right = aC;

        b->left = bB;
        b->right = bC;

        BOOST_CHECK( maxDepth( a ) > 0 );
        BOOST_CHECK( maxDepth( b ) > 0 );

        auto depth = maxDepth( a );

        BOOST_CHECK( maxDepth( a ) == 2 );
        BOOST_CHECK( maxDepth( b ) == 2 );

        auto aD = workphone::make_ptr<TreeNode>();
        auto aE = workphone::make_ptr<TreeNode>();

        bB->val = 4;
        bC->val = 5;

        aB->addChild( aD );
        aB->addChild( aE );

        aB->left = aD;
        aB->right = aE;

        depth = maxDepth( a );
        BOOST_CHECK( maxDepth( a ) == 3 );

        a->removeAllChildren();
        b->removeAllChildren();

        aB->removeAllChildren();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
