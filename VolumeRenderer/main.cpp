#define _USE_MATH_DEFINES

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glui.h>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include <algorithm>
#include <limits>



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

// GLUI vars
GLUI *glui;
float rotation[16] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0 , 0.0, 0.0, 0.0, 1.0 };
int main_window;
float minVal = 0.1;
float val1 = 0.2;
cyPoint4f val1rgba = cyPoint4f(1.0, 0.5, 0.0, 0.2);
float val2 = 0.3;
cyPoint4f val2rgba = cyPoint4f(0.0, 1.0, 0.0, 0.2);
float val3 = 0.4;
cyPoint4f val3rgba = cyPoint4f(1.0, 0.0, 1.0, 0.2);
float val4 = 1;
cyPoint4f val4rgba = cyPoint4f(1.0, 1.0, 0.0, 0.2);

const int SMALL_VOL_X = 246;
const int SMALL_VOL_Y = 246;
const int SMALL_VOL_Z = 221;
const int LG_VOL_X = 492;
const int LG_VOL_Y = 492;
const int LG_VOL_Z = 442;

GLuint texture;

float transformedMaxX;
float transformedMinX;
float transformedMaxY;
float transformedMinY;
float transformedMaxZ;
float transformedMinZ;
cyPoint3f hitPointEnter;
cyPoint3f hitPointExit;
cyPoint3f viewDir;

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

void findMinMax(float &min, float &max, float min1, float min2, float max1, float max2)
{
	if (min1 != -FLT_MAX  && min2 != -FLT_MAX)
	{
		min = (min2 > min1) ? min2 : min1;
	}
	else if (min1 == -FLT_MAX)
	{
		min = min2;
	}
	else
	{
		min = min1;
	}
	if (max1 != FLT_MAX  && max2 != FLT_MAX)
	{
		max = (max2 < max1) ? max2 : max1;
	}
	else if (max1 == FLT_MAX)
	{
		max = max2;
	}
	else
	{
		max = max1;
	}
}

void getBoxIntersectionPts(float &t, float &t1, cyPoint3f tranformedCameraPos, cyPoint3f viewDir)
{
	float tMin = -FLT_MAX;
	float tMax = FLT_MAX;
	float tXMin;
	float tXMax;
	if (viewDir.x == 0)
	{
		tXMin = -FLT_MAX;
		tXMax = FLT_MAX;
	}
	else {
		tXMin = (transformedMinX - tranformedCameraPos.x) / viewDir.x;
		tXMax = (transformedMaxX - tranformedCameraPos.x) / viewDir.x;
	}
	if (tXMin > tXMax)
	{
		std::swap(tXMin, tXMax);
	}
	float tYMin;
	float tYMax;
	if (viewDir.y == 0)
	{
		tYMin = -FLT_MAX;
		tYMax = FLT_MAX;
	}
	else {
		tYMin = (transformedMinY - tranformedCameraPos.y) / viewDir.y;
		tYMax = (transformedMaxY - tranformedCameraPos.y) / viewDir.y;
	}
	if (tYMin > tYMax)
	{
		std::swap(tYMin, tYMax);
	}
	if ((tXMin > tYMax) || (tYMin > tXMax))
	{
		t = -1;
		t1 = -1;
	}
	findMinMax(tMin, tMax, tXMin, tYMin, tXMax, tYMax);
	float tZMin;
	float tZMax;
	if (viewDir.z == 0)
	{
		tZMin = -FLT_MAX;
		tZMax = FLT_MAX;
	}
	else {
		tZMin = (transformedMinZ - tranformedCameraPos.z) / viewDir.z;
		tZMax = (transformedMaxZ - tranformedCameraPos.z) / viewDir.z;
	}
	if (tZMin > tZMax)
	{
		std::swap(tZMin, tZMax);
	}
	if ((tMin > tZMax) || (tZMin > tMax))
	{
		t = -1;
		t1 = -1;
	}
	findMinMax(tMin, tMax, tZMin, tMin, tZMax, tMax);
	t = tMin;
	t1 = tMax;
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

void populateRayEndStart()
{
	cyMatrix4f transformationMatrix = perspectiveMatrix * view * cameraTransformationMatrix;
	cyPoint3f transformedMeshMin = transformToGlobal(mesh.GetBoundMin(), transformationMatrix);
	cyPoint3f transformedMeshMax = transformToGlobal(mesh.GetBoundMax(), transformationMatrix);
	transformedMaxX = transformedMeshMax.x > transformedMeshMin.x ? transformedMeshMax.x : transformedMeshMin.x;
	transformedMinX = transformedMeshMax.x > transformedMeshMin.x ? transformedMeshMin.x : transformedMeshMax.x;
	transformedMaxY = transformedMeshMax.y > transformedMeshMin.y ? transformedMeshMax.y : transformedMeshMin.y;
	transformedMinY = transformedMeshMax.y > transformedMeshMin.y ? transformedMeshMin.y : transformedMeshMax.y;
	transformedMaxZ = transformedMeshMax.z > transformedMeshMin.z ? transformedMeshMax.z : transformedMeshMin.z;
	transformedMinZ = transformedMeshMax.z > transformedMeshMin.z ? transformedMeshMin.z : transformedMeshMax.z;
	cyPoint3f transformedCameraPos = transformToGlobal(origin, transformationMatrix);
	cyPoint3f transformedOrigin = transformToGlobal(cyPoint3f(0, 0, 0), transformationMatrix);
	viewDir = transformedOrigin - transformedCameraPos;

	float tValEnter = 0.0;
	float tValExit = 0.0;
	getBoxIntersectionPts(tValEnter, tValExit, transformedCameraPos, viewDir);
	hitPointEnter = transformedCameraPos + (tValEnter * viewDir);
    hitPointExit = transformedCameraPos + (tValExit * viewDir);

}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	volume_shaders.Bind();
	volume_shaders.SetUniform(1, cameraTransformationMatrix);
	volume_shaders.SetUniform(7, minVal);
	cyPoint4f tfVals = cyPoint4f(val1, val2, val3, val4);
	volume_shaders.SetUniform(8, tfVals);
	cyMatrix4f rgbas = cyMatrix4f(val1rgba, val2rgba, val3rgba, val4rgba);
	volume_shaders.SetUniform(9, rgbas);
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
	populateRayEndStart();

	volume_shaders = cy::GLSLProgram();
	volume_shaders.BuildFiles("vertex_shader.glsl", "fragment_shader.glsl");
	volume_shaders.Bind();
	volume_shaders.RegisterUniform(1, "cameraTransformation");
	volume_shaders.SetUniform(1, cameraTransformationMatrix);
	volume_shaders.RegisterUniform(2, "perspective");
	volume_shaders.SetUniform(2, perspectiveMatrix);
	volume_shaders.RegisterUniform(3, "view");
	volume_shaders.SetUniform(3, view);
	volume_shaders.RegisterUniform(4, "rayStart");
	volume_shaders.SetUniform(4, hitPointEnter);
	volume_shaders.RegisterUniform(5, "rayEnd");
	volume_shaders.SetUniform(5, hitPointExit);
	volume_shaders.RegisterUniform(6, "viewDir");
	volume_shaders.SetUniform(6, viewDir);
	volume_shaders.RegisterUniform(7, "minimumValue");
	volume_shaders.SetUniform(7, minVal);
	cyPoint4f tfVals = cyPoint4f(val1, val2, val3, val4);
	volume_shaders.RegisterUniform(8, "tfVals");
	volume_shaders.SetUniform(7, tfVals);
	cyMatrix4f rgbas = cyMatrix4f(val1rgba, val2rgba, val3rgba, val4rgba);
	volume_shaders.RegisterUniform(9, "rgbaVals");
	volume_shaders.SetUniform(8, rgbas);

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

void myGlutIdle(void)
{
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);

	glutPostRedisplay();
}

