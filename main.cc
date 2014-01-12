#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "noise.hh"

// Config

const int VSYNC = 1;

// GUI
//#define GLFW_INCLUDE_GLCOREARB
#ifndef __APPLE_CC__
	#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

int width;
int height;

// Render::Matrix

typedef float Vector3f[3];
typedef float Vector4f[4];
typedef float Matrix3f[9];
typedef float Matrix4f[16];

float dot(Vector3f a, Vector3f b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void normalize(float* x, float* y, float* z)
{
	float d = sqrtf(*x**x + *y**y + *z**z);
	*x /= d;
	*y /= d;
	*z /= d;
}

void normalize3(Vector3f a)
{
	float d = sqrtf(dot(a, a));
	a[0] /= d;
	a[1] /= d;
	a[2] /= d;
}

void normalize4(Vector4f a)
{
	float d = sqrtf(a[0]*a[0] + a[1]*a[1] + a[2]*a[2] + a[3]*a[3]);
	a[0] /= d;
	a[1] /= d;
	a[2] /= d;
	a[3] /= d;
}

void quaternion_from_axis_angle(Vector4f quat, Vector3f axis, float angle)
{
	float d = sinf(angle / 2) / sqrt(dot(axis, axis));
	quat[0] = axis[0] * d;
	quat[1] = axis[1] * d;
	quat[2] = axis[2] * d;
	quat[3] = cosf(angle / 2);
}

void quaternion_multiply(Vector4f quat, Vector4f a, Vector4f b)
{
	float ix = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	float iy = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
	float iz = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
	float iw = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	quat[0] = ix;
	quat[1] = iy;
	quat[2] = iz;
	quat[3] = iw;
}

void quaternion_to_matrix3(Vector4f quat, Matrix3f mat)
{
	float x = quat[0];
	float y = quat[1];
	float z = quat[2];
	float w = quat[3];

	float dx = x * 2, x2 = x * dx, wx = w * dx;
	float dy = y * 2, xy = x * dy, y2 = y * dy, wy = w * dy;
	float dz = z * 2, xz = x * dz, yz = y * dz, z2 = z * dz, wz = w * dz;

	mat[0] = 1 - y2 - z2;
	mat[1] = xy - wz;
	mat[2] = xz + wy;

	mat[3] = xy + wz;
	mat[4] = 1 - x2 - z2;
	mat[5] = yz - wx;

	mat[6] = xz - wy;
	mat[7] = yz + wx;
	mat[8] = 1 - x2 - y2;
}

void quaternion_to_matrix4(Vector4f quat, Matrix4f mat, int inverse)
{
	float x = quat[0];
	float y = quat[1];
	float z = quat[2];
	float w = quat[3];

	float dx = x * 2, x2 = x * dx, wx = w * dx;
	float dy = y * 2, xy = x * dy, y2 = y * dy, wy = w * dy;
	float dz = z * 2, xz = x * dz, yz = y * dz, z2 = z * dz, wz = w * dz;

	mat[0] = 1 - y2 - z2;
	mat[1] = inverse ? xy + wz : xy - wz;
	mat[2] = inverse ? xz - wy : xz + wy;
	mat[3] = 0;

	mat[4] = inverse ? xy - wz : xy + wz;
	mat[5] = 1 - x2 - z2;
	mat[6] = inverse ? yz + wx : yz - wx;
	mat[7] = 0;

	mat[8] = inverse ? xz + wy : xz - wy;
	mat[9] = inverse ? yz - wx : yz + wx;
	mat[10] = 1 - x2 - y2;
	mat[11] = 0;

	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
}

void matrix_identity(float matrix[16])
{
	matrix[0] = 1;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 1;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}

void matrix_translate(float matrix[16], Vector3f d)
{
	matrix[0] = 1;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 1;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;
	matrix[12] = d[0];
	matrix[13] = d[1];
	matrix[14] = d[2];
	matrix[15] = 1;
}

void matrix_translate_inverse(float matrix[16], Vector3f d)
{
	matrix[0] = 1;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 1;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;
	matrix[12] = -d[0];
	matrix[13] = -d[1];
	matrix[14] = -d[2];
	matrix[15] = 1;
}

void matrix_rotate(float matrix[16], float x, float y, float z, float angle)
{
	normalize(&x, &y, &z);
	float s = sinf(angle);
	float c = cosf(angle);
	float m = 1 - c;
	matrix[0] = m * x * x + c;
	matrix[1] = m * x * y - z * s;
	matrix[2] = m * z * x + y * s;
	matrix[3] = 0;
	matrix[4] = m * x * y + z * s;
	matrix[5] = m * y * y + c;
	matrix[6] = m * y * z - x * s;
	matrix[7] = 0;
	matrix[8] = m * z * x - y * s;
	matrix[9] = m * y * z + x * s;
	matrix[10] = m * z * z + c;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}

void matrix_vector_multiply(Vector4f vector, float a[16], float b[4])
{
	float result[4];
	for (int i = 0; i < 4; i++)
	{
		float total = 0;
		for (int j = 0; j < 4; j++)
		{
			int p = j * 4 + i;
			int q = j;
			total += a[p] * b[q];
		}
		vector[i] = total;
	}
	memcpy(vector, result, 4 * sizeof(float));
}

void matrix_multiply(float matrix[16], float a[16], float b[16])
{
	float result[16];
	for (int c = 0; c < 4; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			float total = 0;
			for (int i = 0; i < 4; i++)
			{
				total += a[i * 4 + r] * b[c * 4 + i];
			}
			result[c * 4 + r] = total;
		}
	}
	memcpy(matrix, result, 16 * sizeof(float));
}

