#include <iostream>
#include <memory>
#include <stack>

#include "glew.h"
#include "freeglut.h"
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl3.h"
#include "renderer.h"

struct MouseState
{
    int mButton;
    int mX;
    int mY;
};

std::unique_ptr<Renderer> renderer = nullptr;
std::stack<MouseState> mouseStates;

void APIENTRY debugMessageCallback(
    const GLenum  source, 
    const GLenum  type, 
    const GLuint  id,
    const GLenum  severity, 
    const GLsizei length,
    const GLchar  *msg, 
    const void    *data)
{
    std::string sourceString;
    std::string typeString;
    std::string severityString;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
        sourceString = "API";
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceString = "WINDOW SYSTEM";
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceString = "SHADER COMPILER";
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceString = "THIRD PARTY";
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
        sourceString = "APPLICATION";
        break;

        case GL_DEBUG_SOURCE_OTHER:
        sourceString = "UNKNOWN";
        break;

        default:
        sourceString = "UNKNOWN";
        break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
        typeString = "ERROR";
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeString = "DEPRECATED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeString = "UDEFINED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
        typeString = "PORTABILITY";
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
        typeString = "PERFORMANCE";
        break;

        case GL_DEBUG_TYPE_OTHER:
        typeString = "OTHER";
        break;

        case GL_DEBUG_TYPE_MARKER:
        typeString = "MARKER";
        break;

        default:
        typeString = "UNKNOWN";
        break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        severityString = "HIGH";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        severityString = "MEDIUM";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        severityString = "LOW";
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityString = "NOTIFICATION";
        break;

        default:
        severityString = "UNKNOWN";
        break;
    }

    if (severity == GL_DEBUG_SEVERITY_HIGH ||
        severity == GL_DEBUG_SEVERITY_MEDIUM ||
        severity == GL_DEBUG_SEVERITY_LOW)
    {
        printf("%d: %s of %s severity, raised from %s: %s\n",
            id, typeString.c_str(), severityString.c_str(), sourceString.c_str(), msg);
    }
}


void mouseClick(
    const int button, 
    const int state,
    const int x, 
    const int y)
{
    if (state == GLUT_DOWN)
    {
        mouseStates.push(MouseState{button, x, y});
    }
    else if (state == GLUT_UP)
    {
        if (mouseStates.size() == 0)
        {
            return;
        }
        mouseStates.pop();
    }
    ImGui_ImplGLUT_MouseFunc(button, state, x, y);
}


void mouseMotion(
    const int x,
    const int y)
{
    if (mouseStates.size() == 0)
    {
        // this somehow gets triggered when double clicking on title bar
        return;
    }
    MouseState newState = mouseStates.top();

    const float deltaX = x - newState.mX;
    const float deltaY = y - newState.mY;

    if (newState.mButton == GLUT_RIGHT_BUTTON)
    {
        renderer->updateCamera(deltaX, deltaY);
    }

    newState.mX = x;
    newState.mY = y;

    mouseStates.pop();
    mouseStates.push(newState);
    ImGui_ImplGLUT_MotionFunc(x, y);
}

void mouseWheel(int button, int dir, int x, int y)
{
    if (dir > 0)
    {
        renderer->updateCameraZoom(dir);
    }
    else
    {
        renderer->updateCameraZoom(dir);
    }

    return;
}


void resize(
    int width,
    int height)
{
    if (renderer)
    {
        renderer->resize(width, height);
    }
}


void render()
{
    if (renderer)
    {
        renderer->preRender();
        renderer->render();

        // start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGLUT_NewFrame();

        // render gui
        renderer->renderGUI();

        // imgui rendering
        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap buffers
        glutSwapBuffers();
        renderer->postRender();
    }
}


void update()
{
    render();
}


int main(
    int     argc, 
    char**  argv)
{
    // init GLUT and create Window
    glutInit(&argc, argv);
    glutInitContextVersion(4, 6);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(1600, 900);
    glutCreateWindow("OglRenderer");

    glewExperimental = GL_TRUE;
    const GLenum glewStatus = glewInit();
    if (glewStatus == GLEW_OK)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.DisplaySize = ImVec2(1600, 900);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x,
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].y,
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].z,
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w * 0.333f);

        // Setup Platform/Renderer backends
        ImGui_ImplGLUT_Init();
        ImGui_ImplGLUT_InstallFuncs();
        ImGui_ImplOpenGL3_Init();

        renderer = std::make_unique<Renderer>();

        // register callbacks
        glutDisplayFunc(render);
        glutIdleFunc(update);
        glutMouseFunc(mouseClick);
        glutMotionFunc(mouseMotion);
        glutMouseWheelFunc(mouseWheel);

        // Here is our new entry in the main function
        glutReshapeFunc(resize);

#ifdef _DEBUG
        // GL_DEBUG_OUTPUT / GL_DEBUG_OUTPUT_SYNCHRONUS
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
            GL_DEBUG_SEVERITY_NOTIFICATION, -1, "Started debugging");
#endif

        // enter GLUT event processing cycle
        glutMainLoop();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
    }
    else
    {
        std::cout << glewGetErrorString(glewStatus) << std::endl;
    }
    return 1;
}