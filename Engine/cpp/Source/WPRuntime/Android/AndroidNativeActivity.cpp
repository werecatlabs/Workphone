#if defined( __ANDROID__ )

#    include <android/log.h>
#    include <android_native_app_glue.h>

int WPRuntimeMain( int argc, char *argv[] );

void android_main( android_app *app )
{
    app_dummy();

    char appName[] = "WPRuntime";
    char appPath[] = "./";
    char *argv[] = { appName, appPath };

    __android_log_print( ANDROID_LOG_INFO, "WPRuntime", "Starting WPRuntime NativeActivity" );
    WPRuntimeMain( 2, argv );
}

#endif
