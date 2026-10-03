#ifndef workphone_config_h__
#define workphone_config_h__

#ifndef WORKPHONE_API
#    ifdef WORKPHONE_PRIVATE
#        if ( defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199409L ) )
#            define WORKPHONE_API static inline
#        elif defined( __cplusplus )
#            define WORKPHONE_API static inline
#        else
#            define WORKPHONE_API static
#        endif
#    else
#        define WORKPHONE_API extern
#    endif
#endif

#ifndef WORKPHONE_LIB
#    ifdef WORKPHONE_SINGLE_FILE
#        define WORKPHONE_LIB static
#    else
//#        define WORKPHONE_LIB extern
#        define WORKPHONE_LIB
#    endif
#endif

#define WORKPHONE_INTERN static
#define WORKPHONE_STORAGE static
#ifdef __cplusplus
#    define WORKPHONE_GLOBAL inline
#else
#    define WORKPHONE_GLOBAL static
#endif

//#ifndef WORKPHONE_ALIGNOF
//#    if defined( __cplusplus ) && ( __cplusplus >= 201103L )
//#        define WORKPHONE_ALIGNOF( t ) alignof( t )
//#    elif defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 201112L )
//#        define WORKPHONE_ALIGNOF( t ) _Alignof( t )
//#    elif defined( __GNUC__ ) || defined( __clang__ )
//#        define WORKPHONE_ALIGNOF( t ) __alignof__( t )
//#    elif defined( _MSC_VER )
//#        define WORKPHONE_ALIGNOF( t ) __alignof( t )
//#    else
//#        define WORKPHONE_ALIGNOF( t ) sizeof( t )
//#    endif
//#endif

#ifndef WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS
#    define WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS 16
#endif

#ifndef WORKPHONE_CHART_MAX_SLOT
#    define WORKPHONE_CHART_MAX_SLOT 4
#endif

#define WORKPHONE_VALUE_PAGE_CAPACITY \
    ( ( ( WORKPHONE_MAX( sizeof( struct wp_window ), sizeof( struct wp_panel ) ) / \
          sizeof( wp_u32 ) ) ) / \
      2 )

#ifdef __cplusplus
extern "C++" {
template <typename T>
struct wp_alignof;
template <typename T, int size_diff>
struct wp_helper
{
    enum
    {
        value = size_diff
    };
};
template <typename T>
struct wp_helper<T, 0>
{
    enum
    {
        value = wp_alignof<T>::value
    };
};
template <typename T>
struct wp_alignof
{
    struct Big
    {
        T x;
        char c;
    };
    enum
    {
        diff = sizeof( Big ) - sizeof( T ),
        value = wp_helper<Big, diff>::value
    };
};
}  // extern "C++"
#    define WORKPHONE_ALIGNOF( t ) ( wp_alignof<t>::value )
#else
#    define WORKPHONE_ALIGNOF( t ) \
        WORKPHONE_OFFSETOF( \
            struct { \
                char c; \
                t _h; \
            }, \
            _h )

#endif

#define WORKPHONE_CONTAINER_OF( ptr, type, member ) \
    (type *)( (void *)( (char *)( 1 ? ( ptr ) : &( (type *)0 )->member ) - \
                        WORKPHONE_OFFSETOF( type, member ) ) )

#endif  // workphone_config_h__
