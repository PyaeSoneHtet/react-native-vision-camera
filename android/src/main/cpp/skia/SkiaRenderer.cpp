//
// Created by Marc Rousavy on 10.08.23.
//

#include "SkiaRenderer.h"
#include <android/log.h>
#include <gpu/gl/GrGLInterface.h>
#include <gpu/GrBackendSurface.h>
#include <gpu/gl/GrGLTypes.h>
#include <gpu/gl/GrGLInterface.h>
#include <gpu/GrDirectContext.h>
#include "OpenGLError.h"

namespace vision {

SkiaRenderer::SkiaRenderer() {
  __android_log_print(ANDROID_LOG_INFO, TAG, "Initializing OpenGL Context...");

  _gl = createOpenGLContext();
  __android_log_print(ANDROID_LOG_INFO, TAG, "Successfully initialized OpenGL Context!");

  auto pbuffer = eglCreatePbufferSurface(_gl.display, _gl.config, nullptr);
  if (glGetError() != GL_NO_ERROR) throw OpenGLError("Failed to create pixelbuffer surface!");

  eglMakeCurrent(_gl.display, pbuffer, pbuffer, _gl.context);
  if (glGetError() != GL_NO_ERROR) throw OpenGLError("Failed to set current OpenGL context!");
  /*_shader = createPassThroughShader();
  __android_log_print(ANDROID_LOG_INFO, TAG, "Successfully loaded OpenGL pass-through Shader!");*/

  _skia = createSkiaContext();
  __android_log_print(ANDROID_LOG_INFO, TAG, "Successfully created Skia Context!");
}

OpenGLContext SkiaRenderer::createOpenGLContext() {
  auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (display == EGL_NO_DISPLAY) throw OpenGLError("Failed to get default OpenGL Display!");

  EGLint major;
  EGLint minor;
  bool successful = eglInitialize(display, &major, &minor);
  if (!successful) throw OpenGLError("Failed to initialize OpenGL!");

  EGLint attributes[] = {EGL_RENDERABLE_TYPE,
                         EGL_OPENGL_ES2_BIT,
                         EGL_SURFACE_TYPE,
                         EGL_WINDOW_BIT,
                         EGL_ALPHA_SIZE,
                         8,
                         EGL_BLUE_SIZE,
                         8,
                         EGL_GREEN_SIZE,
                         8,
                         EGL_RED_SIZE,
                         8,
                         EGL_DEPTH_SIZE,
                         0,
                         EGL_STENCIL_SIZE,
                         0,
                         EGL_NONE};
  EGLint numConfigs;
  EGLConfig config;
  successful = eglChooseConfig(display, attributes, &config, 1, &numConfigs);
  if (!successful || numConfigs == 0) throw OpenGLError("Failed to choose OpenGL config!");

  EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  auto context = eglCreateContext(display, config, nullptr, contextAttributes);
  if (context == EGL_NO_CONTEXT) throw OpenGLError("Failed to create OpenGL context!");

  return {
    .display = display,
    .context = context,
    .config = config
  };
}

PassThroughShader SkiaRenderer::createPassThroughShader() {
  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), VertexData(), GL_STATIC_DRAW);

  GLuint program = CreateProgram(VertexShaderCode(), FragmentShaderCode());
  if (!program) throw OpenGLError("Failed to load pass-through Shader program!");

  glUseProgram(program);
  GLint aPosition = glGetAttribLocation(program, "aPosition");
  GLint aTexCoord = glGetAttribLocation(program, "aTexCoord");
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if (glGetError() != GL_NO_ERROR) {
    glDeleteProgram(program);
    throw OpenGLError("Failed to load pass-through Shader program!");
  }

  return {
    .vertexBuffer = vertexBuffer,
    .program = program,
    .aPosition = aPosition,
    .aTexCoord = aTexCoord,
  };
}

SkiaContext SkiaRenderer::createSkiaContext() {
  auto interface = GrGLMakeNativeInterface();
  auto skiaContext = GrDirectContext::MakeGL(interface);
  if (skiaContext == nullptr) throw OpenGLError("Failed to create Skia Context from OpenGL context!");

  return {
    .context = skiaContext
  };
}

int SkiaRenderer::createTexture() const {
  eglMakeCurrent(_gl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, _gl.context);
  GLuint textures[1];
  glGenTextures(1, textures);
  return static_cast<int>(textures[0]);
}

