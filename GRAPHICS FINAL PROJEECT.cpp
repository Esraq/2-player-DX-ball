#include<iostream>
#include <stdlib.h>
#include <GL\glut.h>
#include <assert.h>
#include <fstream>
using namespace std;

//Represents an image
class Image {
public:
	Image(char* ps, int w, int h);
	~Image();

	/* An array of the form (R1, G1, B1, R2, G2, B2, ...) indicating the
	* color of each pixel in image.  Color components range from 0 to 255.
	* The array starts the bottom-left pixel, then moves right to the end
	* of the row, then moves up to the next column, and so on.  This is the
	* format in which OpenGL likes images.
	*/
	char* pixels;
	int width;
	int height;
};
//Reads a bitmap image from file.
Image* loadBMP(const char* filename);


Image::Image(char* ps, int w, int h) : pixels(ps), width(w), height(h) {

}

Image::~Image() {
	delete[] pixels;
}

namespace {
	//Converts a four-character array to an integer, using little-endian form
	int toInt(const char* bytes) {
		return (int)(((unsigned char)bytes[3] << 24) |
			((unsigned char)bytes[2] << 16) |
			((unsigned char)bytes[1] << 8) |
			(unsigned char)bytes[0]);
	}

	//Converts a two-character array to a short, using little-endian form
	short toShort(const char* bytes) {
		return (short)(((unsigned char)bytes[1] << 8) |
			(unsigned char)bytes[0]);
	}

	//Reads the next four bytes as an integer, using little-endian form
	int readInt(ifstream &input) {
		char buffer[4];
		input.read(buffer, 4);
		return toInt(buffer);
	}

	//Reads the next two bytes as a short, using little-endian form
	short readShort(ifstream &input) {
		char buffer[2];
		input.read(buffer, 2);
		return toShort(buffer);
	}

	//Just like auto_ptr, but for arrays
	template<class T>
	class auto_array {
	private:
		T* array;
		mutable bool isReleased;
	public:
		explicit auto_array(T* array_ = NULL) :
			array(array_), isReleased(false) {
		}

		auto_array(const auto_array<T> &aarray) {
			array = aarray.array;
			isReleased = aarray.isReleased;
			aarray.isReleased = true;
		}

		~auto_array() {
			if (!isReleased && array != NULL) {
				delete[] array;
			}
		}

		T* get() const {
			return array;
		}

		T &operator*() const {
			return *array;
		}

		void operator=(const auto_array<T> &aarray) {
			if (!isReleased && array != NULL) {
				delete[] array;
			}
			array = aarray.array;
			isReleased = aarray.isReleased;
			aarray.isReleased = true;
		}

		T* operator->() const {
			return array;
		}

		T* release() {
			isReleased = true;
			return array;
		}

		void reset(T* array_ = NULL) {
			if (!isReleased && array != NULL) {
				delete[] array;
			}
			array = array_;
		}

		T* operator+(int i) {
			return array + i;
		}

		T &operator[](int i) {
			return array[i];
		}
	};
}

Image* loadBMP(const char* filename) {
	ifstream input;
	input.open(filename, ifstream::binary);
	assert(!input.fail() || !"Could not find file");
	char buffer[2];
	input.read(buffer, 2);
	assert(buffer[0] == 'B' && buffer[1] == 'M' || !"Not a bitmap file");
	input.ignore(8);
	int dataOffset = readInt(input);

	//Read the header
	int headerSize = readInt(input);
	int width;
	int height;
	switch (headerSize) {
	case 40:
		//V3
		width = readInt(input);
		height = readInt(input);
		input.ignore(2);
		assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
		assert(readShort(input) == 0 || !"Image is compressed");
		break;
	case 12:
		//OS/2 V1
		width = readShort(input);
		height = readShort(input);
		input.ignore(2);
		assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
		break;
	case 64:
		//OS/2 V2
		assert(!"Can't load OS/2 V2 bitmaps");
		break;
	case 108:
		//Windows V4
		assert(!"Can't load Windows V4 bitmaps");
		break;
	case 124:
		//Windows V5
		assert(!"Can't load Windows V5 bitmaps");
		break;
	default:
		assert(!"Unknown bitmap format");
	}

	//Read the data
	int bytesPerRow = ((width * 3 + 3) / 4) * 4 - (width * 3 % 4);
	int size = bytesPerRow * height;
	auto_array<char> pixels(new char[size]);
	input.seekg(dataOffset, ios_base::beg);
	input.read(pixels.get(), size);

	//Get the data into the right format
	auto_array<char> pixels2(new char[width * height * 3]);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			for (int c = 0; c < 3; c++) {
				pixels2[3 * (width * y + x) + c] =
					pixels[bytesPerRow * y + 3 * x + (2 - c)];
			}
		}
	}

	input.close();
	return new Image(pixels2.release(), width, height);
}

