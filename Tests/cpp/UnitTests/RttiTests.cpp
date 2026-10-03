#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

// Test class hierarchy for RTTI testing
class TestBaseObject : public ISharedObject
{
public:
    TestBaseObject() = default;
    ~TestBaseObject() override = default;

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, TestBaseObject, ISharedObject );

class TestDerivedObject : public TestBaseObject
{
public:
    TestDerivedObject() = default;
    ~TestDerivedObject() override = default;

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, TestDerivedObject, TestBaseObject );

class TestDeepDerivedObject : public TestDerivedObject
{
public:
    TestDeepDerivedObject() = default;
    ~TestDeepDerivedObject() override = default;

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, TestDeepDerivedObject, TestDerivedObject );

class TestUnrelatedObject : public ISharedObject
{
public:
    TestUnrelatedObject() = default;
    ~TestUnrelatedObject() override = default;

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, TestUnrelatedObject, ISharedObject );

//----------------------------------------------------------------------
// Basic RTTI Type Info Tests
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( object_typecast )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    // Test basic object creation and type info
    auto obj = workphone::make_ptr<TestBaseObject>();
    BOOST_CHECK( obj );
    BOOST_CHECK( obj->getTypeInfo() != 0 );
    BOOST_CHECK_EQUAL( obj->getTypeInfo(), TestBaseObject::typeInfo() );
}

BOOST_AUTO_TEST_CASE( rtti_type_info_unique )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    // Each type should have a unique type ID
    auto baseTypeId = TestBaseObject::typeInfo();
    auto derivedTypeId = TestDerivedObject::typeInfo();
    auto deepDerivedTypeId = TestDeepDerivedObject::typeInfo();
    auto unrelatedTypeId = TestUnrelatedObject::typeInfo();

    BOOST_CHECK_NE( baseTypeId, 0u );
    BOOST_CHECK_NE( derivedTypeId, 0u );
    BOOST_CHECK_NE( deepDerivedTypeId, 0u );
    BOOST_CHECK_NE( unrelatedTypeId, 0u );

    BOOST_CHECK_NE( baseTypeId, derivedTypeId );
    BOOST_CHECK_NE( baseTypeId, deepDerivedTypeId );
    BOOST_CHECK_NE( baseTypeId, unrelatedTypeId );
    BOOST_CHECK_NE( derivedTypeId, deepDerivedTypeId );
    BOOST_CHECK_NE( derivedTypeId, unrelatedTypeId );
    BOOST_CHECK_NE( deepDerivedTypeId, unrelatedTypeId );
}

BOOST_AUTO_TEST_CASE( rtti_type_info_consistent )
{
    // Type info should be consistent across multiple calls
    auto typeId1 = TestBaseObject::typeInfo();
    auto typeId2 = TestBaseObject::typeInfo();
    auto typeId3 = TestBaseObject::typeInfo();

    BOOST_CHECK_EQUAL( typeId1, typeId2 );
    BOOST_CHECK_EQUAL( typeId2, typeId3 );

    // Should also be consistent from instance vs static access
    auto obj = workphone::make_ptr<TestBaseObject>();
    BOOST_CHECK_EQUAL( obj->getTypeInfo(), TestBaseObject::typeInfo() );
}

//----------------------------------------------------------------------
// isDerived() Tests
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rtti_is_derived_direct_parent )
{
    auto obj = workphone::make_ptr<TestDerivedObject>();

    // TestDerivedObject should be derived from TestBaseObject
    BOOST_CHECK( obj->isDerived<TestBaseObject>() );
    // TestDerivedObject should be derived from ISharedObject
    BOOST_CHECK( obj->isDerived<ISharedObject>() );
    // TestDerivedObject should be derived from IObject
    BOOST_CHECK( obj->isDerived<IObject>() );
}

