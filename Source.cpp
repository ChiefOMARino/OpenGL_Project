#include "GL\freeglut.h"
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "FreeImage.h"
#include "glm/glm.hpp"
using namespace std;

//Data related to the HeightMap
BYTE** heightMap;	//Dynamic Allocation will be used for this 2D array
int heightMapWidth, heightMapHeight;
int stepSize = 2;


//Link to the freeimage library during compilation
#pragma comment(lib, "freeimage.lib")

float ang = 0;

GLfloat ambientLight[] = { 1.0, 1.0, 1, 1 };
GLfloat diffuseLight[] = { 1, 1, 1, 1 };
GLfloat lightPosition[] = { 0, 0, 5, 1 };

//Camera Parameters
float cameraX = -50, cameraZ = 0, cameraY = 50;
float headingX = 0, headingZ = -1, headingY = 4;

float cameraAng = 0;
float cameraAngY = 0;
int stepsDirection = 0;


int deltax, deltay;

float steps = 0;

GLuint tex1;
GLuint tex2;

GLuint terrainTexture;

int loadTexture(const char* filename)
{
	GLuint texture;

	FIBITMAP* originalImage = FreeImage_Load(FreeImage_GetFileType(filename, 0), filename);	//Load original image from the storage device
	FIBITMAP* finalImage = FreeImage_ConvertTo32Bits(originalImage);	//Unify the image format to 32 Bits

	int width = FreeImage_GetWidth(finalImage);
	int height = FreeImage_GetHeight(finalImage);

	//OpenGL related routines

	//Let OpenGL know that we need to generate 1 texture
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);	//Set 'texture' as the active texture

											//Set the MAG and MIN Filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	//Min Filter, use interpolation
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	//Mag Filter, use interpolation

																		//Convert from image to a texture
																		//GL_BGRA_EXT is used because Freeimage API uses the big endian format (bytes are in reversed order)
																		//GL_UNSIGNED_BYTE means that each channel is rep. using a value from 0~255
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, FreeImage_GetBits(finalImage));

	return texture;

}

//Particle Engine Data

class MiniSnowEngine
{
public:
	//Create a Particle Data Structure (SnowFlake)
	struct SnowFlake
	{
		GLuint texture;
		float x, y, z;
		float xr, yr, zr;

		float rotAng;
		float rotVel;

		float gravity;
		float lifetime;
		float fading;

		float speed;
		float scale;

		bool isAlive;
	};

	struct SnowFlakeRemaining
	{
		float x, y, z;
		float lifetime;
		float fading;
		float scale;

		SnowFlakeRemaining()
		{
			fading = 0.01;	//Fixed Fading Speed
		}
	};

	SnowFlake* flakes;	//Dynamic allocation for the required number of flakes
	int count;	//Number of Particles
	int groundLevel, skyLevel;
	int regionSize;

	float windZ, windX;

	char snowFlakesTexturesLocation[3][50];	//Max of 3 textures. 50 is the max path size
	GLuint snowFlakeTextures[3];

	vector<SnowFlakeRemaining> remainings;

	//Constructor for the Snow Engine
	MiniSnowEngine(int numberOfParticles, int skyL, int groundL, int region)
	{
		//Initialize Textures Locations
		strcpy(snowFlakesTexturesLocation[0], "snowflake0.png");
		strcpy(snowFlakesTexturesLocation[1], "snowflake1.png");
		strcpy(snowFlakesTexturesLocation[2], "snowflake2.png");

		count = numberOfParticles;
		skyLevel = skyL;
		groundL = groundL;
		regionSize = region;

		flakes = new SnowFlake[count];

		windX = -0.05;
		windZ = 0;
	}

	void initializeAFlake(int n)
	{
		flakes[n].y = skyLevel;
		flakes[n].x = rand() % (regionSize + 1) - regionSize / 2.0f;
		flakes[n].z = rand() % (regionSize + 1) - regionSize / 2.0f;
		flakes[n].scale = (rand() % 50) / 50.0f + 0.5f; //(rand()%50 / 50) + minSize

		flakes[n].xr = (rand() % 1000) / 1000.0f;
		flakes[n].yr = (rand() % 1000) / 1000.0f;
		flakes[n].zr = (rand() % 1000) / 1000.0f;

		flakes[n].rotVel = (rand() % 1000) / 1000.0f;
		flakes[n].rotAng = 0;

		int tIndex = rand() % 3;
		flakes[n].texture = snowFlakeTextures[tIndex];

		flakes[n].isAlive = true;
		flakes[n].lifetime = 1.0f;
		flakes[n].fading = (rand() % 10 / 1000.0f) + 0.0001;
		flakes[n].gravity = -0.8;
		flakes[n].speed = 0;	//Will be used for acceleration

	}

