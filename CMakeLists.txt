cmake_minimum_required(VERSION 3.10.2)
cmake_policy(SET CMP0091 NEW) # for CMAKE_MSVC_RUNTIME_LIBRARY
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project("avhttp")

set(CMAKE_CXX_STANDARD 17)
include(CheckCXXCompilerFlag)
include(CheckLibraryExists)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (VCPKG_TARGET_ANDROID)
	include("cmake/vcpkg_android.cmake")
endif()

if (MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /MP")

	add_definitions(
		-D_CRT_SECURE_NO_DEPRECATE
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_NONSTDC_NO_DEPRECATE
		-D_CRT_NONSTDC_NO_WARNINGS
		-D_SCL_SECURE_NO_DEPRECATE
		-D_SCL_SECURE_NO_WARNINGS
	)

	add_definitions(
		-DWIN32_LEAN_AND_MEAN
		-D_WIN32_WINNT=0x0601
		-DNOMINMAX
		-DUNICODE
		-D_UNICODE
		-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
	)

	add_definitions(
		-DBOOST_ALL_STATIC_LINK
		-DBOOST_THREAD_USE_LIB
		-DBOOST_FILESYSTEM_STATIC_LINK
		-DBOOST_USE_WINAPI_VERSION=0x0601
	)
endif()


set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

set(Boost_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boost)

add_definitions(-DBOOST_LOCALE_HIDE_AUTO_PTR)
add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
add_definitions(-DBOOST_DISABLE_PRAGMA_MESSAGE)
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
#add_definitions(-DBOOST_USE_UCONTEXT)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/boost)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt)
if (VCPKG_TARGET_ANDROID)
	find_package(OpenSSL REQUIRED)
else()
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/openssl)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/zlib EXCLUDE_FROM_ALL)

set(DEPEND_INCLUDES
	${CMAKE_CURRENT_SOURCE_DIR}
	${Boost_INCLUDE_DIRS}
	${FMT_INCLUDE_DIR}
	${OPENSSL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)

set(DEPEND_LIBRARYS
			Boost::coroutine
			Boost::context
			Boost::thread
			Boost::date_time
			Boost::filesystem
			Boost::program_options
			Boost::regex
			Boost::json

			${FMT_LIBRARIES}
			${OPENSSL_LIBRARIES}
			${ZLIB_LIBRARIES}
)

file(GLOB avhttp_SRCS
			avhttp.cpp

			async_connect.hpp
			default_cert.hpp
			handler_type_check.hpp
			http_last_modified.hpp
			io.hpp
			logging.hpp
			avhttp.hpp
			simple_http.hpp
			socks_proxy.hpp
			url_parser.hpp
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

if (WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release)
endif()

add_library(avhttp SHARED ${avhttp_SRCS})
add_library(avhttp-static STATIC ${avhttp_SRCS})

target_include_directories(avhttp PRIVATE ${DEPEND_INCLUDES})
target_include_directories(avhttp-static PRIVATE ${DEPEND_INCLUDES})

if (ANDROID)
	find_library(
		log-lib
		log
	)

	set(DEPEND_LIBRARYS ${DEPEND_LIBRARYS} ${log-lib})
	if (VCPKG_TARGET_ANDROID)
		set(DEPEND_LIBRARYS ${DEPEND_LIBRARYS} OpenSSL::SSL OpenSSL::Crypto)
	endif()
endif()

target_link_libraries(avhttp ${DEPEND_LIBRARYS})
target_link_libraries(avhttp-static ${DEPEND_LIBRARYS})

if (ANDROID)
	find_package(SWIG)

	if (SWIG_FOUND)
		add_subdirectory(jni)
	endif()
endif()

