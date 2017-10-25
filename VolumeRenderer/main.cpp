#define _USE_MATH_DEFINES

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include <algorithm>
#include <limits>

#include "nanogui/common.h"

using namespace nanogui;

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

std::vector<cyPoint3f> intersectionPts;
cyPoint3f planeNormal;

float maxX;
float minX;
float maxY;
float minY;
float maxZ;
float minZ;

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

void addIntersectionPt(cyPoint3f point1, cyPoint3f point2, int& numIntersections, float d, cyPoint3f& center)
{
	cyPoint3f normalizedPlane = planeNormal.GetNormalized();
	float point = (d - normalizedPlane.Dot(point1)) / normalizedPlane.Dot(point2 - point1);
	if (point >= 0.0 && point <= 1.0)
	{
		cyPoint3f intersectionPoint = point1 + point * (point2 - point1);
		intersectionPts.push_back(intersectionPoint);
		center += intersectionPoint;
		numIntersections += 1;
	}
}

void findIntersection(cyPoint3f first, cyPoint3f second, cyPoint3f third, cyPoint3f fourth, float d, cyPoint3f& center)
{
	int numIntersections = 0;
	addIntersectionPt(first, second, numIntersections, d, center);
	if (numIntersections == 0)
	{
		addIntersectionPt(second, third, numIntersections, d, center);
		if (numIntersections == 0)
		{
			addIntersectionPt(third, fourth, numIntersections, d, center);
		}
	}
}

bool sortIntersectionPoints(cyPoint3f lhs, cyPoint3f rhs)
{
	// algorithm from http://www.asawicki.info/news_1428_finding_polygon_of_plane-aabb_intersection.html
	const cyPoint3f firstPt = intersectionPts[0];
	return ((lhs - firstPt).Cross(rhs - firstPt)).Dot(planeNormal) < 0;
}

void computeProxyGeo(cyPoint4f meshMax, cyPoint4f meshMin, float d, cyPoint3f& center)
{
	int numIntersections = 0;
	std::vector<cyPoint3f> corners = {
		cyPoint3f(meshMin.x, meshMax.y, meshMin.z), cyPoint3f(meshMin.x, meshMax.y, meshMax.z), meshMin.XYZ(),
		cyPoint3f(meshMax.x, meshMax.y, meshMin.z),meshMax.XYZ(), cyPoint3f(meshMin.x, meshMin.y, meshMax.z),
		cyPoint3f(meshMax.x, meshMin.y, meshMin.z), cyPoint3f(meshMax.x, meshMin.y, meshMax.z)
	};
	findIntersection(corners[0], corners[1], corners[4], corners[6], d, center);
	addIntersectionPt(corners[1], corners[5], numIntersections, d, center);
	findIntersection(corners[0], corners[2], corners[5], corners[7], d, center);
	addIntersectionPt(corners[2], corners[6], numIntersections, d, center);
	findIntersection(corners[0], corners[3], corners[6], corners[7], d, center);
	addIntersectionPt(corners[3], corners[4], numIntersections, d, center);
	if (intersectionPts.size() != 0)
	{
		center = center / intersectionPts.size();
		std::sort(intersectionPts.begin(), intersectionPts.end(), sortIntersectionPoints);
	}
}

void transformPtBackToObjectSpace(cyPoint3f& pt, cyMatrix4f transformationMatrix)
{
	cyPoint4f currentPt = cyPoint4f(pt, 1);
	currentPt = transformationMatrix.GetInverse() * currentPt;
	if (currentPt.x > 10 || currentPt.x < -10)
	{
		if (pt.x == 0)
		{
			currentPt.x = 0;
		}
		else
		{
			float newX = pt.x - 14;
			currentPt.x = newX;
		}
	}
	if (currentPt.y > 10 || currentPt.y < -10)
	{
		if (pt.y == 0)
		{
			currentPt.y = 0;
		}
		else
		{
			float newY = pt.y - 14;
			currentPt.y = newY;
		}
	}
	if (currentPt.z > 10 || currentPt.z < -10)
	{
		if (pt.z == 0)
		{
			currentPt.z = 0;
		}
		else
		{
			float newZ = pt.z - 49.815;
			currentPt.z = newZ;
		}
	}

	pt = cyPoint3f(currentPt);
}