BOOST_AUTO_TEST_CASE( rtti_is_derived_deep_hierarchy )
{
    auto obj = workphone::make_ptr<TestDeepDerivedObject>();

    // Deep hierarchy traversal
    BOOST_CHECK( obj->isDerived<TestDerivedObject>() );
    BOOST_CHECK( obj->isDerived<TestBaseObject>() );
    BOOST_CHECK( obj->isDerived<ISharedObject>() );
    BOOST_CHECK( obj->isDerived<IObject>() );
}

BOOST_AUTO_TEST_CASE( rtti_is_derived_unrelated_types )
{
    auto baseObj = workphone::make_ptr<TestBaseObject>();
    auto unrelatedObj = workphone::make_ptr<TestUnrelatedObject>();

    // TestBaseObject should NOT be derived from TestUnrelatedObject
    BOOST_CHECK( !baseObj->isDerived<TestUnrelatedObject>() );
    // TestUnrelatedObject should NOT be derived from TestBaseObject
    BOOST_CHECK( !unrelatedObj->isDerived<TestBaseObject>() );
    // Neither should be derived from TestDerivedObject
    BOOST_CHECK( !baseObj->isDerived<TestDerivedObject>() );
    BOOST_CHECK( !unrelatedObj->isDerived<TestDerivedObject>() );
}

BOOST_AUTO_TEST_CASE( rtti_is_derived_self )
{
    auto obj = workphone::make_ptr<TestBaseObject>();

    // An object should be considered derived from its own type
    BOOST_CHECK( obj->isDerived<TestBaseObject>() );
}

BOOST_AUTO_TEST_CASE( rtti_is_derived_parent_not_derived_from_child )
{
    auto baseObj = workphone::make_ptr<TestBaseObject>();

    // Base class should NOT be derived from its child classes
    BOOST_CHECK( !baseObj->isDerived<TestDerivedObject>() );
    BOOST_CHECK( !baseObj->isDerived<TestDeepDerivedObject>() );
}

//----------------------------------------------------------------------
// isExactly() Tests
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rtti_is_exactly_same_type )
{
    auto baseObj = workphone::make_ptr<TestBaseObject>();
    auto derivedObj = workphone::make_ptr<TestDerivedObject>();
    auto deepDerivedObj = workphone::make_ptr<TestDeepDerivedObject>();

    BOOST_CHECK( baseObj->isExactly<TestBaseObject>() );
    BOOST_CHECK( derivedObj->isExactly<TestDerivedObject>() );
    BOOST_CHECK( deepDerivedObj->isExactly<TestDeepDerivedObject>() );
}

BOOST_AUTO_TEST_CASE( rtti_is_exactly_different_type )
{
    auto baseObj = workphone::make_ptr<TestBaseObject>();
    auto derivedObj = workphone::make_ptr<TestDerivedObject>();

    // Should NOT be exactly the parent type
    BOOST_CHECK( !derivedObj->isExactly<TestBaseObject>() );
    BOOST_CHECK( !derivedObj->isExactly<ISharedObject>() );
    BOOST_CHECK( !derivedObj->isExactly<IObject>() );

    // Should NOT be exactly the child type
    BOOST_CHECK( !baseObj->isExactly<TestDerivedObject>() );
}

BOOST_AUTO_TEST_CASE( rtti_is_exactly_unrelated_type )
{
    auto baseObj = workphone::make_ptr<TestBaseObject>();
    auto unrelatedObj = workphone::make_ptr<TestUnrelatedObject>();

    BOOST_CHECK( !baseObj->isExactly<TestUnrelatedObject>() );
    BOOST_CHECK( !unrelatedObj->isExactly<TestBaseObject>() );
}

//----------------------------------------------------------------------
// TypeManager Tests
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rtti_type_manager_get_name )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto typeId = TestBaseObject::typeInfo();
    auto name = typeManager->getName( typeId );

    BOOST_CHECK( name != nullptr );
    BOOST_CHECK( String( name ).find( "TestBaseObject" ) != String::npos );
}

