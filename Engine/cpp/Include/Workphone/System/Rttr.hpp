#ifndef Rttr_h__
#define Rttr_h__

#include <Workphone/System/ReflectionStorage.hpp>
#include <string>
#include <string_view>
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
        using Name = std::basic_string<char, std::char_traits<char>, SourceBudgetAllocator<char>>;
        struct NameHash
        {
            size_t operator()(const Name &name) const noexcept
            { return std::hash<std::string_view>{}({name.data(), name.size()}); }
        };
        template<class T> using Table = std::unordered_map<Name, T, NameHash, std::equal_to<Name>,
            SourceBudgetAllocator<std::pair<const Name, T>>>;

        struct Property
        {
            Name name;
            Callback<Any( void * )> getter;
            Callback<void( void *, Any )> setter;
        };

        struct Method
        {
            Name name;
            Callback<Any( void *, const Arguments & )> invoker;
        };

        struct Type : SourceBudgetObject
        {
            Name name;
            Callback<void *()> constructor;
            Table<Property> properties;
            Table<Method> methods;

            void addProperty( const Property &prop )
            {
                Property candidate(prop);
                properties.insert_or_assign(candidate.name, std::move(candidate));
            }

            void addMethod( const Method &method )
            {
                Method candidate(method);
                methods.insert_or_assign(candidate.name, std::move(candidate));
            }
        };

    }  // namespace reflection
}  // namespace workphone

#define REFLECT_TYPE( TYPE )                                  \
    if( auto t = workphone::make_shared<workphone::reflection::Type>() ) \
    {                                                         \
        t->name = #TYPE;                                      \
        t->constructor = []() -> void * { return new TYPE(); };

#define REFLECT_PROPERTY( CLASS, FIELD )                                                                \
    t->addProperty(                                                                                     \
        { #FIELD, []( void *obj ) -> workphone::reflection::Any { return static_cast<CLASS *>( obj )->FIELD; },           \
          []( void *obj, workphone::reflection::Any newValue ) {                                                          \
              static_cast<CLASS *>( obj )->FIELD = workphone::reflection::copyToExternal<decltype( CLASS::FIELD )>( newValue ); \
          } } );

#define REFLECT_METHOD( CLASS, METHOD )                                                       \
    t->addMethod( { #METHOD, []( void *obj, const workphone::reflection::Arguments &args ) -> workphone::reflection::Any { \
                       return static_cast<CLASS *>( obj )->METHOD();                          \
                   } } );

#define REGISTER_TYPE( TYPE )                                     \
    TypeManager::instance()->registerType( TYPE::typeInfo(), t ); \
    }

#endif  // Rttr_h__
