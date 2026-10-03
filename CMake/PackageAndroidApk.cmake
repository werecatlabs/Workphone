set(required_vars
    AAPT_EXECUTABLE
    APK_STAGE_DIR
    APK_UNSIGNED
    ANDROID_ABI
    ANDROID_JAR
    MANIFEST_FILE
    WPRUNTIME_NATIVE_LIBRARY)

foreach(var ${required_vars})
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "PackageAndroidApk.cmake missing ${var}")
    endif()
endforeach()

file(REMOVE_RECURSE "${APK_STAGE_DIR}")
file(MAKE_DIRECTORY "${APK_STAGE_DIR}/lib/${ANDROID_ABI}")
configure_file("${WPRUNTIME_NATIVE_LIBRARY}"
               "${APK_STAGE_DIR}/lib/${ANDROID_ABI}/libWPRuntime.so"
               COPYONLY)

execute_process(
    COMMAND "${AAPT_EXECUTABLE}" package
        -f
        -M "${MANIFEST_FILE}"
        -I "${ANDROID_JAR}"
        -F "${APK_UNSIGNED}"
        --debug-mode
        --min-sdk-version 24
        --target-sdk-version 35
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "aapt package failed with exit code ${result}")
endif()

execute_process(
    COMMAND "${AAPT_EXECUTABLE}" add
        "${APK_UNSIGNED}"
        "lib/${ANDROID_ABI}/libWPRuntime.so"
    WORKING_DIRECTORY "${APK_STAGE_DIR}"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "aapt add native library failed with exit code ${result}")
endif()
