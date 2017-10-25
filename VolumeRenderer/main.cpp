#define _USE_MATH_DEFINES

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include <algorithm>
#include <limits>

//#include "nanogui/common.h"
//#include "nanogui/formhelper.h"
//
//using namespace nanogui;

#define width 300
#define far_plane 500.0f
#define inital_z -70.0f
#define fov_degrees 45.0f
#define flip_degree 135.1f
#define y_trans -7

cyPoint3f lightVector = cyPoint3f(18, 60, 20);

cyTriMesh mesh;
GLuint vertexArrayObj;
cy::GLSLProgram volume_shaders;
cy::Matrix4<float> cameraTransformationMatrix;
cy::Matrix4<float> lightCameraTransformationMatrix;
cy::Matrix4<float> perspectiveMatrix;
cy::Matrix4<float> totalRotationMatrix;
cy::Matrix4<float> translationMatrix;
cy::Matrix4<float> lightTransformationMatrix;
cy::Matrix4<float> lightRotationMatrix;
int selected;
int selectedKey;
bool zoom_in = true;
int movement = 0;
std::vector<cyPoint3f> vertices;
std::vector<cyPoint3f> textureVertices;
std::vector<cyPoint3f> normals;
cyPoint3f origin = cyPoint3f(0, 0, 50);
cyMatrix4f view = cyMatrix4f::MatrixView(origin, cyPoint3f(0, 0, 0), cyPoint3f(0, 1, 0));
cyMatrix4f lightView = cyMatrix4f::MatrixView(lightVector, cyPoint3f(0, 0, 0), cyPoint3f(0, 1, 0));
cyMatrix4f lightProj = cyMatrix4f::MatrixPerspective(M_PI / 8, 1, 20, 200);
cyMatrix4f bias = cyMatrix4f::MatrixTrans(cyPoint3f(0.5f, 0.5f, 0.495f)) * cyMatrix4f::MatrixScale(0.5f, 0.5f, 0.5f);
cyMatrix4f teapotLightMVP;

const int SMALL_VOL_X = 246;
const int SMALL_VOL_Y = 246;
const int SMALL_VOL_Z = 221;
const int LG_VOL_X = 492;
const int LG_VOL_Y = 492;
const int LG_VOL_Z = 442;

GLuint texture;


void setInitialRotationAndTranslation()
{
	cyPoint3f max = mesh.GetBoundMax();
	cyPoint3f min = mesh.GetBoundMin();
	cy::Matrix4<float> rotationZ = cyMatrix4f::MatrixRotationZ(0);
	cy::Matrix4<float> rotationX = cyMatrix4f::MatrixRotationX(0);
	cyPoint3f translation = cyPoint3f(0, 0, 0);
	translationMatrix = cyMatrix4f::MatrixTrans(translation);

	totalRotationMatrix = rotationX * rotationZ;
	cameraTransformationMatrix = translationMatrix * totalRotationMatrix;

	lightRotationMatrix = cyMatrix4f::MatrixRotationX(0);
	teapotLightMVP = lightProj * lightView * cameraTransformationMatrix;
}

cyPoint3f getTextureVertexFor(cyPoint3f pt)
{

	return (pt / 20) + 0.5;
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	volume_shaders.Bind();
	glBindVertexArray(vertexArrayObj);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	glutSwapBuffers();
}


void zoom()
{
	volume_shaders.Bind();
	int data = translationMatrix.data[14];
	cyPoint3f translation = cyPoint3f(0.0f, 0.0f, 1.0f);
	if (data > -25 || !zoom_in)
	{
		zoom_in = false;
		translation = cyPoint3f(0.0f, 0.0f, -1.0f);
	}
	translationMatrix = cyMatrix4f::MatrixTrans(translation) * translationMatrix;
	cameraTransformationMatrix = translationMatrix * totalRotationMatrix;
	volume_shaders.SetUniform(1, cameraTransformationMatrix);
	volume_shaders.SetUniform(3, cameraTransformationMatrix.GetInverse().GetTranspose());
	glutPostRedisplay();
}

void rotate()
{
	volume_shaders.Bind();
	totalRotationMatrix = cyMatrix4f::MatrixRotationX(0.5) * totalRotationMatrix;
	cameraTransformationMatrix = translationMatrix * totalRotationMatrix;
	volume_shaders.SetUniform(1, cameraTransformationMatrix);
	glutPostRedisplay();
}


void onClick(int button, int state, int x, int y)
{
	if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			selected = GLUT_RIGHT_BUTTON;
			zoom();
		}
		else
		{
			selected = NULL;
		}
	}
	else if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			selected = GLUT_LEFT_BUTTON;
			rotate();
		}
		else
		{
			selected = NULL;
		}
	}
}