cyPoint3f getTextureVertexFor(cyPoint3f pt)
{

	return (pt / 20) + 0.5;
}

cyPoint3f transformToGlobal(cyPoint3f pt, cyMatrix4f transformationMatrix)
{
	cyPoint4f transformedPt = transformationMatrix * cyPoint4f(pt, 1);
	if (transformedPt.w != 0)
	{
		transformedPt = transformedPt / transformedPt.w;
	}
	return cyPoint3f(transformedPt);
}

void sendIntersectionPts(cyPoint3f center, cyMatrix4f transformationMatrix)
{
	cyPoint3f centerTextureVertex = getTextureVertexFor(center);
	for (int i = 0; i < intersectionPts.size(); i++)
	{
		cyPoint3f secondPt;
		if (i == intersectionPts.size() - 1)
		{
			secondPt = intersectionPts[0];
		}
		else
		{
			secondPt = intersectionPts[i + 1];
		}
		cyPoint3f firstPt = intersectionPts[i];
		transformPtBackToObjectSpace(firstPt, transformationMatrix);
		transformPtBackToObjectSpace(secondPt, transformationMatrix);
		vertices.push_back(cyPoint3f(firstPt));
		vertices.push_back(cyPoint3f(secondPt));
		vertices.push_back(cyPoint3f(center));
		normals.push_back(planeNormal);
		normals.push_back(planeNormal);
		normals.push_back(planeNormal);
		textureVertices.push_back(getTextureVertexFor(firstPt));
		textureVertices.push_back(getTextureVertexFor(secondPt));
		textureVertices.push_back(centerTextureVertex);
	}
}

void working()
{
	cyMatrix4f transformationMatrix = perspectiveMatrix * view * cameraTransformationMatrix;
	cyPoint4f transformedMeshMin = transformationMatrix * cyPoint4f(mesh.GetBoundMin(), 1);
	cyPoint4f transformedMeshMax = transformationMatrix * cyPoint4f(mesh.GetBoundMax(), 1);
	if (transformedMeshMin.z > transformedMeshMax.z)
	{
		std::swap(transformedMeshMax, transformedMeshMin);
	}
	cyPoint4f transformedCameraPos = transformationMatrix * cyPoint4f(origin, 1);
	float volumeDist = transformedMeshMax.z - transformedMeshMin.z;
	float samplingRate = 25.0;
	float samplingDistance = volumeDist / samplingRate;
	float startZ = transformedCameraPos.z;
	while (startZ < transformedMeshMax.z)
	{
		startZ = startZ + samplingDistance;
		if (startZ > transformedMeshMin.z && startZ < transformedMeshMax.z)
		{
			intersectionPts = {};

			cyPoint3f hitPoint = transformedCameraPos.XYZ() + (startZ * -transformedCameraPos.XYZ().GetNormalized());
			planeNormal = -transformedCameraPos.XYZ();
			float d = planeNormal.x * hitPoint.x + planeNormal.y * hitPoint.y + planeNormal.z * hitPoint.z;
			float distance = d / sqrt(
				planeNormal.x * planeNormal.x + planeNormal.y * planeNormal.y + planeNormal.z * planeNormal.z);
			cyPoint3f center = cyPoint3f(0, 0, 0);
			computeProxyGeo(transformedMeshMax, transformedMeshMin, distance, center);
			transformPtBackToObjectSpace(center, transformationMatrix);
			sendIntersectionPts(center, transformationMatrix);
		}
	}
}

void addSlicesToVBO()
{
	working();
}

void display()
{
	vertices.clear();
	normals.clear();
	textureVertices.clear();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	addSlicesToVBO();
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
	volume_shaders.SetUniform(3, cameraTransformationMatrix.GetInverse().GetTranspose());
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


void createObj()
{
	
	mesh = cyTriMesh();
	mesh.LoadFromFileObj("cube.obj");
	mesh.ComputeBoundingBox();
	mesh.ComputeNormals();

	setInitialRotationAndTranslation();
	addSlicesToVBO();

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
