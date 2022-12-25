#include <iostream>
#include <windows.h>
#include <gl/gl.h>
#include <gl/glut.h>
#include <cmath>
#include <fstream>

#define _CRT_SECURE_NO_WARNINGS
#define GLEW_STATIC
#define FREEGLUT_STATIC
#define M_PI 3.14159265358979323846

using namespace std;

typedef float point3[3];
static GLfloat viewer[] = { 0.0, 0.0, 10.0 };
GLfloat light_position[] = { 30.0, 0.0, 8.0, 1.0 };
GLfloat light_position2[] = { -30.0, 0.0, 8.0, 1.0 };

int model = 1; // 1-jajko, 2-czajnik

static GLfloat theta = 0.0; //kąt obrotu obiektu
static GLfloat phi = 0.0;
static GLfloat R = 10.0;
static GLfloat pix2angle; //przelicznik pikseli na stopnie

static GLint status = 0; //stan klawiszy myszy
						 //0 - nie naciśnięto żadnego klawisza
						 //1 - naciśnięty został lewy klawisz

static GLint mouseStatus = 0; //0 - obsługa kamery
							  //1 - obsługa świateł

static GLint sPiramida = 4; //0 - wyświtla sie tylko podstawa ostrosłupa
							//1 - wyświtla się 1 ściana boczna ostrosłupa
                            //2 - wyświetlają się 2 ściany boczne ostrosłupa
                            //3 - wyświetlają się 3 ściany boczne ostrosłupa
                            //4 - wyświetlają się 4 ściany boczne ostrosłupa

static int x_pos_old = 0; //poprzednia pozycja kursora myszy
static int y_pos_old = 0;

float fix = 1.0;

static int delta_x = 0; //różnica pomiędzy pozycją bieżącą, a poprzednią kursora myszy
static int delta_y = 0;

static GLfloat center_x = 0.0; //Punkt na, który potrzy obserwator
static GLfloat center_y = 0.0;
static GLfloat center_z = 0.0;

GLbyte* LoadTGAImage(const char* FileName, GLint* ImWidth, GLint* ImHeight, GLint* ImComponents, GLenum* ImFormat)
{
	/*************************************************************************************/

	// Struktura dla nagłówka pliku  TGA
#pragma pack(1)           
	typedef struct
	{
		GLbyte    idlength;
		GLbyte    colormaptype;
		GLbyte    datatypecode;
		unsigned short    colormapstart;
		unsigned short    colormaplength;
		unsigned char     colormapdepth;
		unsigned short    x_orgin;
		unsigned short    y_orgin;
		unsigned short    width;
		unsigned short    height;
		GLbyte    bitsperpixel;
		GLbyte    descriptor;
	}TGAHEADER;
#pragma pack(8)

	FILE* pFile;
	TGAHEADER tgaHeader;
	unsigned long lImageSize;
	short sDepth;
	GLbyte* pbitsperpixel = NULL;

	/*************************************************************************************/
	// Wartości domyślne zwracane w przypadku błędu

	*ImWidth = 0;
	*ImHeight = 0;
	*ImFormat = GL_BGR_EXT;
	*ImComponents = GL_RGB8;

	fopen_s(&pFile, FileName, "rb");
	if (pFile == NULL)
		return NULL;

	/*************************************************************************************/
	// Przeczytanie nagłówka pliku 

	fread(&tgaHeader, sizeof(TGAHEADER), 1, pFile);

	/*************************************************************************************/
	// Odczytanie szerokości, wysokości i głębi obrazu

	*ImWidth = tgaHeader.width;
	*ImHeight = tgaHeader.height;
	sDepth = tgaHeader.bitsperpixel / 8;

	/*************************************************************************************/
	// Sprawdzenie, czy głębia spełnia założone warunki (8, 24, lub 32 bity)

	if (tgaHeader.bitsperpixel != 8 && tgaHeader.bitsperpixel != 24 && tgaHeader.bitsperpixel != 32)
		return NULL;

	/*************************************************************************************/

	// Obliczenie rozmiaru bufora w pamięci

	lImageSize = tgaHeader.width * tgaHeader.height * sDepth;

	/*************************************************************************************/
	// Alokacja pamięci dla danych obrazu

	pbitsperpixel = (GLbyte*)malloc(lImageSize * sizeof(GLbyte));

	if (pbitsperpixel == NULL)
		return NULL;

	if (fread(pbitsperpixel, lImageSize, 1, pFile) != 1)
	{
		free(pbitsperpixel);
		return NULL;
	}
	/*************************************************************************************/
	// Ustawienie formatu OpenGL
	switch (sDepth)
	{
	case 3:
		*ImFormat = GL_BGR_EXT;
		*ImComponents = GL_RGB8;
		break;
	case 4:
		*ImFormat = GL_BGRA_EXT;
		*ImComponents = GL_RGBA8;
		break;
	case 1:
		*ImFormat = GL_LUMINANCE;
		*ImComponents = GL_LUMINANCE8;
		break;
	};
	fclose(pFile);
	return pbitsperpixel;
}


