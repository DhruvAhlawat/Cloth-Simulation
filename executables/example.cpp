#include "camera.hpp"
#include "cloth.hpp"
#include <iostream>

using namespace COL781;
namespace GL = COL781::OpenGL;
using namespace glm;
using namespace std;

GL::Rasterizer r;
GL::ShaderProgram program;
float gravity = 9.8f;

const int nv = 4, nt = 2, ne = 4;
vec3 vertices[nv];
vec3 normals[nv];
ivec3 triangles[nt];
ivec2 edges[ne];

GL::Object object, planeObject;
vector<GL::Object> sphereObjects;
GL::AttribBuf vertexBuf, normalBuf;

CameraControl camCtl;

Cloth cloth;
Plane p;
vector<Sphere*> spheres;

bool shadeSphereEdges = true; //turn off for faster computation.


void initializePlane()
{
	p = Plane(-2, vec3(0,1,0), vec3(0.5, 0.5, 0.5), 0.1, 0.02);
	planeObject = p.setupObject(r);
}

void initializeCloth()
{
	//Cannot set different resolution for x and y. Must keep both the same. 
	//probs some positional calculation requirement missing. 

	int vertices = 31;
	// cloth = Cloth(1, 1, vertices, 31, 6000, 100, 10, 20, 10, 1, 1);
	cloth = Cloth(1, 1, vertices, vertices, 6000, 1000, 100, 80, 30, 10, 1);
	object = cloth.setupObject(r);
}

void initializeSphere()
{
	//spheres.push_back(new Sphere(20, 20, 0.15, vec3(0, -0.8, 0.5), vec3(1, 0.2, 0.5)));
	spheres.push_back(new Sphere(20, 20, 0.15, vec3(0, -0.8, 0.5), vec3(1, 0.2, 0.5)));
	spheres[0]->velocity = vec3(0.0, 0, 0);
	sphereObjects.push_back(spheres[0]->setupObject(r));

	spheres.push_back(new Sphere(20,20, 0.1, vec3(-2, -0.5, 0.5), vec3(0.3, 0.9, 0.75), 0.9, 0.5));
	spheres[1]->velocity = vec3(2,0,0);
	sphereObjects.push_back(spheres[1]->setupObject(r));
}


void initializeScene1() 
{
	initializeCloth();
	initializeSphere();
}

void initializeDrapeScene()
{
	int vertices = 31;
	cloth = Cloth(1, 1, vertices, 31, 6000, 100, 10, 20, 10, 1, 1);
	object = cloth.setupObject(r);
	
	//unfixing the cloth.
	cloth.isFixed[0] = false; cloth.isFixed[cloth.nYvertices - 1] = false;	

	spheres.push_back(new Sphere(20,20, 0.3, vec3(0.5, -0.5, 0.5), vec3(0.3, 0.9, 0.75), 0.01, 0.9));
	// spheres[0]->velocity = vec3(0.8,0,0);
	spheres[0]->angularVelocity = vec3(0,10,0);
	sphereObjects.push_back(spheres[0]->setupObject(r));
}

void update(float dt)
{
	//we move each sphere.
	for(int i = 0; i < spheres.size(); i++)
	{
		spheres[i]->update(dt, shadeSphereEdges);
		r.updateVertexAttribs(spheres[i]->vertexBuf, spheres[i]->positions.size(), spheres[i]->positions.data());
		if(shadeSphereEdges)
			r.updateVertexAttribs(spheres[i]->normalBuf, spheres[i]->normals.size(), spheres[i]->normals.data());
	}
	r.updateVertexAttribs(p.vertexBuf, p.positions.size(), p.positions.data());
	cloth.update(dt, gravity, spheres, p);  //also pass it the spheres that it will collide with.

	r.updateVertexAttribs(cloth.vertexBuf, cloth.positions.size(), cloth.positions.data());
	r.updateVertexAttribs(cloth.normalBuf, cloth.normals.size(), cloth.normals.data());
}

void print_fps(float deltaT)
{
	cout << "\r " << round((1/deltaT)*100)/100 << "     \r";
}

