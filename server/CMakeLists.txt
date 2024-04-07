cmake_minimum_required(VERSION 3.0)  # Minimum CMake version required

# Define the project and its version
project(Server VERSION 1.0 LANGUAGES CXX)

find_package(google_cloud_cpp_speech REQUIRED)
find_package(google_cloud_cpp_texttospeech REQUIRED)

if (VCPKG_TARGET_TRIPLET MATCHES "-static$")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else ()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif ()

# Add all source files to a variable
set(SOURCES test/test_tts.cpp src/gcp/google_speech_2_text.cpp src/connection.cpp)



# Add the executable target
add_executable(server ${SOURCES})

# Optionally, set C++ standard (C++11 used here)
target_compile_features(server PUBLIC cxx_std_20)

# Link libraries
target_link_libraries(server PRIVATE pthread)
target_link_libraries(server PRIVATE curlpp)
target_link_libraries(server PRIVATE curl)
target_link_libraries(server PRIVATE dl)
target_link_libraries(server PUBLIC google-cloud-cpp::speech)
target_link_libraries(server PUBLIC google-cloud-cpp::texttospeech)

#cd server
#cmake -S . -B .build -DCMAKE_TOOLCHAIN_FILE=/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake
#cmake --build .build