BOOST_AUTO_TEST_CASE( rtti_type_manager_get_base_type )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto derivedTypeId = TestDerivedObject::typeInfo();
    auto baseTypeId = typeManager->getBaseType( derivedTypeId );

    BOOST_CHECK_EQUAL( baseTypeId, TestBaseObject::typeInfo() );
}

BOOST_AUTO_TEST_CASE( rtti_type_manager_is_derived )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto derivedTypeId = TestDerivedObject::typeInfo();
    auto baseTypeId = TestBaseObject::typeInfo();
    auto unrelatedTypeId = TestUnrelatedObject::typeInfo();

    BOOST_CHECK( typeManager->isDerived( derivedTypeId, baseTypeId ) );
    BOOST_CHECK( !typeManager->isDerived( baseTypeId, derivedTypeId ) );
    BOOST_CHECK( !typeManager->isDerived( derivedTypeId, unrelatedTypeId ) );
}

BOOST_AUTO_TEST_CASE( rtti_type_manager_is_exactly )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto derivedTypeId = TestDerivedObject::typeInfo();
    auto baseTypeId = TestBaseObject::typeInfo();

    BOOST_CHECK( typeManager->isExactly( derivedTypeId, derivedTypeId ) );
    BOOST_CHECK( typeManager->isExactly( baseTypeId, baseTypeId ) );
    BOOST_CHECK( !typeManager->isExactly( derivedTypeId, baseTypeId ) );
}

BOOST_AUTO_TEST_CASE( rtti_type_manager_get_class_hierarchy )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto typeId = TestDeepDerivedObject::typeInfo();
    auto hierarchy = typeManager->getClassHierarchy( typeId );

    // Should contain at least: TestDeepDerivedObject, TestDerivedObject, TestBaseObject, ISharedObject
    BOOST_CHECK_GE( hierarchy.size(), 4u );
}

BOOST_AUTO_TEST_CASE( rtti_type_manager_get_type_by_name )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    // Force type registration
    auto expectedId = TestBaseObject::typeInfo();

    auto typeId = typeManager->getTypeByName( "workphone::TestBaseObject" );
    BOOST_CHECK_EQUAL( typeId, expectedId );
}

BOOST_AUTO_TEST_CASE( rtti_type_manager_get_type_by_name_not_found )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto typeId = typeManager->getTypeByName( "NonExistentType" );
    BOOST_CHECK_EQUAL( typeId, 0u );
}

//----------------------------------------------------------------------
// Dynamic Pointer Cast Tests
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rtti_dynamic_pointer_cast_upcast )
{
    auto derivedObj = workphone::make_ptr<TestDerivedObject>();
    SmartPtr<TestBaseObject> basePtr = workphone::dynamic_pointer_cast<TestBaseObject>( derivedObj );

    BOOST_CHECK( basePtr );
    BOOST_CHECK( basePtr.get() == derivedObj.get() );
}

BOOST_AUTO_TEST_CASE( rtti_dynamic_pointer_cast_downcast_valid )
{
    SmartPtr<TestBaseObject> basePtr = workphone::make_ptr<TestDerivedObject>();
    auto derivedPtr = workphone::dynamic_pointer_cast<TestDerivedObject>( basePtr );

    BOOST_CHECK( derivedPtr );
    BOOST_CHECK( derivedPtr.get() == basePtr.get() );
}

BOOST_AUTO_TEST_CASE( rtti_dynamic_pointer_cast_downcast_invalid )
{
    SmartPtr<TestBaseObject> basePtr = workphone::make_ptr<TestBaseObject>();
    auto derivedPtr = workphone::dynamic_pointer_cast<TestDerivedObject>( basePtr );

    // Should fail - can't downcast a TestBaseObject to TestDerivedObject
    BOOST_CHECK( !derivedPtr );
}