void Axes(void)
{
	//Początek i koniec obrazu osi x
	point3 x_min = { -5.0, 0.0, 0.0 };
	point3 x_max = { 5.0, 0.0, 0.0 };

	//Początek i koniec obrazu osi y
	point3 y_min = { 0.0, -5.0, 0.0 };
	point3 y_max = { 0.0, 5.0, 0.0 };

	//Początek i koniec obrazu osi z
	point3 z_min = { 0.0, 0.0, -5.0 };
	point3 z_max = { 0.0, 0.0, 5.0 };

	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	glVertex3fv(x_min);
	glVertex3fv(x_max);
	glEnd();

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINES);
	glVertex3fv(y_min);
	glVertex3fv(y_max);
	glEnd();

	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex3fv(z_min);
	glVertex3fv(z_max);
	glEnd();
}

// Wzory opisujące jajko
float policzX(float u, float v)
{
	float PIV = M_PI * v;
	return((-90 * pow(u, 5) + 225 * pow(u, 4) - 270 * pow(u, 3) + 180 * pow(u, 2) - 45 * u) * cos(PIV));
}

float policzY(float u, float v)
{
	return(160 * pow(u, 4) - 320 * pow(u, 3) + 160 * pow(u, 2));
}

float policzZ(float u, float v)
{
	float PIV = M_PI * v;
	return((-90 * pow(u, 5) + 225 * pow(u, 4) - 270 * pow(u, 3) + 180 * pow(u, 2) - 45 * u) * sin(PIV));
}

// Wzory opisujące wektory normalne
float policzXu(float u, float v)
{
	float PIV = M_PI * v;
	return ((-450 * pow(u, 4) + 900 * pow(u, 3) - 810 * pow(u, 2) + 360 * u - 45) * cos(PIV));
}

float policzXv(float u, float v)
{
	float PIV = M_PI * v;
	return (M_PI * ((90 * pow(u, 5) - 225 * pow(u, 4) + 270 * pow(u, 3) - 180 * pow(u, 2) + 45 * u) * sin(PIV)));
}

float policzYu(float u, float v)
{
	return (640 * pow(u, 3) - 960 * pow(u, 2) + 320 * u);
}

float policzYv(float u, float v)
{
	return 0;
}

float policzZu(float u, float v)
{
	float PIV = M_PI * v;
	return ((-450 * pow(u, 4) + 900 * pow(u, 3) - 810 * pow(u, 2) + 360 * u - 45) * sin(PIV));
}

float policzZv(float u, float v)
{
	float PIV = M_PI * v;
	return (-M_PI * ((90 * pow(u, 5) - 225 * pow(u, 4) + 270 * pow(u, 3) - 180 * pow(u, 2) + 45 * u) * cos(PIV)));
}

float dlugoscWektora(float xv, float yv, float zv)
{
	return sqrt(pow(xv, 2) + pow(yv, 2) + pow(zv, 2));
}

// Struktura Punkt
struct Punkt
{
	float x;
	float y;
	float z;
	float xRGB;
	float yRGB;
	float zRGB;
	float xV;
	float yV;
	float zV;
	float i;
	float j;
};

const int N = 120;

struct Punkt Jajko[N][N];

float krok = 1.0 / N;

float x_swiatla = 0;
float y_swiatla = 0;
float x_swiatla2 = 0;
float y_swiatla2 = 0;