void matrix_apply(float data[3], float matrix[16])
{
	float vec[4] = {0, 0, 0, 1};
	memcpy(vec, data, sizeof(float) * 3);
		matrix_vector_multiply(vec, matrix, vec);
	memcpy(data, vec, sizeof(float) * 3);
}

void mat_frustum(float* matrix, float left, float right, float bottom, float top, float znear, float zfar)
{
	float temp = 2.0 * znear;
	float temp2 = right - left;
	float temp3 = top - bottom;
	float temp4 = zfar - znear;
	matrix[0] = temp / temp2;
	matrix[1] = 0.0;
	matrix[2] = 0.0;
	matrix[3] = 0.0;
	matrix[4] = 0.0;
	matrix[5] = temp / temp3;
	matrix[6] = 0.0;
	matrix[7] = 0.0;
	matrix[8] = (right + left) / temp2;
	matrix[9] = (top + bottom) / temp3;
	matrix[10] = (-zfar - znear) / temp4;
	matrix[11] = -1.0;
	matrix[12] = 0.0;
	matrix[13] = 0.0;
	matrix[14] = (-temp * zfar) / temp4;
	matrix[15] = 0.0;
}

void matrix_perspective(float matrix[16], float fov, float aspect, float znear, float zfar)
{
	float ymax = znear * tanf(fov * M_PI / 360.0);
	float xmax = ymax * aspect;
	mat_frustum(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void matrix_ortho(float matrix[16], float left, float right, float bottom, float top, float near, float far)
{
	matrix[0] = 2 / (right - left);
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 2 / (top - bottom);
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = -2 / (far - near);
	matrix[11] = 0;
	matrix[12] = -(right + left) / (right - left);
	matrix[13] = -(top + bottom) / (top - bottom);
	matrix[14] = -(far + near) / (far - near);
	matrix[15] = 1;
}

void matrix_2d(float matrix[16], int width, int height)
{
	matrix_ortho(matrix, 0, width, 0, height, -1, 1);
}

void matrix_3d(float matrix[16], Vector3f position, Vector4f orientation, float aspect, float fov)
{
	float a[16];
	float b[16];
	float c[16];
	matrix_translate_inverse(a, position);
	quaternion_to_matrix4(orientation, b, 1/*inverse*/);
	matrix_multiply(a, b, a);
	matrix_perspective(b, fov, aspect, 0.1, 100.0);
	matrix_multiply(matrix, b, a);
}

void matrix_3d_ab(float matrix[16], Vector3f position, float yaw, float pitch, float aspect, float fov)
{
	float a[16];
	float b[16];
	float c[16];
	matrix_translate_inverse(a, position);
	matrix_rotate(b, 1, 0, 0, M_PI / 2);
	matrix_multiply(a, b, a);
	matrix_rotate(b, 0, 1, 0, -yaw);
	matrix_multiply(a, b, a);
	matrix_rotate(b, 1, 0, 0, -pitch);
	matrix_multiply(a, b, a);
	matrix_perspective(b, fov, aspect, 0.03, 100.0);
	matrix_multiply(matrix, b, a);
}

// Render::Buffers

int gen_buffer(GLenum target, GLsizei size, const void* data)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(target, buffer);
	glBufferData(target, size, data, GL_STATIC_DRAW);
	glBindBuffer(target, 0);
	return buffer;
}

void gen_buffers(int components, int faces,
	GLfloat* position_data,  GLfloat* normal_data,  GLfloat* uv_data,
	GLuint*  position_buffer, GLuint* normal_buffer, GLuint* uv_buffer)
{
	if (position_buffer)
	{
		glDeleteBuffers(1, position_buffer);
		*position_buffer = gen_buffer(GL_ARRAY_BUFFER, sizeof(GLfloat) * faces * 6 * components, position_data);
		delete[] position_data;
	}
	if (normal_buffer)
	{
		glDeleteBuffers(1, normal_buffer);
		*normal_buffer = gen_buffer(GL_ARRAY_BUFFER, sizeof(GLfloat) * faces * 6 * components, normal_data);
		delete[] normal_data;
	}
	if (uv_buffer)
	{
		glDeleteBuffers(1, uv_buffer);
		*uv_buffer = gen_buffer(GL_ARRAY_BUFFER, sizeof(GLfloat) * faces * 6 * 2, uv_data);
		delete[] uv_data;
	}
}

void malloc_buffers(int components, int faces, GLfloat** position_data, GLfloat** normal_data, GLfloat** uv_data)
{
	if (position_data)
	{
		*position_data = new GLfloat[faces * 6 * components];
	}
	if (normal_data)
	{
		*normal_data = new GLfloat[faces * 6 * components];
	}
	if (uv_data)
	{
		*uv_data = new GLfloat[faces * 6 * 2];
	}
}

// Render::Texture

#define LODEPNG_COMPILE_CPP
#include "lodepng/lodepng.h"

void load_png_texture(std::string filename)
{
	unsigned char* image;
	unsigned width, height;
	unsigned error = lodepng_decode32_file(&image, &width, &height, filename.c_str());
	if (error)
	{
		fprintf(stderr, "lodepgn_decode32_file error %u: %s\n", error, lodepng_error_text(error));
		exit(1);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	free(image);
}

// Render::Shader

std::string read_file(std::string filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return contents;
	}
	throw errno;
}

GLuint make_shader(GLenum type, std::string source)
{
	GLuint shader = glCreateShader(type);
	const GLchar* c = source.c_str();
	glShaderSource(shader, 1, &c, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		GLchar* info = new GLchar[length];
		glGetShaderInfoLog(shader, length, NULL, info);
		info[length] = 0;
		std::cerr << "glCompileShader failed on [" << source << "]:\n" << info << std::endl;
		exit(1);
	}
	return shader;
}

GLuint load_shader(GLenum type, std::string path)
{
	return make_shader(type, read_file(path));
}

GLuint make_program(GLuint shader1, GLuint shader2)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, shader1);
	glAttachShader(program, shader2);
	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		GLchar* info = new GLchar[length];
		glGetProgramInfoLog(program, length, NULL, info);
		info[length] = 0;
		fprintf(stderr, "glLinkProgram failed: %s\n", info);
		exit(1);
	}
	glDetachShader(program, shader1);
	glDetachShader(program, shader2);
	glDeleteShader(shader1);
	glDeleteShader(shader2);
	return program;
}

