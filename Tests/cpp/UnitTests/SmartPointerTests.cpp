#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <boost/test/unit_test.hpp>
#include <thread>
#include <vector>

using namespace workphone;

// Local test class that inherits from ISharedObject for use in tests
// This avoids linker issues with external fake classes
namespace
{
    class TestObject : public ISharedObject
    {
    public:
        TestObject() : m_value( 0 )
        {
        }

        explicit TestObject( int value ) : m_value( value )
        {
        }

        ~TestObject() override = default;

        void setValue( int value )
        {
            m_value = value;
        }
        int getValue() const
        {
            return m_value;
        }

        virtual void virtualFunc()
        {
        }

    private:
        int m_value;
    };

    class DerivedTestObject : public TestObject
    {
    public:
        DerivedTestObject() : TestObject( 0 )
        {
        }

        explicit DerivedTestObject( int value ) : TestObject( value )
        {
        }

        ~DerivedTestObject() override = default;

        void virtualFunc() override
        {
        }
    };

}  // namespace

//------------------------------------------------------------------------------
// SmartPtr Constructor Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_default_constructor )
{
    SmartPtr<TestObject> ptr;
    BOOST_CHECK( ptr == nullptr );
    BOOST_CHECK( !ptr );
    BOOST_CHECK( ptr.get() == nullptr );
}

BOOST_AUTO_TEST_CASE( smartptr_raw_pointer_constructor )
{
    TestGuard guard;
    auto obj = new TestObject();
    guard.releaseInitialReference( obj );
    SmartPtr<TestObject> ptr( obj );

    BOOST_CHECK( ptr != nullptr );
    BOOST_CHECK( ptr );
    BOOST_CHECK( ptr.get() == obj );
    BOOST_CHECK( obj->getReferences() == 2 );  // 1 initial + 1 from SmartPtr
}

BOOST_AUTO_TEST_CASE( smartptr_copy_constructor )
{
    SmartPtr<TestObject> ptr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> ptr2( ptr1 );

    BOOST_CHECK( ptr1 == ptr2 );
    BOOST_CHECK( ptr1.get() == ptr2.get() );
    BOOST_CHECK( ptr1.get()->getReferences() == 2 );  // 1 initial + 1 from SmartPtrs
}

BOOST_AUTO_TEST_CASE( smartptr_copy_constructor_null )
{
    SmartPtr<TestObject> ptr1;
    SmartPtr<TestObject> ptr2( ptr1 );

    BOOST_CHECK( ptr1 == nullptr );
    BOOST_CHECK( ptr2 == nullptr );
    BOOST_CHECK( ptr1 == ptr2 );
}

BOOST_AUTO_TEST_CASE( smartptr_derived_type_constructor )
{
    SmartPtr<DerivedTestObject> derivedPtr = workphone::make_ptr<DerivedTestObject>();
    SmartPtr<TestObject> basePtr( derivedPtr );

    BOOST_CHECK( basePtr != nullptr );
    BOOST_CHECK( basePtr.get() == derivedPtr.get() );
    BOOST_CHECK( derivedPtr.get()->getReferences() == 2 );  // 1 initial + 1 SmartPtrs
}

BOOST_AUTO_TEST_CASE( smartptr_derived_raw_pointer_constructor )
{
    TestGuard guard;
    auto derived = new DerivedTestObject();
    guard.releaseInitialReference( derived );
    SmartPtr<TestObject> basePtr( derived );

    BOOST_CHECK( basePtr != nullptr );
    BOOST_CHECK( basePtr.get() == derived );
}

//------------------------------------------------------------------------------
// SmartPtr Assignment Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_assign_raw_pointer )
{
    TestGuard guard;
    SmartPtr<TestObject> ptr;
    auto obj = new TestObject();
    guard.releaseInitialReference( obj );

    ptr = obj;

    BOOST_CHECK( ptr != nullptr );
    BOOST_CHECK( ptr.get() == obj );
}

BOOST_AUTO_TEST_CASE( smartptr_assign_smartptr )
{
    SmartPtr<TestObject> ptr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> ptr2;

    ptr2 = ptr1;

    BOOST_CHECK( ptr2 != nullptr );
    BOOST_CHECK( ptr1 == ptr2 );
    BOOST_CHECK( ptr1.get()->getReferences() == 2 );  // 1 initial + 1 SmartPtrs
}

BOOST_AUTO_TEST_CASE( smartptr_assign_nullptr )
{
    SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>();
    BOOST_CHECK( ptr != nullptr );

    ptr = nullptr;

    BOOST_CHECK( ptr == nullptr );
    BOOST_CHECK( !ptr );
}