void myGlutReshape(int x, int y)
{
	float xy_aspect;

	xy_aspect = (float)x / (float)y;
	GLUI_Master.auto_set_viewport();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-xy_aspect*.08, xy_aspect*.08, -.08, .08, .1, 15.0);

	glutPostRedisplay();
}

void onRotate(int param)
{
	totalRotationMatrix = cyMatrix4f(rotation) * totalRotationMatrix;
	cameraTransformationMatrix = translationMatrix * totalRotationMatrix;
}

void addFloatSpinner(char *title, float *value, GLUI_Panel *panel)
{
	glui->add_spinner_to_panel(panel, title, GLUI_SPINNER_FLOAT, value)
		->set_float_limits(0.0, 1.0);
}

void addTransferValuePanel(char *title, float &value, cyPoint4f &rgbaValue, GLUI_Panel *panel)
{
	GLUI_Panel *valuePanel = glui->add_rollout_to_panel(panel, title);
	addFloatSpinner("Value: ", &value, valuePanel);
	addFloatSpinner("R: ", &rgbaValue.x, valuePanel);
	addFloatSpinner("G: ", &rgbaValue.y, valuePanel);
	addFloatSpinner("B: ", &rgbaValue.z, valuePanel);
	addFloatSpinner("A: ", &rgbaValue.w, valuePanel);
}

void setUpGLUI()
{
	glui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_LEFT);
	GLUI_Rotation *rotator = glui->add_rotation("Rotation", rotation, 2, onRotate);
	rotator->set_spin(0);

	GLUI_Panel *transferPanel = glui->add_rollout("Transfer Function");

	addFloatSpinner("Minimum value: ", &minVal, transferPanel);
	addTransferValuePanel("Value 1: ", val1, val1rgba, transferPanel);
	addTransferValuePanel("Value 2: ", val2, val2rgba, transferPanel);
	addTransferValuePanel("Value 3: ", val3, val3rgba, transferPanel);
	addTransferValuePanel("Value 4: ", val4, val4rgba, transferPanel);


	GLUI_Master.set_glutReshapeFunc(myGlutReshape);
	GLUI_Master.set_glutIdleFunc(myGlutIdle);
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(width*2.2 + 150, width*2.2);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	main_window = glutCreateWindow("Volume Render of a Present");
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

	setUpGLUI();


	glutMainLoop();

	return 0;
}
