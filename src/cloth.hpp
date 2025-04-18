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
	public:
	float xWidth, yHeight; 
	int nXvertices, nYvertices;
	float structK, shearK, bendK;
	float dt, damping; 

	vector<vec3> positions; 
	vector<vec3> normals; 
	vector<vec3> forces; 
	vector<vec3> velocities;
	vector<ivec3> triangles;
	vector<ivec2> edges;

	float mass; 

	private:
	float structlenX, structlenY, shearlenX, shearlenY, bendlenX, bendlenY;
	Cloth()
	{

	} //default constructor does nothing. 

	Cloth(float xWidth, float yHeight, int nXvertices, int nYvertices, float structK = 1, float shearK = 0.3, float bendK = 0.05, float dt = 0.1, float damping = 0.1)
	{
		this->xWidth = xWidth;
		this->yHeight = yHeight;
		this->nXvertices = nXvertices;
		this->nYvertices = nYvertices;
		this->structK = structK;
		this->shearK = shearK;
		this->bendK = bendK;
		this->dt = dt;
		this->damping = damping;

		positions.resize(nXvertices * nYvertices);
		normals.resize(nXvertices * nYvertices);
		forces.resize(nXvertices * nYvertices);
		velocities.resize(nXvertices * nYvertices);

		setupVertices();
		mass = 1;
	}

	COL781::OpenGL::Object setupObject(COL781::OpenGL::Rasterizer &r);
	void setupVertices();
	void calculateForces();
	void update(float t, float g);
	
};

#endif