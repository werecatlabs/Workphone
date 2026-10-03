if(MSVC)
	add_compile_options(/wd4251)
	add_compile_options(/wd26495)
    add_compile_options(/wd4275)
endif()

# Set up iOS overrides.
if (WP_BUILD_APPLE_IOS)
    # Set build variables
    if(NOT CMAKE_OSX_SYSROOT)
        set(CMAKE_OSX_SYSROOT iphoneos)
    endif()

    set(CMAKE_EXE_LINKER_FLAGS "-framework OpenGLES -framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit")
    set( CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES )

    set(XCODE_ATTRIBUTE_GCC_UNROLL_LOOPS "YES")
    set(XCODE_ATTRIBUTE_LLVM_VECTORIZE_LOOPS "YES")
    set(XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
    set(XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")
    #set(OGRE_BUILD_RENDERSYSTEM_GLES2 TRUE CACHE BOOL "Forcing OpenGL ES 2 RenderSystem for iOS" FORCE)
    set(OGRE_STATIC TRUE CACHE BOOL "Forcing static build for iOS" FORCE)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.\${PRODUCT_NAME:rfc1034identifier}")
    set( CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" )

    set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_INCLUDING_64_BIT)")
    string (REPLACE "-msse2" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string (REPLACE "-msse" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif(ANDROID)
   add_compile_options(
       -Wno-unused-variable
       -Wno-unused-parameter
   )

   set(CMAKE_C_FLAGS_DEBUG "-g -w")
   set(CMAKE_C_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
   set(CMAKE_C_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -finline-functions -DNDEBUG")
   set(CMAKE_CXX_FLAGS_DEBUG "-g -w")
   set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -finline-functions -DNDEBUG")
   
   if(WP_SUPPRESS_WARNINGS)
		add_compile_options(
			$<$<COMPILE_LANGUAGE:C>:-w>
			$<$<COMPILE_LANGUAGE:CXX>:-w>
		)
	endif()
elseif(WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qvec-report:2")
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
   
   add_compile_options(/wd4251)
   add_compile_options(/wd4275)
   add_compile_options(/wd4290)
   
   if (NOT WP_PLATFORM_X64)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
   endif()

   # Set up iOS overrides.
   if (WP_BUILD_APPLE_IOS)
       # Set build variables
       set(XCODE_ATTRIBUTE_SDKROOT iphoneos)
       if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
           execute_process(COMMAND xcodebuild -version -sdk "${XCODE_ATTRIBUTE_SDKROOT}" Path | head -n 1 OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
           string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
       else()
           set(CMAKE_OSX_SYSROOT iphoneos)
       endif()

       set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit")
       set( CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES )

       set(XCODE_ATTRIBUTE_GCC_UNROLL_LOOPS "YES")
       set(XCODE_ATTRIBUTE_LLVM_VECTORIZE_LOOPS "YES")
       set(XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
       set(XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")
       #set(OGRE_BUILD_RENDERSYSTEM_GLES2 TRUE CACHE BOOL "Forcing OpenGL ES 2 RenderSystem for iOS" FORCE)
       set(OGRE_STATIC TRUE CACHE BOOL "Forcing static build for iOS" FORCE)
       set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.\${PRODUCT_NAME:rfc1034identifier}")
       set( CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" )

       set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_INCLUDING_64_BIT)")
       string (REPLACE "-msse2" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
       string (REPLACE "-msse" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   elseif(WIN32)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
       #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qvec-report:2")

       if (NOT WP_PLATFORM_X64)
           set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
       endif()

       if (WP_STATIC_LINK_CRT)
           if(WP_REPLACE_MACROS)
               set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /GF /Ob0 /Od /EP /P /C /W3")
           else()
               set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MP /MTd /Zi /GF /Ob0 /Od /W3")
           endif()

           set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /MP /Zi /O2 /Ob2 /D NDEBUG /W3")
           set(CMAKE_CXX_FLAGS_RELEASE "/MT /MP /O2 /Ob2 /Zi /GF /D NDEBUG /W3")
           set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /MP /Zi /GF /Ob0 /Od /D NDEBUG /W3")
       else()
           if(WP_REPLACE_MACROS)
               set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /EP /P /C /W3")
           else()
               set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MP /MDd /Zi /Ob0 /Od /W3")
           endif()

           set(CMAKE_CXX_FLAGS_MINSIZEREL "/MD /MP /Zi /O2 /Ob2 /D NDEBUG /W3")
           set(CMAKE_CXX_FLAGS_RELEASE "/MD /MP /O2 /Ob2 /Zi /D NDEBUG /W3")
           set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /MP /Zi /Ob0 /Ob0 /D NDEBUG /W3")
       endif()
   elseif (APPLE AND NOT APPLE_IOS)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3")
       set(CMAKE_CXX_FLAGS_DEBUG "-g -w -allow-multiple-definition")
       set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
       set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
       set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -w -finline-functions -DNDEBUG")
   else()
       #set(CMAKE_CXX_FLAGS_INIT "-w")
       #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -w")
       #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -Wno-everything")
       set(CMAKE_CXX_FLAGS_DEBUG "-g -w")
       set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
       set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
       set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -finline-functions -DNDEBUG")
   endif()
endif()

if (WIN32)
   if (WP_STATIC_LINK_CRT)
        if(WP_REPLACE_MACROS)
            set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /GF /Ob0 /Od /EP /P /C /W3 /Zc:__cplusplus /bigobj")
        else()
            set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MP /MTd /Zi /GF /Ob0 /Od /W3 /Zc:__cplusplus /bigobj")
        endif()
		
		set(CMAKE_C_FLAGS_DEBUG "/D_DEBUG /MP /MTd /Zi /GF /Ob0 /Od /W3 /bigobj")
		set(CMAKE_C_FLAGS_MINSIZEREL "/MT /MP /Zi /O2 /Ob2 /D NDEBUG /W3 /bigobj")
        set(CMAKE_C_FLAGS_RELEASE "/MT /MP /Zi /GF /O2 /Ob1 /D NDEBUG /W3 /bigobj")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "/MT /MP /Zi /GF /O2 /Ob1 /D NDEBUG /W3 /bigobj")

        set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /MP /Zi /O2 /Ob2 /D NDEBUG /W3 /Zc:__cplusplus /bigobj")
        set(CMAKE_CXX_FLAGS_RELEASE "/MT /MP /Zi /GF /O2 /Ob1 /D NDEBUG /W3 /Zc:__cplusplus /bigobj")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /MP /Zi /GF /O2 /Ob1 /D NDEBUG /W3 /Zc:__cplusplus /bigobj")
   else()
        if(WP_REPLACE_MACROS)
            set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /EP /P /C /W3 /Zc:__cplusplus /bigobj")
        else()
            set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MP /MDd /Zi /Ob0 /Od /W3 /Zc:__cplusplus /bigobj")
        endif()

        set(CMAKE_CXX_FLAGS_MINSIZEREL "/MD /MP /Zi /O2 /Ob2 /D NDEBUG /W3 /Zc:__cplusplus /bigobj")
        set(CMAKE_CXX_FLAGS_RELEASE "/MD /MP /Zi /GF /O2 /Ob1 /D NDEBUG /W3 /Zc:__cplusplus /bigobj")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /MP /Zi /GF /O2 /Ob1 /D NDEBUG /W3 /Zc:__cplusplus /bigobj")
   endif()
elseif (APPLE_IOS)
	add_compile_options(
		-Wno-unused-variable
		-Wno-unused-parameter
	)

    set(CMAKE_C_FLAGS_DEBUG "-g -w")
    set(CMAKE_C_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -w -finline-functions -DNDEBUG")

    set(CMAKE_CXX_FLAGS_DEBUG "-g -w")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -w -finline-functions -DNDEBUG")
elseif (APPLE)
	add_compile_options(
		-Wno-unused-variable
		-Wno-unused-parameter
	)
	
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3")
    set(CMAKE_C_FLAGS_DEBUG "-g -w")
    set(CMAKE_C_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -w -finline-functions -DNDEBUG")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -w")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -w -finline-functions -DNDEBUG")
else()
	add_compile_options(
		-Wno-unused-variable
		-Wno-unused-parameter
	)

    #set(CMAKE_CXX_FLAGS_INIT "-w")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -w")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -Wno-everything")    
    set(CMAKE_CXX_FLAGS_DEBUG "-g -w")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -finline-functions -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -finline-functions -DNDEBUG")
endif()

if(MSVC)
  # Force to always compile with W4
  #if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
  #  string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  #else()
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  #endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsyntax-only")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()