int main() {
	int width = 640, height = 480;
	if (!r.initialize("Animation", width, height)) {
		return EXIT_FAILURE;
	}
	camCtl.initialize(width, height);
	camCtl.camera.setCameraView(vec3(0.5, -0.5, 1.5), vec3(0.5, -0.5, 0.0), vec3(0.0, 1.0, 0.0));
	program = r.createShaderProgram(
		r.vsBlinnPhong(),
		r.fsBlinnPhong()
	);

	// initializeScene();
	// initializeScene1();
	initializePlane();
	initializeDrapeScene();
	glm::mat4 identityMat = glm::mat4(1.0);

    glm::vec3 orange(1.0f, 0.6f, 0.2f);
    glm::vec3 white(1.0f, 1.0f, 1.0f);

	float last = SDL_GetTicks64()*1e-3;
	while (!r.shouldQuit()) 
	{
		float cur = SDL_GetTicks64()*1e-3;
        float deltaT = cur - last;
		last = cur;
		// updateScene(t);
		// cout << deltaT << endl;
		print_fps(deltaT);
		update(0.005);
		camCtl.update();
		Camera &camera = camCtl.camera;

		r.clear(vec4(0.4, 0.4, 0.4, 1.0));
		r.enableDepthTest();
		r.useShaderProgram(program);

		r.setUniform(program, "model", identityMat);
		r.setUniform(program, "view", camera.getViewMatrix());
		r.setUniform(program, "projection", camera.getProjectionMatrix());
		r.setUniform(program, "lightPos", camera.position);
		r.setUniform(program, "viewPos", camera.position);
		r.setUniform(program, "lightColor", vec3(1.0f, 1.0f, 1.0f));

		r.setupFilledFaces();
        r.setUniform(program, "ambientColor", 0.2f*white);
        r.setUniform(program, "extdiffuseColor", 0.9f*orange);
        r.setUniform(program, "intdiffuseColor", 0.4f*orange);
        r.setUniform(program, "specularColor", 0.6f*white);
        r.setUniform(program, "phongExponent", 20.f);
		r.drawTriangles(object);

		r.setupFilledFaces();
        r.setUniform(program, "ambientColor", 0.2f*white);
        r.setUniform(program, "extdiffuseColor", 0.9f*orange);
        r.setUniform(program, "intdiffuseColor", 0.4f*orange);
        r.setUniform(program, "specularColor", 0.6f*white);
        r.setUniform(program, "phongExponent", 20.f);
		r.drawTriangles(planeObject);

		
		for(int cursphere = 0; cursphere < spheres.size(); cursphere++)
		{
			r.setupFilledFaces(); 
			r.setUniform(program, "ambientColor", 0.2f*white);
			r.setUniform(program, "phongExponent", 20.f);
			r.setUniform(program, "extdiffuseColor", 0.9f*spheres[cursphere]->color);
			r.setUniform(program, "intdiffuseColor", 0.4f*spheres[cursphere]->color);
			r.setUniform(program, "specularColor", 0.7f*white + 0.2f*spheres[cursphere]->color);
			r.drawTriangles(sphereObjects[cursphere]);

			r.setupWireFrame();

        	glm::vec3 black(0.0f, 0.0f, 0.0f);
        	r.setUniform(program, "ambientColor", black);
        	r.setUniform(program, "extdiffuseColor", black);
        	r.setUniform(program, "intdiffuseColor", black);
        	r.setUniform(program, "specularColor", black);
        	r.setUniform(program, "phongExponent", 0.f);
			r.drawEdges(sphereObjects[cursphere]);
		}

		r.setupWireFrame();
        glm::vec3 black(0.0f, 0.0f, 0.0f);
        r.setUniform(program, "ambientColor", black);
        r.setUniform(program, "extdiffuseColor", black);
        r.setUniform(program, "intdiffuseColor", black);
        r.setUniform(program, "specularColor", black);
        r.setUniform(program, "phongExponent", 0.f);
		r.drawEdges(object);

		r.show();
	}
}
