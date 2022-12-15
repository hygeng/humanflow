set(PROJECT_HEADER_FILES
        utilities/utility.h
        utilities/mobility.h
        utilities/man.h
        engine/epidemic.h
        utilities/location.h
        engine/intervene.h)

set(PROJECT_SOURCE_FILES
        utilities/utility.cpp
        utilities/mobility.cpp
        utilities/man.cpp
        engine/epidemic.cpp
        utilities/location.cpp
        engine/intervene.cpp)

set(PROJECT_LIB_NAME ${PROJECT_NAME}_lib CACHE INTERNAL "")

#find_package(Boost 1.53.0 REQUIRED COMPONENTS thread system filesystem)
find_package(Threads REQUIRED Thread)

add_library(${PROJECT_LIB_NAME} ${PROJECT_HEADER_FILES} ${PROJECT_SOURCE_FILES})
set_target_properties(${PROJECT_LIB_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
target_link_libraries(${PROJECT_LIB_NAME} PRIVATE Threads::Threads ${Boost_LIBRARIES})
target_include_directories(${PROJECT_LIB_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