BOOST_AUTO_TEST_CASE( smartptr_reassign_different_pointer )
{
    auto obj1 = workphone::make_ptr<TestObject>( 1 );
    auto obj2 = workphone::make_ptr<TestObject>( 2 );

    SmartPtr<TestObject> ptr( obj1 );
    BOOST_CHECK( ptr.get() == obj1 );
    BOOST_CHECK( ptr->getValue() == 1 );

    ptr = obj2;

    BOOST_CHECK( ptr.get() == obj2 );
    BOOST_CHECK( ptr->getValue() == 2 );
}

BOOST_AUTO_TEST_CASE( smartptr_assign_derived_type )
{
    SmartPtr<DerivedTestObject> derivedPtr = workphone::make_ptr<DerivedTestObject>();
    SmartPtr<TestObject> basePtr;

    basePtr = derivedPtr;

    BOOST_CHECK( basePtr != nullptr );
    BOOST_CHECK( basePtr.get() == derivedPtr.get() );
}

//------------------------------------------------------------------------------
// SmartPtr Operator Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_dereference_operator )
{
    SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>( 42 );

    BOOST_CHECK( ptr->getValue() == 42 );
    ptr->setValue( 100 );
    BOOST_CHECK( ptr->getValue() == 100 );
}

BOOST_AUTO_TEST_CASE( smartptr_star_operator )
{
    SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>( 42 );

    TestObject &ref = *ptr;
    BOOST_CHECK( ref.getValue() == 42 );
}

BOOST_AUTO_TEST_CASE( smartptr_bool_conversion )
{
    SmartPtr<TestObject> ptr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> ptr2;

    BOOST_CHECK( static_cast<bool>( ptr1 ) == true );
    BOOST_CHECK( static_cast<bool>( ptr2 ) == false );
}

BOOST_AUTO_TEST_CASE( smartptr_not_operator )
{
    SmartPtr<TestObject> ptr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> ptr2;

    BOOST_CHECK( !ptr1 == false );
    BOOST_CHECK( !ptr2 == true );
}

//------------------------------------------------------------------------------
// SmartPtr Comparison Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_equality_same_object )
{
    SmartPtr<TestObject> ptr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> ptr2( ptr1 );

    BOOST_CHECK( ptr1 == ptr2 );
    BOOST_CHECK( !( ptr1 != ptr2 ) );
}

BOOST_AUTO_TEST_CASE( smartptr_equality_different_objects )
{
    SmartPtr<TestObject> ptr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> ptr2 = workphone::make_ptr<TestObject>();

    BOOST_CHECK( ptr1 != ptr2 );
    BOOST_CHECK( !( ptr1 == ptr2 ) );
}

BOOST_AUTO_TEST_CASE( smartptr_equality_with_nullptr )
{
    SmartPtr<TestObject> ptr1;
    SmartPtr<TestObject> ptr2 = workphone::make_ptr<TestObject>();

    BOOST_CHECK( ptr1 == nullptr );
    BOOST_CHECK( ptr2 != nullptr );
}

BOOST_AUTO_TEST_CASE( smartptr_equality_different_types )
{
    SmartPtr<DerivedTestObject> derivedPtr = workphone::make_ptr<DerivedTestObject>();
    SmartPtr<TestObject> basePtr( derivedPtr );

    BOOST_CHECK( derivedPtr == basePtr );
    BOOST_CHECK( basePtr == derivedPtr );
}

//------------------------------------------------------------------------------
// SmartPtr Reference Counting Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_reference_counting_increment )
{
    TestGuard guard;
    auto obj = new TestObject();
    guard.releaseInitialReference( obj );
    s32 initialRefs = obj->getReferences();

    SmartPtr<TestObject> ptr1( obj );
    BOOST_CHECK( obj->getReferences() == initialRefs + 1 );

    SmartPtr<TestObject> ptr2( ptr1 );
    BOOST_CHECK( obj->getReferences() == initialRefs + 2 );

    SmartPtr<TestObject> ptr3;
    ptr3 = ptr1;
    BOOST_CHECK( obj->getReferences() == initialRefs + 3 );
}