void SkiaRenderer::drawFrame() {
  throw std::runtime_error("Not yet implemented!");
  /*
  glBindTexture(GL_TEXTURE_2D, texture);

  int viewportX = 0;
  int viewportY = 0;
  int viewportWidth = _surfaceWidth;
  int viewportHeight = _surfaceHeight;

  int candidateWidth = (int) (((float) textureWidth / (float) textureHeight) * (float)_surfaceHeight);
  int candidateHeight = (int) (((float) textureHeight / (float) textureWidth) * (float)_surfaceWidth);

  if (candidateWidth > _surfaceWidth) {
    viewportX = -1 * (candidateWidth - _surfaceWidth) / 2;
    viewportWidth = candidateWidth;
  } else if (candidateHeight > _surfaceHeight) {
    viewportY = -1 * (candidateHeight - _surfaceHeight) / 2;
    viewportHeight = candidateHeight;
  }

  glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

  glUseProgram(_program);
  glVertexAttribPointer(_aPosition, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const GLvoid*) (0 * sizeof(GLfloat)));
  glEnableVertexAttribArray(_aPosition);
  glVertexAttribPointer(_aTexCoord, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const GLvoid*) (4 * sizeof(GLfloat)));
  glEnableVertexAttribArray(_aTexCoord);
  glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, VertexIndices());

  auto surface = createSkiaSurface(0, 1280, 720);
  auto canvas = surface->getCanvas();
  auto rect = SkRect::MakeXYWH(50, 150, 30, 50);
  auto paint = SkPaint();
  paint.setColor(SkColors::kRed);
  canvas->drawRect(rect, paint);
  canvas->flush();
  surface->flushAndSubmit();

  glFlush();
   */
}

SkiaRenderer::~SkiaRenderer() {
  if (_skia.context != nullptr) {
    // TODO: Do abandonContext()?
    __android_log_print(ANDROID_LOG_INFO, TAG, "Destroying Skia Context...");
    _skia.context = nullptr;
  }
  if (_gl.display != EGL_NO_DISPLAY) {
    if (_gl.context != EGL_NO_CONTEXT) {
      __android_log_print(ANDROID_LOG_INFO, TAG, "Destroying OpenGL Context...");
      eglDestroyContext(_gl.display, _gl.context);
      _gl.context = EGL_NO_CONTEXT;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "Destroying OpenGL Display...");
    eglTerminate(_gl.display);
    _gl.display = EGL_NO_DISPLAY;
  }
}







const GLfloat* SkiaRenderer::VertexData() {
  static const GLfloat vertexData[] = {
      -1.0f, -1.0f, 0.0, 1.0, 0.0f, 0.0f,
      +1.0f, -1.0f, 0.0, 1.0, 1.0f, 0.0f,
      -1.0f, +1.0f, 0.0, 1.0, 0.0f, 1.0f,
      +1.0f, +1.0f, 0.0, 1.0, 1.0f, 1.0f,
  };

  return vertexData;
}

const GLushort* SkiaRenderer::VertexIndices() {
  static const GLushort vertexIndices[] = {
      0, 1, 2, 3
  };

  return vertexIndices;
}

const char* SkiaRenderer::VertexShaderCode() {
  static const char vertexShader[] =
      "attribute vec4 aPosition;\n"
      "attribute vec4 aTexCoord;\n"
      "varying vec2 vTexCoord;\n"
      "void main() {\n"
      "    gl_Position = aPosition;\n"
      "    vTexCoord = aTexCoord.xy;\n"
      "}\n";

  return vertexShader;
}

const char* SkiaRenderer::FragmentShaderCode() {
  static const char fragmentShader[] =
      "precision mediump float;\n"
      "uniform sampler2D uTexture;\n"
      "varying vec2 vTexCoord;\n"
      "void main() {\n"
      "    vec4 color = texture2D(uTexture, vTexCoord);\n"
      "    gl_FragColor = color;\n"
      "}\n";

  return fragmentShader;
}

GLuint SkiaRenderer::LoadShader(GLenum shaderType, const char* shaderCode) {
  GLuint shader = glCreateShader(shaderType);
  if (shader) {
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);
    GLint isCompiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (!isCompiled) {
      glDeleteShader(shader);
      throw OpenGLError("Failed to load OpenGL shader!");
    }
  }
  return shader;
}

GLuint SkiaRenderer::CreateProgram(const char* vertexShaderCode, const char* fragmentShaderCode) {
  GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vertexShaderCode);
  if (!vertexShader) throw OpenGLError("Failed to load pass-through Vertex Shader!");

  GLuint fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragmentShaderCode);
  if (!fragmentShader) throw OpenGLError("Failed to load pass-through Fragment Shader!");

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    if (glGetError() != GL_NO_ERROR) throw OpenGLError("Failed to attach pass-through Vertex Shader!");

    glAttachShader(program, fragmentShader);
    if (glGetError() != GL_NO_ERROR) throw OpenGLError("Failed to attach pass-through Fragment Shader!");

    glLinkProgram(program);
    GLint isLinked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (!isLinked) {
      glDeleteProgram(program);
      throw OpenGLError("Failed to link OpenGL program!");
    }
  }
  return program;
}


} // namespace vision