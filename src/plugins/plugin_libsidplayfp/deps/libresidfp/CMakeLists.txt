cmake_minimum_required(VERSION 3.28)

set(NAME "libresidfp")
project(${NAME})

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

set(VERSION "1.2.3")
set(NAME_VERSIONED "${NAME}-${VERSION}")
set(FILENAME "${NAME_VERSIONED}.tar.gz")
set(FILENAME_URL "${VERSION}.tar.gz")
set(URL "https://github.com/libsidplayfp/${NAME}/releases/download/v${VERSION}/${FILENAME}")
set(SHA_256_HASH "e9b53a984ca708c3e8d9139eb6aad6141f73ffc22d70a020f7966803f4e2f76d")
download_patch_and_add(${NAME_VERSIONED} ${FILENAME} ${URL} ${SHA_256_HASH} false "" "")

set(EXTERNAL_SOURCE_DIR_${NAME} "${EXTERNAL_SOURCE_DIR}" PARENT_SCOPE)

set(PACKAGE_VERSION "${VERSION}")

string(REPLACE "." ";" VERSION_LIST ${VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)
set(LIB_MAJOR "${VERSION_MAJOR}")
set(LIB_MINOR "${VERSION_MINOR}")
set(LIB_LEVEL "${VERSION_PATCH}")
configure_file(
        ${EXTERNAL_SOURCE_DIR}/src/residfp/sidversion.h.in
        ${EXTERNAL_SOURCE_DIR}/src/residfp/sidversion.h
        @ONLY NEWLINE_STYLE UNIX
)

set(HAVE_BUILTIN_EXPECT 1)
set(RESIDFP_BRANCH_HINTS 1)
set(RESIDFP_INLINING 1)
set(RESIDFP_INLINE "inline")
configure_file(
        ${EXTERNAL_SOURCE_DIR}/src/siddefs-fp.h.in
        ${EXTERNAL_SOURCE_DIR}/src/siddefs-fp.h
        @ONLY NEWLINE_STYLE UNIX
)

file(GLOB SOURCES
        "${EXTERNAL_SOURCE_DIR}/src/resample/*.cpp"
        "${EXTERNAL_SOURCE_DIR}/src/residfp/*.cpp"
        "${EXTERNAL_SOURCE_DIR}/src/*.cpp")

list(REMOVE_ITEM SOURCES ${EXTERNAL_SOURCE_DIR}/src/resample/test.cpp)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_SOURCE_DIR}/src)

target_compile_definitions(${PROJECT_NAME} PRIVATE RUNTIME_DISPATCH)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(${NAME}_VERSION ${VERSION} PARENT_SCOPE)