// Funkcja wypełniająca tablicę punktami jajka
void policz()
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			Jajko[i][j].x = policzX(i * krok, j * krok);
			Jajko[i][j].y = policzY(i * krok, j * krok);
			Jajko[i][j].z = policzZ(i * krok, j * krok);

			// Wektory normalne
			float xU = policzXu(i * krok, j * krok);
			float xV = policzXv(i * krok, j * krok);

			float yU = policzYu(i * krok, j * krok);
			float yV = policzYv(i * krok, j * krok);

			float zU = policzZu(i * krok, j * krok);
			float zV = policzZv(i * krok, j * krok);

			float xVector = (yU * zV - zU * yV);
			float yVector = (zU * xV - xU * zV);
			float zVector = (xU * yV - yU * xV);

			float dlugosc = dlugoscWektora(xVector, yVector, zVector);

			Jajko[i][j].xV = -xVector / dlugosc;
			Jajko[i][j].yV = -yVector / dlugosc;
			Jajko[i][j].zV = -zVector / dlugosc;

			if (i > N / 2)
			{
				Jajko[i][j].xV = xVector / dlugosc;
				Jajko[i][j].yV = yVector / dlugosc;
				Jajko[i][j].zV = zVector / dlugosc;
			}

			Jajko[i][j].i = -i * krok;
			Jajko[i][j].j = -j * krok;

		}
	}
}

