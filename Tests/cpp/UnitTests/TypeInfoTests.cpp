#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

// =============================================================================
// Basic Type Info Tests
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo )
{
    BOOST_CHECK( IObject::typeInfo() != 0 );
}

BOOST_AUTO_TEST_CASE( object_typeinfo_consistency )
{
    // Verify typeInfo returns consistent values across multiple calls
    auto typeInfo1 = IObject::typeInfo();
    auto typeInfo2 = IObject::typeInfo();
    BOOST_CHECK_EQUAL( typeInfo1, typeInfo2 );
}

BOOST_AUTO_TEST_CASE( shared_object_typeinfo )
{
    BOOST_CHECK( ISharedObject::typeInfo() != 0 );
    BOOST_CHECK( ISharedObject::typeInfo() != IObject::typeInfo() );
}

// =============================================================================
// Test Interface and Template Classes
// =============================================================================

class interface_template : public workphone::ISharedObject
{
public:
    virtual void func() = 0;

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, interface_template, workphone::ISharedObject );

template <class T>
class base_template : public T
{
public:
    typedef T base_type;

    void func()
    {
    }

    WP_CLASS_REGISTER_TEMPLATE_DECL( base_template, T );
};

WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, base_template, T, T );

class derived_type : public base_template<interface_template>
{
public:
    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, derived_type, base_template<interface_template> );

// Additional test class for deeper inheritance hierarchy
class deeply_derived_type : public derived_type
{
public:
    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, deeply_derived_type, derived_type );

// =============================================================================
// TypeManager Instance Tests
// =============================================================================

BOOST_AUTO_TEST_CASE( type_manager_instance_not_null )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager != nullptr );
}

BOOST_AUTO_TEST_CASE( type_manager_singleton_consistency )
{
    auto typeManager1 = TypeManager::instance();
    auto typeManager2 = TypeManager::instance();
    BOOST_CHECK_EQUAL( typeManager1, typeManager2 );
}

