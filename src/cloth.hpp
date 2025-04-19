#ifndef CLOTH_HPP
#define CLOTH_HPP


#include "hw.hpp"
#include <glm/glm.hpp>
#include <vector>
// #include <iostream>
using namespace glm;
using namespace std;



class Sphere
{
	public:
	float radius, collisionRadius;
	vec3 center;
	vec3 velocity;
	vec3 color;

	COL781::OpenGL::AttribBuf vertexBuf, normalBuf;
    std::vector<vec3> positions;
    std::vector<ivec3> triangles;
    std::vector<vec3> normals;
    std::vector<ivec2> edges; 
	Sphere() { } //default constructor does nothing.
	Sphere( int m, int n, float radius = 1, vec3 center = vec3(0,0,0), vec3 color = vec3(1,0.2,0.5));

	COL781::OpenGL::Object setupObject(COL781::OpenGL::Rasterizer &r);

};



class Cloth
{
	private:
	float structlenX, structlenY, shearlen, bendlenX, bendlenY;

	public:
	float xWidth, yHeight; 
	int nXvertices, nYvertices, nY;
	float structK, shearK, bendK, structDamp, shearDamp, bendDamp;
	float damping; 
	COL781::OpenGL::AttribBuf vertexBuf, normalBuf;

	vector<vec3> positions; 
	vector<vec3> intermediatePositions; 
	vector<vec3> normals; 
	vector<vec3> forces; 
	vector<vec3> velocities;
	vector<ivec3> triangles;
	vector<ivec2> edges;
	vector<bool> isFixed; 

	float mass; 
	bool usingConstrains = true; 
	Cloth()
	{

	} //default constructor does nothing. 

	Cloth(float xWidth, float yHeight, int nXvertices, int nYvertices, float structK = 1000, float shearK = 100, float bendK = 10, float structDamp = 50, float shearDamp = 15, float bendDamp = 0.1, float mass = 1)
	{
		this->xWidth = xWidth;
		this->yHeight = yHeight;
		this->nXvertices = nXvertices;
		this->nYvertices = nYvertices;
		this->structK = structK;
		this->shearK = shearK;
		this->bendK = bendK;
		this->structDamp = structDamp;
		this->shearDamp = shearDamp;
		this->bendDamp = bendDamp;
		this->mass = mass;

		nY = nYvertices; //for easy access.
		positions.resize(nXvertices * nYvertices);
		intermediatePositions.resize(nXvertices * nYvertices);
		triangles.reserve(2 * (nXvertices - 1) * (nYvertices - 1)); //2 triangles for each quad.
		edges.reserve(2 * (nXvertices - 1) * (nYvertices - 1)); //2 edges for each quad.
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
	}

	COL781::OpenGL::Object setupObject(COL781::OpenGL::Rasterizer &r);
	void setupVertices();
	void calculateForces(float g, vector<Sphere*> spheres);
    void handleCollisionForces(int i,int j, vector<Sphere*> spheres);
	void recalculateNormals();
	void updateForce(int a, int b, int x, int y, float k, float deflen, float dampK); //k is for spring constant, len is defaultLen
	void constrain(int a, int b, int x, int y, float k, float deflen, vector<Sphere*> spheres); //k is for spring constant, len is defaultLen
	void update(float dt, float g, vector<Sphere*> spheres);
	void updateConstraints(int solverIterations = 10, float constrain_K = 0.8, float deltaT = 0.005, vector<Sphere*> spheres = {});
	// void collisionDetection();
};

void handleCollisions(Cloth &c, Sphere &s, float coeff); //coeff is the coefficient of restitution.
#endif