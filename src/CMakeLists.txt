find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(SDL3_shadercross REQUIRED CONFIG)
find_package(glad REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)
find_package(assimp REQUIRED CONFIG)

# Add source to this project's executable.
add_executable (ChessCPP "main.cpp" "stb_image.cpp")

set_property(TARGET ChessCPP PROPERTY CXX_STANDARD 20)

target_include_directories(ChessCPP PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
)

target_link_libraries(ChessCPP PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3_shadercross::SDL3_shadercross
    glad::glad
    glm::glm
    assimp::assimp
)

# TODO: Add tests and install targets if needed.
