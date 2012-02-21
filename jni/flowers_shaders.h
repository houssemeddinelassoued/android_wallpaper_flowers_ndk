#ifndef FLOWERS_SHADERS_H__
#define FLOWERS_SHADERS_H__

#define FLOWERS_BACKGROUND_VS " \
uniform vec2 uOffset; \
uniform vec2 uAspectRatio; \
attribute vec2 aPosition; \
attribute vec3 aColor; \
varying vec3 vColor; \
varying vec2 vPosition; \
void main() { \
    gl_Position = vec4(aPosition, 0.0, 1.0); \
    vColor = aColor; \
    vPosition = (aPosition + uOffset) * uAspectRatio * 10.0; \
} "

#define FLOWERS_BACKGROUND_FS " \
precision mediump float; \
uniform vec2 uLineWidth; \
varying vec3 vColor; \
varying vec2 vPosition; \
void main() { \
    gl_FragColor = vec4(vColor, 1.0); \
    vec2 f = fract(vPosition); \
    if (f.x < uLineWidth.x || f.y < uLineWidth.y) { \
        gl_FragColor.rgb *= 0.98; \
    } \
} "

#endif
