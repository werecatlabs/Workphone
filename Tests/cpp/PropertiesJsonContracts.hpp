#pragma once

#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <initializer_list>
#include <stdexcept>

namespace properties_json_contracts
{
    using namespace workphone;

    inline void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    inline void run()
    {
        auto input = make_ptr<Properties>();
        input->setName( "root" );
        Property alpha( "alpha", "quoted \"value\"\nline\\end", "string" );
        alpha.setReadOnly( true );
        alpha.setAttribute( "unit", "metres" );
        alpha.setAttribute( "hint", "editor\nlabel" );
        input->addProperty( alpha );
        input->addProperty( Property( "zeta", "", "string" ) );

        auto first = make_ptr<Properties>();
        first->setName( "part" );
        first->addProperty( Property( "count", "42", "int" ) );
        auto leaf = make_ptr<Properties>();
        leaf->setName( "leaf" );
        leaf->addProperty( Property( "flag", "true", "bool" ) );
        first->addChild( leaf );
        input->addChild( first );
        auto second = make_ptr<Properties>();
        second->setName( "part" );
        input->addChild( second );

        const String compact =
            R"({"__wp_properties_v1":[{"attributes":{"hint":"editor\nlabel","unit":"metres"},"name":"alpha","readOnly":true,"type":"string"},{"attributes":{},"name":"zeta","readOnly":false,"type":"string"}],"alpha":"quoted \"value\"\nline\\end","children":[{"__wp_properties_v1":[{"attributes":{},"name":"count","readOnly":false,"type":"s32"}],"children":[{"__wp_properties_v1":[{"attributes":{},"name":"flag","readOnly":false,"type":"bool"}],"children":[],"flag":"true","handle_name":"leaf"}],"count":"42","handle_name":"part"},{"__wp_properties_v1":[],"children":[],"handle_name":"part"}],"handle_name":"root","zeta":""})";
        const String formatted = R"({
    "__wp_properties_v1" : [
        {
            "attributes" : {
                "hint" : "editor\nlabel",
                "unit" : "metres"
            },
            "name" : "alpha",
            "readOnly" : true,
            "type" : "string"
        },
        {
            "attributes" : {},
            "name" : "zeta",
            "readOnly" : false,
            "type" : "string"
        }
    ],
    "alpha" : "quoted \"value\"\nline\\end",
    "children" : [
        {
            "__wp_properties_v1" : [
                {
                    "attributes" : {},
                    "name" : "count",
                    "readOnly" : false,
                    "type" : "s32"
                }
            ],
            "children" : [
                {
                    "__wp_properties_v1" : [
                        {
                            "attributes" : {},
                            "name" : "flag",
                            "readOnly" : false,
                            "type" : "bool"
                        }
                    ],
                    "children" : [],
                    "flag" : "true",
                    "handle_name" : "leaf"
                }
            ],
            "count" : "42",
            "handle_name" : "part"
        },
        {
            "__wp_properties_v1" : [],
            "children" : [],
            "handle_name" : "part"
        }
    ],
    "handle_name" : "root",
    "zeta" : ""
}
)";
        require( DataUtil::toString( input.get(), false ) == compact,
                 "compact JSON preserves canonical values, metadata, escaping and ordered nested children" );
        require( DataUtil::toString( input.get(), true ) == formatted,
                 "formatted JSON preserves canonical indentation, field order and final newline" );
        for( const bool pretty : { false, true } )
        {
            auto restored = make_ptr<Properties>();
            restored->setName( "previous root" );
            DataUtil::parse( pretty ? formatted : compact, restored.get() );
            require( restored->getName() == "root",
                     "typed JSON parse restores the serialized root handle_name" );
            require( restored->getNumProperties() == 2 && restored->getNumChildren() == 2,
                     "JSON restores property counts and repeated-name children" );
            const auto &field = restored->getPropertyObject( "alpha" );
            require( field.getValue() == alpha.getValue() && field.getTypeName() == "string" &&
                         field.isReadOnly() && field.getAttributes().size() == 2 &&
                         field.getAttribute( "unit" ) == "metres" &&
                         field.getAttribute( "hint" ) == "editor\nlabel",
                     "JSON roundtrip preserves property type, read-only flag and every property attribute" );
            const auto &empty = restored->getPropertyObject( "zeta" );
            require( empty.getValue().empty() && empty.getTypeName() == "string" &&
                         !empty.isReadOnly() && empty.getAttributes().empty(),
                     "empty values and empty attribute objects survive JSON roundtrip" );
            auto restoredFirst = restored->getChild( 0 );
            auto restoredSecond = restored->getChild( 1 );
            require( restoredFirst && restoredSecond && restoredFirst->getName() == "part" &&
                         restoredSecond->getName() == "part" && restoredSecond->getNumProperties() == 0 &&
                         restoredSecond->getNumChildren() == 0 &&
                         restoredFirst->getPropertyAsInt( "count" ) == 42 &&
                         restoredFirst->getPropertyObject( "count" ).getTypeName() == "s32",
                     "repeated child names retain array order, typed values and empty child nodes" );
            auto restoredLeaf = restoredFirst->getChild( "leaf" );
            require( restoredLeaf && restoredLeaf->getPropertyAsBool( "flag" ) &&
                         restoredLeaf->getPropertyObject( "flag" ).getTypeName() == "bool",
                     "deep children retain their own typed property metadata" );
            require( DataUtil::toString( restored.get(), false ) == compact &&
                         DataUtil::toString( restored.get(), true ) == formatted,
                     "both JSON input formats roundtrip to identical canonical output" );
            auto sharedDestination = make_ptr<Properties>();
            sharedDestination->setName( "previous shared root" );
            DataUtil::parse( pretty ? formatted : compact,
                             static_cast<ISharedObject *>( sharedDestination.get() ) );
            require( sharedDestination->getName() == "root" &&
                         DataUtil::toString( sharedDestination.get(), false ) == compact,
                     "shared-object JSON parse preserves the same root name, metadata and children" );
        }
        require( DataUtil::toString( input.get(), false ) == compact && first->getNumChildren() == 1 &&
                     leaf->getPropertyAsBool( "flag" ),
                 "moving completed JSON trees leaves source properties and nested children intact" );
        require( DataUtil::toString( static_cast<Properties *>( nullptr ), false ) == "{}" &&
                     DataUtil::toString( static_cast<Properties *>( nullptr ), true ) == "{}\n",
                 "null properties retain empty-object serialization in both formats" );
    }
}
