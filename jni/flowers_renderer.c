#include <stdlib.h>
#include <time.h>
#include <GLES2/gl2.h>
#include "gl_utils.h"
#include "flowers_shaders.h"

gl_utils_program_t program_bg;

typedef struct {
	float x;
	float y;
} flowers_point_t;

#define GLOBALS flowers_renderer_globals
typedef struct {
	long int offsetTime;
	flowers_point_t offsetSource;
	flowers_point_t offsetTarget;

} flowers_renderer_globals_t;
flowers_renderer_globals_t GLOBALS;

void flowers_OnRenderFrame() {

	// Update offset.
	++GLOBALS.offsetTime;
	// If time passed generate new target.
	if (GLOBALS.offsetTime > 500) {
		GLOBALS.offsetTime = 0;
		memcpy(&GLOBALS.offsetSource, &GLOBALS.offsetTarget,
				sizeof GLOBALS.offsetSource);
		GLOBALS.offsetTarget.x = ((rand() % 2048) / 1024.f) - 1.f;
		GLOBALS.offsetTarget.y = ((rand() % 2048) / 1024.f) - 1.f;
	}
	// Calculate final offset values.
	float t = GLOBALS.offsetTime / 500.f;
	t = t * t * (3 - 2 * t);
	float ox = GLOBALS.offsetSource.x
			+ t * (GLOBALS.offsetTarget.x - GLOBALS.offsetSource.x);
	float oy = GLOBALS.offsetSource.y
			+ t * (GLOBALS.offsetTarget.y - GLOBALS.offsetSource.y);

	glUseProgram(program_bg.program);
	GLint uAspectRatio = gl_ProgramGetLocation(&program_bg, "uAspectRatio");
	GLint uOffset = gl_ProgramGetLocation(&program_bg, "uOffset");
	GLint uLineWidth = gl_ProgramGetLocation(&program_bg, "uLineWidth");
	GLint aPosition = gl_ProgramGetLocation(&program_bg, "aPosition");
	GLint aColor = gl_ProgramGetLocation(&program_bg, "aColor");

	glUniform2f(uAspectRatio, 1.f, 1.f);
	glUniform2f(uOffset, ox, oy);
	glUniform2f(uLineWidth, .1f, .1f);

	GLbyte vertices[] = { -1, 1, -1, -1, 1, 1, 1, -1 };
	GLfloat colors[] = { .8f, 0.f, 0.f, 1.f, 0.f, .8f, 0.f, 1.f, 0.f, 0.f, .8f,
			1.f, .8f, .8f, 0.f, 1.f };

	glVertexAttribPointer(aPosition, 2, GL_BYTE, GL_FALSE, 0, &vertices);
	glEnableVertexAttribArray(aPosition);
	glVertexAttribPointer(aColor, 4, GL_FLOAT, GL_FALSE, 0, &colors);
	glEnableVertexAttribArray(aColor);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void flowers_OnSurfaceChanged(int width, int height) {
	glViewport(0, 0, width, height);
}

void flowers_OnSurfaceCreated() {
	srand(time(NULL));

	GLOBALS.offsetTarget.x = ((rand() % 2048) / 1024.f) - 1.f;
	GLOBALS.offsetTarget.y = ((rand() % 2048) / 1024.f) - 1.f;

	GLchar bg_vs[] = FLOWERS_BACKGROUND_VS;
	GLchar bg_fs[] = FLOWERS_BACKGROUND_FS;
	gl_ProgramCreate(&program_bg, bg_vs, bg_fs);
}