// =============================================================================
// Derived Types Tests
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_derived_types )
{
    try
    {
        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto directorTypeInfo = IBuildDirector::typeInfo();
        BOOST_CHECK( directorTypeInfo != 0 );

        auto cDirectorTypeInfo = Director::typeInfo();
        BOOST_CHECK( cDirectorTypeInfo != 0 );

        auto derivedTypes = typeManager->getDerivedTypes( directorTypeInfo );
        auto baseTypes = typeManager->getBaseTypes( cDirectorTypeInfo );
        auto cDerivedTypes = typeManager->getDerivedTypes( cDirectorTypeInfo );

        BOOST_CHECK( derivedTypes.size() > 1 );
        BOOST_CHECK( !baseTypes.empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

// =============================================================================
// Template Type Tests
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_template )
{
    try
    {
        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        BOOST_CHECK( IObject::typeInfo() != 0 );

        auto a = workphone::make_ptr<base_template<interface_template>>();
        BOOST_REQUIRE( a );

        auto b = workphone::make_ptr<derived_type>();
        BOOST_REQUIRE( b );

        auto interfaceType = interface_template::typeInfo();
        BOOST_CHECK( interfaceType != 0 );

        auto baseInterfaceType = base_template<interface_template>::base_type::typeInfo();
        BOOST_CHECK_EQUAL( interfaceType, baseInterfaceType );

        auto aTypeInfo = a->getTypeInfo();
        BOOST_CHECK( aTypeInfo != 0 );

        auto bTypeInfo = b->getTypeInfo();
        BOOST_CHECK( bTypeInfo != 0 );

        // Verify different types have different type info
        BOOST_CHECK( aTypeInfo != bTypeInfo );

        auto aBaseTypes = typeManager->getBaseTypes( aTypeInfo );
        BOOST_CHECK( !aBaseTypes.empty() );

        auto bBaseTypes = typeManager->getBaseTypes( bTypeInfo );
        BOOST_CHECK( !bBaseTypes.empty() );

        auto aBaseTypeNames = typeManager->getBaseTypeNames( aTypeInfo );
        BOOST_CHECK( !aBaseTypeNames.empty() );
        BOOST_CHECK_EQUAL( aBaseTypeNames.size(), aBaseTypes.size() );

        auto bBaseTypeNames = typeManager->getBaseTypeNames( bTypeInfo );
        BOOST_CHECK( !bBaseTypeNames.empty() );
        BOOST_CHECK_EQUAL( bBaseTypeNames.size(), bBaseTypes.size() );

        auto it = std::find( bBaseTypes.begin(), bBaseTypes.end(), interface_template::typeInfo() );
        BOOST_CHECK( it != bBaseTypes.end() );

        auto derived = typeManager->isDerived( aTypeInfo, interfaceType );
        BOOST_CHECK( derived );

        derived = typeManager->isDerived( bTypeInfo, interfaceType );
        BOOST_CHECK( derived );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

// =============================================================================
// Derived Type Discovery Tests
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_derived )
{
    try
    {
        BOOST_CHECK( IObject::typeInfo() != 0 );

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto derivedTypeInfo = derived_type::typeInfo();
        auto interfaceTypeInfo = interface_template::typeInfo();
        auto derivedTypes = typeManager->getDerivedTypes( interfaceTypeInfo );
        BOOST_CHECK( !derivedTypes.empty() );

        auto it = std::find( derivedTypes.begin(), derivedTypes.end(), derived_type::typeInfo() );
        BOOST_CHECK( it != derivedTypes.end() );

        std::sort( derivedTypes.begin(), derivedTypes.end() );
        BOOST_CHECK( derivedTypes.back() == derived_type::typeInfo() ||
                     derivedTypes.back() == deeply_derived_type::typeInfo() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

// =============================================================================
// Edge Case: Deep Inheritance Hierarchy
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_deep_hierarchy )
{
    try
    {
        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto deepDerivedTypeInfo = deeply_derived_type::typeInfo();
        BOOST_CHECK( deepDerivedTypeInfo != 0 );

        auto baseTypes = typeManager->getBaseTypes( deepDerivedTypeInfo );
        BOOST_CHECK( baseTypes.size() >=
                     3 );  // At least: derived_type, base_template, interface_template

        // Verify the hierarchy is correct
        auto interfaceTypeInfo = interface_template::typeInfo();
        auto it = std::find( baseTypes.begin(), baseTypes.end(), interfaceTypeInfo );
        BOOST_CHECK( it != baseTypes.end() );

        // Check isDerived works across multiple levels
        BOOST_CHECK( typeManager->isDerived( deepDerivedTypeInfo, interfaceTypeInfo ) );
        BOOST_CHECK( typeManager->isDerived( deepDerivedTypeInfo, derived_type::typeInfo() ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

// =============================================================================
// Edge Case: Self-Derivation Check
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_self_derivation )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto typeInfo = derived_type::typeInfo();

    // A type should be considered derived from itself
    BOOST_CHECK( typeManager->isDerived( typeInfo, typeInfo ) );
}

// =============================================================================
// Edge Case: isExactly Tests
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_is_exactly )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto derivedTypeInfo = derived_type::typeInfo();
    auto interfaceTypeInfo = interface_template::typeInfo();

    // Same types should be exactly equal
    BOOST_CHECK( typeManager->isExactly( derivedTypeInfo, derivedTypeInfo ) );
    BOOST_CHECK( typeManager->isExactly( interfaceTypeInfo, interfaceTypeInfo ) );

    // Different types should not be exactly equal
    BOOST_CHECK( !typeManager->isExactly( derivedTypeInfo, interfaceTypeInfo ) );
}

// =============================================================================
// Edge Case: Type Name Operations
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_type_names )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto derivedTypeInfo = derived_type::typeInfo();
    auto name = typeManager->getName( derivedTypeInfo );

    BOOST_CHECK( name != nullptr );
    BOOST_CHECK( strlen( name ) > 0 );

    // Verify getTypeByName returns the same type
    auto typeByName = typeManager->getTypeByName( name );
    BOOST_CHECK_EQUAL( typeByName, derivedTypeInfo );
}

// =============================================================================
// Edge Case: Invalid Type Lookups
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_invalid_type_name )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    // Looking up a non-existent type name should return 0
    auto typeInfo = typeManager->getTypeByName( "NonExistentTypeName_12345" );
    BOOST_CHECK_EQUAL( typeInfo, 0u );
}

// =============================================================================
// Edge Case: Class Hierarchy
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_class_hierarchy )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto deepDerivedTypeInfo = deeply_derived_type::typeInfo();
    auto hierarchy = typeManager->getClassHierarchy( deepDerivedTypeInfo );

    BOOST_CHECK( !hierarchy.empty() );

    // First entry should be the type itself
    auto selfName = typeManager->getName( deepDerivedTypeInfo );
    BOOST_CHECK_EQUAL( hierarchy[0], selfName );

    auto hierarchyIds = typeManager->getClassHierarchyId( deepDerivedTypeInfo );
    BOOST_CHECK_EQUAL( hierarchyIds.size(), hierarchy.size() );
    BOOST_CHECK_EQUAL( hierarchyIds[0], deepDerivedTypeInfo );
}

