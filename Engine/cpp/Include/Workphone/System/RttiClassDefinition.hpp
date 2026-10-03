#ifndef RttiClassDefinition_h__
#define RttiClassDefinition_h__

#include <Workphone/Memory/TypeManager.hpp>

#define WP_STRINGIZE_HELPER( x ) #x
#define WP_STRINGIZE( x ) WP_STRINGIZE_HELPER( x )

#define WP_CLASS_REGISTER( NAMESPACE, TYPE )                                     \
    void TYPE::setTypeInfo( u32 id )                                             \
    {                                                                            \
        iTypeInfo = id;                                                          \
    }                                                                            \
                                                                                 \
    u32 TYPE::typeInfo()                                                         \
    {                                                                            \
        if( sTypeInfo == 0 )                                                     \
        {                                                                        \
            setupTypeInfo();                                                     \
        }                                                                        \
                                                                                 \
        return TYPE::sTypeInfo;                                                  \
    }                                                                            \
                                                                                 \
    void TYPE::setupTypeInfo()                                                   \
    {                                                                            \
        if( TYPE::sTypeInfo == 0 )                                               \
        {                                                                        \
            auto typeManager = TypeManager::instance();                          \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TYPE ); \
            auto newTypeInfo = typeManager->getNewTypeId( name, 0 );             \
            TYPE::sTypeInfo = newTypeInfo;                                       \
            typeManager->setLabel( newTypeInfo, #TYPE );                         \
        }                                                                        \
    }                                                                            \
                                                                                 \
    u32 TYPE::getTypeInfo() const                                                \
    {                                                                            \
        if( iTypeInfo == 0 )                                                     \
        {                                                                        \
            iTypeInfo = TYPE::typeInfo();                                        \
        }                                                                        \
                                                                                 \
        return iTypeInfo;                                                        \
    }                                                                            \
                                                                                 \
    u32 TYPE::sTypeInfo = 0;

#define WP_CLASS_REGISTER_DERIVED( NAMESPACE, TYPE, BASE_TYPE )                  \
    void TYPE::setTypeInfo( u32 id )                                             \
    {                                                                            \
        iTypeInfo = id;                                                          \
    }                                                                            \
                                                                                 \
    u32 TYPE::typeInfo()                                                         \
    {                                                                            \
        if( sTypeInfo == 0 )                                                     \
        {                                                                        \
            setupTypeInfo();                                                     \
        }                                                                        \
                                                                                 \
        return TYPE::sTypeInfo;                                                  \
    }                                                                            \
                                                                                 \
    void TYPE::setupTypeInfo()                                                   \
    {                                                                            \
        if( TYPE::sTypeInfo == 0 )                                               \
        {                                                                        \
            BASE_TYPE::setupTypeInfo();                                          \
            auto typeManager = TypeManager::instance();                          \
            auto baseType = BASE_TYPE::typeInfo();                               \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TYPE ); \
            auto newTypeInfo = typeManager->getNewTypeId( name, baseType );      \
            TYPE::sTypeInfo = newTypeInfo;                                       \
            typeManager->setLabel( newTypeInfo, #TYPE );                         \
        }                                                                        \
    }                                                                            \
                                                                                 \
    u32 TYPE::getTypeInfo() const                                                \
    {                                                                            \
        if( iTypeInfo == 0 )                                                     \
        {                                                                        \
            iTypeInfo = TYPE::typeInfo();                                        \
        }                                                                        \
                                                                                 \
        return iTypeInfo;                                                        \
    }                                                                            \
                                                                                 \
    u32 TYPE::sTypeInfo = 0;