GLuint loadTexture(Image* image) {

	GLuint textureId;

	glGenTextures(1, &textureId); //Make room for our texture

	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit

											 //Map the image to the texture

	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D

		0,                            //0 for now

		GL_RGB,                       //Format OpenGL uses for image

		image->width, image->height,  //Width and height

		0,                            //The border of the image

		GL_RGB, //GL_RGB, because pixels are stored in RGB format

		GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored

						  //as unsigned numbers

		image->pixels);               //The actual pixel data

	return textureId; //Returns the id of the texture

}

int level = 0;
int score1 = 0;
int score2 = 0;
float _angle = 0.0;
float _cameraAngle = 0.0;
float _ang_tri = 0.0;
float xbot, xtop = 0;
float ballx, bally = 0;
float xspeed = 0;
float yspeed = 0;
int st = 0;
float storex = 0;
int pause = 0;
int stage = 1;
int kupdown = 0;
int mupdown = 0;
GLuint _stage1;
GLuint _plank1;
GLuint _ball;
GLuint _barrier;

GLuint _stage2;
GLuint _plank2;
GLuint _ball2;
GLuint _barrier2;

GLuint _plank3;
GLuint _fan;

void init(void)
{
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);

	Image* plank3 = loadBMP("plank3.bmp");
	_plank3 = loadTexture(plank3);
	delete plank3;
	Image* fan = loadBMP("fan.bmp");
	_fan = loadTexture(fan);
	delete fan;

	Image* stage1 = loadBMP("stage1.bmp");
	Image* plank1 = loadBMP("plank1.bmp");
	Image* ball = loadBMP("ball.bmp");
	Image* barrier = loadBMP("barrier.bmp");
	_stage1 = loadTexture(stage1);
	_plank1 = loadTexture(plank1);
	_ball = loadTexture(ball);
	_barrier = loadTexture(barrier);
	delete stage1;
	delete plank1;
	delete ball;
	delete barrier;

	Image* stage2 = loadBMP("stage2.bmp");
	Image* plank2 = loadBMP("plank2.bmp");
	Image* ball2 = loadBMP("ball2.bmp");
	Image* barrier2 = loadBMP("barrier2.bmp");
	_stage2 = loadTexture(stage2);
	_plank2 = loadTexture(plank2);
	_ball2 = loadTexture(ball2);
	_barrier2 = loadTexture(barrier2);
	delete stage2;
	delete plank2;
	delete ball2;
	delete barrier2;

	GLfloat light_position[] = { 0, 0, 3, .0 };
	GLfloat red_light_position[] = { ballx, bally, 1, .0 };
	GLfloat blue_light_position[] = { xtop, 5.3,0., 0.0 };
	GLfloat white_light[] = { 1, 1, 1, .0 };
	GLfloat red_light[] = { 1.0, 0.0,0.0, 1.0 };
	GLfloat blue_light[] = { 0.0, 0.0,1.0, 1.0 };
	GLfloat lmodel_ambient[] = { 0.2, 0.2, 0.2, 0.2 };
	glClearColor(0, 0, 0, 0);
	glShadeModel(GL_SMOOTH);

	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);

	glLightfv(GL_LIGHT1, GL_POSITION, red_light_position);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, red_light);
	glLightfv(GL_LIGHT1, GL_SPECULAR, red_light);

	glLightfv(GL_LIGHT2, GL_POSITION, blue_light_position);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blue_light);
	glLightfv(GL_LIGHT2, GL_SPECULAR, blue_light);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//glEnable(GL_LIGHT1);
	//glEnable(GL_LIGHT2);
	glEnable(GL_DEPTH_TEST);

}

