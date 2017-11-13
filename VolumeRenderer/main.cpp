#define _USE_MATH_DEFINES

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glui.h>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include <algorithm>
#include "Eigen/Dense"
using namespace Eigen;

#define width 660
#define far_plane 500.0f
#define inital_z -70.0f
#define fov_degrees 45.0f
#define flip_degree 135.1f
#define y_trans -7

cyTriMesh mesh;
cyTriMesh lightObj;
GLuint vertexArrayObj;
GLuint lightVertexArrayObj;
cy::GLSLProgram volume_shaders;
cy::GLSLProgram light_shaders;
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
std::vector<cyPoint3f> lightVertices;
std::vector<cyPoint3f> textureVertices;
std::vector<cyPoint3f> normals;
cyPoint3f lightPos = cyPoint3f(0, 1, 0);
cyPoint3f origin = cyPoint3f(0, 0, 3.5);
cyMatrix4f view = cyMatrix4f::MatrixView(origin, cyPoint3f(0, 0, 0), cyPoint3f(0, 1, 0));

// GLUI vars
GLUI* glui;
float volume_rotation[16] = {
	1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0 , 0.0, 0.0, 0.0, 1.0
};
float light_rotation[16] = {
	1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0 , 0.0, 0.0, 0.0, 1.0
};
float volume_rotation_x;
float volume_rotation_y;
float volume_rotation_z;
float light_rotation_x;
float light_rotation_y;
float light_rotation_z;
float light_translate_xy [2] = {0, 0};
float light_translate_x [1] = {0};
float light_translate_y [1] = {0};
float light_translate_z [1] = {0};
int main_window;
int numSamples = 30;
float minVal = 0.3985;
float val1 = 0.464156;
cyPoint4f val1rgba = cyPoint4f(1.0, 0.5, 0.0, 0.1);
float val2 = 0.510348;
cyPoint4f val2rgba = cyPoint4f(0.0, 1.0, 0.0, 0.1);
float val3 = 0.659515;
cyPoint4f val3rgba = cyPoint4f(1.0, 0.0, 1.0, 0.1);
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
	cy::Matrix4<float> rotationZ = cyMatrix4f::MatrixRotationZ(0);
	cy::Matrix4<float> rotationX = cyMatrix4f::MatrixRotationX(0);
	cyPoint3f translation = cyPoint3f(0, 0, 0);
	translationMatrix = cyMatrix4f::MatrixTrans(translation);

	totalRotationMatrix = rotationX * rotationZ;
	cameraTransformationMatrix = translationMatrix * totalRotationMatrix;

	lightRotationMatrix = cyMatrix4f::MatrixRotationX(0);
}


cyPoint3f getTextureVertexFor(cyPoint3f pt)
{
	return pt + 0.5;
}


void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	volume_shaders.Bind();
	volume_shaders.SetUniform(1, cameraTransformationMatrix);
	volume_shaders.SetUniform(5, numSamples);
	volume_shaders.SetUniform(6, minVal);
	cyPoint4f tfVals = cyPoint4f(val1, val2, val3, val4);
	volume_shaders.SetUniform(7, tfVals);
	cyMatrix4f rgbas = cyMatrix4f(val1rgba, val2rgba, val3rgba, val4rgba);
	volume_shaders.SetUniform(8, rgbas);
	glBindVertexArray(vertexArrayObj);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	light_shaders.Bind();
	light_shaders.SetUniform(1, lightCameraTransformationMatrix);
	glBindVertexArray(lightVertexArrayObj);
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