	void update()
	{
		for (int i = 0; i < count; i++)
		{
			if (flakes[i].isAlive)
			{
				flakes[i].y += flakes[i].speed / 400.0f;
				flakes[i].x += windX * (rand() % 5);
				flakes[i].z += windZ;

				flakes[i].speed += flakes[i].gravity;
				flakes[i].rotAng += flakes[i].rotVel;

				flakes[i].lifetime -= flakes[i].fading;

				if (flakes[i].lifetime <= 0 || flakes[i].y <= groundLevel)
				{
					flakes[i].isAlive = false;
					if (flakes[i].y <= groundLevel) //Hit the ground
					{
						SnowFlakeRemaining sfr;
						sfr.x = flakes[i].x;
						sfr.z = flakes[i].z;
						sfr.y = groundLevel + 0.08;
						sfr.scale = flakes[i].scale;
						sfr.lifetime = sfr.scale * 10;
						remainings.push_back(sfr);	//Insert at the end !
					}
				}
			}
			else
			{
				initializeAFlake(i);
			}
		}
		//Update Remainings
		vector<SnowFlakeRemaining>::iterator it = remainings.begin();
		//		for (int i = 0; i < remainings.size(); i++)
		//	{

		//	}
		while (it != remainings.end())
		{
			it->lifetime -= it->fading;
			it->scale -= it->fading*0.3;
			it++;
		}
		remainings.erase(remove_if(remainings.begin(), remainings.end(), [](SnowFlakeRemaining r) { return r.lifetime <= 0 ? true : false; }), remainings.end());
		remainings.erase(remove_if(remainings.begin(), remainings.end(), [](SnowFlakeRemaining r) { return r.scale <= 0 ? true : false; }), remainings.end());
	}

	void render()
	{
		for (int i = 0; i < count; i++)
		{
			if (flakes[i].isAlive)
			{
				glDisable(GL_LIGHTING);

				//Random Blinking Colors
				/*float color1 = (rand() % 100) / 100.0f;
				float color2 = (rand() % 100) / 100.0f;
				float color3 = (rand() % 100) / 100.0f;*/

				glColor4f(1, 1, 1, 1);
				//Create a snow flake sprite (remove black background)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glEnable(GL_BLEND);

				glPushMatrix();

				glScalef(flakes[i].scale, flakes[i].scale, flakes[i].scale);
				glTranslatef(flakes[i].x, flakes[i].y, flakes[i].z);
				glRotatef(flakes[i].rotAng, flakes[i].xr, flakes[i].yr, flakes[i].zr);

				glBindTexture(GL_TEXTURE_2D, flakes[i].texture);

				glBegin(GL_QUADS);

				glTexCoord2d(0, 0);	glVertex3f(-1, -1, 0);
				glTexCoord2d(0, 1);	glVertex3f(-1, 1, 0);
				glTexCoord2d(1, 1);	glVertex3f(1, 1, 0);
				glTexCoord2d(1, 0);	glVertex3f(1, -1, 0);

				glEnd();

				glPopMatrix();
				glDisable(GL_BLEND);
				glEnable(GL_LIGHTING);
			}
		}

		//Render Remainings
		for (int i = 0; i < remainings.size(); i++)
		{
			glDisable(GL_TEXTURE_2D);
			glPushMatrix();
			glTranslatef(remainings[i].x, remainings[i].y, remainings[i].z);
			glScalef(remainings[i].scale, remainings[i].scale, remainings[i].scale);
			glColor3f(1, 1, 1);
			glutSolidSphere(0.5, 10, 10);
			glPopMatrix();
			glEnable(GL_TEXTURE_2D);
		}
	}
	void initialize()
	{
		//Load Textures
		for (int i = 0; i < 3; i++)
			snowFlakeTextures[i] = loadTexture(snowFlakesTexturesLocation[i]);

		//Initialize Snowflakes
		for (int i = 0; i < count; i++)
			initializeAFlake(i);
	}
};
MiniSnowEngine snowEngine(500, 250, 0, 500);
//