void BatBall() {

	GLfloat no_mat[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat mat_ambient[] = { 0.7, 0.7, 0.7, 1.0 };
	GLfloat mat_ambient_color[] = { 0.8, 0.8, 0.2, 1.0 };
	GLfloat mat_diffuse[] = { 0.1, 0.5, 0.8, 1.0 };
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat no_shininess[] = { 0.0 };
	GLfloat low_shininess[] = { 5.0 };
	GLfloat high_shininess[] = { 100.0 };
	GLfloat mat_emission[] = { 0.3, 0.2, 0.2, 0.0 };

	glEnable(GL_TEXTURE_2D);
	if (stage == 1)
		glBindTexture(GL_TEXTURE_2D, _stage1);
	else glBindTexture(GL_TEXTURE_2D, _stage2);

	glPushMatrix();       /////////STAGE background
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, mat_ambient);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(1, 1); glVertex3f(10, 10, -2);
	glTexCoord2f(1, 0); glVertex3f(10, -10, -2);
	glTexCoord2f(0, 0); glVertex3f(-10, -10, -2);
	glTexCoord2f(0, 1); glVertex3f(-10, 10, -2);
	glEnd();
	glPopMatrix();

	glPushMatrix(); // BOTTOM PLAYER
	glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
	glEnable(GL_TEXTURE_GEN_T);
	if (stage == 1)
		glBindTexture(GL_TEXTURE_2D, _plank1);
	else glBindTexture(GL_TEXTURE_2D, _plank2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTranslatef(0, -8.4, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glTranslatef(xbot, 0, 0.0);
	if (kupdown > 0)
		glRotatef(20, 0, 0, 1);
	else if (kupdown < 0)
		glRotatef(-20, 0, 0, 1);
	else glRotatef(0, 0, 0, 1);
	glScalef(3, 1, 1);
	glutSolidCube(1);
	glPopMatrix();


	glPushMatrix(); // TOP PLAYER
	glTranslatef(0, 8.4, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glTranslatef(xtop, 0, 0.0);
	if (mupdown < 0)
		glRotatef(20, 0, 0, 1);
	else if (mupdown>0)
		glRotatef(-20, 0, 0, 1);
	else glRotatef(0, 0, 0, 1);
	glScalef(3, 1, 1);
	glutSolidCube(1);
	glPopMatrix();

	glColor3f(0.5, 0.5, 0.5);

	if (stage == 1)
	{
		glPushMatrix();// middle berricade
		glTranslatef(0, 0, -1);
		glBindTexture(GL_TEXTURE_2D, _barrier);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(_ang_tri, 1, 0, 0);
		glScalef(11, .3, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-9, 0, -1);//LEFT BARRIER
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-_ang_tri, 1, .0, 0);
		glScalef(2, .3, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();// RIGHT Barrier
		glTranslatef(9, 0, -1);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-_ang_tri, 1, .0, 0);
		glScalef(2, .3, 1);
		glutSolidCube(1);
		glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
		if (stage == 1)
			glColor3f(1, 0, 0);
		else glColor3f(0, 0, 1);

		glPushMatrix();
		glTranslatef(6.8, 0, 0);// RIGHT FAN
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-10, 1, 0, 0);
		glRotatef(_angle, 0, .0, 1);
		glScalef(2, .3, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(6.8, 0, 0);// RIGHT FAN2
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-10, 1, 0, 0);
		glRotatef(90, 0, 0, 1);
		glRotatef(_angle, 0, .0, 1);
		glScalef(2, .3, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // LEFT FAN
		glTranslatef(-6.8, 0, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-10, 1, 0, 0);
		glRotatef(-_angle, 0, .0, 1);
		glScalef(2, .3, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // LEFT FAN2
		glTranslatef(-6.8, 0, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-10, 1, 0, 0);
		glRotatef(90, 0, 0, 1);
		glRotatef(-_angle, 0, .0, 1);
		glScalef(2, .3, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // Left barricade
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
		glEnable(GL_TEXTURE_GEN_T);
		glBindTexture(GL_TEXTURE_2D, _plank3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTranslatef(-9.95, 0, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glScalef(.2, 20, .2);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // RIGHT barricade
		glBindTexture(GL_TEXTURE_2D, _plank3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTranslatef(9.95, 0, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glScalef(.2, 20, .2);
		glutSolidCube(1);
		glPopMatrix();
	}
	if (stage == 2) {

		glPushMatrix(); // MIDDLE FAN
		glDisable(GL_TEXTURE_GEN_S); //disable texture coordinate generation
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _fan);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glPushMatrix();////////////////////right fan
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-10, 1, 0, 0);
		glRotatef(_angle, 0, .0, 1);
		glScalef(4, .5, 1);
		glColor3f(0, 0, 1);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // MIDDLE FAN2
		glDisable(GL_TEXTURE_GEN_S); //disable texture coordinate generation
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _fan);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glPushMatrix();////////////////////right fan
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glRotatef(-10, 1, 0, 0);
		glRotatef(90, 0, 0, 1);
		glRotatef(_angle, 0, .0, 1);
		glScalef(4, .5, 1);
		glColor3f(0, 0, 1);
		glutSolidCube(1);
		glPopMatrix();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
		glEnable(GL_TEXTURE_GEN_T);
		glPushMatrix(); // LEFT bottom barricade stage2
		glBindTexture(GL_TEXTURE_2D, _barrier2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTranslatef(-9.95, -8, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glScalef(.2, 7, .2);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // RIGHT bottom barricade stage 2
		glBindTexture(GL_TEXTURE_2D, _barrier2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTranslatef(9.95, -8, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glScalef(.2, 7, .2);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // LEFT TOP barricade stage2
		glBindTexture(GL_TEXTURE_2D, _barrier2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTranslatef(-9.95, 8, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glScalef(.2, 7, .2);
		glutSolidCube(1);
		glPopMatrix();

		glPushMatrix(); // RIGHT TOP barricade stage 2
		glBindTexture(GL_TEXTURE_2D, _barrier2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTranslatef(9.95, 8, 0);
		glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
		glScalef(.2, 7, .2);
		glutSolidCube(1);
		glPopMatrix();

	}
	glPushMatrix();//////////////////////////sphereeeeeeeeeeeeeeee
	if (stage == 1)
		glBindTexture(GL_TEXTURE_2D, _ball);
	else glBindTexture(GL_TEXTURE_2D, _ball2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTranslatef(ballx, bally, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glColor3f(1, 1, 1);
	glutSolidSphere(.5, 30, 30);
	glPopMatrix();

	glDisable(GL_TEXTURE_GEN_S); //disable texture coordinate generation
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_2D);
}

void handleKeypress(unsigned char key, //The key that was pressed                                                                                                           
	int x, int y) {    //The current mouse coordinates                                                                                  
	switch (key) {
	case 27: //Escape key                                                                                                                                       
		exit(0); //Exit the program                                                                                                                               
	case 'p':
		if (pause == 0)
			pause = 1;
		else pause = 0;
		break;
	}
}
void myMouse(int button, int state, int x, int y) {      // mouse click callback
	if (state == GLUT_DOWN) {


		if (button == GLUT_LEFT_BUTTON) {
			if (mupdown >= 0)
				mupdown = mupdown - 1;

		}

		else if (button == GLUT_RIGHT_BUTTON) {

			if (mupdown <= 0)
				mupdown = mupdown + 1;

		}
	}
}

void keyboard(int key, int x, int y)
{
	switch (key) {

	case GLUT_KEY_PAGE_UP:
		if (xspeed > 0) {
			xspeed = xspeed + .01;
		}
		else if (xspeed <0) {
			xspeed = xspeed - .01;
		}
		if (yspeed > 0)
			yspeed = yspeed + .01;
		else yspeed = yspeed - .01;
		break;

	case GLUT_KEY_PAGE_DOWN:
		if (xspeed > 0) {
			xspeed = xspeed - .01;
		}
		else if (xspeed < 0) {
			xspeed = xspeed + .01;
		}
		if (yspeed > 0)
			yspeed = yspeed - .01;
		else yspeed = yspeed + .01;
		break;

	case GLUT_KEY_RIGHT:
		xbot = xbot + .6;
		if (xbot > 8.6)
			xbot = 8.6;
		glutPostRedisplay();

		break;

	case GLUT_KEY_LEFT:
		xbot = xbot - .6;
		if (xbot < -8.6)
			xbot = -8.6;
		glutPostRedisplay();

		break;
	case GLUT_KEY_UP:
		if (kupdown <= 0)
			kupdown = kupdown + 1;

		break;
	case GLUT_KEY_DOWN:
		if (kupdown >= 0)
			kupdown = kupdown - 1;

		break;
	case GLUT_KEY_F1:
		if (stage == 1)
			stage = 2;
		else stage = 1;

		break;

	default:
		break;


	}

}


void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	BatBall();
	glFlush();
	glutSwapBuffers();
}


void start() {

	if (st == 0) {
		_ang_tri = 31;
		xspeed = 0;
		if (rand() % 2 == 0)
			yspeed = .15;
		else yspeed = -.15;
	}
	st = 1;
}

float mouseChange = .1;
int  tempY = 0;
void myMouseMove(int x, int y)
{


	if (tempY > x && tempY >= 0 && tempY <= 899)
	{
		if (xtop < -8.6)
			xtop = -8.6;
		else
			xtop = xtop - mouseChange;

	}

	else if (tempY < x && tempY >= 0 && tempY <= 899)
	{

		if (xtop >8.6)
			xtop = 8.6;
		else
			xtop = xtop + mouseChange;
	}
	tempY = x;
	glutPostRedisplay();
}

void update(int value) {
	if (pause == 0) {
		_angle += 20.0f;
		if (_angle > 360) {
			_angle -= 360;
		}
		_ang_tri += 5.0f;
		if (_ang_tri > 360) {
			_ang_tri -= 360;
		}
	}
	start();

	if (stage == 1 && ballx <= 6.8 + 1.4 && ballx >= 6.8 - 1.4 && bally < .1 && bally > -.1) {// Right FAN EFFECT
		int x = 0;
		if (xspeed == 0) {
			if (rand() % 2 == 0)
				xspeed = storex;
			else xspeed = -storex;

		}
		else if (rand() % 2 == 0) {
			xspeed = -xspeed;
			x = 1;
		}
		else if (yspeed < 0 && xspeed < 0) {
			xspeed = xspeed - .01;
			yspeed = yspeed - .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		else if (yspeed > 0 && xspeed < 0) {
			xspeed = xspeed - .01;
			yspeed = yspeed + .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed<< "\n";
		}
		else if (yspeed > 0 && xspeed > 0) {
			xspeed = xspeed + .01;
			yspeed = yspeed + .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		else if (yspeed < 0 && xspeed > 0) {
			xspeed = xspeed + .01;
			yspeed = yspeed - .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		if (x == 0 && (ballx <= 6.8 + .5 && ballx >= 6.8 - .5))
			xspeed = -xspeed;
	}

	if (stage == 1 && ballx <= -6.8 + 1.4 && ballx >= -6.8 - 1.4 && bally < .1 && bally > -.1) {// LEFT FAN EFFECT
		int x = 0;
		if (xspeed == 0) {
			if (rand() % 2 == 0)
				xspeed = storex;
			else xspeed = -storex;
		}
		else if (rand() % 2 == 0) {
			xspeed = -xspeed;
			x = 1;
		}
		else if (yspeed < 0 && xspeed < 0) {
			xspeed = xspeed - .01;
			yspeed = yspeed - .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		else if (yspeed > 0 && xspeed < 0) {
			xspeed = xspeed - .01;
			yspeed = yspeed + .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		else if (yspeed > 0 && xspeed > 0) {
			xspeed = xspeed + .01;
			yspeed = yspeed + .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		else if (yspeed < 0 && xspeed > 0) {
			xspeed = xspeed + .01;
			yspeed = yspeed - .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		if (x == 0 && (ballx <= -6.8 + .5 && ballx >= -6.8 - .5))
			xspeed = -xspeed;
	}

	if (stage == 2 && ballx <= 2.5 && ballx >= -2.5 && bally < .1 && bally > -.1) {// MIDDLE FAN EFFECT
		int x = 0;
		if (xspeed == 0) {
			if (storex == 0) {
				if (rand() % 2 == 0)
					xspeed = -.12;
				else xspeed = .12;
			}
			else if (rand() % 2 == 0)
				xspeed = storex;
			else xspeed = -storex;

		}
		else if (rand() % 2 == 0) {
			xspeed = -xspeed;
			x = 1;
		}
		else if (yspeed < 0 && xspeed < 0) {
			xspeed = xspeed - .01;
			yspeed = yspeed - .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		else if (yspeed > 0 && xspeed < 0) {
			xspeed = xspeed - .01;
			yspeed = yspeed + .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";

		}
		else if (yspeed > 0 && xspeed > 0) {
			xspeed = xspeed + .01;
			yspeed = yspeed + .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";

		}
		else if (yspeed < 0 && xspeed > 0) {
			xspeed = xspeed + .01;
			yspeed = yspeed - .01;
			level = level + 1;
			//cout << xspeed << "  " << yspeed << "\n";
		}
		if (x == 0 && (ballx <= 1 && ballx >= -1))
			xspeed = -xspeed;
	}

	// BARRIER EFFECT
	if (stage == 1 && ballx <= 14 && ballx >= -14 && bally < .1 && bally > -.1 && ((_ang_tri >= 335 && _ang_tri <= 360) || (_ang_tri <= 25 && _ang_tri >= 0) || (_ang_tri <= -335 && _ang_tri >= -360) || (_ang_tri >= -25 && _ang_tri <= 0) || (_ang_tri >= 155 && _ang_tri <= 205) || (_ang_tri <= -155 && _ang_tri >= -205)))
	{
		yspeed = -yspeed;
		if (xspeed == 0) {
			if (storex == 0) {
				if (rand() % 2 == 0)
					xspeed = .12;
				else xspeed = -.12;
			}
			else if (rand() % 2 == 0)
				xspeed = storex;
			else xspeed = -storex;
		}
		else if (rand() % 2 == 0)
			xspeed = -xspeed;
	}

	if (ballx <= xbot + 2 && ballx >= xbot - 2 && bally < -7.6 && bally > -7.8) {//Bottom player effect
		int x = rand() % 3 + 1;
		if (kupdown < 0) {
			if (xspeed > 0) {
			}
			else if (xspeed < 0) {
				xspeed = -xspeed;
			}
			else if (xspeed == 0 && storex == 0) {
				xspeed = .12;
			}
			else if (xspeed == 0 && storex>0) {
				xspeed = storex;
			}
			else if (xspeed == 0 && storex < 0) {
				xspeed = -storex;
			}
		}
		else if (kupdown == 0) {
			if (xspeed == 0) {

				if (x == 1) {
					if (storex == 0)
						xspeed = -.12;
					else if (storex > 0)
						xspeed = -storex;
					else xspeed = storex;
				}
				else if (x == 2) {
					if (storex == 0)
						xspeed = .12;
					else if (storex > 0)
						xspeed = storex;
					else xspeed = -storex;
				}
			}
			else if (x == 1)
				xspeed = -xspeed;
			else if (x == 2) {
				storex = xspeed;
				xspeed = 0;
			}
		}
		else if (kupdown > 0) {
			if (xspeed < 0) {
			}
			else if (xspeed > 0) {
				xspeed = -xspeed;
			}
			else if (xspeed == 0 && storex == 0) {
				xspeed = -.12;
			}
			else if (xspeed == 0 && storex>0) {
				xspeed = -storex;
			}
			else if (xspeed == 0 && storex < 0) {
				xspeed = storex;
			}
		}
		yspeed = -yspeed;
		// cout << xspeed << "  " << yspeed << "\n";
	}
	if (ballx <= xtop + 2 && ballx >= xtop - 2 && bally > 7.6 && bally < 7.8) {//top player effect

		int x = rand() % 3 + 1;
		if (mupdown < 0) {
			if (xspeed > 0) {
			}
			else if (xspeed < 0) {
				xspeed = -xspeed;
			}
			else if (xspeed == 0 && storex == 0) {
				xspeed = .12;
			}
			else if (xspeed == 0 && storex>0) {
				xspeed = storex;
			}
			else if (xspeed == 0 && storex < 0) {
				xspeed = -storex;
			}
		}
		else if (mupdown == 0) {
			if (xspeed == 0) {

				if (x == 1) {
					if (storex == 0)
						xspeed = -.12;
					else if (storex > 0)
						xspeed = -storex;
					else xspeed = storex;
				}
				else if (x == 2) {
					if (storex == 0)
						xspeed = .12;
					else if (storex > 0)
						xspeed = storex;
					else xspeed = -storex;
				}
			}
			else if (x == 1)
				xspeed = -xspeed;
			else if (x == 2) {
				storex = xspeed;
				xspeed = 0;
			}
		}
		else if (mupdown > 0) {
			if (xspeed < 0) {
			}
			else if (xspeed > 0) {
				xspeed = -xspeed;
			}
			else if (xspeed == 0 && storex == 0) {
				xspeed = -.12;
			}
			else if (xspeed == 0 && storex>0) {
				xspeed = -storex;
			}
			else if (xspeed == 0 && storex < 0) {
				xspeed = storex;
			}
		}
		yspeed = -yspeed;
		//cout << xspeed << "  " << yspeed << "\n";
	}
	if (bally > 8.3 || bally < -8.3)////////////reset
	{
		if (bally > 8.3)
			score1 = score1 + 1;
		else score2 = score2 + 1;
		ballx = 0;
		if (stage == 2)
			bally = 0;
		else bally = .1;
		st = 0;
		xtop = 0;
		xbot = 0;
		cout << "Player ONE :" << score1 << " -- Player TWO : " << score2 << "\n";
		cout << "At speed" << xspeed << "  " << yspeed << "\n";
		cout << "At Level " << level << "\n";
		cout << "In Stage " << stage << "\n" << "\n";
		level = 0;
		storex = 0;
		pause = 1;
		if (stage == 1)
			stage = 2;
		else stage = 1;
	}
	if (pause == 0) {
		bally = bally + yspeed;
		ballx = ballx + xspeed;
	}
	if (stage == 2) {
		if (ballx < -9.8 && bally >-4.5 && bally < 4.5) ///////////////LEFT Wall effect
		{
			ballx = -ballx;
			ballx = ballx - .1;

		}
		else if (ballx > 9.8 && bally > -4.5 && bally < 4.5) ///////////////RIGHT Wall effect
		{
			ballx = -ballx;
			ballx = ballx + .1;

		}

		else if (bally <= -4.5 || bally >= 4.5) {
			if (ballx > 9.8)
				xspeed = -xspeed;
			if (ballx < -9.8)
				xspeed = -xspeed;

		}

	}
	if (stage == 1) {
		if (ballx > 9.4)
			xspeed = -xspeed;
		if (ballx < -9.4)
			xspeed = -xspeed;
	}
	glutPostRedisplay(); //Tell GLUT that the display has changed

						 //Tell GLUT to call update again in 25 milliseconds
	glutTimerFunc(25, update, 0);
}



void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (w <= (h * 2))
		glOrtho(-10.0, 10.0, -4.0*((GLfloat)h * 3) / (GLfloat)w,
			4.0*((GLfloat)h * 3) / (GLfloat)w, -10.0, 10.0);
	else
		glOrtho(-7.0*(GLfloat)w / ((GLfloat)h * 2),
			7.0*(GLfloat)w / ((GLfloat)h * 2), -3.0, 3.0, -10.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(900, 700);
	glutCreateWindow(argv[0]);
	init();
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutTimerFunc(1, update, 1); //Add a timer
	glutKeyboardFunc(handleKeypress);
	glutSpecialFunc(keyboard);
	glutPassiveMotionFunc(myMouseMove);
	glutMouseFunc(myMouse);
	glutMainLoop();
	return 0;
}
