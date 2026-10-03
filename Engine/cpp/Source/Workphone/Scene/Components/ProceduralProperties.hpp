#pragma once
#include <Workphone/Core/Properties.hpp>
#include <initializer_list>
namespace workphone::scene::procedural_properties
{
    inline void setEnum( SmartPtr<Properties> p, const String &key, s32 selected,
                         std::initializer_list<const char *> labels )
    {
        String options, value;
        s32 index = 0;
        for( const auto label : labels )
        {
            options += String( label ) + ";";
            if( index++ == selected )
                value = label;
        }
        p->setProperty( key, value );
        auto &property = p->getPropertyObject( key );
        property.setTypeName( "enum" );
        property.setAttribute( "enum", options );
    }
    inline s32 getEnum( SmartPtr<Properties> p, const String &key, s32 selected,
                        std::initializer_list<const char *> labels )
    {
        String value;
        p->getPropertyValue( key, value );
        s32 index = 0;
        for( const auto label : labels )
        {
            if( value == label )
                return index;
            ++index;
        }
        return selected;
    }
}  // namespace workphone::scene::procedural_properties
