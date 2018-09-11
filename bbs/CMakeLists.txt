set(BBS_SRC
    bbs.cpp
)

add_library(bbs ${BBS_SRC})
add_dependencies(bbs p2p)
target_link_libraries(bbs p2p utility)

add_library(bbs_client STATIC bbs_client.cpp)
add_dependencies(bbs_client p2p)
target_link_libraries(bbs_client p2p utility)

if(NOT ANDROID)
    add_subdirectory(unittest)
endif()