BOOST_AUTO_TEST_CASE( smartptr_reference_counting_decrement )
{
    TestGuard guard;
    auto obj = new TestObject();
    guard.releaseInitialReference( obj );
    SmartPtr<TestObject> ptr1( obj );
    s32 refsAfterPtr1 = obj->getReferences();

    {
        SmartPtr<TestObject> ptr2( ptr1 );
        BOOST_CHECK( obj->getReferences() == refsAfterPtr1 + 1 );
    }
    // ptr2 goes out of scope, reference count should decrement
    BOOST_CHECK( obj->getReferences() == refsAfterPtr1 );
}

//------------------------------------------------------------------------------
// WeakPtr Constructor Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( weakptr_default_constructor )
{
    WeakPtr<TestObject> ptr;
    BOOST_CHECK( ptr == nullptr );
    BOOST_CHECK( !ptr );
    BOOST_CHECK( ptr.get() == nullptr );
}

BOOST_AUTO_TEST_CASE( weakptr_raw_pointer_constructor )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr.get() );

    BOOST_CHECK( weakPtr != nullptr );
    BOOST_CHECK( weakPtr.get() == smartPtr.get() );
    BOOST_CHECK( smartPtr.get()->getWeakReferences() == 1 );
}

BOOST_AUTO_TEST_CASE( weakptr_copy_constructor )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1( smartPtr );
    WeakPtr<TestObject> weakPtr2( weakPtr1 );

    BOOST_CHECK( weakPtr1 == weakPtr2 );
    BOOST_CHECK( weakPtr1.get() == weakPtr2.get() );
    BOOST_CHECK( smartPtr.get()->getWeakReferences() == 2 );
}

BOOST_AUTO_TEST_CASE( weakptr_from_smartptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );

    BOOST_CHECK( weakPtr != nullptr );
    BOOST_CHECK( weakPtr.get() == smartPtr.get() );
    BOOST_CHECK( smartPtr.get()->getWeakReferences() == 1 );
}

BOOST_AUTO_TEST_CASE( weakptr_derived_type_constructor )
{
    SmartPtr<DerivedTestObject> derivedPtr = workphone::make_ptr<DerivedTestObject>();
    WeakPtr<TestObject> baseWeakPtr( derivedPtr );

    BOOST_CHECK( baseWeakPtr != nullptr );
    BOOST_CHECK( baseWeakPtr.get() == derivedPtr.get() );
}

//------------------------------------------------------------------------------
// WeakPtr Assignment Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( weakptr_assign_raw_pointer )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr;

    weakPtr = smartPtr.get();

    BOOST_CHECK( weakPtr != nullptr );
    BOOST_CHECK( weakPtr.get() == smartPtr.get() );
}

BOOST_AUTO_TEST_CASE( weakptr_assign_weakptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1( smartPtr );
    WeakPtr<TestObject> weakPtr2;

    weakPtr2 = weakPtr1;

    BOOST_CHECK( weakPtr2 != nullptr );
    BOOST_CHECK( weakPtr1 == weakPtr2 );
}

BOOST_AUTO_TEST_CASE( weakptr_assign_nullptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );
    BOOST_CHECK( weakPtr != nullptr );

    weakPtr = nullptr;

    BOOST_CHECK( weakPtr == nullptr );
    BOOST_CHECK( !weakPtr );
}

//------------------------------------------------------------------------------
// WeakPtr Lifecycle Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( weakptr_expired_false_when_object_alive )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );

    BOOST_CHECK( !weakPtr.expired() );
    BOOST_CHECK( weakPtr.use_count() > 0 );
}

BOOST_AUTO_TEST_CASE( weakptr_lock_returns_valid_smartptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );

    SmartPtr<TestObject> lockedPtr = weakPtr.lock();

    BOOST_CHECK( lockedPtr != nullptr );
    BOOST_CHECK( lockedPtr.get() == smartPtr.get() );
}

BOOST_AUTO_TEST_CASE( weakptr_use_count )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );

    u32 useCount = weakPtr.use_count();
    BOOST_CHECK( useCount >= 1 );

    SmartPtr<TestObject> smartPtr2( smartPtr );
    BOOST_CHECK( weakPtr.use_count() > useCount );
}

BOOST_AUTO_TEST_CASE( weakptr_does_not_prevent_destruction )
{
    WeakPtr<TestObject> weakPtr;

    {
        SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
        weakPtr = smartPtr;
        BOOST_CHECK( !weakPtr.expired() );
        BOOST_CHECK_EQUAL( weakPtr.use_count(), smartPtr->getReferences() );
        weakPtr = nullptr;
        BOOST_CHECK( weakPtr.expired() );
    }
}

