cmake_minimum_required(VERSION 3.15)
project (xel_test)
set(project_version 1.0)
include(../cmake/shared_cpp_settings.cmake)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${XEL_PREFIX})

find_package(xel     REQUIRED CONFIG)
find_package(RdKafka REQUIRED CONFIG)

include_directories(
	"${CMAKE_INSTALL_PREFIX}/include"
)
link_directories(
	"${CMAKE_INSTALL_PREFIX}/lib"
)
set(shared_lib_dependency_private pp_common xel::xel)

################### protocol

aux_source_directory(ppp/pp_protocol                 PPP_SRC)
aux_source_directory(ppp/pp_protocol/_backend        PPP_SRC)
aux_source_directory(ppp/pp_protocol/_backend/kfk    PPP_SRC)
aux_source_directory(ppp/abstract                    PPP_SRC)
aux_source_directory(ppp/lib_component               PPP_SRC)
aux_source_directory(ppp/lib_util                    PPP_SRC)

add_library(ppp  ${PPP_SRC})
target_link_libraries(ppp PUBLIC RdKafka::rdkafka++ ${shared_lib_dependency_private})


################### apps
set(PPP_HEADER_DIR ${PROJECT_SOURCE_DIR}/ppp)
function(build_app sub_dir_name)
	aux_source_directory       (${sub_dir_name} LOCAL_SOURCE_CODE)
	add_executable             (${sub_dir_name} ${LOCAL_SOURCE_CODE})
	target_include_directories (${sub_dir_name} PRIVATE ${PPP_HEADER_DIR})
	target_link_libraries      (${sub_dir_name} PRIVATE ppp)
endfunction()

set(SUB_APPS)
set(SUB_DIRS ${SUB_DIRS}
	s_small_server_list
	s_target_collector
	)
set(SUB_DIRS ${SUB_DIRS}
	t_sign
	)
foreach(item IN LISTS SUB_DIRS)
	build_app(${item})
	set(SUB_APPS ${SUB_APPS} ${item})
endforeach()

install(
	TARGETS ${SUB_APPS}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION bin
	RUNTIME DESTINATION bin
	)
