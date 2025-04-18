#include "cloth.hpp"
#include <iostream>
void Cloth::setupVertices()
{
    structlenX = xWidth/(nXvertices - 1);
    for (int i = 0; i < nXvertices; i++)
    {
        for (int j = 0; j < nYvertices; j++)
        {
            float x = (float)i * structlenX;
            float y = (float)j / (nYvertices - 1) * yHeight;
            positions[i * nYvertices + j] = vec3(x, 0, y);
            normals[i * nYvertices + j] = vec3(0.0f, 0.0f, 1.0f); //initial normals. 
            forces[i * nYvertices + j] = vec3(0.0f, 0.0f, 0.0f); //initial forces. 
            velocities[i * nYvertices + j] = vec3(0.0f, 0.0f, 0.0f); //initial velocities. 
            std::cout << "created vertex " << i * nYvertices + j << " at position " << x << ", " << y << std::endl;
            if(i != nXvertices - 1)
            {
                edges.push_back(ivec2(i * nYvertices + j, (i + 1) * nYvertices + j));
            }
            if(j != nYvertices - 1)
            {
                edges.push_back(ivec2(i * nYvertices + j, i * nYvertices + (j+1)));
            }

            if(i != 0 && j != 0)
            {
                triangles.push_back(ivec3(i * nYvertices + j, (i - 1) * nYvertices + j, i * nYvertices + (j - 1)));
            }
            if(i != nXvertices - 1 && j != nYvertices - 1)
            {
                triangles.push_back(ivec3( i * nYvertices + (j+1),i * nYvertices + j, (i+1) * nYvertices + j));
            }
        }
    }
}

COL781::OpenGL::Object Cloth::setupObject(COL781::OpenGL::Rasterizer &r)
{
    COL781::OpenGL::Object object = r.createObject();   
    r.createVertexAttribs(object, 0, positions.size(), positions.data()); 
    r.createVertexAttribs(object, 1, normals.size(), normals.data()); 
    r.createTriangleIndices(object, triangles.size(), triangles.data());
    r.createEdgeIndices(object, edges.size(), edges.data());
    std::cout << "done this " << std::endl;
    return object;
}

void Cloth::calculateForces()
{
    //calculates the forces applied on each particle.

    //first we calculate the structural stretching forces. 

}
void Cloth::update(float t, float g)
{

}