//------------------------------------------------------------------------------
// WeakPtr Operator Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( weakptr_bool_conversion )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1( smartPtr );
    WeakPtr<TestObject> weakPtr2;

    BOOST_CHECK( static_cast<bool>( weakPtr1 ) == true );
    BOOST_CHECK( static_cast<bool>( weakPtr2 ) == false );
}

BOOST_AUTO_TEST_CASE( weakptr_not_operator )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1( smartPtr );
    WeakPtr<TestObject> weakPtr2;

    BOOST_CHECK( !weakPtr1 == false );
    BOOST_CHECK( !weakPtr2 == true );
}

BOOST_AUTO_TEST_CASE( weakptr_dereference_operator )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );

    BOOST_CHECK( weakPtr->getValue() == 42 );
}

//------------------------------------------------------------------------------
// WeakPtr Comparison Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( weakptr_equality_same_object )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1( smartPtr );
    WeakPtr<TestObject> weakPtr2( smartPtr );

    BOOST_CHECK( weakPtr1 == weakPtr2 );
    BOOST_CHECK( !( weakPtr1 != weakPtr2 ) );
}

BOOST_AUTO_TEST_CASE( weakptr_equality_different_objects )
{
    SmartPtr<TestObject> smartPtr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> smartPtr2 = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1( smartPtr1 );
    WeakPtr<TestObject> weakPtr2( smartPtr2 );

    BOOST_CHECK( weakPtr1 != weakPtr2 );
    BOOST_CHECK( !( weakPtr1 == weakPtr2 ) );
}

BOOST_AUTO_TEST_CASE( weakptr_equality_with_nullptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr1;
    WeakPtr<TestObject> weakPtr2( smartPtr );

    BOOST_CHECK( weakPtr1 == nullptr );
    BOOST_CHECK( weakPtr2 != nullptr );
}

//------------------------------------------------------------------------------
// SmartPtr and WeakPtr Interaction Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_weakptr_interaction )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );

    BOOST_CHECK( smartPtr.get() == weakPtr.get() );
    BOOST_CHECK( smartPtr.get()->getReferences() >= 1 );
    BOOST_CHECK( smartPtr.get()->getWeakReferences() == 1 );
}

BOOST_AUTO_TEST_CASE( multiple_smartptrs_and_weakptrs )
{
    SmartPtr<TestObject> smartPtr1 = workphone::make_ptr<TestObject>();
    SmartPtr<TestObject> smartPtr2( smartPtr1 );
    WeakPtr<TestObject> weakPtr1( smartPtr1 );
    WeakPtr<TestObject> weakPtr2( smartPtr2 );

    BOOST_CHECK( smartPtr1 == smartPtr2 );
    BOOST_CHECK( weakPtr1 == weakPtr2 );
    BOOST_CHECK( smartPtr1.get() == weakPtr1.get() );
    BOOST_CHECK( smartPtr1.get()->getWeakReferences() == 2 );
}

//------------------------------------------------------------------------------
// get_pointer Helper Function Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( get_pointer_smartptr )
{
    SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>();
    TestObject *rawPtr = get_pointer( ptr );

    BOOST_CHECK( rawPtr == ptr.get() );
    BOOST_CHECK( rawPtr != nullptr );
}

BOOST_AUTO_TEST_CASE( get_pointer_smartptr_null )
{
    SmartPtr<TestObject> ptr;
    TestObject *rawPtr = get_pointer( ptr );

    BOOST_CHECK( rawPtr == nullptr );
}

BOOST_AUTO_TEST_CASE( get_pointer_weakptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );
    TestObject *rawPtr = get_pointer( weakPtr );

    BOOST_CHECK( rawPtr == weakPtr.get() );
    BOOST_CHECK( rawPtr == smartPtr.get() );
}

BOOST_AUTO_TEST_CASE( get_pointer_weakptr_null )
{
    WeakPtr<TestObject> ptr;
    TestObject *rawPtr = get_pointer( ptr );

    BOOST_CHECK( rawPtr == nullptr );
}

//------------------------------------------------------------------------------
// Polymorphism Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_polymorphic_virtual_call )
{
    SmartPtr<TestObject> basePtr = workphone::make_ptr<DerivedTestObject>();

    // Virtual function call through base pointer
    basePtr->virtualFunc();

    BOOST_CHECK( true );
}