void initializeHeightMap(const char* path)
{
	FIBITMAP* heightMapImage = FreeImage_Load(FreeImage_GetFileType(path, 0), path);
	FIBITMAP* gHeightMap = FreeImage_ConvertTo8Bits(heightMapImage);
	gHeightMap = FreeImage_ConvertToGreyscale(gHeightMap);

	heightMapWidth = FreeImage_GetWidth(gHeightMap);
	heightMapHeight = FreeImage_GetHeight(gHeightMap);

	//Dynamic Allocation for the height map
	heightMap = new BYTE *[heightMapWidth];
	for (int i = 0; i < heightMapWidth; i++)
		heightMap[i] = new BYTE[heightMapHeight];

	//C-style allocation
	//heightMap = (BYTE**)malloc(sizeof(BYTE*) * heightMapWidth);
	for (int y = 0; y < heightMapHeight; y++)
	{
		BYTE* sLine = FreeImage_GetScanLine(gHeightMap, y);
		for (int x = 0; x < heightMapWidth; x++)
			heightMap[x][heightMapHeight - y - 1] = sLine[x];

	}
}

	struct Vertex
	{
		float x, y, z;
		Vertex()
		{
			x = y = z = -1;
		}
	};

	struct Face
	{
		int type; //1=Triangle, 2=Quad
		int texLocation;
		Vertex a, b, c, d;
		Vertex at, bt, ct, dt;
		Vertex an, bn, cn, dn;
		Face()
		{
			texLocation = 0;
			type = 1;
		}
	};

	class WavefrontModel
	{
		vector<Vertex> vertices;
		vector<Vertex> normals;
		vector<Vertex> textures;

		vector<Face> faces;
		vector<GLuint> tex;
	public:

		void renderModel()
		{
			for (int i = 0; i < faces.size(); i++)
			{
				glBindTexture(GL_TEXTURE_2D, tex[faces[i].texLocation]);
				if (faces[i].type == 1)
				{
					glBegin(GL_TRIANGLES);

					glTexCoord2d(faces[i].at.x, faces[i].at.y);
					glNormal3f(faces[i].an.x, faces[i].an.y, faces[i].an.z);
					glVertex3f(faces[i].a.x, faces[i].a.y, faces[i].a.z);

					glTexCoord2d(faces[i].bt.x, faces[i].bt.y);
					glNormal3f(faces[i].bn.x, faces[i].bn.y, faces[i].bn.z);
					glVertex3f(faces[i].b.x, faces[i].b.y, faces[i].b.z);

					glTexCoord2d(faces[i].ct.x, faces[i].ct.y);
					glNormal3f(faces[i].cn.x, faces[i].cn.y, faces[i].cn.z);
					glVertex3f(faces[i].c.x, faces[i].c.y, faces[i].c.z);


					glEnd();
				}
				else

				{
					glBegin(GL_QUADS);

					glTexCoord2d(faces[i].at.x, faces[i].at.y);
					glNormal3f(faces[i].an.x, faces[i].an.y, faces[i].an.z);
					glVertex3f(faces[i].a.x, faces[i].a.y, faces[i].a.z);

					glTexCoord2d(faces[i].bt.x, faces[i].bt.y);
					glNormal3f(faces[i].bn.x, faces[i].bn.y, faces[i].bn.z);
					glVertex3f(faces[i].b.x, faces[i].b.y, faces[i].b.z);

					glTexCoord2d(faces[i].ct.x, faces[i].ct.y);
					glNormal3f(faces[i].cn.x, faces[i].cn.y, faces[i].cn.z);
					glVertex3f(faces[i].c.x, faces[i].c.y, faces[i].c.z);

					glTexCoord2d(faces[i].dt.x, faces[i].dt.y);
					glNormal3f(faces[i].dn.x, faces[i].dn.y, faces[i].dn.z);
					glVertex3f(faces[i].d.x, faces[i].d.y, faces[i].d.z);


					glEnd();

				}
			}
		}
		bool loadModel(const char* filename)
		{
			FILE* in = fopen(filename, "r");
			if (!in)
				return false;

			char header[50];
			int texLocation = -1;
			while (fscanf(in, "%s", header) != EOF)
			{
				if (!strcmp(header, "v"))
				{
					Vertex p;
					fscanf(in, "%f%f%f", &p.x, &p.y, &p.z);
					vertices.push_back(p);
				}
				if (!strcmp(header, "vt"))
				{
					Vertex p;
					fscanf(in, "%f%f%f", &p.x, &p.y, &p.z);
					textures.push_back(p);
				}
				if (!strcmp(header, "vn"))
				{
					Vertex p;
					fscanf(in, "%f%f%f", &p.x, &p.y, &p.z);
					normals.push_back(p);
				}
				if (!strcmp(header, "o"))
				{
					char tName[50];
					fscanf(in, "%s", tName);
					GLuint tmp = loadTexture(tName);
					tex.push_back(tmp);
					texLocation++;
				}
				if (!strcmp(header, "f"))
				{
					Face f;
					int av = -1, bv = -1, cv = -1, dv = -1;
					int an = -1, bn = -1, cn = -1, dn = -1;
					int at = -1, bt = -1, ct = -1, dt = -1;

					char cLine[100];
					fgets(cLine, 99, in);
					sscanf(cLine, " %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &av, &at, &an, &bv, &bt,
						&bn, &cv, &ct, &cn, &dv, &dt, &dn);
					if (bv == -1)
					{
						sscanf(cLine, " %d/%d %d/%d %d/%d %d/%d", &av, &at, &bv, &bt, &cv, &ct,
							&dv, &dt);
						an = bn = cn = dn = 0;
					}
					f.a = vertices[av - 1];
					f.b = vertices[bv - 1];
					f.c = vertices[cv - 1];

					f.at = textures[at - 1];
					f.bt = textures[bt - 1];
					f.ct = textures[ct - 1];

					if (normals.size() > 0)
					{
						f.an = normals[an - 1];
						f.bn = normals[bn - 1];
						f.cn = normals[cn - 1];
					}

					if (dv != -1)
					{
						f.type = 2;
						f.d = vertices[dv - 1];
						f.dt = textures[dt - 1];
						if (normals.size() > 0)
							f.dn = normals[dn - 1];
					}
					f.texLocation = texLocation;
					faces.push_back(f);
				}

			}
		}
	};
	WavefrontModel Airplane;





