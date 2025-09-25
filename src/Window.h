#pragma once
#include "pch.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace WinChess {
    struct WindowProps {
        std::string Title = "Default Window Title";
        uint32_t Width = 960, Height = 720;
    };

    class Window {
    private:
        /* Singleton */
        inline static Window* s_Instance = nullptr;

        GLFWwindow* m_Window;
        uint32_t m_Width, m_Height;
        std::string m_Title;

        bool m_IsClosed = false;
    public:
        Window(const WindowProps& props = WindowProps{});
        ~Window();

        inline static Window* GetInstance() { return s_Instance; }
        inline GLFWwindow* GetNativeWindow() const { return m_Window; }
        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }
        inline const std::string& GetTitle() const { return m_Title; }
        inline ImGuiID GetActiveWidgetID() const { return GImGui->ActiveId; } // Returns currently active id
        inline bool GetIsClosed() const { return m_IsClosed; }

        void BeginImGui() const;
        void EndImGui() const;
        void UpdateFrame() const;

        // These should only be called by callback
        inline void CloseWindow() { m_IsClosed = true; }
        inline void SetWidthAndHeight(uint32_t width, uint32_t height) { m_Width = width; m_Height = height; }
    private:
        void InitGLFW();
        void InitGlad();
        void InitImGui();
        void SetDarkThemeColors() const;
        void EntireWindowDocking() const;
    };
}