#define WP_CLASS_REGISTER_DERIVED_TEMPLATE( NAMESPACE, TEMPLATE_CLASS, TYPE, BASE_TYPE )          \
    template <class T>                                                                            \
    inline void TEMPLATE_CLASS<T>::setTypeInfo( u32 id )                                          \
    {                                                                                             \
        TEMPLATE_CLASS<T>::iTypeInfo = id;                                                        \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline u32 TEMPLATE_CLASS<T>::typeInfo()                                                      \
    {                                                                                             \
        if( sTypeInfo == 0 )                                                                      \
        {                                                                                         \
            setupTypeInfo();                                                                      \
        }                                                                                         \
                                                                                                  \
        return sTypeInfo;                                                                         \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline void TEMPLATE_CLASS<T>::setupTypeInfo()                                                \
    {                                                                                             \
        if( TEMPLATE_CLASS<T>::sTypeInfo == 0 )                                                   \
        {                                                                                         \
            BASE_TYPE::setupTypeInfo();                                                           \
            auto typeManager = TypeManager::instance();                                           \
            auto baseType = BASE_TYPE::typeInfo();                                                \
            auto classType = typeManager->getName( T::typeInfo() );                               \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TEMPLATE_CLASS ) + "<" + \
                        String( classType ) + ">";                                                \
            auto newTypeInfo = typeManager->getNewTypeId( name, baseType );                       \
            TEMPLATE_CLASS<T>::sTypeInfo = newTypeInfo;                                           \
            typeManager->setLabel( newTypeInfo, #TEMPLATE_CLASS );                                \
        }                                                                                         \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline u32 TEMPLATE_CLASS<T>::getTypeInfo() const                                             \
    {                                                                                             \
        if( TEMPLATE_CLASS<T>::iTypeInfo == 0 )                                                   \
        {                                                                                         \
            TEMPLATE_CLASS<T>::iTypeInfo = TEMPLATE_CLASS<T>::typeInfo();                         \
        }                                                                                         \
                                                                                                  \
        return TEMPLATE_CLASS<T>::iTypeInfo;                                                      \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline u32 TEMPLATE_CLASS<T>::sTypeInfo = 0;

#define WP_CLASS_REGISTER_DERIVED_TEMPLATE_TYPEID( NAMESPACE, TEMPLATE_CLASS, TYPE, BASE_TYPE )   \
    template <class T>                                                                            \
    inline void TEMPLATE_CLASS<T>::setTypeInfo( u32 id )                                          \
    {                                                                                             \
        TEMPLATE_CLASS<T>::iTypeInfo = id;                                                        \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline u32 TEMPLATE_CLASS<T>::typeInfo()                                                      \
    {                                                                                             \
        if( sTypeInfo == 0 )                                                                      \
        {                                                                                         \
            setupTypeInfo();                                                                      \
        }                                                                                         \
                                                                                                  \
        return sTypeInfo;                                                                         \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline void TEMPLATE_CLASS<T>::setupTypeInfo()                                                \
    {                                                                                             \
        if( TEMPLATE_CLASS<T>::sTypeInfo == 0 )                                                   \
        {                                                                                         \
            BASE_TYPE::setupTypeInfo();                                                           \
            auto typeManager = TypeManager::instance();                                           \
            auto baseType = BASE_TYPE::typeInfo();                                                \
            auto classType = typeid( T ).name();                                                  \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TEMPLATE_CLASS ) + "<" + \
                        String( classType ) + ">";                                                \
            auto newTypeInfo = typeManager->getNewTypeId( name, baseType );                       \
            TEMPLATE_CLASS<T>::sTypeInfo = newTypeInfo;                                           \
            typeManager->setLabel( newTypeInfo, #TEMPLATE_CLASS );                                \
        }                                                                                         \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline u32 TEMPLATE_CLASS<T>::getTypeInfo() const                                             \
    {                                                                                             \
        if( TEMPLATE_CLASS<T>::iTypeInfo == 0 )                                                   \
        {                                                                                         \
            TEMPLATE_CLASS<T>::iTypeInfo = TEMPLATE_CLASS<T>::typeInfo();                         \
        }                                                                                         \
                                                                                                  \
        return TEMPLATE_CLASS<T>::iTypeInfo;                                                      \
    }                                                                                             \
                                                                                                  \
    template <class T>                                                                            \
    inline u32 TEMPLATE_CLASS<T>::sTypeInfo = 0;

#define WP_CLASS_REGISTER_DERIVED_TEMPLATE_PAIR( NAMESPACE, TEMPLATE_CLASS, TYPE, SECOND_TYPE,    \
                                                 BASE_TYPE )                                      \
    template <class T, class U>                                                                   \
    inline void TEMPLATE_CLASS<T, U>::setTypeInfo( u32 id )                                       \
    {                                                                                             \
        TEMPLATE_CLASS<T, U>::iTypeInfo = id;                                                     \
    }                                                                                             \
                                                                                                  \
    template <class T, class U>                                                                   \
    inline u32 TEMPLATE_CLASS<T, U>::typeInfo()                                                   \
    {                                                                                             \
        if( sTypeInfo == 0 )                                                                      \
        {                                                                                         \
            setupTypeInfo();                                                                      \
        }                                                                                         \
                                                                                                  \
        return sTypeInfo;                                                                         \
    }                                                                                             \
                                                                                                  \
    template <class T, class U>                                                                   \
    inline void TEMPLATE_CLASS<T, U>::setupTypeInfo()                                             \
    {                                                                                             \
        if( TEMPLATE_CLASS<T, U>::sTypeInfo == 0 )                                                \
        {                                                                                         \
            BASE_TYPE::setupTypeInfo();                                                           \
            auto typeManager = TypeManager::instance();                                           \
            auto baseType = BASE_TYPE::typeInfo();                                                \
            auto classTypeT = typeManager->getName( T::typeInfo() );                              \
            auto classTypeU = typeManager->getName( U::typeInfo() );                              \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TEMPLATE_CLASS ) + "<" + \
                        String( classTypeT ) + ", " + String( classTypeU ) + ">";                 \
            auto newTypeInfo = typeManager->getNewTypeId( name, baseType );                       \
            TEMPLATE_CLASS<T, U>::sTypeInfo = newTypeInfo;                                        \
            typeManager->setLabel( newTypeInfo, #TEMPLATE_CLASS );                                \
        }                                                                                         \
    }                                                                                             \
                                                                                                  \
    template <class T, class U>                                                                   \
    inline u32 TEMPLATE_CLASS<T, U>::getTypeInfo() const                                          \
    {                                                                                             \
        if( TEMPLATE_CLASS<T, U>::iTypeInfo == 0 )                                                \
        {                                                                                         \
            TEMPLATE_CLASS<T, U>::iTypeInfo = TEMPLATE_CLASS<T, U>::typeInfo();                   \
        }                                                                                         \
                                                                                                  \
        return TEMPLATE_CLASS<T, U>::iTypeInfo;                                                   \
    }                                                                                             \
                                                                                                  \
    template <class T, class U>                                                                   \
    inline u32 TEMPLATE_CLASS<T, U>::sTypeInfo = 0;

#define WP_CLASS_REGISTER_DERIVED_TEMPLATE_PAIR_TYPEID( NAMESPACE, TEMPLATE_CLASS, TYPE, SECOND_TYPE, \
                                                        BASE_TYPE )                                   \
    template <class T, class U>                                                                       \
    inline void TEMPLATE_CLASS<T, U>::setTypeInfo( u32 id )                                           \
    {                                                                                                 \
        TEMPLATE_CLASS<T, U>::iTypeInfo = id;                                                         \
    }                                                                                                 \
                                                                                                      \
    template <class T, class U>                                                                       \
    inline u32 TEMPLATE_CLASS<T, U>::typeInfo()                                                       \
    {                                                                                                 \
        if( sTypeInfo == 0 )                                                                          \
        {                                                                                             \
            setupTypeInfo();                                                                          \
        }                                                                                             \
                                                                                                      \
        return sTypeInfo;                                                                             \
    }                                                                                                 \
                                                                                                      \
    template <class T, class U>                                                                       \
    inline void TEMPLATE_CLASS<T, U>::setupTypeInfo()                                                 \
    {                                                                                                 \
        if( TEMPLATE_CLASS<T, U>::sTypeInfo == 0 )                                                    \
        {                                                                                             \
            BASE_TYPE::setupTypeInfo();                                                               \
            auto typeManager = TypeManager::instance();                                               \
            auto baseType = BASE_TYPE::typeInfo();                                                    \
            auto classTypeT = typeid( T ).name();                                                     \
            auto classTypeU = typeid( U ).name();                                                     \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TEMPLATE_CLASS ) + "<" +     \
                        String( classTypeT ) + ", " + String( classTypeU ) + ">";                     \
            auto newTypeInfo = typeManager->getNewTypeId( name, baseType );                           \
            TEMPLATE_CLASS<T, U>::sTypeInfo = newTypeInfo;                                            \
            typeManager->setLabel( newTypeInfo, #TEMPLATE_CLASS );                                    \
        }                                                                                             \
    }                                                                                                 \
                                                                                                      \
    template <class T, class U>                                                                       \
    inline u32 TEMPLATE_CLASS<T, U>::getTypeInfo() const                                              \
    {                                                                                                 \
        if( TEMPLATE_CLASS<T, U>::iTypeInfo == 0 )                                                    \
        {                                                                                             \
            TEMPLATE_CLASS<T, U>::iTypeInfo = TEMPLATE_CLASS<T, U>::typeInfo();                       \
        }                                                                                             \
                                                                                                      \
        return TEMPLATE_CLASS<T, U>::iTypeInfo;                                                       \
    }                                                                                                 \
                                                                                                      \
    template <class T, class U>                                                                       \
    inline u32 TEMPLATE_CLASS<T, U>::sTypeInfo = 0;

#define WP_CLASS_REGISTER_DERIVED_TEMPLATE_SPECIALISE( NAMESPACE, TEMPLATE_CLASS, TYPE, BASE_TYPE ) \
    template <>                                                                                     \
    inline void TEMPLATE_CLASS<TYPE>::setTypeInfo( u32 id )                                         \
    {                                                                                               \
        TEMPLATE_CLASS<TYPE>::iTypeInfo = id;                                                       \
    }                                                                                               \
                                                                                                    \
    template <>                                                                                     \
    inline u32 TEMPLATE_CLASS<TYPE>::typeInfo()                                                     \
    {                                                                                               \
        if( sTypeInfo == 0 )                                                                        \
        {                                                                                           \
            setupTypeInfo();                                                                        \
        }                                                                                           \
                                                                                                    \
        return sTypeInfo;                                                                           \
    }                                                                                               \
                                                                                                    \
    template <>                                                                                     \
    inline void TEMPLATE_CLASS<TYPE>::setupTypeInfo()                                               \
    {                                                                                               \
        if( TEMPLATE_CLASS<TYPE>::sTypeInfo == 0 )                                                  \
        {                                                                                           \
            BASE_TYPE::setupTypeInfo();                                                             \
            auto typeManager = TypeManager::instance();                                             \
            auto baseType = BASE_TYPE::typeInfo();                                                  \
            auto classType = typeManager->getName( TYPE::typeInfo() );                              \
            auto name = String( #NAMESPACE ) + String( "::" ) + String( #TEMPLATE_CLASS ) + "<" +   \
                        String( classType ) + ">";                                                  \
            auto newTypeInfo = typeManager->getNewTypeId( name, baseType );                         \
            TEMPLATE_CLASS<TYPE>::sTypeInfo = newTypeInfo;                                          \
            typeManager->setLabel( newTypeInfo, #TEMPLATE_CLASS );                                  \
        }                                                                                           \
    }                                                                                               \
                                                                                                    \
    template <>                                                                                     \
    inline u32 TEMPLATE_CLASS<TYPE>::getTypeInfo() const                                            \
    {                                                                                               \
        if( TEMPLATE_CLASS<TYPE>::iTypeInfo == 0 )                                                  \
        {                                                                                           \
            TEMPLATE_CLASS<TYPE>::iTypeInfo = TEMPLATE_CLASS<TYPE>::typeInfo();                     \
        }                                                                                           \
                                                                                                    \
        return TEMPLATE_CLASS<TYPE>::iTypeInfo;                                                     \
    }                                                                                               \
                                                                                                    \
    template <>                                                                                     \
    inline u32 TEMPLATE_CLASS<TYPE>::sTypeInfo = 0;

#endif  // RttiClassDefinition_h__