void Jajo()
{
	policz();

	// Model jajka narysowany za pomocą trójkątów
	if (model == 1)
	{
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				if (j < N - 1)
				{
					//Trójkąty połowy góry oraz dołu i trójkąty skośne w jedną stronę
					if (i <= N / 2)
					{
						glBegin(GL_TRIANGLES);

						glNormal3f(-Jajko[i][j].xV, -Jajko[i][j].yV, -Jajko[i][j].zV);
						glTexCoord2f(-Jajko[i][j].i, -Jajko[i][j].j);
						glVertex3f(Jajko[i][j].x, Jajko[i][j].y - 5, Jajko[i][j].z);

						glNormal3f(-Jajko[(i + 1) % N][j].xV, -Jajko[(i + 1) % N][j].yV, -Jajko[(i + 1) % N][j].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j].i, -Jajko[(i + 1) % N][j].j);
						glVertex3f(Jajko[(i + 1) % N][j].x, Jajko[(i + 1) % N][j].y - 5, Jajko[(i + 1) % N][j].z);

						glNormal3f(-Jajko[i][j + 1].xV, -Jajko[i][j + 1].yV, -Jajko[i][j + 1].zV);
						glTexCoord2f(-Jajko[i][j + 1].i, -Jajko[i][j + 1].j);
						glVertex3f(Jajko[i][j + 1].x, Jajko[i][j + 1].y - 5, Jajko[i][j + 1].z);

						glEnd();
					}

					if (i > N / 2)
					{
						glBegin(GL_TRIANGLES);

						glNormal3f(-Jajko[i][j].xV, -Jajko[i][j].yV, -Jajko[i][j].zV);
						glTexCoord2f(-Jajko[i][j].i, -Jajko[i][j].j);
						glVertex3f(Jajko[i][j].x, Jajko[i][j].y - 5, Jajko[i][j].z);

						glNormal3f(-Jajko[i][j + 1].xV, -Jajko[i][j + 1].yV, -Jajko[i][j + 1].zV);
						glTexCoord2f(-Jajko[i][j + 1].i, -Jajko[i][j + 1].j);
						glVertex3f(Jajko[i][j + 1].x, Jajko[i][j + 1].y - 5, Jajko[i][j + 1].z);

						glNormal3f(-Jajko[(i + 1) % N][j].xV, -Jajko[(i + 1) % N][j].yV, -Jajko[(i + 1) % N][j].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j].i, -Jajko[(i + 1) % N][j].j);
						glVertex3f(Jajko[(i + 1) % N][j].x, Jajko[(i + 1) % N][j].y - 5, Jajko[(i + 1) % N][j].z);

						glEnd();
					}
					
					//Rysowanie drugiej połowy dołu i góry oraz trójkąty skośne w drugą stronę
					if (i <= N / 2)
					{
						glBegin(GL_TRIANGLES);

						glNormal3f(-Jajko[i][j + 1].xV, -Jajko[i][j + 1].yV, -Jajko[i][j + 1].zV);
						glTexCoord2f(-Jajko[i][j + 1].i, -Jajko[i][j + 1].j);
						glVertex3f(Jajko[i][j + 1].x, Jajko[i][j + 1].y - 5, Jajko[i][j + 1].z);

						glNormal3f(-Jajko[(i + 1) % N][j].xV, -Jajko[(i + 1) % N][j].yV, -Jajko[(i + 1) % N][j].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j].i, -Jajko[(i + 1) % N][j].j);
						glVertex3f(Jajko[(i + 1) % N][j].x, Jajko[(i + 1) % N][j].y - 5, Jajko[(i + 1) % N][j].z);

						glNormal3f(-Jajko[(i + 1) % N][j + 1].xV, -Jajko[(i + 1) % N][j + 1].yV, -Jajko[(i + 1) % N][j + 1].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j + 1].i, -Jajko[(i + 1) % N][j + 1].j);
						glVertex3f(Jajko[(i + 1) % N][j + 1].x, Jajko[(i + 1) % N][j + 1].y - 5, Jajko[(i + 1) % N][j + 1].z);

						glEnd();
					}

					if (i > N / 2)
					{
						glBegin(GL_TRIANGLES);

						glNormal3f(-Jajko[i][j + 1].xV, -Jajko[i][j + 1].yV, -Jajko[i][j + 1].zV);
						glTexCoord2f(-Jajko[i][j + 1].i, -Jajko[i][j + 1].j);
						glVertex3f(Jajko[i][j + 1].x, Jajko[i][j + 1].y - 5, Jajko[i][j + 1].z);

						glNormal3f(-Jajko[(i + 1) % N][j + 1].xV, -Jajko[(i + 1) % N][j + 1].yV, -Jajko[(i + 1) % N][j + 1].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j + 1].i, -Jajko[(i + 1) % N][j + 1].j);
						glVertex3f(Jajko[(i + 1) % N][j + 1].x, Jajko[(i + 1) % N][j + 1].y - 5, Jajko[(i + 1) % N][j + 1].z);

						glNormal3f(-Jajko[(i + 1) % N][j].xV, -Jajko[(i + 1) % N][j].yV, -Jajko[(i + 1) % N][j].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j].i, -Jajko[(i + 1) % N][j].j);
						glVertex3f(Jajko[(i + 1) % N][j].x, Jajko[(i + 1) % N][j].y - 5, Jajko[(i + 1) % N][j].z);

						glEnd();
					}

				}
				//Trójkąty łączące połowy jajka
				
				else
				{
						glBegin(GL_TRIANGLES);

						glNormal3f(-Jajko[(i + 1) % N][j].xV, -Jajko[(i + 1) % N][j].yV, -Jajko[(i + 1) % N][j].zV);
						glTexCoord2f(-Jajko[(i + 1) % N][j].i, -Jajko[(i + 1) % N][j].j);
						glVertex3f(Jajko[(i + 1) % N][j].x, Jajko[(i + 1) % N][j].y - 5, Jajko[(i + 1) % N][j].z);

						glNormal3f(-Jajko[(N - i) % N][0].xV, -Jajko[(N - i) % N][0].yV, -Jajko[(N - i) % N][0].zV);
						glTexCoord2f(-Jajko[(N - i) % N][0].i, -Jajko[(N - i) % N][0].j);
						glVertex3f(Jajko[(N - i) % N][0].x, Jajko[(N - i) % N][0].y - 5, Jajko[(N - i) % N][0].z);

						glNormal3f(-Jajko[N - i - 1][0].xV, -Jajko[N - i - 1][0].yV, -Jajko[N - i - 1][0].zV);
						glTexCoord2f(-Jajko[N - i - 1][0].i, -Jajko[N - i - 1][0].j);
						glVertex3f(Jajko[N - i - 1][0].x, Jajko[N - i - 1][0].y - 5, Jajko[N - i - 1][0].z);

						glEnd();
				}
				if (i > 0)
				{
					glBegin(GL_TRIANGLES);
				
					glNormal3f(-Jajko[i][j].xV, -Jajko[i][j].yV, -Jajko[i][j].zV);
					glTexCoord2f(-Jajko[i][j].i, -Jajko[i][j].j);
					glVertex3f(Jajko[i][j].x, Jajko[i][j].y - 5, Jajko[i][j].z);

					glNormal3f(-Jajko[(i + 1) % N][j].xV, -Jajko[(i + 1) % N][j].yV, -Jajko[(i + 1) % N][j].zV);
					glTexCoord2f(-Jajko[(i + 1) % N][j].i, -Jajko[(i + 1) % N][j].j);
					glVertex3f(Jajko[(i + 1) % N][j].x, Jajko[(i + 1) % N][j].y - 5, Jajko[(i + 1) % N][j].z);

					glNormal3f(-Jajko[N - i][0].xV, -Jajko[N - i][0].yV, -Jajko[N - i][0].zV);
					glTexCoord2f(-Jajko[N - i][0].i, -Jajko[N - i][0].j);
					glVertex3f(Jajko[N - i][0].x, Jajko[N - i][0].y - 5, Jajko[N - i][0].z);

					glEnd();
				}
			}
		}
	}
	else if (model == 2) // Trójkąt
	{
		glBegin(GL_TRIANGLES);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(0.0, 0.0, 0.0);

			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(4.0, 0.0, 0.0);

			glTexCoord2f(0.5f, 1.0f);
			glVertex3f(2.0, 4.0, 0.0);
		glEnd();
	}
	else if (model == 3) // Piramida
	{
		// Rysowanie podstawy ostrosłupa
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-5.0, 0.0, -5.0);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(5.0, 0.0, -5.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-5.0, 0.0, 5.0);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(5.0, 0.0, 5.0);
		glEnd();

		// Rysowanie ścian bocznych ostrosłupa
		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0.5, 0.5);
		glVertex3f(0.0, 5.0, 0.0);

		glTexCoord2f(0.0, 0.0);
		glVertex3f(-5.0, 0.0, 5.0);

		if (sPiramida > 0)
		{
			glTexCoord2f(1.0, 0.0);
			glVertex3f(5.0, 0.0, 5.0);

			if (sPiramida > 1)
			{
				glTexCoord2f(1.0, 1.0);
				glVertex3f(5.0, 0.0, -5.0);

				if (sPiramida > 2)
				{
					glTexCoord2f(0.0, 1.0);
					glVertex3f(-5.0, 0.0, -5.0);

					if (sPiramida == 4)
					{
						glTexCoord2f(0.0, 0.0);
						glVertex3f(-5.0, 0.0, 5.0);
					}
				}
			}
		}
		glEnd();
	}
}