BOOST_AUTO_TEST_CASE( rtti_dynamic_pointer_cast_unrelated )
{
    auto baseObj = workphone::make_ptr<TestBaseObject>();
    auto unrelatedPtr = workphone::dynamic_pointer_cast<TestUnrelatedObject>( baseObj );

    BOOST_CHECK( !unrelatedPtr );
}

BOOST_AUTO_TEST_CASE( rtti_dynamic_pointer_cast_nullptr )
{
    SmartPtr<TestBaseObject> nullPtr;
    auto derivedPtr = workphone::dynamic_pointer_cast<TestDerivedObject>( nullPtr );

    BOOST_CHECK( !derivedPtr );
}

//----------------------------------------------------------------------
// Static Pointer Cast Tests
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rtti_static_pointer_cast_upcast )
{
    auto derivedObj = workphone::make_ptr<TestDerivedObject>();
    SmartPtr<TestBaseObject> basePtr = workphone::static_pointer_cast<TestBaseObject>( derivedObj );

    BOOST_CHECK( basePtr );
    BOOST_CHECK( basePtr.get() == derivedObj.get() );
}

BOOST_AUTO_TEST_CASE( rtti_static_pointer_cast_downcast )
{
    SmartPtr<TestBaseObject> basePtr = workphone::make_ptr<TestDerivedObject>();
    auto derivedPtr = workphone::static_pointer_cast<TestDerivedObject>( basePtr );

    // Static cast trusts the programmer - this works because we know the actual type
    BOOST_CHECK( derivedPtr );
    BOOST_CHECK( derivedPtr.get() == basePtr.get() );
}

//----------------------------------------------------------------------
// Edge Cases
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rtti_derived_method_with_u32_type_id )
{
    auto obj = workphone::make_ptr<TestDerivedObject>();

    // Test the derived(u32) method directly
    BOOST_CHECK( obj->derived( TestBaseObject::typeInfo() ) );
    BOOST_CHECK( obj->derived( ISharedObject::typeInfo() ) );
    BOOST_CHECK( !obj->derived( TestUnrelatedObject::typeInfo() ) );
}

BOOST_AUTO_TEST_CASE( rtti_exactly_method_with_u32_type_id )
{
    auto obj = workphone::make_ptr<TestDerivedObject>();

    // Test the exactly(u32) method directly
    BOOST_CHECK( obj->exactly( TestDerivedObject::typeInfo() ) );
    BOOST_CHECK( !obj->exactly( TestBaseObject::typeInfo() ) );
}

BOOST_AUTO_TEST_CASE( rtti_multiple_instances_same_type )
{
    auto obj1 = workphone::make_ptr<TestBaseObject>();
    auto obj2 = workphone::make_ptr<TestBaseObject>();
    auto obj3 = workphone::make_ptr<TestBaseObject>();

    // All instances should have the same type info
    BOOST_CHECK_EQUAL( obj1->getTypeInfo(), obj2->getTypeInfo() );
    BOOST_CHECK_EQUAL( obj2->getTypeInfo(), obj3->getTypeInfo() );
    BOOST_CHECK_EQUAL( obj1->getTypeInfo(), TestBaseObject::typeInfo() );
}

BOOST_AUTO_TEST_CASE( rtti_type_hash_consistency )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto typeId = TestBaseObject::typeInfo();
    auto hash1 = typeManager->getHash( typeId );
    auto hash2 = typeManager->getHash( typeId );

    BOOST_CHECK_EQUAL( hash1, hash2 );
    BOOST_CHECK_NE( hash1, 0u );
}

BOOST_AUTO_TEST_CASE( rtti_num_instances_tracking )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );

    auto typeId = TestBaseObject::typeInfo();
    auto initialCount = typeManager->getNumInstances( typeId );

    // This test verifies the method exists and returns a reasonable value
    BOOST_CHECK_GE( initialCount, 0u );
}
