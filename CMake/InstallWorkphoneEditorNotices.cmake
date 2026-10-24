# Retain the original bundled texts, including complete headers/readmes when
# they are the local source of a dependency notice.
if(NOT DEFINED WP_ROOT)
    message(FATAL_ERROR "Editor dependency notices require WP_ROOT")
endif()

set(_wp_editor_notice_entries
    "Dependencies/FreeImage/license-fi.txt|FreeImage-license-fi.txt"
    "Dependencies/FreeImage/license-gplv2.txt|FreeImage-license-gplv2.txt"
    "Dependencies/FreeImage/license-gplv3.txt|FreeImage-license-gplv3.txt"
    "Dependencies/FreeImage/Source/LibPNG/LICENSE|FreeImage-LibPNG-LICENSE"
    "Dependencies/FreeImage/Source/LibWebP/COPYING|FreeImage-LibWebP-COPYING"
    "Dependencies/FreeImage/Source/LibOpenJPEG/LICENSE|FreeImage-LibOpenJPEG-LICENSE"
    "Dependencies/FreeImage/Source/OpenEXR/Copyrights/openexr/COPYING|FreeImage-OpenEXR-COPYING"
    "Dependencies/FreeImage/Source/OpenEXR/Copyrights/ilmbase/COPYING|FreeImage-IlmBase-COPYING"
    "Dependencies/tinyxml/readme.txt|TinyXML-readme-and-license.txt"
    "Dependencies/zlib/include/zlib.h|zlib-notice-source-zlib.h"
    "Dependencies/minizip/include/unzip.h|MiniZip-notice-source-unzip.h"
    "Dependencies/luabind/inc/luabind/luabind.hpp|Luabind-notice-source-luabind.hpp")

foreach(_wp_editor_notice_entry IN LISTS _wp_editor_notice_entries)
    string(REPLACE "|" ";" _wp_editor_notice_fields "${_wp_editor_notice_entry}")
    list(GET _wp_editor_notice_fields 0 _wp_editor_notice_relative)
    list(GET _wp_editor_notice_fields 1 _wp_editor_notice_name)
    set(_wp_editor_notice_source "${WP_ROOT}/${_wp_editor_notice_relative}")
    if(NOT EXISTS "${_wp_editor_notice_source}" OR IS_DIRECTORY "${_wp_editor_notice_source}")
        message(FATAL_ERROR "Editor dependency notice is missing: ${_wp_editor_notice_source}")
    endif()
    install(FILES "${_wp_editor_notice_source}" DESTINATION licenses
        COMPONENT WorkphoneEditor RENAME "${_wp_editor_notice_name}")
endforeach()

unset(_wp_editor_notice_entries)
unset(_wp_editor_notice_entry)
unset(_wp_editor_notice_fields)
unset(_wp_editor_notice_relative)
unset(_wp_editor_notice_name)
unset(_wp_editor_notice_source)
