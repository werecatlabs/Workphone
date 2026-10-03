#ifndef Rttr_h__
#define Rttr_h__

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <iostream>
#include <typeindex>
#include <typeinfo>
#include <any>

namespace workphone
{
    namespace reflection
    {

        struct Property
        {
            std::string name;
            std::function<std::any( void * )> getter;
            std::function<void( void *, std::any )> setter;
        };

        struct Method
        {
            std::string name;
            std::function<std::any( void *, const std::vector<std::any> & )> invoker;
        };

        struct Type
        {
            std::string name;
            std::function<void *()> constructor;
            std::unordered_map<std::string, Property> properties;
            std::unordered_map<std::string, Method> methods;

            void addProperty( const Property &prop )
            {
                properties[prop.name] = prop;
            }

            void addMethod( const Method &method )
            {
                methods[method.name] = method;
            }
        };

    }  // namespace reflection
}  // namespace workphone

#define REFLECT_TYPE( TYPE )                                  \
    if( auto t = workphone::make_shared<reflection::Type>() ) \
    {                                                         \
        t->name = #TYPE;                                      \
        t->constructor = []() -> void * { return new TYPE(); };

#define REFLECT_PROPERTY( CLASS, FIELD )                                                                \
    t->addProperty(                                                                                     \
        { #FIELD, []( void *obj ) -> std::any { return static_cast<CLASS *>( obj )->FIELD; },           \
          []( void *obj, std::any newValue ) {                                                          \
              static_cast<CLASS *>( obj )->FIELD = std::any_cast<decltype( CLASS::FIELD )>( newValue ); \
          } } );

#define REFLECT_METHOD( CLASS, METHOD )                                                       \
    t->addMethod( { #METHOD, []( void *obj, const std::vector<std::any> &args ) -> std::any { \
                       return static_cast<CLASS *>( obj )->METHOD();                          \
                   } } );

#define REGISTER_TYPE( TYPE )                                     \
    TypeManager::instance()->registerType( TYPE::typeInfo(), t ); \
    }

#endif  // Rttr_h__
