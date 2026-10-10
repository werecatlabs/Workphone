project(WorkphoneNetwork C)
add_library(WorkphoneNetwork STATIC ../../Source/WorkphoneNetwork/workphone_network.c)
target_include_directories(WorkphoneNetwork PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Include/WorkphoneNetwork")
set_target_properties(WorkphoneNetwork PROPERTIES
    C_STANDARD 99 C_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON FOLDER Engine/C)
if(WIN32)
    target_link_libraries(WorkphoneNetwork PUBLIC ws2_32 bcrypt)
endif()