void setElevationColor(int x, int z)
{
	float color = 0.05 + heightMap[x][z] / 255.0f;
	glColor3f(color, color, color);
}





void renderHeightMap()
{
	
	/*glDisable(GL_LIGHTING);*/

	

	glTranslatef(-heightMapWidth / 2, 0, -heightMapHeight / 2);
		glScalef(0.5, 0.5, 0.5);


	int X = 0, Y = 0;	//For HeightMap Array
	int x, y, z;	//For OpenGL Scene

	float cross_P[3];
	float cross_P2[3];
	float cross_P3[3];
	float cross_P4[3];
	float avg[3];

	float nl;
	float nr;
	float nd;
	float nu;

	glm::vec3 normal;



	


	for (X = 0; X < heightMapWidth - stepSize; X += stepSize)
	{
		for (Y = 0; Y < heightMapHeight - stepSize; Y += stepSize)
		{
			
			//FDM 


			x = X;
			z = Y;
			y = heightMap[x][z];
			
				glBindTexture(GL_TEXTURE_2D, tex2);
			glBegin(GL_QUADS);
			nl = heightMap[x - stepSize<0 ? x : x - stepSize][z];
			nr = heightMap[x + stepSize >= heightMapWidth ? x : x + stepSize][z];
			nd = heightMap[x][z - stepSize < 0 ? z : z - stepSize];
			nu = heightMap[x][z + stepSize >= heightMapHeight ? z : z + stepSize];

			normal.x = nl - nr;
			normal.y = nd - nu;
			normal.z = 2;

			normal = glm::normalize(normal);

			glTexCoord2d(0, 0); glNormal3f(normal.x, normal.y, normal.z);
			glVertex3f(x, y, z);

		
			x = X;
			z = Y + stepSize;
			y = heightMap[x][z];
		
			nl = heightMap[x - stepSize < 0 ? x : x - stepSize][z];
			nr = heightMap[x + stepSize >= heightMapWidth ? x : x + stepSize][z];
			nd = heightMap[x][z - stepSize < 0 ? z : z - stepSize];
			nu = heightMap[x][z + stepSize >= heightMapHeight ? z : z + stepSize];

			normal.x = nl - nr;
			normal.y = nd - nu;
			normal.z = 2;

			normal = glm::normalize(normal);

			glNormal3f(normal.x, normal.y, normal.z);

			glTexCoord2d(0, 1); glVertex3f(x, y, z);

		

			
			x = X + stepSize;
			z = Y + stepSize;
			y = heightMap[x][z];
		
			nl = heightMap[x - stepSize < 0 ? x : x - stepSize][z];
			nr = heightMap[x + stepSize >= heightMapWidth ? x : x + stepSize][z];
			nd = heightMap[x][z - stepSize < 0 ? z : z - stepSize];
			nu = heightMap[x][z + stepSize >= heightMapHeight ? z : z + stepSize];

			normal.x = nl - nr;
			normal.y = nd - nu;
			normal.z = 2;

			normal = glm::normalize(normal);

			glNormal3f(normal.x, normal.y, normal.z);

			glTexCoord2d(1, 1);  glVertex3f(x, y, z);

		


			
			x = X + stepSize;
			z = Y;
			y = heightMap[x][z];

			nl = heightMap[x - stepSize < 0 ? x : x - stepSize][z];
			nr = heightMap[x + stepSize >= heightMapWidth ? x : x + stepSize][z];
			nd = heightMap[x][z - stepSize < 0 ? z : z - stepSize];
			nu = heightMap[x][z + stepSize >= heightMapHeight ? z : z + stepSize];

			normal.x = nl - nr;
			normal.y = nd - nu;
			normal.z = 2;

			normal = glm::normalize(normal);

			glNormal3f(normal.x, normal.y, normal.z);


			glTexCoord2d(1, 0); glVertex3f(x, y, z);
			glEnd();
		}
	}
	



	
	


}