GLuint load_program(std::string path1, std::string path2)
{
	GLuint shader1 = load_shader(GL_VERTEX_SHADER, path1);
	GLuint shader2 = load_shader(GL_FRAGMENT_SHADER, path2);
	return make_program(shader1, shader2);
}

// Render::Text

GLuint text_texture;
GLuint text_program;
GLuint text_matrix_loc;
GLuint text_sampler_loc;
GLuint text_position_loc;
GLuint text_uv_loc;

void text_init()
{
	text_program = load_program("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
	text_matrix_loc = glGetUniformLocation(text_program, "matrix");
	text_sampler_loc = glGetUniformLocation(text_program, "sampler");
	text_position_loc = glGetAttribLocation(text_program, "position");
	text_uv_loc = glGetAttribLocation(text_program, "uv");
	//glBindFragDataLocation(text_program, 0, "color");

	glGenTextures(1, &text_texture);
	glBindTexture(GL_TEXTURE_2D, text_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	load_png_texture("font.png");
}

void make_character(float* vertex, float* texture, float x, float y, float n, float m, char c)
{
	float* v = vertex;
	*v++ = x - n; *v++ = y - m;
	*v++ = x + n; *v++ = y - m;
	*v++ = x + n; *v++ = y + m;
	*v++ = x - n; *v++ = y - m;
	*v++ = x + n; *v++ = y + m;
	*v++ = x - n; *v++ = y + m;

	float a = 0.0625;
	float b = a * 2;
	int w = c - 32;
	float du = (w % 16) * a;
	float dv = 1 - (w / 16) * b - b;
	float p = 0;
	float* t = texture;
	*t++ = du + 0; *t++ = dv + p;
	*t++ = du + a; *t++ = dv + p;
	*t++ = du + a; *t++ = dv + b - p;
	*t++ = du + 0; *t++ = dv + p;
	*t++ = du + a; *t++ = dv + b - p;
	*t++ = du + 0; *t++ = dv + b - p;
}

void text_gen_buffers(GLuint* position_buffer, GLuint* uv_buffer, float x, float y, float n, std::string text)
{
	int length = text.length();
	GLfloat* position_data;
	GLfloat* uv_data;
	malloc_buffers(2, length, &position_data, 0, &uv_data);
	for (int i = 0; i < length; i++)
	{
		make_character(position_data + i * 12, uv_data + i * 12, x, y, n / 2, n, text[i]);
		x += n;
	}
	gen_buffers(2, length, position_data, 0, uv_data, position_buffer, 0, uv_buffer);
}

void text_draw_buffers(GLuint position_buffer, GLuint uv_buffer, GLuint position_loc, GLuint uv_loc, int length)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableVertexAttribArray(text_position_loc);
	glEnableVertexAttribArray(text_uv_loc);

	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDrawArrays(GL_TRIANGLES, 0, length * 6);

	glDisableVertexAttribArray(position_loc);
	glDisableVertexAttribArray(uv_loc);
	glDisable(GL_BLEND);
}

void text_print(GLuint position_loc, GLuint uv_loc, float x, float y, float n, std::string text)
{
	GLuint position_buffer = 0;
	GLuint uv_buffer = 0;
	text_gen_buffers(&position_buffer, &uv_buffer, x, y, n, text);
	text_draw_buffers(position_buffer, uv_buffer, position_loc, uv_loc, text.length());
	glDeleteBuffers(1, &position_buffer);
	glDeleteBuffers(1, &uv_buffer);
}

// Map

typedef unsigned char block;

#define RENDER_LIMIT 61
#define CHUNK_SIZE 16

#define MAP_SIZE 128 // >= (1 + RENDER_LIMIT * 2 + 4)
std::vector<block> map_cache;
std::vector<int> map_height_cache;

long map_xmin = 1000000000, map_ymin = 1000000000, map_zmin = 1000000000;

long mod(long x)
{
	return ((unsigned long)x) % MAP_SIZE;
}

block map_get(long x, long y, long z)
{
	return map_cache[(mod(x) * MAP_SIZE + mod(y)) * MAP_SIZE + mod(z)];
}

float map_refresh_time_ms = 0;

// extend world if player moves
void map_refresh(long player_x, long player_y, long player_z)
{
	if (player_x == map_xmin + MAP_SIZE / 2 && player_y == map_ymin + MAP_SIZE / 2 && player_z == map_zmin + MAP_SIZE / 2)
	{
		return;
	}

	float time_start = glfwGetTime();
	// TODO simplex2 is very slow!
	long xmin = player_x - MAP_SIZE / 2;
	long ymin = player_y - MAP_SIZE / 2;
	long zmin = player_z - MAP_SIZE / 2;
	for (long x = xmin; x < xmin + MAP_SIZE; x++)
	{
		for (long y = ymin; y < ymin + MAP_SIZE; y++)
		{
			long q = mod(x) * MAP_SIZE + mod(y);
			if (mod(x) + map_xmin != x || mod(y) + map_ymin != y)			
			{
				map_height_cache[q] = (simplex2(x * 0.007, y * 0.007, 4, 1.0 / 2, 2) - 0.5) * 60;
			}
			long i = map_height_cache[q];
			for (long z = zmin; z < zmin + MAP_SIZE; z++)
			{
				if (mod(x) + map_xmin != x || mod(y) + map_ymin != y || mod(z) + map_zmin != z)
				{
					block b = (z > i) ? 0 : (1 + simplex2(x * -0.044, y * -0.044, 2, 1.0 / 2, 2) * 10);
					map_cache[q * MAP_SIZE + mod(z)] = b;
				}
			}
		} 
	}
	map_xmin = xmin;
	map_ymin = ymin;
	map_zmin = zmin;
	map_refresh_time_ms = (glfwGetTime() - time_start) * 1000;
}

// Model

static Vector3f player_position = { 0, 0, 20 };
static float player_yaw = 0, player_pitch = 0;
float last_time;

void model_init(GLFWwindow* window)
{
	last_time = glfwGetTime();
	glfwSetCursorPos(window, 0, 0);
	map_cache.resize(MAP_SIZE * MAP_SIZE * MAP_SIZE);
	map_height_cache.resize(MAP_SIZE * MAP_SIZE);
}

// TODO non-sticking collision responce
int collision_with_blocks()
{
	float px = player_position[0];
	float py = player_position[1];
	float pz = player_position[2];
	float r = 0.5 + sqrtf(3) / 2 + 0.15; 
	for (long x = (long)px - 1; x <= (long)px + 1; x++)
	{
		float dx = x + 0.5 - px;
		for (long y = (long)py - 1; y <= (long)py + 1; y++)
		{
			float dy = y + 0.5 - py;
			for (long z = (long)pz - 1; z <= (long)pz + 1; z++)
			{
				float dz = z + 0.5 - pz;
				if (map_get(x, y, z) != 0 && dx * dx + dy * dy + dz * dz < r * r)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

void model_move_player(GLFWwindow* window, double dt)
{
	Matrix4f ma, mb;
	matrix_rotate(mb, 0, 0, 1, player_yaw);
	matrix_rotate(ma, 1, 0, 0, player_pitch);
	matrix_multiply(ma, mb, ma);

	Vector3f dir = { 0, 0, 0 };
 	if (glfwGetKey(window, 'A'))
	{
		for (int i = 0; i < 3; i++) dir[i] -= ma[i];
	}
 	if (glfwGetKey(window, 'D'))
	{
		for (int i = 0; i < 3; i++) dir[i] += ma[i];
	}
 	if (glfwGetKey(window, 'W'))
	{
		for (int i = 0; i < 3; i++) dir[i] += ma[i+4];
	}
 	if (glfwGetKey(window, 'S'))
	{
		for (int i = 0; i < 3; i++) dir[i] -= ma[i+4];
	}
 	if (glfwGetKey(window, 'Q'))
	{
		dir[2] += 1;
	}
 	if (glfwGetKey(window, 'E'))
	{
		dir[2] -= 1;
	}

	if (dir[0] != 0 || dir[1] != 0 || dir[2] != 0)
	{
		normalize3(dir);
		float speed = 5;
		for (int k = 0; k < 100; k++)
		{
			for (int i = 0; i < 3; i++) player_position[i] += dir[i] * speed * dt * 0.01;

			if (collision_with_blocks())
			{
				for (int i = 0; i < 3; i++) player_position[i] -= dir[i] * speed * dt * 0.01;
				break;
			}
		}
	}
}

void model_frame(GLFWwindow* window)
{
	double time = glfwGetTime();
	double dt = (time - last_time) < 0.5 ? (time - last_time) : 0.5;
	last_time = time;

 	if (glfwGetKey(window, GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	double cursor_x, cursor_y;
	glfwGetCursorPos(window, &cursor_x, &cursor_y);
	if (cursor_x != 0 || cursor_y != 0)
	{
		player_yaw += cursor_x / 100;
		player_pitch += cursor_y / 100;
		if (player_pitch > M_PI / 2) player_pitch = M_PI / 2;
		if (player_pitch < -M_PI / 2) player_pitch = -M_PI / 2;
		glfwSetCursorPos(window, 0, 0);
	}
	
	model_move_player(window, dt);
	
	map_refresh(player_position[0], player_position[1], player_position[2]);
}

// Render

GLuint block_texture;
Vector3f light_direction = { 0.5, 1, -1 };

void render_init()
{
	printf("OpenGL version: [%s]\n", glGetString(GL_VERSION));
	glEnable(GL_CULL_FACE);
	glClearColor(0, 0, 0, 1.0);

	glGenTextures(1, &block_texture);
	glBindTexture(GL_TEXTURE_2D, block_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	load_png_texture("noise.png");

	normalize3(light_direction);
}

float minf(float a, float b)
{
	return a < b ? a : b;
}

float maxf(float a, float b)
{
	return a > b ? a : b;
}

void light_color(float nx, float ny, float nz, float red, float green, float blue)
{
	Vector3f normal = { nx, ny, nz };
	float a = maxf(0, -dot(normal, light_direction));
	float c = 0.3 + a * 0.7;
	glColor3f(c * red, c * green, c * blue);
}

// Block types:

// 0 empty
// 1 cube
// 2 prism (12 orientations)
// 3 pyramid (8 orientations)
// 4 anti-pyramid (8 orientations)

int B(int a) { return a & 1; }
int S(int a) { return 1 << a; }

void draw_quad(long x, long y, long z, int va, int vb, int vc, int vd)
{
	glTexCoord2f(0, 0); glVertex3f(x + B(va), y + B(va>>1), z + B(va>>2));
	glTexCoord2f(0, 1); glVertex3f(x + B(vb), y + B(vb>>1), z + B(vb>>2));
	glTexCoord2f(1, 1); glVertex3f(x + B(vc), y + B(vc>>1), z + B(vc>>2));

	glTexCoord2f(0, 0); glVertex3f(x + B(va), y + B(va>>1), z + B(va>>2));
	glTexCoord2f(1, 1); glVertex3f(x + B(vc), y + B(vc>>1), z + B(vc>>2));
	glTexCoord2f(1, 0); glVertex3f(x + B(vd), y + B(vd>>1), z + B(vd>>2));
}

void draw_triangle(long x, long y, long z, int va, int vb, int vc)
{
	glTexCoord2f(0, 0); glVertex3f(x + B(va), y + B(va>>1), z + B(va>>2));
	glTexCoord2f(0, 1); glVertex3f(x + B(vb), y + B(vb>>1), z + B(vb>>2));
	glTexCoord2f(1, 1); glVertex3f(x + B(vc), y + B(vc>>1), z + B(vc>>2));
}

void draw_face(long x, long y, long z, int block, int va, int vb, int vc, int vd)
{
	int vertices = 0;
	vertices += (block & (1 << va)) >> va;
	vertices += (block & (1 << vb)) >> vb;
	vertices += (block & (1 << vc)) >> vc;
	vertices += (block & (1 << vd)) >> vd;

	if (vertices == 4)
	{
		glTexCoord2f(0, 0); glVertex3f(x + B(va), y + B(va>>1), z + B(va>>2));
		glTexCoord2f(0, 1); glVertex3f(x + B(vb), y + B(vb>>1), z + B(vb>>2));
		glTexCoord2f(1, 1); glVertex3f(x + B(vc), y + B(vc>>1), z + B(vc>>2));

		glTexCoord2f(0, 0); glVertex3f(x + B(va), y + B(va>>1), z + B(va>>2));
		glTexCoord2f(1, 1); glVertex3f(x + B(vc), y + B(vc>>1), z + B(vc>>2));
		glTexCoord2f(1, 0); glVertex3f(x + B(vd), y + B(vd>>1), z + B(vd>>2));
	}
	if (vertices == 3)
	{
		if (block & (1 << va)) { glTexCoord2f(0, 0); glVertex3f(x + B(va), y + B(va>>1), z + B(va>>2)); }
		if (block & (1 << vb)) { glTexCoord2f(0, 1); glVertex3f(x + B(vb), y + B(vb>>1), z + B(vb>>2)); }
		if (block & (1 << vc)) { glTexCoord2f(1, 1); glVertex3f(x + B(vc), y + B(vc>>1), z + B(vc>>2)); }
		if (block & (1 << vd)) { glTexCoord2f(1, 0); glVertex3f(x + B(vd), y + B(vd>>1), z + B(vd>>2)); }
	}
}

// every bit from 0 to 7 in block represents once vertex (can be off or on)
// cube is 255, empty is 0, prisms / pyramids / anti-pyramids are in between
void render_general(long x, long y, long z, int block, float red, float green, float blue)
{
	// common faces
	if (map_get(x - 1, y, z) != 255)
	{
		light_color(-1, 0, 0, red, green, blue);
		draw_face(x, y, z, block, 0, 4, 6, 2);
	}
	if (map_get(x + 1, y, z) != 255)
	{
		light_color(1, 0, 0, red, green, blue);
		draw_face(x, y, z, block, 1, 3, 7, 5);
	}
	if (map_get(x, y - 1, z) != 255)
	{
		light_color(0, -1, 0, red, green, blue);
		draw_face(x, y, z, block, 0, 1, 5, 4);
	}
	if (map_get(x, y + 1, z) != 255)
	{
		light_color(0, 1, 0, red, green, blue);
		draw_face(x, y, z, block, 2, 6, 7, 3);
	}
	if (map_get(x, y, z - 1) != 255)
	{
		light_color(0, 0, -1, red, green, blue);
		draw_face(x, y, z, block, 0, 2, 3, 1);
	}
	if (map_get(x, y, z + 1) != 255)
	{
		light_color(0, 0, 1, red, green, blue);
		draw_face(x, y, z, block, 4, 5, 7, 6);
	}

	// prism faces
	light_color(1, 1, 1, red, green, blue);
	if (block == 255 - 128 - 64)
	{
		draw_quad(x, y, z, 2, 4, 5, 3);
	}
	if (block == 255 - 2 - 1)
	{
		draw_quad(x, y, z, 4, 2, 3, 5);
	}
	if (block == 255 - 32 - 16)
	{
		draw_quad(x, y, z, 1, 7, 6, 0);
	}
	if (block == 255 - 8 - 4)
	{
		draw_quad(x, y, z, 7, 1, 0, 6);
	}
	if (block == 255 - 64 - 16)
	{
		draw_quad(x, y, z, 0, 5, 7, 2);
	}
	if (block == 255 - 8 - 2)
	{
		draw_quad(x, y, z, 5, 0, 2, 7);
	}
	if (block == 255 - 128 - 32)
	{
		draw_quad(x, y, z, 3, 6, 4, 1);
	}
	if (block == 255 - 4 - 1)
	{
		draw_quad(x, y, z, 6, 3, 1, 4);
	}
	if (block == 255 - 32 - 2)
	{
		draw_quad(x, y, z, 0, 3, 7, 4);
	}
	if (block == 255 - 64 - 4)
	{
		draw_quad(x, y, z, 3, 0, 4, 7);
	}
	if (block == 255 - 16 - 1)
	{
		draw_quad(x, y, z, 5, 6, 2, 1);
	}
	if (block == 255 - 128 - 8)
	{
		draw_quad(x, y, z, 6, 5, 1, 2);
	}

	// pyramid faces
	light_color(1, 1, 1, red, green, blue);
	if (block == 23)
	{
		draw_triangle(x, y, z, 1, 2, 4);
	}
	if (block == 43)
	{
		draw_triangle(x, y, z, 0, 5, 3);
	}
	if (block == 13 + 64)
	{
		draw_triangle(x, y, z, 3, 6, 0);
	}
	if (block == 8 + 4 + 2 + 128)
	{
		draw_triangle(x, y, z, 2, 1, 7);
	}
	if (block == 16 + 32 + 64 + 1)
	{
		draw_triangle(x, y, z, 5, 0, 6);
	}
	if (block == 32 + 16 + 128 + 2)
	{
		draw_triangle(x, y, z, 4, 7, 1);
	}
	if (block == 64 + 128 + 16 + 4)
	{
		draw_triangle(x, y, z, 7, 4, 2);
	}
	if (block == 128 + 64 + 32 + 8)
	{
		draw_triangle(x, y, z, 6, 3, 5);
	}

	// anti-pyramid faces
	light_color(1, 1, 1, red, green, blue);
	if (block == 254)
	{
		draw_triangle(x, y, z, 1, 4, 2);
	}
	if (block == 253)
	{
		draw_triangle(x, y, z, 0, 3, 5);
	}
	if (block == 251)
	{
		draw_triangle(x, y, z, 3, 0, 6);
	}
	if (block == 247)
	{
		draw_triangle(x, y, z, 2, 7, 1);
	}
	if (block == 255 - 16)
	{
		draw_triangle(x, y, z, 5, 6, 0);
	}
	if (block == 255 - 32)
	{
		draw_triangle(x, y, z, 4, 1, 7);
	}
	if (block == 255 - 64)
	{
		draw_triangle(x, y, z, 7, 2, 4);
	}
	if (block == 255 - 128)
	{
		draw_triangle(x, y, z, 6, 5, 3);
	}
}

void render_block(long x, long y, long z, float red, float green, float blue)
{
	float ax = 0, ay = 0;
	float bx = 1, by = 1;
	if (map_get(x - 1, y, z) == 0)
	{
		light_color(-1, 0, 0, red, green, blue);
		draw_quad(x, y, z, 0, 4, 6, 2);
	}
	if (map_get(x + 1, y, z) == 0)
	{
		light_color(1, 0, 0, red, green, blue);
		draw_quad(x, y, z, 1, 3, 7, 5);
	}
	if (map_get(x, y - 1, z) == 0)
	{
		light_color(0, -1, 0, red, green, blue);
		draw_quad(x, y, z, 0, 1, 5, 4);
	}
	if (map_get(x, y + 1, z) == 0)
	{
		light_color(0, 1, 0, red, green, blue);
		draw_quad(x, y, z, 2, 6, 7, 3);
	}
	if (map_get(x, y, z - 1) == 0)
	{
		light_color(0, 0, -1, red, green, blue);
		draw_quad(x, y, z, 0, 2, 3, 1);
	}
	if (map_get(x, y, z + 1) == 0)
	{
		light_color(0, 0, 1, red, green, blue);
		draw_quad(x, y, z, 4, 5, 7, 6);
	}
}

float block_render_time_ms = 0;

void render_world_blocks()
{
	float time_start = glfwGetTime();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, block_texture);
	long px = player_position[0];
	long py = player_position[1];
	long pz = player_position[2];
	glBegin(GL_TRIANGLES);
	for (long x = px - RENDER_LIMIT; x <= px + RENDER_LIMIT; x++)
	{
		// TODO lower bound on ymin and ymax here, sphere instead of cube
		for (long y = py - RENDER_LIMIT; y <= py + RENDER_LIMIT; y++)
		{
			for (long z = pz - RENDER_LIMIT; z <= pz + RENDER_LIMIT; z++)
			{
				block v = map_get(x, y, z);
				if (v != 0)
				{
				int color = v - 1;
				render_block(x, y, z, (color % 4) * 0.33, (color / 4) % 4 * 0.33, (color / 16) % 4 * 0.33);
				}
			}
		}
	}

	for (int a = 0; a < 16; a++)
	{
		render_block(-2, 0, 10+a*2, 1, 1, 0);
	}

	for (int a = 0; a < 8; a++)
	{
		render_general(0, 0, 10+a*2, 255 - S(a), 1, 0, 0);
	}

	for (int a = 0; a < 8; a++)
	{
		render_general(0, 0, 10+a*2 + 16, S(a) + S(a^1) + S(a^2) + S(a^4), 0, 0, 1);
	}

	int q = 0;
	for (int a = 0; a < 8; a += 1)
	{
		for (int b = 0; b < a; b += 1)
		{
		if ((a ^ b) == 1)
		{
			render_general(-4, 0, 10+q, 255 - S(a ^ 6) - S(b ^ 6), 0, 1, 0);
			q += 2;
		}
		if ((a ^ b) == 2)
		{
			render_general(-4, 0, 10+q, 255 - S(a ^ 5) - S(b ^ 5), .5, .5, 1);
			q += 2;
		}
		if ((a ^ b) == 4)
		{
			render_general(-4, 0, 10+q, 255 - S(a ^ 3) - S(b ^ 3), 1, 0.5, 0);
			q += 2;
		}
		}
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);
	block_render_time_ms = (glfwGetTime() - time_start) * 1000;
}

void render_world()
{
	float matrix[16];
		matrix_3d_ab(matrix, player_position, player_yaw, player_pitch, width / (float)height, 65.0);
	glLoadMatrixf(matrix);
	
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render_world_blocks();
	
	glDisable(GL_DEPTH_TEST);
}

void render_gui()
{
	float matrix[16];
		matrix_2d(matrix, width, height);
	
	// Text test
	glBindTexture(GL_TEXTURE_2D, text_texture);
	glUseProgram(text_program);
	glUniformMatrix4fv(text_matrix_loc, 1, GL_FALSE, matrix);
		glUniform1i(text_sampler_loc, 0/*text_texture*/);
		char text_buffer[1024];
		float ts = height / 80;
		float tx = ts / 2;
		float ty = height - ts;
		snprintf(text_buffer, sizeof(text_buffer), "position: %.1f %.1f %.1f, blocks: %.0fms, map: %.0f", player_position[0], player_position[1], player_position[2], block_render_time_ms, map_refresh_time_ms);
		text_print(text_position_loc, text_uv_loc, tx, ty, ts, text_buffer);
	glUseProgram(0);

	#ifdef NEVER
	// Simple quad
	glUseProgram(text_program);
	glUniformMatrix4fv(text_matrix_loc, 1, GL_FALSE, matrix);
		glUniform1i(text_sampler_loc, 0/*text_texture*/);

	GLuint vao_quad;
	glGenVertexArrays(1, &vao_quad);
		glBindVertexArray(vao_quad);

	float vertices[] = { 0, 0, 100, 0, 0, 100 };
	int v = gen_buffer(GL_ARRAY_BUFFER, sizeof(vertices), vertices);
	float colors[] = { 0, 0, 100, 0, 0, 100 };
	int c = gen_buffer(GL_ARRAY_BUFFER, sizeof(colors), colors);

		glDrawArrays(GL_QUADS, 0, 4);

	glDeleteVertexArrays(1, &vao_quad);
	#endif
}

void render_frame()
{
	glViewport(0, 0, width, height);
	render_world();
	render_gui();
}

int main(int argc, char** argv)
{
	if (!glfwInit())
	{
		return 0;
	}

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Arena", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(VSYNC);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	model_init(window);
	render_init();
	text_init();
	
	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &width, &height);

		model_frame(window);
		render_frame();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}
