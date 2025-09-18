/* Builds imgui implementations */
// Allows compatibility with stdlibs
#include <misc/cpp/imgui_stdlib.cpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
// Backend implementations
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>