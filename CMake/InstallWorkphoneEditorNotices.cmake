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
    "Dependencies/luabind/inc/luabind/luabind.hpp|Luabind-notice-source-luabind.hpp"
    "Dependencies/FreeImage/Source/LibJPEG/README|FreeImage-LibJPEG-README"
    "Dependencies/FreeImage/Source/LibTIFF4/tiff.h|FreeImage-LibTIFF-notice-source-tiff.h"
    "Dependencies/FreeImage/Source/LibRawLite/LICENSE.CDDL|FreeImage-LibRaw-LICENSE.CDDL"
    "Dependencies/FreeImage/Source/LibRawLite/LICENSE.LGPL|FreeImage-LibRaw-LICENSE.LGPL"
    "Dependencies/FreeImage/Source/ZLib/zlib.h|FreeImage-ZLib-notice-source-zlib.h"
    "Dependencies/lua_cjson/src/lua_cjson.c|LuaCJSON-notice-source-lua_cjson.c"
    "Dependencies/utf8/source/utf8.h|UTF8CPP-notice-source-utf8.h"
    "Dependencies/zzip/include/zzip/lib.h|ZZipLib-notice-source-lib.h"
    "Engine/cpp/Source/WPAssimp/Extern/assimp/Common/Importer.h|Assimp-notice-source-Importer.h"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/Open3DGC/o3dgcCommon.h|Assimp-Open3DGC-notice-source-o3dgcCommon.h"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/openddlparser/LICENSE|Assimp-OpenDDLParser-LICENSE"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/poly2tri/LICENSE|Assimp-Poly2Tri-LICENSE"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/clipper/License.txt|Assimp-Clipper-License.txt"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/pugixml/readme.txt|Assimp-PugiXML-readme-and-license.txt"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/zip/UNLICENSE|Assimp-Zip-UNLICENSE"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/utf8cpp/doc/LICENSE|Assimp-UTF8CPP-LICENSE"
    "Engine/cpp/Source/WPAssimp/Extern/contrib/rapidjson/license.txt|Assimp-RapidJSON-license.txt")

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