void move(int x, int y)
{
	if (selected == GLUT_RIGHT_BUTTON)
	{
		zoom();
	}
	else if (selected == GLUT_LEFT_BUTTON)
	{
		rotate();
	}
}


void reset(int key, int x, int y)
{
	if (key == GLUT_KEY_F6)
	{
		setInitialRotationAndTranslation();
		volume_shaders.Bind();
		volume_shaders.SetUniform(1, cameraTransformationMatrix);
		volume_shaders.SetUniform(3, cameraTransformationMatrix.GetInverse().GetTranspose());
		glutPostRedisplay();
	}
}

void populateVerticesAndNormals() {
	vertices = {};
	for (int i = 0; i < mesh.NF(); i = i + 1) {
		cy::TriMesh::TriFace face = mesh.F(i);
		vertices.push_back(mesh.V(face.v[0]));
		vertices.push_back(mesh.V(face.v[1]));
		vertices.push_back(mesh.V(face.v[2]));
		cy::TriMesh::TriFace nface = mesh.FN(i);
		normals.push_back(mesh.VN(nface.v[0]).GetNormalized());
		normals.push_back(mesh.VN(nface.v[1]).GetNormalized());
		normals.push_back(mesh.VN(nface.v[2]).GetNormalized());
	}
}


void createObj()
{
	
	mesh = cyTriMesh();
	mesh.LoadFromFileObj("cube.obj");
	mesh.ComputeBoundingBox();
	mesh.ComputeNormals();

	setInitialRotationAndTranslation();
	populateVerticesAndNormals();

	volume_shaders = cy::GLSLProgram();
	volume_shaders.BuildFiles("vertex_shader.glsl", "fragment_shader.glsl");
	volume_shaders.Bind();
	volume_shaders.RegisterUniform(1, "cameraTransformation");
	volume_shaders.SetUniform(1, cameraTransformationMatrix);
	volume_shaders.RegisterUniform(2, "perspective");
	volume_shaders.SetUniform(2, perspectiveMatrix);
	volume_shaders.RegisterUniform(3, "view");
	volume_shaders.SetUniform(3, view);

	GLuint vertexBufferObj[2];
	GLuint textureBufferObj[1];

	glGenVertexArrays(1, &vertexArrayObj);
	glBindVertexArray(vertexArrayObj);

	glGenBuffers(1, vertexBufferObj);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(cyPoint3f), NULL);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * normals.size(), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(cyPoint3f), NULL);

	for(int i = 0; i < vertices.size(); i++)
	{
		textureVertices.push_back(getTextureVertexFor(vertices.at(i)));
	}

	glGenBuffers(1, textureBufferObj);
	glBindBuffer(GL_ARRAY_BUFFER, textureBufferObj[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * textureVertices.size(), &textureVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, 0, sizeof(cyPoint3f), NULL);

	glBindVertexArray(0);
}


bool loadDatFileToTexture(char* name)
{
	FILE* fp = fopen(name, "rb");

	unsigned short vuSize[3];
	fread((void*)vuSize, 3, sizeof(unsigned short), fp);
	int uCount = int(vuSize[0]) * int(vuSize[1]) * int(vuSize[2]);
	unsigned short* chBuffer = new unsigned short[uCount];
	fread((void*)chBuffer, uCount, sizeof(unsigned short), fp);
	fclose(fp);


	char* rgbaBuffer = new char[uCount * 4];
	glGenTextures(1, (GLuint*)&texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	for (int i = 0; i < uCount; i++)
	{
		char val = chBuffer[i];
		rgbaBuffer[i * 4] = val;
		rgbaBuffer[i * 4 + 1] = val;
		rgbaBuffer[i * 4 + 2] = val;
		rgbaBuffer[i * 4 + 3] = val;
	}
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, SMALL_VOL_X, SMALL_VOL_Y, SMALL_VOL_Z, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)rgbaBuffer);

	delete[] chBuffer;
	delete[] rgbaBuffer;
	return true;
}


int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(width, width);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("Volume Render of a Present");
	cyGL::PrintVersion();
	GLenum res = glewInit();
	if (res != GLEW_OK)
	{
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}


	float fov = fov_degrees * (M_PI / 180.0f);
	perspectiveMatrix = cyMatrix4f::MatrixPerspective(fov, 1.0f, 0.1f, far_plane);

	loadDatFileToTexture("present246x246x221.dat");
	createObj();

	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.03f);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glutDisplayFunc(display);
	glutMouseFunc(onClick);
	glutMotionFunc(move);
	glutSpecialFunc(reset);


	glutMainLoop();

	return 0;
}
