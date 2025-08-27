// cube_gl.c
// Компилировать: make gl
#include <stdio.h>
#include <stdlib.h>
#include <GLUT/glut.h> // на macOS заголовок в GLUT/glut.h

static float angleX = 30.0f, angleY = 30.0f, angleZ = 0.0f;
static int lastTime = 0;

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0,0.0,6.0,  0.0,0.0,0.0,  0.0,1.0,0.0);

    glRotatef(angleX, 1.0, 0.0, 0.0);
    glRotatef(angleY, 0.0, 1.0, 0.0);
    glRotatef(angleZ, 0.0, 0.0, 1.0);

    // simple material/color
    GLfloat mat_diffuse[] = {0.2f, 0.7f, 0.9f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);

    glutSolidCube(2.0); // solid cube

    glutSwapBuffers();
}

void idle(void) {
    int t = glutGet(GLUT_ELAPSED_TIME);
    int dt = t - lastTime;
    if (lastTime == 0) dt = 0;
    lastTime = t;

    // rotate degrees per second
    float speed = 0.03f * dt;
    angleX += 0.05f * dt * 0.06f;
    angleY += 0.04f * dt * 0.06f;
    angleZ += 0.03f * dt * 0.06f;

    if (angleX > 360) angleX -= 360;
    if (angleY > 360) angleY -= 360;
    if (angleZ > 360) angleZ -= 360;

    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h==0) h=1;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w/(double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    glutCreateWindow("Rotating Cube - OpenGL (GLUT)");

    // basic lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightpos[] = {5.0f,5.0f,5.0f,1.0f};
    GLfloat lightamb[] = {0.2f,0.2f,0.2f,1.0f};
    GLfloat lightdiff[] = {0.8f,0.8f,0.8f,1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdiff);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