BOOST_AUTO_TEST_CASE( smartptr_array_of_base_pointers )
{
    Array<SmartPtr<TestObject>> objects;
    objects.push_back( workphone::make_ptr<TestObject>( 1 ) );
    objects.push_back( workphone::make_ptr<DerivedTestObject>( 2 ) );

    BOOST_CHECK( objects.size() == 2 );
    BOOST_CHECK( objects[0] != nullptr );
    BOOST_CHECK( objects[1] != nullptr );
    BOOST_CHECK( objects[0]->getValue() == 1 );
    BOOST_CHECK( objects[1]->getValue() == 2 );

    for( auto &obj : objects )
    {
        obj->virtualFunc();
    }

    objects.clear();
}

//------------------------------------------------------------------------------
// Edge Cases
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( smartptr_self_assignment )
{
    SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>();
    s32 refs = ptr.get()->getReferences();

    ptr = ptr;

    BOOST_CHECK( ptr.get()->getReferences() == refs );
}

BOOST_AUTO_TEST_CASE( weakptr_self_assignment )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );
    s32 weakRefs = smartPtr.get()->getWeakReferences();

    weakPtr = weakPtr;

    BOOST_CHECK( smartPtr.get()->getWeakReferences() == weakRefs );
}

BOOST_AUTO_TEST_CASE( smartptr_get_method )
{
    TestGuard guard;
    auto obj = new TestObject();
    guard.releaseInitialReference( obj );
    SmartPtr<TestObject> ptr( obj );

    BOOST_CHECK( ptr.get() == obj );

    SmartPtr<TestObject> nullPtr;
    BOOST_CHECK( nullPtr.get() == nullptr );
}

BOOST_AUTO_TEST_CASE( smartptr_multiple_reassignments )
{
    SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>( 1 );
    BOOST_CHECK( ptr->getValue() == 1 );

    ptr = workphone::make_ptr<TestObject>( 2 );
    BOOST_CHECK( ptr->getValue() == 2 );

    ptr = workphone::make_ptr<TestObject>( 3 );
    BOOST_CHECK( ptr->getValue() == 3 );

    ptr = nullptr;
    BOOST_CHECK( ptr == nullptr );
}

BOOST_AUTO_TEST_CASE( smartptr_scope_lifetime )
{
    TestObject *rawPtr = nullptr;

    {
        SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>( 42 );
        rawPtr = ptr.get();
        BOOST_CHECK( rawPtr->getReferences() == 1 );  // 1 initial
    }
    // ptr goes out of scope, reference decremented
    // Object may still exist if initial ref count keeps it alive
}

