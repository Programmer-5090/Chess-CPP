find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(SDL3_shadercross REQUIRED CONFIG)
find_package(glad REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)
find_package(assimp REQUIRED CONFIG)
find_package(imgui CONFIG REQUIRED)

# Collect all source files automatically
file(GLOB_RECURSE CHESS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_SOURCE_DIR}/geometry/*.cpp"
)

add_executable(ChessCPP ${CHESS_SOURCES} )

set_property(TARGET ChessCPP PROPERTY CXX_STANDARD 20)

target_include_directories(ChessCPP PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/utils/include"
    "${CMAKE_SOURCE_DIR}/geometry/include"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
    "${CMAKE_SOURCE_DIR}/include/chess_engine"
)

target_link_libraries(ChessCPP PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3_shadercross::SDL3_shadercross
    glad::glad
    glm::glm
    assimp::assimp
    imgui::imgui
)

# Copy shaders folder next to the executable after every build
add_custom_command(TARGET ChessCPP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:ChessCPP>/shaders"
)

# TODO: Add tests and install targets if needed.
