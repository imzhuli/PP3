cmake_minimum_required(VERSION 3.15)
project (xel_test)
set(project_version 1.0)
include(../cmake/shared_cpp_settings.cmake)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${XEL_PREFIX})
find_package(xel REQUIRED CONFIG PATHS ${XEL_PREFIX} NO_DEFAULT_PATH)

set(shared_lib_dependency_private xel::xel)

################### libcommon

aux_source_directory(./pp_common      LIB_PP_COMMON_SRC)

add_library(pp_common ${LIB_PP_COMMON_SRC})
target_link_libraries(pp_common PUBLIC ${shared_lib_dependency_private})

install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/
	DESTINATION include
	FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hpp"
	)

install(
	TARGETS
	pp_common
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

