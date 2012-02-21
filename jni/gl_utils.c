#include <stdlib.h>
#include "gl_utils.h"
#include "log.h"

GLuint gl_ShaderCreate(GLenum type, const GLchar *source) {
	GLuint shader = glCreateShader(type);
	LOGD("gl_ShaderCreate", "shader=%d", shader);
	if (shader) {
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);
		int compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled != GL_TRUE) {
			LOGD("gl_ShaderCreate", "shader=%d failed", shader);
			glDeleteShader(shader);
			shader = 0;
		}
	}
	return shader;
}

void gl_ProgramCreate(gl_utils_program_t *shader, const GLchar *vertexShader,
		const GLchar *fragmentShader) {
	shader->shader_v = gl_ShaderCreate(GL_VERTEX_SHADER, vertexShader);
	shader->shader_f = gl_ShaderCreate(GL_FRAGMENT_SHADER, fragmentShader);
	shader->program = glCreateProgram();
	LOGD("gl_ProgramCreate", "program=%d", shader->program);
	if (shader->program != 0) {
		glAttachShader(shader->program, shader->shader_v);
		glAttachShader(shader->program, shader->shader_f);
		glLinkProgram(shader->program);
		int linkStatus;
		glGetProgramiv(shader->program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			LOGD("gl_ProgramCreate", "program=%d failed", shader->program);
			gl_ProgramRelease(shader);
		}
	}
}

void gl_ProgramRelease(gl_utils_program_t *shader) {
	glDeleteProgram(shader->program);
	glDeleteShader(shader->shader_f);
	glDeleteShader(shader->shader_v);
	memset(shader, 0, sizeof *shader);
}

GLint gl_ProgramGetLocation(const GLuint program, const GLchar *name) {
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) {
		location = glGetAttribLocation(program, name);
	}
	return location;
}