void myReshapeFunc(int w, int h)
{
	float aR = w / (float)h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);	//Load the projection matrix into the memory (screen setup)
	glLoadIdentity();	//Restores the current matrix to its default state


						//gluOrtho2D(-5*aR, 5*aR, -5, 5);	//2D
	gluPerspective(45.0f, w / (float)h, 1, 500);

	glMatrixMode(GL_MODELVIEW);	//Load the modelview matrix into the memory (objects)
	glLoadIdentity();	//Restores the current matrix to its default state 

}
float AirAng = 0;
void myDisplayFunc()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//Instructs OpenGL to clear previous Colors on screen and also the depth buffer (3D)

	glLoadIdentity();
	gluLookAt(cameraX, cameraY, cameraZ, cameraX + headingX, cameraY + headingY, cameraZ + headingZ, 0, 1, 0);
	glTranslatef(0, 0, -5);
	glEnable(GL_DEPTH_TEST);


	////Render a ground terrain
	//glBindTexture(GL_TEXTURE_2D, terrainTexture);
	//glPushMatrix();
	//glBegin(GL_QUADS);

	//glTexCoord2d(0, 0); glVertex3f(-500, 0, 500);
	//glTexCoord2d(1, 0); glVertex3f(500, 0, 500);
	//glTexCoord2d(1, 1); glVertex3f(500, 0, -500);
	//glTexCoord2d(0, 1); glVertex3f(-500, 0, -500);




	glEnd();
	glPopMatrix();

	snowEngine.render();
	glPushMatrix();
	glCallList(1);
	glPopMatrix();
	//glTranslatef(cameraX,cameraY-2, cameraZ-15);
	glPushMatrix();
	
	glTranslatef(cameraX+(15*sin(cameraAng)),cameraY+(15 * sin(cameraAngY)),cameraZ + (15 * -cos(cameraAng)));
	glRotatef(180, 0, 1, 0);
	glTranslatef(0, 0, -6);
	glRotatef(-cameraAng*2, 0, 1, 0);
	glTranslatef(0, 0, +6);
	glScalef(0.02, 0.02, 0.02);
	
	glCallList(2);
	glPopMatrix();



	glutSwapBuffers();	//Swap the buffers for the next frame
						//	glutPostRedisplay();	//let openGL know that we need to call myDisplayFunc
}