// Funkcja odczytująca klawisze
void keys(unsigned char key, int x, int y)
{
	if (key == 'j') model = 1; // Jajko
	if (key == 't') model = 2; // Trójkąt
	if (key == 'p') model = 3; // Piramida
	if (key == 'k') mouseStatus = 0; // Kamera
	if (key == 'o') mouseStatus = 1; // Obiekt
	if (key == 's') mouseStatus = 2; // Światła
	if (key == '0') sPiramida = 0;
	if (key == '1') sPiramida = 1;
	if (key == '2') sPiramida = 2;
	if (key == '3') sPiramida = 3;
	if (key == '4') sPiramida = 4;
}

void RenderScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	gluLookAt(viewer[0], viewer[1], viewer[2], center_x, center_y, center_z, 0.0, fix, 0.0);
	glColor3f(1.0f, 1.0f, 1.0f);
	Axes();

	if (mouseStatus == 0)
	{
		if (status == 1)
		{
			theta += delta_x * pix2angle / 20.0;
			phi += delta_y * pix2angle / 20.0;
			if (phi > 2 * M_PI)
				phi = 0;
			if (phi < 0)
				phi = 2 * M_PI;
			if (phi > M_PI / 2)
				fix = -1.0;
			else
				fix = 1.0;
			if (phi > M_PI + (M_PI / 2))
				fix = 1.0;
		}
		else if (status == 2)
		{
			R += delta_y * pix2angle / 20.0;
		}
		viewer[0] = R * cos(theta) * cos(phi);
		viewer[1] = R * sin(phi);
		viewer[2] = R * sin(theta) * cos(phi);
	}

	if (mouseStatus == 1)
	{
		if (status == 1)
		{
			theta += delta_x * pix2angle;
			phi += delta_y * pix2angle;
		}
		else if (status == 2)
		{
			R += delta_y * pix2angle / 20.0;
			viewer[0] = R * cos(theta) * cos(phi);
			viewer[1] = R * sin(phi);
			viewer[2] = R * sin(theta) * cos(phi);
		}

		glRotatef(theta, 0.0, 1.0, 0.0); // Obrót obiektu o nowy kąt
		glRotatef(phi, 1.0, 0.0, 0.0);
	}

	if (mouseStatus == 2)
	{
		if (status == 1)
		{
			x_swiatla -= delta_x * pix2angle / 40.0;
			y_swiatla -= delta_y * pix2angle / 40.0;
		}

		else if (status == 2)
		{
			x_swiatla2 -= delta_x * pix2angle / 40.0;
			y_swiatla2 -= delta_y * pix2angle / 40.0;
		}

		light_position[0] = R * cos(x_swiatla) * cos(y_swiatla);
		light_position[1] = R * sin(y_swiatla);
		light_position[2] = R * sin(x_swiatla) * cos(y_swiatla);

		light_position2[0] = R * cos(x_swiatla2) * cos(y_swiatla2);
		light_position2[1] = R * sin(y_swiatla2);
		light_position2[2] = R * sin(x_swiatla2) * cos(y_swiatla2);
	}

	glColor3f(1.0f, 1.0f, 1.0f);

	Jajo();

	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position2);

	glFlush();

	glutSwapBuffers();
}