void populateVerticesAndNormals()
{
	vertices = {};
	for (int i = 0; i < mesh.NF(); i = i + 1)
	{
		cy::TriMesh::TriFace face = mesh.F(i);
		vertices.push_back(mesh.V(face.v[0]) / 20);
		vertices.push_back(mesh.V(face.v[1]) / 20);
		vertices.push_back(mesh.V(face.v[2]) / 20);
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
	volume_shaders.RegisterUniform(4, "cameraPos");
	volume_shaders.SetUniform(4, origin);
	volume_shaders.RegisterUniform(5, "numSamples");
	volume_shaders.SetUniform(5, numSamples);
	volume_shaders.RegisterUniform(6, "minimumValue");
	volume_shaders.SetUniform(6, minVal);
	cyPoint4f tfVals = cyPoint4f(val1, val2, val3, val4);
	volume_shaders.RegisterUniform(7, "tfVals");
	volume_shaders.SetUniform(7, tfVals);
	cyMatrix4f rgbas = cyMatrix4f(val1rgba, val2rgba, val3rgba, val4rgba);
	volume_shaders.RegisterUniform(8, "rgbaVals");
	volume_shaders.SetUniform(8, rgbas);

	GLuint vertexBufferObj[2];
	GLuint textureBufferObj[1];

	glGenVertexArrays(1, &vertexArrayObj);
	glBindVertexArray(vertexArrayObj);

	glGenBuffers(1, vertexBufferObj);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(cyPoint3f), nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * normals.size(), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(cyPoint3f), nullptr);

	for (int i = 0; i < vertices.size(); i++)
	{
		textureVertices.push_back(getTextureVertexFor(vertices.at(i)));
	}

	glGenBuffers(1, textureBufferObj);
	glBindBuffer(GL_ARRAY_BUFFER, textureBufferObj[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * textureVertices.size(), &textureVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, 0, sizeof(cyPoint3f), nullptr);

	glBindVertexArray(0);
}

void createLightObj()
{
	lightObj = cyTriMesh();
	lightObj.LoadFromFileObj("light.obj");
	lightObj.ComputeBoundingBox();
	lightObj.ComputeNormals();
	lightVertices = {};
	for (int i = 0; i < lightObj.NF(); i = i + 1)
	{
		cy::TriMesh::TriFace face = lightObj.F(i);
		lightVertices.push_back(lightObj.V(face.v[0]) / 7);
		lightVertices.push_back(lightObj.V(face.v[1]) / 7);
		lightVertices.push_back(lightObj.V(face.v[2]) / 7);
	}

	lightTransformationMatrix = cyMatrix4f::MatrixTrans(lightPos);
	lightCameraTransformationMatrix = lightTransformationMatrix * cyMatrix4f::MatrixRotationY(-10);

	light_shaders = cy::GLSLProgram();
	light_shaders.BuildFiles("light_vertex_shader.glsl", "light_fragment_shader.glsl");
	light_shaders.Bind();
	light_shaders.RegisterUniform(1, "cameraTransformation");
	light_shaders.SetUniform(1, lightCameraTransformationMatrix);
	light_shaders.RegisterUniform(2, "perspective");
	light_shaders.SetUniform(2, perspectiveMatrix);
	light_shaders.RegisterUniform(3, "view");
	light_shaders.SetUniform(3, view);

	GLuint lightVertexBufferObj[2];

	glGenVertexArrays(1, &lightVertexArrayObj);
	glBindVertexArray(lightVertexArrayObj);

	glGenBuffers(1, lightVertexBufferObj);

	glBindBuffer(GL_ARRAY_BUFFER, lightVertexBufferObj[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cyPoint3f) * lightVertices.size(), &lightVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(cyPoint3f), nullptr);

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
	glFrustum(-xy_aspect * .08, xy_aspect * .08, -.08, .08, .1, 15.0);

	glutPostRedisplay();
}

void onVolumeRotate(int param)
{
	if (param == 2)
	{
		totalRotationMatrix = cyMatrix4f(volume_rotation) * totalRotationMatrix;
	}
	else if (param == 3)
	{
		volume_rotation_x += 0.1;
		totalRotationMatrix = cyMatrix4f::MatrixRotationX(volume_rotation_x) * totalRotationMatrix;
	}
	else if (param == 4)
	{
		volume_rotation_y += 0.1;
		totalRotationMatrix = cyMatrix4f::MatrixRotationY(volume_rotation_y) * totalRotationMatrix;
	}
	else if (param == 5)
	{
		volume_rotation_z += 0.1;
		totalRotationMatrix = cyMatrix4f::MatrixRotationZ(volume_rotation_z) * totalRotationMatrix;
	}
	cameraTransformationMatrix = translationMatrix * totalRotationMatrix;
}

void onLightRotate(int param)
{
	if (param == 2)
	{
		lightRotationMatrix = cyMatrix4f(light_rotation) * lightRotationMatrix;
	}
	else if (param == 3)
	{
		light_rotation_x += 0.1;
		lightRotationMatrix = cyMatrix4f::MatrixRotationX(light_rotation_x) * lightRotationMatrix;
	}
	else if (param == 4)
	{
		light_rotation_y += 0.1;
		lightRotationMatrix = cyMatrix4f::MatrixRotationY(light_rotation_y) * lightRotationMatrix;
	}
	else if (param == 5)
	{
		light_rotation_z += 0.1;
		lightRotationMatrix = cyMatrix4f::MatrixRotationZ(light_rotation_z) * lightRotationMatrix;
	}
	lightCameraTransformationMatrix = lightTransformationMatrix * lightRotationMatrix;
}

void onLightTranslate(int param)
{
	cyPoint3f translation;
	if(param == 0)
	{
		translation = cyPoint3f(light_translate_xy[0], light_translate_xy[1], 0);
	} else if (param == 1)
	{
		translation = cyPoint3f(light_translate_x[0], 0, 0);
	} else if (param == 2)
	{
		translation = cyPoint3f(0, light_translate_y[0], 0);
	} else if (param == 3)
	{
		translation = cyPoint3f(0, 0, light_translate_z[0]);
	}
	lightTransformationMatrix = cyMatrix4f::MatrixTrans(translation) * lightTransformationMatrix;
	lightCameraTransformationMatrix = lightTransformationMatrix * lightRotationMatrix;
}

void addFloatSpinner(char* title, float* value, GLUI_Panel* panel)
{
	glui->add_spinner_to_panel(panel, title, GLUI_SPINNER_FLOAT, value)
	    ->set_float_limits(0.0, 1.0);
}

void addTransferValuePanel(char* title, float& value, cyPoint4f& rgbaValue, GLUI_Panel* panel)
{
	GLUI_Panel* valuePanel = glui->add_rollout_to_panel(panel, title);
	addFloatSpinner("Value: ", &value, valuePanel);
	addFloatSpinner("R: ", &rgbaValue.x, valuePanel);
	addFloatSpinner("G: ", &rgbaValue.y, valuePanel);
	addFloatSpinner("B: ", &rgbaValue.z, valuePanel);
	addFloatSpinner("A: ", &rgbaValue.w, valuePanel);
}

void addLightPanel()
{
	GLUI_Panel* lightPanel = glui->add_rollout("Light Values", false);
	GLUI_Panel* lightRotationPanel = glui->add_rollout_to_panel(lightPanel, "Light Rotation");
	glui->add_rotation_to_panel(lightRotationPanel, "Rotation", light_rotation, 2, onLightRotate)->set_spin(0);
	glui->add_button_to_panel(lightRotationPanel, "Rotate X", 3, onLightRotate);
	glui->add_button_to_panel(lightRotationPanel, "Rotate Y", 4, onLightRotate);
	glui->add_button_to_panel(lightRotationPanel, "Rotate Z", 5, onLightRotate);

	GLUI_Panel* lightTransformPanel = glui->add_rollout_to_panel(lightPanel, "Light Movement");
	glui->add_translation_to_panel(lightTransformPanel, "Translate XY", GLUI_TRANSLATION_XY,
		light_translate_xy, 0, onLightTranslate)->set_speed(0.0002);
	glui->add_translation_to_panel(lightTransformPanel, "Translate X", GLUI_TRANSLATION_X,
		light_translate_x, 1, onLightTranslate)->set_speed(0.0002);
	glui->add_translation_to_panel(lightTransformPanel, "Translate Y", GLUI_TRANSLATION_Y,
		light_translate_y, 2, onLightTranslate)->set_speed(0.0002);
	glui->add_translation_to_panel(lightTransformPanel, "Translate Z", GLUI_TRANSLATION_Z,
		light_translate_z, 3, onLightTranslate)->set_speed(0.0002);
}

void setUpGLUI()
{
	glui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_LEFT);

	GLUI_Panel* volumeRotationPanel = glui->add_rollout("Volume Rotation");
	glui->add_rotation_to_panel(volumeRotationPanel, "Rotation", volume_rotation, 2, onVolumeRotate)->set_spin(0);
	glui->add_button_to_panel(volumeRotationPanel, "Rotate X", 3, onVolumeRotate);
	glui->add_button_to_panel(volumeRotationPanel, "Rotate Y", 4, onVolumeRotate);
	glui->add_button_to_panel(volumeRotationPanel, "Rotate Z", 5, onVolumeRotate);

	glui->add_spinner("Number of Samples", GLUI_SPINNER_INT, &numSamples)
	    ->set_float_limits(0.0, 1000.0);

	GLUI_Panel* transferPanel = glui->add_rollout("Transfer Function", false);

	addFloatSpinner("Minimum value: ", &minVal, transferPanel);
	addTransferValuePanel("Value 1: ", val1, val1rgba, transferPanel);
	addTransferValuePanel("Value 2: ", val2, val2rgba, transferPanel);
	addTransferValuePanel("Value 3: ", val3, val3rgba, transferPanel);
	addTransferValuePanel("Value 4: ", val4, val4rgba, transferPanel);

	addLightPanel();

	GLUI_Master.set_glutReshapeFunc(myGlutReshape);
	GLUI_Master.set_glutIdleFunc(myGlutIdle);
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(width + 150, width);
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
	createLightObj();

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
