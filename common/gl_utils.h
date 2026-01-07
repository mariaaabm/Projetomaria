#pragma once

#include <GL/glew.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline std::string LoadTextFile(const std::string &path) {
  std::ifstream file(path);
  if (!file) {
    return {};
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

inline GLuint CompileShader(GLenum type, const std::string &source, const std::string &label) {
  GLuint shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(length, '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    std::cerr << "Falha ao compilar shader " << label << ":\n" << log << "\n";
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

inline GLuint CreateProgram(const std::string &vertexPath, const std::string &fragmentPath) {
  std::string vertexSrc = LoadTextFile(vertexPath);
  std::string fragmentSrc = LoadTextFile(fragmentPath);
  if (vertexSrc.empty() || fragmentSrc.empty()) {
    std::cerr << "Nao foi possivel ler os shaders em " << vertexPath << " e " << fragmentPath << "\n";
    return 0;
  }

  GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc, vertexPath);
  GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc, fragmentPath);
  if (!vertexShader || !fragmentShader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(length, '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    std::cerr << "Falha ao linkar programa:\n" << log << "\n";
    glDeleteProgram(program);
    return 0;
  }

  return program;
}