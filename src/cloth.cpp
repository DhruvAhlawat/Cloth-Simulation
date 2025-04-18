#include "cloth.hpp"
#include <iostream>
void Cloth::setupVertices()
{
    structlenX = xWidth/(nXvertices - 1);
    structlenY = yHeight/(nYvertices - 1);

    shearlen = sqrt(structlenX * structlenX + structlenY * structlenY);

    bendlenX = 2 * structlenX;
    bendlenY = 2 * structlenY;


    for (int i = 0; i < nXvertices; i++)
    {
        for (int j = 0; j < nYvertices; j++)
        {
            float x = (float)i * structlenX;
            float y = (float)j * structlenY;
            positions[i * nYvertices + j] = vec3(x, 0, y);
            normals[i * nYvertices + j] = vec3(0.0f, 0.0f, 1.0f); //initial normals. 
            forces[i * nYvertices + j] = vec3(0.0f, 0.0f, 0.0f); //initial forces. 
            velocities[i * nYvertices + j] = vec3(0.0f, 0.0f, 0.0f); //initial velocities. 
            // std::cout << "created vertex " << i * nYvertices + j << " at position " << x << ", " << y << std::endl;
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
    vertexBuf = r.createVertexAttribs(object, 0, positions.size(), positions.data()); 
    normalBuf = r.createVertexAttribs(object, 1, normals.size(), normals.data()); 
    r.createTriangleIndices(object, triangles.size(), triangles.data());
    r.createEdgeIndices(object, edges.size(), edges.data());
    std::cout << "done this " << std::endl;
    return object;
}


void Cloth::recalculateNormals()
{
    //this function will recalculate the normals based on the triangles. 
    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            normals[i * nYvertices + j] = vec3(0.0f, 0.0f, 0.0f);
        }
    }

    for(auto &tri : triangles)
    {
        vec3 v1 = positions[tri[1]] - positions[tri[0]];
        vec3 v2 = positions[tri[2]] - positions[tri[0]];
        vec3 normal = glm::normalize(glm::cross(v1, v2));
        normals[tri[0]] += normal;
        normals[tri[1]] += normal;
        normals[tri[2]] += normal;
    }

    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            normals[i * nYvertices + j] = glm::normalize(normals[i * nYvertices + j]);
        }
    }
}


void Cloth::updateForce(int a, int b, int x, int y, float k, float deflen, float dampK) //k is for spring constant, len is defaultLen
{
    //update forces for both a,b and x,y based on force for each other.    
    vec3 dir12 = positions[x * nY + y] - positions[a * nY + b];
    float length = glm::length(dir12);
    float stretch = length - deflen;
    dir12 = dir12/length; //normalizing.

    vec3 relative_velocity = velocities[x * nY + y] - velocities[a * nY + b];
    vec3 damping_force = dampK * glm::dot(relative_velocity, dir12) * dir12;

    forces[a * nY + b] += k * dir12 * stretch  + damping_force; //force on a,b
    forces[x * nY + y] += k * -dir12 * stretch - damping_force; //force on x,y

    //updates the forces on both the particles due to the spring between them. EZ
}


void Cloth::calculateForces(float g)
{
    //calculates the forces applied on each particle.

    //first we calculate the structural stretching forces. 
    std::fill(forces.begin(), forces.end(), glm::vec3(0,-g * mass,0)); //first get gravity on this

    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            //lets calculate structural force first. 
            {
                //the way we calculate force is that we calculate forces for the right and bottom ones only.
                if(i != nXvertices - 1)
                {
                    updateForce(i, j, i + 1, j, structK, structlenX, structDamp);
                }
                if(j != nYvertices - 1)
                {
                    updateForce(i, j, i, j+1, structK, structlenY, structDamp); 
                }
            } //just the structural forces for now. 

            {
                if(i != nXvertices - 1 && j != nYvertices - 1)
                {
                    updateForce(i, j, i + 1, j + 1, shearK, shearlen, shearDamp);
                }
                if(i != nXvertices - 1 && j != 0)
                {
                    updateForce(i, j, i + 1, j - 1, shearK, shearlen, shearDamp);
                }
            }
        }
    }
}

void Cloth::update(float t, float g)
{
    calculateForces(g);
    //then after we have the forces ready, we calculate the updated positions.

    //we need to consider their velocities as well as their positoins. 
    //first we update their velocities, then update their positions based on these new velocities.
    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            
            int cur = i*nY + j;
            if(isFixed[cur]) 
            {
                continue; //not updating fixed ones. 
            }
            velocities[cur] += (t) * forces[cur] / mass;
            //after updating the velocities, we can update the positions as well. 
            positions[cur] += (t) * velocities[cur];
        }
    }

    recalculateNormals();

}