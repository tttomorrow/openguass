#This is the main CMAKE for building MOT core/storage component.

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} TGT_mot_core_storage_SRC)

set(TGT_mot_core_storage_INC
    ${PROJECT_SRC_DIR}/include
    ${MOT_CORE_INCLUDE_PATH}
)

add_static_objtarget(gausskernel_storage_mot_core_storage TGT_mot_core_storage_SRC TGT_mot_core_storage_INC
    "${mot_core_DEF_OPTIONS}" "${mot_core_COMPILE_OPTIONS}" "${mot_core_LINK_OPTIONS}")

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/index
    ${CMAKE_CURRENT_SOURCE_DIR}/sentinel
)

add_subdirectory(index)
add_subdirectory(sentinel)

