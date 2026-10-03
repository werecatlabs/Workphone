#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>

#if WP_STATIC
#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysxAllocator.hpp>
#        include <FBPhysx/FBPhysxPoolAllocator.hpp>
#    endif
#endif

#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( AllocateAndDeallocate )
{
#if WP_STATIC
#    if WP_BUILD_PHYSX
    physics::PhysxAllocator allocator;

    // Allocate some memory
    void *ptr1 = allocator.allocate( 100, "", "", 0 );
    BOOST_CHECK( ptr1 != nullptr );

    void *ptr2 = allocator.allocate( 200, "", "", 0 );
    BOOST_CHECK( ptr2 != nullptr );

    void *ptr3 = allocator.allocate( 300, "", "", 0 );
    BOOST_CHECK( ptr3 != nullptr );

    // Deallocate some memory
    allocator.deallocate( ptr2 );

    // Allocate memory that was deallocated
    void *ptr4 = allocator.allocate( 200, "", "", 0 );
    BOOST_CHECK_EQUAL( ptr4, ptr2 );
#    endif
#endif
}

BOOST_AUTO_TEST_CASE( AllocatorPoolAllocateAndDeallocate )
{
#if WP_STATIC
#    if WP_BUILD_PHYSX
    physics::PhysxPoolAllocator allocator( 1024, 16 );

    // Allocate some memory
    void *ptr1 = allocator.allocate( 100, "", "", 0 );
    BOOST_CHECK( ptr1 != nullptr );

    void *ptr2 = allocator.allocate( 200, "", "", 0 );
    BOOST_CHECK( ptr2 != nullptr );

    void *ptr3 = allocator.allocate( 300, "", "", 0 );
    BOOST_CHECK( ptr3 != nullptr );

    // Deallocate some memory
    allocator.deallocate( ptr2 );

    // Allocate memory that was deallocated
    void *ptr4 = allocator.allocate( 200, "", "", 0 );
    BOOST_CHECK_EQUAL( ptr4, ptr2 );
#    endif
#endif
}

BOOST_AUTO_TEST_CASE( AllocatorAlignment )
{
#if WP_STATIC
#    if WP_BUILD_PHYSX
    physics::PhysxAllocator allocator;

    // Allocate some memory
    void *ptr1 = allocator.allocate( 100, "", "", 0 );
    BOOST_CHECK( ptr1 != nullptr );

    void *ptr2 = allocator.allocate( 200, "", "", 0 );
    BOOST_CHECK( ptr2 != nullptr );

    // Check memory alignment
    uintptr_t iptr1 = reinterpret_cast<uintptr_t>( ptr1 );
    uintptr_t iptr2 = reinterpret_cast<uintptr_t>( ptr2 );
    BOOST_CHECK_EQUAL( iptr1 % 16, 0 );
    BOOST_CHECK_EQUAL( iptr2 % 16, 0 );
#    endif
#endif
}

BOOST_AUTO_TEST_CASE( AllocatorPoolAlignment )
{
#if WP_STATIC
#    if WP_BUILD_PHYSX
    physics::PhysxPoolAllocator allocator( 1024, 16 );

    // Allocate some memory
    void *ptr1 = allocator.allocate( 100, "", "", 0 );
    BOOST_CHECK( ptr1 != nullptr );

    void *ptr2 = allocator.allocate( 200, "", "", 0 );
    BOOST_CHECK( ptr2 != nullptr );

    // Check memory alignment
    uintptr_t iptr1 = reinterpret_cast<uintptr_t>( ptr1 );
    uintptr_t iptr2 = reinterpret_cast<uintptr_t>( ptr2 );
    BOOST_CHECK_EQUAL( iptr1 % 16, 0 );
    BOOST_CHECK_EQUAL( iptr2 % 16, 0 );
#    endif
#endif
}

BOOST_AUTO_TEST_CASE( shared_object_partial_properties_preserve_runtime_ownership )
{
    auto object = workphone::make_ptr<ISharedObject>();
    BOOST_REQUIRE( object );

    const auto references = object->getReferences();
    const auto weakReferences = object->getWeakReferences();
    const auto loadingState = object->getLoadingState();

    auto properties = workphone::make_ptr<Properties>();
    properties->setProperty( "unrelatedProperty", 42 );
    properties->setProperty( ISharedObject::referencesStr, 0 );
    properties->setProperty( ISharedObject::weakReferencesStr, 0 );

    object->setProperties( properties );

    BOOST_CHECK_EQUAL( object->getReferences(), references );
    BOOST_CHECK_EQUAL( object->getWeakReferences(), weakReferences );
    BOOST_CHECK( object->getLoadingState() == loadingState );
}
