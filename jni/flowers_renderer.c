#include <stdlib.h>
#include <GLES2/gl2.h>

void flowers_OnRenderFrame() {
	GLclampf r = (rand() % 256) / 255.0;
	GLclampf g = (rand() % 256) / 255.0;
	GLclampf b = (rand() % 256) / 255.0;

	glClearColor(r, g, b, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void flowers_OnSurfaceChanged(int width, int height) {
	glViewport(0, 0, width, height);
}

void flowers_OnSurfaceCreated() {
	srand(time(NULL));
}