void MyInit(void)
{
	/*************************************************************************************/
	// Zmienne dla obrazu tekstury
	GLbyte* pBytes;
	GLint ImWidth, ImHeight, ImComponents;
	GLenum ImFormat;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Definicja materiału z jakiego zrobiony jest czajnik i jajko

	GLfloat mat_ambient[] = { 0.4, 0.4, 0.4, 0.4 };
	// współczynniki ka =[kar,kag,kab] dla światła otoczenia

	GLfloat mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	// współczynniki kd =[kdr,kdg,kdb] światła rozproszonego

	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	// współczynniki ks =[ksr,ksg,ksb] dla światła odbitego

	GLfloat mat_shininess = { 20.0 };
	// współczynnik n opisujący połysk powierzchni

	// Definicja źródła światła
	GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };

	GLfloat light_ambient[] = { 0.4, 0.4, 0.4, 0.4 };
	// składowe intensywności świecenia źródła światła otoczenia
	// Ia = [Iar,Iag,Iab]

	GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	// składowe intensywności świecenia źródła światła powodującego
	// odbicie dyfuzyjne Id = [Idr,Idg,Idb]

	GLfloat light_specular[] = { 0.8, 0.8, 0.8, 1.0 };
	// składowe intensywności świecenia źródła światła powodującego
	// odbicie kierunkowe Is = [Isr,Isg,Isb]

	GLfloat att_constant = { 1.0 };
	// składowa stała ds dla modelu zmian oświetlenia w funkcji
	// odległości od źródła

	GLfloat att_linear = { 0.05 };
	// składowa liniowa dl dla modelu zmian oświetlenia w funkcji
	// odległości od źródła

	GLfloat att_quadratic = { 0.001 };
	// składowa kwadratowa dq dla modelu zmian oświetlenia w funkcji
	// odległości od źródła

	// Definicja drugiego źródła światła
	GLfloat light_position2[] = { 10.0f, 0.0f, 0.0f, 1.0f };

	GLfloat light_ambient2[] = { 0.0, 0.0, 0.4, 0.4 };
	// składowe intensywności świecenia źródła światła otoczenia
	// Ia = [Iar,Iag,Iab]

	GLfloat light_diffuse2[] = { 0.0, 0.0, 1.0, 1.0 };
	// składowe intensywności świecenia źródła światła powodującego
	// odbicie dyfuzyjne Id = [Idr,Idg,Idb]

	GLfloat light_specular2[] = { 0.6, 0.6, 0.8, 1.0 };
	// składowe intensywności świecenia źródła światła powodującego
	// odbicie kierunkowe Is = [Isr,Isg,Isb]

	GLfloat att_constant2 = { 1.0 };
	// składowa stała ds dla modelu zmian oświetlenia w funkcji
	// odległości od źródła

	GLfloat att_linear2 = { 0.05 };
	// składowa liniowa dl dla modelu zmian oświetlenia w funkcji
	// odległości od źródła

	GLfloat att_quadratic2 = { 0.001 };
	// składowa kwadratowa dq dla modelu zmian oświetlenia w funkcji
	// odległości od źródła

	// Ustawienie parametrów materiału i źródła światła
	// Ustawienie parametrów materiału

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

	// Ustawienie parametrów pierwszego źródła światła

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, att_constant);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, att_linear);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, att_quadratic);

	// Ustawienie parametrów drugiego źródła światła
	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse2);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular2);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position2);

	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, att_constant2);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, att_linear2);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, att_quadratic2);

	// Ustawienie opcji systemu oświetlenia sceny

	glShadeModel(GL_SMOOTH); // właczenie łagodnego cieniowania
	glEnable(GL_LIGHTING);   // włączenie systemu oświetlenia sceny
	glEnable(GL_LIGHT0);     // włączenie źródła o numerze 0
	//glEnable(GL_LIGHT1);	 // włączenie źródła o numerze 1
	glEnable(GL_DEPTH_TEST); // włączenie mechanizmu z-bufora

	// Teksturowanie będzie prowadzone tylko po jednej stronie ściany
	glEnable(GL_CULL_FACE);

	// Przeczytanie obrazu tekstury z pliku o nazwie .tga
	pBytes = LoadTGAImage("P3_t.tga", &ImWidth, &ImHeight, &ImComponents, &ImFormat);

	// Zdefiniowanie tekstury 2-D
	glTexImage2D(GL_TEXTURE_2D, 0, ImComponents, ImWidth, ImHeight, 0, ImFormat, GL_UNSIGNED_BYTE, pBytes);

	// Zwolnienie pamięci
	free(pBytes);

	// Włączenie mechanizmu teksturowania
	glEnable(GL_TEXTURE_2D);

	// Ustalenie trybu teksturowania
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Określenie sposobu nakładania tekstur
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void ChangeSize(GLsizei horizontal, GLsizei vertical)
{
	pix2angle = 360.0 / (float)horizontal; //przeliczanie pikseli na stopnie

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	//Ustawienie parametrów dla rzutu perspektywicznego
	gluPerspective(70, 1.0, 1.0, 30.0);

	if (horizontal <= vertical)
	{
		glViewport(0, (vertical - horizontal) / 2, horizontal, horizontal);
	}
	else
	{
		glViewport((horizontal - vertical) / 2, 0, vertical, vertical);
	}

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
}

