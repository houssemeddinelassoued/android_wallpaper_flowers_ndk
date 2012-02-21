#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <GLES2/gl2.h>
#include "gl_utils.h"
#include "flowers_shaders.h"

typedef unsigned long long flowers_time_t;

typedef struct {
	GLfloat x;
	GLfloat y;
} flowers_point_t;

typedef struct {
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} flowers_color_t;

#define GLOBALS flowers_renderer_globals
typedef struct {
	flowers_time_t offsetTime;
	flowers_point_t offsetSource;
	flowers_point_t offsetTarget;
	flowers_point_t aspectRatio;
	flowers_point_t lineWidth;
	gl_utils_program_t program_bg;
} flowers_renderer_globals_t;
flowers_renderer_globals_t GLOBALS;

flowers_time_t flowers_CurrentTimeMillis() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

void flowers_OnRenderFrame() {

	// Update offset.
	flowers_time_t currentTime = flowers_CurrentTimeMillis();
	// If time passed generate new target.
	if (currentTime - GLOBALS.offsetTime > 5000) {
		GLOBALS.offsetTime = currentTime;
		memcpy(&GLOBALS.offsetSource, &GLOBALS.offsetTarget,
				sizeof GLOBALS.offsetSource);
		GLOBALS.offsetTarget.x = ((rand() % 2048) / 1024.f) - 1.f;
		GLOBALS.offsetTarget.y = ((rand() % 2048) / 1024.f) - 1.f;
	}

	// Calculate final offset values.
	float t = (currentTime - GLOBALS.offsetTime) / 5000.f;
	t = t * t * (3 - 2 * t);

	flowers_point_t offset;
	offset.x = GLOBALS.offsetSource.x
			+ t * (GLOBALS.offsetTarget.x - GLOBALS.offsetSource.x);
	offset.y = GLOBALS.offsetSource.y
			+ t * (GLOBALS.offsetTarget.y - GLOBALS.offsetSource.y);

	GLuint program = GLOBALS.program_bg.program;
	GLint uOffset = gl_ProgramGetLocation(program, "uOffset");
	GLint uAspectRatio = gl_ProgramGetLocation(program, "uAspectRatio");
	GLint uLineWidth = gl_ProgramGetLocation(program, "uLineWidth");
	GLint aPosition = gl_ProgramGetLocation(program, "aPosition");
	GLint aColor = gl_ProgramGetLocation(program, "aColor");

	glUseProgram(program);
	glUniform2f(uOffset, offset.x, offset.y);
	glUniform2f(uAspectRatio, GLOBALS.aspectRatio.x, GLOBALS.aspectRatio.y);
	glUniform2f(uLineWidth, GLOBALS.lineWidth.x, GLOBALS.lineWidth.y);

	GLbyte vertices[] = { -1, 1, -1, -1, 1, 1, 1, -1 };
	GLfloat colors[] = { .8f, 0.f, 0.f, 0.f, .8f, 0.f, 0.f, 0.f, .8f, .8f, .8f,
			0.f };

	glVertexAttribPointer(aPosition, 2, GL_BYTE, GL_FALSE, 0, &vertices);
	glEnableVertexAttribArray(aPosition);
	glVertexAttribPointer(aColor, 3, GL_FLOAT, GL_FALSE, 0, &colors);
	glEnableVertexAttribArray(aColor);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void flowers_OnSurfaceChanged(int32_t width, int32_t height) {
	glViewport(0, 0, width, height);
	GLfloat min = width < height ? width : height;
	GLOBALS.aspectRatio.x = min / height;
	GLOBALS.aspectRatio.y = min / width;
	GLOBALS.lineWidth.x = GLOBALS.aspectRatio.x * 40.f / width;
	GLOBALS.lineWidth.y = GLOBALS.aspectRatio.y * 40.f / height;
}

void flowers_OnSurfaceCreated() {
	srand(time(NULL));

	GLOBALS.offsetTime = flowers_CurrentTimeMillis();
	GLOBALS.offsetTarget.x = ((rand() % 2048) / 1024.f) - 1.f;
	GLOBALS.offsetTarget.y = ((rand() % 2048) / 1024.f) - 1.f;

	GLchar bg_vs[] = FLOWERS_BACKGROUND_VS;
	GLchar bg_fs[] = FLOWERS_BACKGROUND_FS;
	gl_ProgramCreate(&GLOBALS.program_bg, bg_vs, bg_fs);
}