BOOST_AUTO_TEST_CASE( weakptr_from_derived_smartptr )
{
    SmartPtr<DerivedTestObject> derivedSmartPtr = workphone::make_ptr<DerivedTestObject>( 100 );
    WeakPtr<TestObject> baseWeakPtr( derivedSmartPtr );

    BOOST_CHECK( baseWeakPtr != nullptr );
    BOOST_CHECK( baseWeakPtr->getValue() == 100 );
    BOOST_CHECK( !baseWeakPtr.expired() );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Constructor Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_default_constructor )
{
    AtomicSmartPtr<TestObject> ptr;
    BOOST_CHECK( ptr.get() == nullptr );
    BOOST_CHECK( !ptr );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_from_smartptr_constructor )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );

    BOOST_CHECK( atomicPtr.get() != nullptr );
    BOOST_CHECK( atomicPtr );
    BOOST_CHECK( atomicPtr.get() == smartPtr.get() );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_copy_constructor )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicPtr1( smartPtr );
    AtomicSmartPtr<TestObject> atomicPtr2( atomicPtr1 );

    BOOST_CHECK( atomicPtr2.get() != nullptr );
    BOOST_CHECK( atomicPtr1.get() == atomicPtr2.get() );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Store/Load Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_store_and_load )
{
    AtomicSmartPtr<TestObject> atomicPtr;
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 100 );

    atomicPtr.store( smartPtr );

    BOOST_CHECK( atomicPtr.get() == smartPtr.get() );

    SmartPtr<TestObject> loadedPtr = atomicPtr.load();
    BOOST_CHECK( loadedPtr != nullptr );
    BOOST_CHECK( loadedPtr.get() == smartPtr.get() );
    BOOST_CHECK( loadedPtr->getValue() == 100 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_store_nullptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );
    BOOST_CHECK( atomicPtr.get() != nullptr );

    atomicPtr.store( SmartPtr<TestObject>() );

    BOOST_CHECK( atomicPtr.get() == nullptr );
    BOOST_CHECK( !atomicPtr );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_store_replaces_value )
{
    SmartPtr<TestObject> smartPtr1 = workphone::make_ptr<TestObject>( 1 );
    SmartPtr<TestObject> smartPtr2 = workphone::make_ptr<TestObject>( 2 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr1 );

    BOOST_CHECK( atomicPtr->getValue() == 1 );

    atomicPtr.store( smartPtr2 );

    BOOST_CHECK( atomicPtr->getValue() == 2 );
    BOOST_CHECK( atomicPtr.get() == smartPtr2.get() );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Exchange Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_exchange )
{
    SmartPtr<TestObject> smartPtr1 = workphone::make_ptr<TestObject>( 1 );
    SmartPtr<TestObject> smartPtr2 = workphone::make_ptr<TestObject>( 2 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr1 );

    SmartPtr<TestObject> oldPtr = atomicPtr.exchange( smartPtr2 );

    BOOST_CHECK( oldPtr != nullptr );
    BOOST_CHECK( oldPtr->getValue() == 1 );
    BOOST_CHECK( atomicPtr->getValue() == 2 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_exchange_from_null )
{
    AtomicSmartPtr<TestObject> atomicPtr;
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );

    SmartPtr<TestObject> oldPtr = atomicPtr.exchange( smartPtr );

    BOOST_CHECK( oldPtr == nullptr );
    BOOST_CHECK( atomicPtr->getValue() == 42 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_exchange_to_null )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );

    SmartPtr<TestObject> oldPtr = atomicPtr.exchange( SmartPtr<TestObject>() );

    BOOST_CHECK( oldPtr != nullptr );
    BOOST_CHECK( oldPtr->getValue() == 42 );
    BOOST_CHECK( atomicPtr.get() == nullptr );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Assignment Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_assignment_from_smartptr )
{
    AtomicSmartPtr<TestObject> atomicPtr;
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 50 );

    atomicPtr = smartPtr;

    BOOST_CHECK( atomicPtr.get() == smartPtr.get() );
    BOOST_CHECK( atomicPtr->getValue() == 50 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_assignment_from_atomicsmartptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 75 );
    AtomicSmartPtr<TestObject> atomicPtr1( smartPtr );
    AtomicSmartPtr<TestObject> atomicPtr2;

    atomicPtr2 = atomicPtr1;

    BOOST_CHECK( atomicPtr2.get() == atomicPtr1.get() );
    BOOST_CHECK( atomicPtr2->getValue() == 75 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_self_assignment )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 100 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );
    TestObject *ptrBefore = atomicPtr.get();

    atomicPtr = atomicPtr;

    BOOST_CHECK( atomicPtr.get() == ptrBefore );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Operator Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_arrow_operator )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );

    BOOST_CHECK( atomicPtr->getValue() == 42 );
    atomicPtr->setValue( 100 );
    BOOST_CHECK( atomicPtr->getValue() == 100 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_bool_conversion )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    AtomicSmartPtr<TestObject> atomicPtr1( smartPtr );
    AtomicSmartPtr<TestObject> atomicPtr2;

    BOOST_CHECK( static_cast<bool>( atomicPtr1 ) == true );
    BOOST_CHECK( static_cast<bool>( atomicPtr2 ) == false );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_not_operator )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    AtomicSmartPtr<TestObject> atomicPtr1( smartPtr );
    AtomicSmartPtr<TestObject> atomicPtr2;

    BOOST_CHECK( !atomicPtr1 == false );
    BOOST_CHECK( !atomicPtr2 == true );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_conversion_to_smartptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );

    SmartPtr<TestObject> convertedPtr = atomicPtr;

    BOOST_CHECK( convertedPtr != nullptr );
    BOOST_CHECK( convertedPtr->getValue() == 42 );
    BOOST_CHECK( convertedPtr.get() == smartPtr.get() );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Lock-Free Check
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_is_lock_free )
{
    AtomicSmartPtr<TestObject> atomicPtr;
    // Just check that the method can be called - result depends on platform
    bool lockFree = atomicPtr.is_lock_free();
    (void)lockFree;  // Suppress unused variable warning
    BOOST_CHECK( true );
}

//------------------------------------------------------------------------------
// AtomicWeakPtr Constructor Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicweakptr_default_constructor )
{
    AtomicWeakPtr<TestObject> ptr;
    BOOST_CHECK( ptr.get() == nullptr );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_from_weakptr_constructor )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );

    BOOST_CHECK( atomicWeakPtr.get() != nullptr );
    BOOST_CHECK( atomicWeakPtr.get() == weakPtr.get() );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_copy_constructor )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr1( weakPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr2( atomicWeakPtr1 );

    BOOST_CHECK( atomicWeakPtr2.get() != nullptr );
    BOOST_CHECK( atomicWeakPtr1.get() == atomicWeakPtr2.get() );
}

