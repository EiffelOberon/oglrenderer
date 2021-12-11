#include <iostream>
#include <memory>

#include "glew.h"
#include "freeglut.h"
#include "renderer.h"

std::unique_ptr<Renderer> renderer = nullptr;

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

    printf("%d: %s of %s severity, raised from %s: %s\n",
            id, typeString.c_str(), severityString.c_str(), sourceString.c_str(), msg);
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
        renderer->render();
    }
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
        renderer = std::make_unique<Renderer>();

        // register callbacks
        glutDisplayFunc(render);

        // Here is our new entry in the main function
        glutReshapeFunc(resize);

#ifdef _DEBUG
        // GL_DEBUG_OUTPUT / GL_DEBUG_OUTPUT_SYNCHRONUS
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, NULL);
#endif

        // enter GLUT event processing cycle
        glutMainLoop();
    }
    else
    {
        std::cout << glewGetErrorString(glewStatus) << std::endl;
    }
    return 1;
}