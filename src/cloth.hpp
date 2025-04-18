#ifndef CLOTH_HPP
#define CLOTH_HPP


#include "hw.hpp"
#include <glm/glm.hpp>
#include <vector>
// #include <iostream>
using namespace glm;
using namespace std;

class Cloth
{
	private:
	float structlenX, structlenY, shearlen, bendlenX, bendlenY;

	public:
	float xWidth, yHeight; 
	int nXvertices, nYvertices, nY;
	float structK, shearK, bendK, structDamp, shearDamp, bendDamp;
	float dt, damping; 
	COL781::OpenGL::AttribBuf vertexBuf, normalBuf;

	vector<vec3> positions; 
	vector<vec3> normals; 
	vector<vec3> forces; 
	vector<vec3> velocities;
	vector<ivec3> triangles;
	vector<ivec2> edges;
	vector<bool> isFixed; 

	float mass; 

	Cloth()
	{

	} //default constructor does nothing. 

	Cloth(float xWidth, float yHeight, int nXvertices, int nYvertices, float structK = 1000, float shearK = 100, float bendK = 10, float dt = 0.1, float structDamp = 50, float shearDamp = 15, float bendDamp = 0.1)
	{
		this->xWidth = xWidth;
		this->yHeight = yHeight;
		this->nXvertices = nXvertices;
		this->nYvertices = nYvertices;
		this->structK = structK;
		this->shearK = shearK;
		this->bendK = bendK;
		this->dt = dt;
		this->structDamp = structDamp;
		this->shearDamp = shearDamp;
		this->bendDamp = bendDamp;

		nY = nYvertices; //for easy access.
		positions.resize(nXvertices * nYvertices);
		normals.resize(nXvertices * nYvertices);
		forces.resize(nXvertices * nYvertices);
		velocities.resize(nXvertices * nYvertices);

		//by default we fix 2 vertices at the top.
		isFixed = vector<bool>(nXvertices * nYvertices, false);
		isFixed[0] = true; //just fixing the first one.
		isFixed[nYvertices - 1] = true; //and the last one as well.
		// for(int i = 0; i < nYvertices; i++)
		// {
		// 	isFixed[i] = true;
		// }
		setupVertices();
		mass = 1;
	}

	COL781::OpenGL::Object setupObject(COL781::OpenGL::Rasterizer &r);
	void setupVertices();
	void calculateForces(float g);
	void recalculateNormals();
	void updateForce(int a, int b, int x, int y, float k, float deflen, float dampK); //k is for spring constant, len is defaultLen
	void update(float t, float g);
	
};

#endif