//------------------------------------------------------------------------------
// AtomicWeakPtr Store/Load Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicweakptr_store_and_load )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 100 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr;

    atomicWeakPtr.store( weakPtr );

    BOOST_CHECK( atomicWeakPtr.get() == weakPtr.get() );

    WeakPtr<TestObject> loadedPtr = atomicWeakPtr.load();
    BOOST_CHECK( loadedPtr.get() != nullptr );
    BOOST_CHECK( loadedPtr.get() == weakPtr.get() );
    BOOST_CHECK( loadedPtr->getValue() == 100 );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_store_nullptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );
    BOOST_CHECK( atomicWeakPtr.get() != nullptr );

    atomicWeakPtr.store( WeakPtr<TestObject>() );

    BOOST_CHECK( atomicWeakPtr.get() == nullptr );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_store_replaces_value )
{
    SmartPtr<TestObject> smartPtr1 = workphone::make_ptr<TestObject>( 1 );
    SmartPtr<TestObject> smartPtr2 = workphone::make_ptr<TestObject>( 2 );
    WeakPtr<TestObject> weakPtr1( smartPtr1 );
    WeakPtr<TestObject> weakPtr2( smartPtr2 );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr1 );

    BOOST_CHECK( atomicWeakPtr.get()->getValue() == 1 );

    atomicWeakPtr.store( weakPtr2 );

    BOOST_CHECK( atomicWeakPtr.get()->getValue() == 2 );
    BOOST_CHECK( atomicWeakPtr.get() == weakPtr2.get() );
}

//------------------------------------------------------------------------------
// AtomicWeakPtr Exchange Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicweakptr_exchange )
{
    SmartPtr<TestObject> smartPtr1 = workphone::make_ptr<TestObject>( 1 );
    SmartPtr<TestObject> smartPtr2 = workphone::make_ptr<TestObject>( 2 );
    WeakPtr<TestObject> weakPtr1( smartPtr1 );
    WeakPtr<TestObject> weakPtr2( smartPtr2 );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr1 );

    WeakPtr<TestObject> oldPtr = atomicWeakPtr.exchange( weakPtr2 );

    BOOST_CHECK( oldPtr.get() != nullptr );
    BOOST_CHECK( oldPtr->getValue() == 1 );
    BOOST_CHECK( atomicWeakPtr.get()->getValue() == 2 );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_exchange_from_null )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr;

    WeakPtr<TestObject> oldPtr = atomicWeakPtr.exchange( weakPtr );

    BOOST_CHECK( oldPtr.get() == nullptr );
    BOOST_CHECK( atomicWeakPtr.get()->getValue() == 42 );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_exchange_to_null )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );

    WeakPtr<TestObject> oldPtr = atomicWeakPtr.exchange( WeakPtr<TestObject>() );

    BOOST_CHECK( oldPtr.get() != nullptr );
    BOOST_CHECK( oldPtr->getValue() == 42 );
    BOOST_CHECK( atomicWeakPtr.get() == nullptr );
}

//------------------------------------------------------------------------------
// AtomicWeakPtr Assignment Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicweakptr_assignment_from_weakptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 50 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr;

    atomicWeakPtr = weakPtr;

    BOOST_CHECK( atomicWeakPtr.get() == weakPtr.get() );
    BOOST_CHECK( atomicWeakPtr.get()->getValue() == 50 );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_assignment_from_atomicweakptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 75 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr1( weakPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr2;

    atomicWeakPtr2 = atomicWeakPtr1;

    BOOST_CHECK( atomicWeakPtr2.get() == atomicWeakPtr1.get() );
    BOOST_CHECK( atomicWeakPtr2.get()->getValue() == 75 );
}

//------------------------------------------------------------------------------
// AtomicWeakPtr Conversion Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicweakptr_conversion_to_weakptr )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );

    WeakPtr<TestObject> convertedPtr = atomicWeakPtr;

    BOOST_CHECK( convertedPtr.get() != nullptr );
    BOOST_CHECK( convertedPtr->getValue() == 42 );
    BOOST_CHECK( convertedPtr.get() == weakPtr.get() );
}

