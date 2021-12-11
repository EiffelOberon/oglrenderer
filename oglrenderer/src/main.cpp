#include <iostream>
#include "glew.h"
#include "freeglut.h"

void resize(
    int width,
    int height)
{

}

void render()
{
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5, -0.5, 0.0);
    glVertex3f(0.5, 0.0, 0.0);
    glVertex3f(0.0, 0.5, 0.0);
    glEnd();

    glutSwapBuffers();
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