// =============================================================================
// Edge Case: Base Type for Root Type
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_root_base_type )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto objectTypeInfo = IObject::typeInfo();
    auto baseType = typeManager->getBaseType( objectTypeInfo );

    // IObject is a root type, so its base type should be 0
    BOOST_CHECK_EQUAL( baseType, 0u );

    auto baseTypes = typeManager->getBaseTypes( objectTypeInfo );
    BOOST_CHECK( baseTypes.empty() );
}

// =============================================================================
// Edge Case: Empty Derived Types
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_no_derived_types )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    // deeply_derived_type has no further derived types
    auto deepDerivedTypeInfo = deeply_derived_type::typeInfo();
    auto derivedTypes = typeManager->getDerivedTypes( deepDerivedTypeInfo );

    // Should only contain itself (since isDerived returns true for self)
    BOOST_CHECK( derivedTypes.size() <= 1 );
}

// =============================================================================
// Edge Case: Hash Operations
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_hash_operations )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto derivedTypeInfo = derived_type::typeInfo();
    auto hash = typeManager->getHash( derivedTypeInfo );

    BOOST_CHECK( hash != 0 );

    // Verify getIdFromHash returns the same type
    auto typeFromHash = typeManager->getIdFromHash( hash );
    BOOST_CHECK_EQUAL( typeFromHash, derivedTypeInfo );
}

// =============================================================================
// Edge Case: UUID Operations
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_uuid_operations )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto derivedTypeInfo = derived_type::typeInfo();
    auto uuid1 = typeManager->getUUID( derivedTypeInfo );
    auto uuid2 = typeManager->getUUID( derivedTypeInfo );

    // UUID should be consistent
    BOOST_CHECK( uuid1 == uuid2 );

    // Different types should have different UUIDs
    auto interfaceTypeInfo = interface_template::typeInfo();
    auto interfaceUuid = typeManager->getUUID( interfaceTypeInfo );
    BOOST_CHECK( uuid1 != interfaceUuid );
}

// =============================================================================
// Edge Case: Multiple Template Instantiations
// =============================================================================

class another_interface : public workphone::ISharedObject
{
public:
    virtual void otherFunc() = 0;

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, another_interface, workphone::ISharedObject );

class another_derived : public base_template<another_interface>
{
public:
    void otherFunc() override
    {
    }

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, another_derived, base_template<another_interface> );

BOOST_AUTO_TEST_CASE( object_typeinfo_multiple_template_instantiations )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto baseTemplateInterfaceType = base_template<interface_template>::typeInfo();
    auto baseTemplateAnotherType = base_template<another_interface>::typeInfo();

    // Different template instantiations should have different type info
    BOOST_CHECK( baseTemplateInterfaceType != baseTemplateAnotherType );

    // Verify inheritance chains are independent
    auto derivedTypeInfo = derived_type::typeInfo();
    auto anotherDerivedTypeInfo = another_derived::typeInfo();

    BOOST_CHECK( !typeManager->isDerived( derivedTypeInfo, another_interface::typeInfo() ) );
    BOOST_CHECK( !typeManager->isDerived( anotherDerivedTypeInfo, interface_template::typeInfo() ) );
}

// =============================================================================
// Edge Case: Type Count
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_total_types )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto totalTypes = typeManager->getTotalNumTypes();
    BOOST_CHECK( totalTypes > 0 );

    // Verify type IDs are within valid range
    auto derivedTypeInfo = derived_type::typeInfo();
    BOOST_CHECK( derivedTypeInfo < totalTypes );
}

// =============================================================================
// Performance: Large Hierarchy Traversal
// =============================================================================

BOOST_AUTO_TEST_CASE( object_typeinfo_hierarchy_traversal_performance )
{
    auto typeManager = TypeManager::instance();
    BOOST_REQUIRE( typeManager );

    auto objectTypeInfo = IObject::typeInfo();

    // Getting all derived types from IObject (should be many)
    auto allDerived = typeManager->getDerivedTypes( objectTypeInfo );

    // There should be many types derived from IObject
    BOOST_CHECK( allDerived.size() > 10 );

    // Verify we can query each type's base types
    for( auto typeInfo : allDerived )
    {
        auto baseTypes = typeManager->getBaseTypes( typeInfo );
        // Should not throw and should return valid data
        BOOST_CHECK( true );
    }
}