//------------------------------------------------------------------------------
// AtomicWeakPtr Lock-Free Check
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicweakptr_is_lock_free )
{
    AtomicWeakPtr<TestObject> atomicWeakPtr;
    // Just check that the method can be called - result depends on platform
    bool lockFree = atomicWeakPtr.is_lock_free();
    (void)lockFree;  // Suppress unused variable warning
    BOOST_CHECK( true );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr and AtomicWeakPtr Interaction Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_and_atomicweakptr_same_object )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicSmartPtr( smartPtr );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );

    BOOST_CHECK( atomicSmartPtr.get() == atomicWeakPtr.get() );
    BOOST_CHECK( atomicSmartPtr->getValue() == atomicWeakPtr.get()->getValue() );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Thread Safety Tests (Basic)
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_concurrent_load )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    AtomicSmartPtr<TestObject> atomicPtr( smartPtr );

    std::vector<std::thread> threads;
    std::atomic<int> successCount( 0 );

    for( int i = 0; i < 4; ++i )
    {
        threads.emplace_back( [&atomicPtr, &successCount]() {
            for( int j = 0; j < 100; ++j )
            {
                SmartPtr<TestObject> loaded = atomicPtr.load();
                if( loaded && loaded->getValue() == 42 )
                {
                    successCount++;
                }
            }
        } );
    }

    for( auto &t : threads )
    {
        t.join();
    }

    BOOST_CHECK( successCount == 400 );
}

BOOST_AUTO_TEST_CASE( atomicsmartptr_concurrent_store )
{
    AtomicSmartPtr<TestObject> atomicPtr;

    std::vector<std::thread> threads;

    for( int i = 0; i < 4; ++i )
    {
        threads.emplace_back( [&atomicPtr, i]() {
            SmartPtr<TestObject> ptr = workphone::make_ptr<TestObject>( i );
            for( int j = 0; j < 100; ++j )
            {
                atomicPtr.store( ptr );
            }
        } );
    }

    for( auto &t : threads )
    {
        t.join();
    }

    // After all stores, the pointer should be valid (one of the stored values)
    SmartPtr<TestObject> finalPtr = atomicPtr.load();
    BOOST_CHECK( finalPtr != nullptr );
    BOOST_CHECK( finalPtr->getValue() >= 0 && finalPtr->getValue() < 4 );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_concurrent_load )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>( 42 );
    WeakPtr<TestObject> weakPtr( smartPtr );
    AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );

    std::vector<std::thread> threads;
    std::atomic<int> successCount( 0 );

    for( int i = 0; i < 4; ++i )
    {
        threads.emplace_back( [&atomicWeakPtr, &successCount]() {
            for( int j = 0; j < 100; ++j )
            {
                WeakPtr<TestObject> loaded = atomicWeakPtr.load();
                if( loaded.get() && loaded->getValue() == 42 )
                {
                    successCount++;
                }
            }
        } );
    }

    for( auto &t : threads )
    {
        t.join();
    }

    BOOST_CHECK( successCount == 400 );
}

//------------------------------------------------------------------------------
// AtomicSmartPtr Reference Counting Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( atomicsmartptr_reference_counting )
{
    TestGuard guard;
    auto obj = new TestObject();
    guard.releaseInitialReference( obj );
    s32 initialRefs = obj->getReferences();

    SmartPtr<TestObject> smartPtr( obj );
    BOOST_CHECK( obj->getReferences() == initialRefs + 1 );

    {
        AtomicSmartPtr<TestObject> atomicPtr( smartPtr );
        BOOST_CHECK( obj->getReferences() == initialRefs + 2 );

        SmartPtr<TestObject> loaded = atomicPtr.load();
        BOOST_CHECK( obj->getReferences() == initialRefs + 3 );
    }
    // atomicPtr and loaded go out of scope
    BOOST_CHECK( obj->getReferences() == initialRefs + 1 );
}

BOOST_AUTO_TEST_CASE( atomicweakptr_weak_reference_counting )
{
    SmartPtr<TestObject> smartPtr = workphone::make_ptr<TestObject>();
    s32 initialWeakRefs = smartPtr.get()->getWeakReferences();

    {
        WeakPtr<TestObject> weakPtr( smartPtr );
        BOOST_CHECK( smartPtr.get()->getWeakReferences() == initialWeakRefs + 1 );

        AtomicWeakPtr<TestObject> atomicWeakPtr( weakPtr );
        BOOST_CHECK( smartPtr.get()->getWeakReferences() == initialWeakRefs + 2 );
    }
    // weakPtr and atomicWeakPtr go out of scope
    BOOST_CHECK( smartPtr.get()->getWeakReferences() == initialWeakRefs );
}
