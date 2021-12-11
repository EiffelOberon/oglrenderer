#include <iostream>
#include <memory>

#include "glew.h"
#include "freeglut.h"
#include "renderer.h"

std::unique_ptr<Renderer> renderer = nullptr;

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

        // enter GLUT event processing cycle
        glutMainLoop();
    }
    else
    {
        std::cout << glewGetErrorString(glewStatus) << std::endl;
    }
    return 1;
}