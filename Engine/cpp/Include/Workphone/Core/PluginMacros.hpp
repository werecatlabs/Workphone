#ifndef PluginMacros_h__
#define PluginMacros_h__

#include <Workphone/WorkphoneTypes.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
using PluginHandle = HMODULE;
#else
#    include <dlfcn.h>
typedef void *PluginHandle;
#endif

#if defined WP_PLATFORM_WIN32

#    define DECLARE_FUNCTION( name, res )                    \
        typedef res ( *name )();                             \
        name h##name##;                                      \
        void init_##name##( PluginHandle hMod )              \
        {                                                    \
            h##name## = (name)GetProcAddress( hMod, #name ); \
        }

#    define DECLARE_FUNCTION_ARG1( name, res, arg1 )         \
        typedef res ( *name )( arg1 );                       \
        name h##name##;                                      \
        void init_##name##( PluginHandle hMod )              \
        {                                                    \
            h##name## = (name)GetProcAddress( hMod, #name ); \
        }

#    define DECLARE_FUNCTION_ARG2( name, res, arg1, arg2 )   \
        typedef res ( *name )( arg1, arg2 );                 \
        name h##name##;                                      \
        void init_##name##( PluginHandle hMod )              \
        {                                                    \
            h##name## = (name)GetProcAddress( hMod, #name ); \
        }

#    define DECLARE_FUNCTION_ARG3( name, res, arg1, arg2, arg3 ) \
        typedef res ( *name )( arg1, arg2, arg3 );               \
        name h##name##;                                          \
        void init_##name##( PluginHandle hMod )                  \
        {                                                        \
            h##name## = (name)GetProcAddress( hMod, #name );     \
        }

#    define DECLARE_FUNCTION_ARG4( name, res, arg1, arg2, arg3, arg4 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4 );               \
        name h##name##;                                                \
        void init_##name##( PluginHandle hMod )                        \
        {                                                              \
            h##name## = (name)GetProcAddress( hMod, #name );           \
        }

#    define DECLARE_FUNCTION_ARG5( name, res, arg1, arg2, arg3, arg4, arg5 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5 );               \
        name h##name##;                                                      \
        void init_##name##( PluginHandle hMod )                              \
        {                                                                    \
            h##name## = (name)GetProcAddress( hMod, #name );                 \
        }

#    define DECLARE_FUNCTION_ARG6( name, res, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6 );               \
        name h##name##;                                                            \
        void init_##name##( PluginHandle hMod )                                    \
        {                                                                          \
            h##name## = (name)GetProcAddress( hMod, #name );                       \
        }

#    define DECLARE_FUNCTION_ARG7( name, res, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6 );               \
        name h##name##;                                                            \
        void init_##name##( PluginHandle hMod )                                    \
        {                                                                          \
            h##name## = (name)GetProcAddress( hMod, #name );                       \
        }

#    define DECLARE_FUNCTION_ARG8( name, res, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6 );               \
        name h##name##;                                                            \
        void init_##name##( PluginHandle hMod )                                    \
        {                                                                          \
            h##name## = (name)GetProcAddress( hMod, #name );                       \
        }

#    define DECLARE_FUNCTION_ARG9( name, res, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 );               \
        name h##name##;                                                                              \
        void init_##name##( PluginHandle hMod )                                                      \
        {                                                                                            \
            h##name## = (name)GetProcAddress( hMod, #name );                                         \
        }

#else

#    define DECLARE_FUNCTION( name, res )         \
        typedef res ( *name )();                  \
        name h##name;                             \
        void init_##name( PluginHandle hMod )     \
        {                                         \
            h##name = (name)dlsym( hMod, #name ); \
        }

#    define DECLARE_FUNCTION_ARG1( name, res, arg1 ) \
        typedef res ( *name )( arg1 );               \
        name h##name;                                \
        void init_##name( PluginHandle hMod )        \
        {                                            \
            h##name = (name)dlsym( hMod, #name );    \
        }

#    define DECLARE_FUNCTION_ARG2( name, res, arg1, arg2 ) \
        typedef res ( *name )( arg1, arg2 );               \
        name h##name;                                      \
        void init_##name( PluginHandle hMod )              \
        {                                                  \
            h##name = (name)dlsym( hMod, #name );          \
        }

#    define DECLARE_FUNCTION_ARG3( name, res, arg1, arg2, arg3 ) \
        typedef res ( *name )( arg1, arg2, arg3 );               \
        name h##name;                                            \
        void init_##name( PluginHandle hMod )                    \
        {                                                        \
            h##name = (name)dlsym( hMod, #name );                \
        }

#    define DECLARE_FUNCTION_ARG4( name, res, arg1, arg2, arg3, arg4 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4 );               \
        name h##name;                                                  \
        void init_##name( PluginHandle hMod )                          \
        {                                                              \
            h##name = (name)dlsym( hMod, #name );                      \
        }

#    define DECLARE_FUNCTION_ARG5( name, res, arg1, arg2, arg3, arg4, arg5 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5 );               \
        name h##name;                                                        \
        void init_##name( PluginHandle hMod )                                \
        {                                                                    \
            h##name = (name)dlsym( hMod, #name );                            \
        }

#    define DECLARE_FUNCTION_ARG6( name, res, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6 );               \
        name h##name;                                                              \
        void init_##name( PluginHandle hMod )                                      \
        {                                                                          \
            h##name = (name)dlsym( hMod, #name );                                  \
        }

#    define DECLARE_FUNCTION_ARG7( name, res, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6 );               \
        name h##name;                                                              \
        void init_##name( PluginHandle hMod )                                      \
        {                                                                          \
            h##name = (name)dlsym( hMod, #name );                                  \
        }

#    define DECLARE_FUNCTION_ARG8( name, res, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6 );               \
        name h##name;                                                              \
        void init_##name( PluginHandle hMod )                                      \
        {                                                                          \
            h##name = (name)dlsym( hMod, #name );                                  \
        }

#    define DECLARE_FUNCTION_ARG9( name, res, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
        typedef res ( *name )( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 );               \
        name h##name;                                                                                \
        void init_##name( PluginHandle hMod )                                                        \
        {                                                                                            \
            h##name = (name)dlsym( hMod, #name );                                                    \
        }

#endif

#endif  // PluginMacros_h__