void Mouse(int btn, int state, int x, int y)
{
	if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) // Wciścnięty lewy klawisz
	{
		x_pos_old = x;
		y_pos_old = y;
		status = 1;
	}
	else if (btn == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) // Wciśnięty prawy klawisz
	{
		x_pos_old = x;
		y_pos_old = y;
		status = 2;

	}
	else status = 0; // Nie został wciśnięty żaden klawisz
}

void Motion(GLsizei x, GLsizei y)
{
	delta_x = x - x_pos_old; // Obliczenie różnicy położenia kursora myszy
	delta_y = y - y_pos_old;
	x_pos_old = x; // Podstawienie bieżącego położenia jako poprzednie
	y_pos_old = y;
	glutPostRedisplay(); // Przerysowanie obrazu sceny
}

int main()
{
	cout << "j - jajko" << endl;
	cout << "t - trojkat" << endl;
	cout << "p - piramida" << endl;
	cout << "k - obsluga kamery" << endl;
	cout << "o - obracanie obiektem" << endl;
	cout << "s - osluga swiatel" << endl;
	cout << "0 - tylko podstawa piramidy" << endl;
	cout << "1 - podstawa i jedna ściana boczna piramidy" << endl;
	cout << "2 - podstawa i dwie ściany boczne piramidy" << endl;
	cout << "3 - podstawa i trzy ściany boczne piramidy" << endl;
	cout << "4 - podstawa i cztry ściany boczne piramidy" << endl;

	int argc = 1;
	char* argv[1] = {(char*) " " };
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	glutInitWindowSize(300, 300);

	glutCreateWindow("Teksturowanie powierzchni");

	glutDisplayFunc(RenderScene);

	glutReshapeFunc(ChangeSize);

	glutMouseFunc(Mouse);

	glutMotionFunc(Motion);

	MyInit();

	glEnable(GL_DEPTH_TEST);

	glutKeyboardFunc(keys);

	glutMainLoop();

	return 0;
}