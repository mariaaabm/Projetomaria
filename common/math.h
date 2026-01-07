#pragma once

#include <cmath>

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

inline Vec3 operator+(const Vec3 &a, const Vec3 &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 operator-(const Vec3 &a, const Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 operator*(const Vec3 &v, float s) {
  return {v.x * s, v.y * s, v.z * s};
}

inline Vec3 operator/(const Vec3 &v, float s) {
  return {v.x / s, v.y / s, v.z / s};
}

inline float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Cross(const Vec3 &a, const Vec3 &b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

inline float Length(const Vec3 &v) {
  return std::sqrt(Dot(v, v));
}

inline Vec3 Normalize(const Vec3 &v) {
  float len = Length(v);
  if (len <= 0.00001f) {
    return {0.0f, 1.0f, 0.0f};
  }
  return v / len;
}

struct Mat4 {
  float m[16] = {0};
};

inline Mat4 Mat4Identity() {
  Mat4 out;
  out.m[0] = 1.0f;
  out.m[5] = 1.0f;
  out.m[10] = 1.0f;
  out.m[15] = 1.0f;
  return out;
}

inline Mat4 Mat4Multiply(const Mat4 &a, const Mat4 &b) {
  Mat4 out;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      out.m[col * 4 + row] =
          a.m[0 * 4 + row] * b.m[col * 4 + 0] +
          a.m[1 * 4 + row] * b.m[col * 4 + 1] +
          a.m[2 * 4 + row] * b.m[col * 4 + 2] +
          a.m[3 * 4 + row] * b.m[col * 4 + 3];
    }
  }
  return out;
}

inline Mat4 Mat4Translate(const Vec3 &t) {
  Mat4 out = Mat4Identity();
  out.m[12] = t.x;
  out.m[13] = t.y;
  out.m[14] = t.z;
  return out;
}

inline Mat4 Mat4Scale(float s) {
  Mat4 out = Mat4Identity();
  out.m[0] = s;
  out.m[5] = s;
  out.m[10] = s;
  return out;
}

inline Mat4 Mat4RotateY(float angleRadians) {
  Mat4 out = Mat4Identity();
  float c = std::cos(angleRadians);
  float s = std::sin(angleRadians);
  out.m[0] = c;
  out.m[2] = s;
  out.m[8] = -s;
  out.m[10] = c;
  return out;
}

inline Mat4 Mat4Perspective(float fovyRadians, float aspect, float zNear, float zFar) {
  Mat4 out;
  float f = 1.0f / std::tan(fovyRadians * 0.5f);
  out.m[0] = f / aspect;
  out.m[5] = f;
  out.m[10] = (zFar + zNear) / (zNear - zFar);
  out.m[11] = -1.0f;
  out.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
  return out;
}

inline Mat4 Mat4LookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &up) {
  Vec3 f = Normalize(target - eye);
  Vec3 s = Normalize(Cross(f, up));
  Vec3 u = Cross(s, f);

  Mat4 out = Mat4Identity();
  out.m[0] = s.x;
  out.m[4] = s.y;
  out.m[8] = s.z;

  out.m[1] = u.x;
  out.m[5] = u.y;
  out.m[9] = u.z;

  out.m[2] = -f.x;
  out.m[6] = -f.y;
  out.m[10] = -f.z;

  out.m[12] = -Dot(s, eye);
  out.m[13] = -Dot(u, eye);
  out.m[14] = Dot(f, eye);
  return out;
}