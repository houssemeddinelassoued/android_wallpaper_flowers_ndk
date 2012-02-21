#ifndef GL_UTILS_H__
#define GL_UTILS_H__

#include <GLES2/gl2.h>

typedef struct {
	GLuint program;
	GLuint shader_v;
	GLuint shader_f;
} gl_utils_program_t;

void gl_ProgramCreate(gl_utils_program_t *program, const GLchar *vertexShader,
		const GLchar *fragmentShader);

void gl_ProgramRelease(gl_utils_program_t *program);

GLint gl_ProgramGetLocation(const GLuint program, const GLchar *name);

#endif