void HandleUserInteraction()
{


	cameraAngY -= 0.01*deltay;
	if (cameraAngY<1.5705 && cameraAngY>-1.5705)
		headingY = sin(cameraAngY);
	else if (cameraAngY < 0)
		cameraAngY = -1.5705;
	else
		cameraAngY = 1.5705;


	cameraAng += deltax*0.01;
	headingX = sin(cameraAng);
	headingZ = -cos(cameraAng);

	if (GetAsyncKeyState('W') != 0) //Key W
	{
		cameraZ += headingZ * 0.5;
		cameraX += headingX * 0.5;
		cameraY += headingY*0.5;

		/*if (cameraY >= 0.03 && stepsDirection == 1)
			stepsDirection = 0;
		else if (cameraY <= 0 && stepsDirection == 0)
			stepsDirection = 1;

		if (stepsDirection)
			cameraY += 0.0025;
		else
			cameraY -= 0.0025;*/

	}

	if (GetAsyncKeyState('S') != 0) //key S
	{
		cameraZ -= headingZ * 0.5;
		cameraX -= headingX * 0.5;
		/*if (cameraY >= 0.03 && stepsDirection == 1)
			stepsDirection = 0;
		else if (cameraY <= 0 && stepsDirection == 0)
			stepsDirection = 1;

		if (stepsDirection)
			cameraY += 0.0025;
		else
			cameraY -= 0.0025;*/

	}
	if (GetAsyncKeyState('A') != 0) //Key A
	{
	/*	cameraZ -= -cos(cameraAng + 1.5705) * 0.5;
		cameraX -= sin(cameraAng + 1.5705)* 0.5;*/
		AirAng += 10;
	/*	if (cameraY >= 0.03 && stepsDirection == 1)
			stepsDirection = 0;
		else if (cameraY <= 0 && stepsDirection == 0)
			stepsDirection = 1;

		if (stepsDirection)
			cameraY += 0.0025;
		else
			cameraY -= 0.0025;*/

	}
	if (GetAsyncKeyState('D') != 0) //Key D
	{
		AirAng -= 10;
		/*cameraZ += -cos(cameraAng + 1.5705) * 0.5;
		cameraX += sin(cameraAng + 1.5705)* 0.5;*/
	/*	if (cameraY >= 0.03 && stepsDirection == 1)
			stepsDirection = 0;
		else if (cameraY <= 0 && stepsDirection == 0)
			stepsDirection = 1;

		if (stepsDirection)
			cameraY += 0.0025;
		else
			cameraY -= 0.0025;*/

	}
	if (GetAsyncKeyState(VK_SPACE) != 0) 
	{
		cameraY += 0.5;

	}
	if (GetAsyncKeyState(VK_CONTROL) != 0) 
	{
		cameraY -= 0.5;

	}

}


void myTimerFunc(int  val)
{
	
	snowEngine.update();	//Apply SnowEngine Physics
	HandleUserInteraction();
	glutPostRedisplay();
	glutTimerFunc(16, myTimerFunc, 16);
}


void myKeyboardFunc(unsigned char k, int x, int y)
{
	
}

void myMotionFunc(int a, int b)
{

}


void myPassiveMotionFunc(int a, int b)
{

	int	xpos = a;
	int ypos = b;
	deltax = xpos - 250;
	deltay = ypos - 250;
	
	glutWarpPointer(250, 250);

}


int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);	//Color mode: RGBA, Buffer: Double
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("LAB2 - OpenGL2D");

	glutDisplayFunc(myDisplayFunc);
	glutReshapeFunc(myReshapeFunc);
	glutTimerFunc(16, myTimerFunc, 16);
	glutKeyboardFunc(myKeyboardFunc);
	glutMotionFunc(myMotionFunc);
	glutPassiveMotionFunc(myPassiveMotionFunc);

	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	tex1 = loadTexture("Grass.jpg");
	tex2 = loadTexture("Rock.jpg");

	glEnable(GL_TEXTURE_2D);	//Use textures

	initializeHeightMap("hm.png");
glNewList(1, GL_COMPILE);
	renderHeightMap();
		glEndList();

		Airplane.loadModel("airplane.obj");
		glNewList(2, GL_COMPILE);
		Airplane.renderModel();
		glEndList();


	terrainTexture = loadTexture("terrain.jpg");
	snowEngine.initialize();
	glutMainLoop();
}