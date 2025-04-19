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

    intermediatePositions = positions; //initially they are the same.
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

void Cloth::constrain(int a, int b, int x, int y, float kDash, float deflen) //k is for spring constant, len is defaultLen
{
    //update forces for both a,b and x,y based on force for each other.    
    vec3 dir12 = intermediatePositions[x * nY + y] - intermediatePositions[a * nY + b];
    float length = glm::length(dir12);
    float stretch = length - deflen;
    dir12 = dir12/length; //normalizing.

    if(!isFixed[a*nY + b])
        intermediatePositions[a*nY + b] += 0.5f * kDash * stretch * (dir12);
    if(!isFixed[x*nY + y])
        intermediatePositions[x*nY + y] += -0.5f * kDash * stretch * (dir12);
    //updates the forces on both the particles due to the spring between them. EZ
}

void Cloth::updateConstraints(int solverIterations, float constrain_K, float deltaT)
{

    float kDash = 1 - pow(1-constrain_K, (1/(float)solverIterations));
    for(int iter = 0; iter < solverIterations; iter++)
    {
        for(int i = 0; i < nXvertices; i++)
        {
            for(int j = 0; j < nYvertices; j++)
            {
                //now we check upon its structural constraints. 
                 if(i != nXvertices - 1)
                {
                    constrain(i, j, i + 1, j, kDash, structlenX);
                }
                if(j != nYvertices - 1)
                {
                    constrain(i, j, i, j+1, kDash, structlenY); 
                }
            }
        }
    }

    //after this our constraints are held up. SO now we instead move on to setting up the velocities. 
    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            //now we check upon its structural constraints. 
            int cur = i*nY + j;
            if(isFixed[cur]) { continue; }
            //otherwise we update the velocities and the positions

            velocities[cur] = (intermediatePositions[cur] - positions[cur])/deltaT;
            positions[cur] = intermediatePositions[cur];
        }
    }
}


void Cloth::calculateForces(float g)
{
    //calculates the forces applied on each particle.

    //first we calculate the structural stretching forces. 
    std::fill(forces.begin(), forces.end(), glm::vec3(0,-g * mass,0)); //first get gravity on this
    float structuralK = (usingConstrains ) ? 0 : structK;
    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            //lets calculate structural force first. 
            if(true) //calculating structural forces ONLY if we are not using constraints to solve this. 
            {
                //the way we calculate force is that we calculate forces for the right and bottom ones only.
                if(i != nXvertices - 1)
                {
                    updateForce(i, j, i + 1, j, structuralK, structlenX, structDamp);
                }
                if(j != nYvertices - 1)
                {
                    updateForce(i, j, i, j+1, structuralK, structlenY, structDamp); 
                }
            } //just the structural forces for now. 

            //shear forces.
            if(true)
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

            if(true)
            //bend forces
            {
                if(i < nXvertices - 2)
                {
                    updateForce(i, j, i + 2, j, bendK, bendlenX, bendDamp);
                }
                if(j < nYvertices - 2)
                {
                    updateForce(i, j, i, j + 2, bendK, bendlenY, bendDamp);
                }
            }
        }
    }
}

void Cloth::update(float dt, float g)
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
            velocities[cur] += (dt) * forces[cur] / mass;
            //after updating the velocities, we can update the positions as well. 
            if(usingConstrains)
            intermediatePositions[cur] = positions[cur] + (dt) * velocities[cur];
            else 
            positions[cur]  += (dt) * velocities[cur];
        }
    }

    //now that we have the intermediate positions, we can iterate and update them based on our constraints. 
    if(usingConstrains) updateConstraints(25, 1, dt);
    // else positions = intermediatePositions;

    recalculateNormals();
}


void generateSphere(int m, int n, Sphere &sphereMesh, vec3 center, float radius) {
    // Generate vertices
    for (int j = 1; j < n; j++) { // Exclude poles
        float phi = M_PI * j / n;
        for (int i = 0; i < m; i++) {
            float theta = 2.0f * M_PI * i / m;
            float z = center.z + radius * cos(theta) * sin(phi);
            float x = center.x + radius * sin(theta) * sin(phi);
            float y = center.y + radius * cos(phi);
            sphereMesh.positions.emplace_back(x, y, z);
            sphereMesh.normals.emplace_back( glm::normalize(vec3(x - center.x, y - center.y, z - center.z)));
            //normals are just outward pointing from the center of the sphere. 
        }
    }

    // // Add poles
    // int northPoleIndex = sphereMesh.positions.size();
    // sphereMesh.positions.emplace_back(0.0f, 0.0f, 1.0f);
    // int southPoleIndex = sphereMesh.positions.size();
    // sphereMesh.positions.emplace_back(0.0f, 0.0f, -1.0f);

    int northPoleIndex = sphereMesh.positions.size();
    sphereMesh.positions.emplace_back(center.x, center.y + radius*1.0f, center.z);
    int southPoleIndex = sphereMesh.positions.size();
    sphereMesh.positions.emplace_back(center.x, center.y + radius*-1.0f, center.z);

    vector<vector<int>> faces;

    // Middle quads (excluding poles)
    for (int j = 0; j < n - 2; j++) { // stacks
        for (int i = 0; i < m; i++) { // slices
            int nextI = (i + 1) % m;
            int currRow = j * m;
            int nextRow = (j + 1) * m;

            // quad face, anticlockwise
            sphereMesh.triangles.push_back({
                currRow + i,
                nextRow + i,
                nextRow + nextI
            });
            sphereMesh.triangles.push_back({
                nextRow + nextI,
                currRow + nextI,
                currRow + i
            });
        }
    }

    // Top cap (fan around north pole)
    for (int i = 0; i < m; i++) {
        int nextI = (i + 1) % m;
        sphereMesh.triangles.push_back({
            northPoleIndex,
            i,
            nextI
        });
    }

    int bottomStart = (n - 2) * m;
    for (int i = 0; i < m; i++) {
        int nextI = (i + 1) % m;
        sphereMesh.triangles.push_back({
            southPoleIndex,
            bottomStart + nextI,
            bottomStart + i
        });
    }
}


Sphere::Sphere( int m, int n, float radius, vec3 center, vec3 color)
{
    this->radius = radius;
    this->collisionRadius = radius * 1.02;
    this->center = center;
    this->color = color;
    generateSphere(m, n, *this, center, radius);
}

COL781::OpenGL::Object Sphere::setupObject(COL781::OpenGL::Rasterizer &r)
{
    COL781::OpenGL::Object object = r.createObject();   
    vertexBuf = r.createVertexAttribs(object, 0, positions.size(), positions.data()); 
    normalBuf = r.createVertexAttribs(object, 1, normals.size(), normals.data()); 
    r.createTriangleIndices(object, triangles.size(), triangles.data());
    r.createEdgeIndices(object, edges.size(), edges.data());
    std::cout << "done this " << std::endl;
    return object;
}

