#include "cloth.hpp"
#include <iostream>

std::vector<glm::vec3> createPlaneVertices(const glm::vec3& normal, float offset, float sideLength) {
    // Ensure the normal is normalized
    glm::vec3 unitNormal = glm::normalize(normal);
    
    // Calculate a point on the plane
    // For plane equation n·x = d, a point on the plane is d*n/|n|^2
    // Since we've normalized the normal, this simplifies to d*n
    glm::vec3 pointOnPlane = offset * unitNormal;
    
    // Find a vector perpendicular to the normal
    // We need any vector not parallel to normal to find a perpendicular vector
    glm::vec3 temp = (std::abs(unitNormal.x) < 0.9f) ? 
        glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    
    // First perpendicular direction (will be one side of our square)
    glm::vec3 side1 = glm::normalize(glm::cross(unitNormal, temp));
    
    // Second perpendicular direction (the other side of our square)
    glm::vec3 side2 = glm::cross(unitNormal, side1);
    
    // Ensure side2 is normalized
    side2 = glm::normalize(side2);
    
    // Scale the sides to the desired length
    side1 *= sideLength / 2.0f;
    side2 *= sideLength / 2.0f;
    
    // Calculate the four vertices of the plane, centered at pointOnPlane
    std::vector<glm::vec3> vertices;
    vertices.push_back(pointOnPlane - side1 - side2);  // Bottom-left
    vertices.push_back(pointOnPlane + side1 - side2);  // Bottom-right
    vertices.push_back(pointOnPlane + side1 + side2);  // Top-right
    vertices.push_back(pointOnPlane - side1 + side2);  // Top-left
    
    return vertices;
}
COL781::OpenGL::Object Plane::setupObject(COL781::OpenGL::Rasterizer &r)
{
    COL781::OpenGL::Object object = r.createObject();   
    vertexBuf = r.createVertexAttribs(object, 0, positions.size(), positions.data()); 
    normalBuf = r.createVertexAttribs(object, 1, normals.size(), normals.data()); 
    r.createTriangleIndices(object, triangles.size(), triangles.data());
    r.createEdgeIndices(object, edges.size(), edges.data());
    return object;
}

Plane::Plane(float offset, vec3 normal, vec3 color, float coeff_friction , float coeff_restitution)
{
    this->offset = offset;
    this->normal = normal;
    this->color = color;
    this->coeff_friction = coeff_friction;
    this->coeff_restitution = coeff_restitution;

    positions = createPlaneVertices(normal, offset, 1.0f); // Create a square plane with side length 1.0
    normals = std::vector<vec3>(4, normal); // All normals are the same for a flat plane
    triangles = {ivec3(0, 1, 2), ivec3(0, 2, 3)}; // Two triangles to form the square
    edges = {ivec2(0, 1), ivec2(1, 2), ivec2(2, 3), ivec2(3, 0)}; // Edges of the square
}

void handleCollisions(Cloth &c, Sphere &s, float coeff) //coeff is the coefficient of restitution.
{
    //detect collisions between c and s and appropriately change the positions of the cloth vertices. 
    for(int i = 0; i < c.nYvertices; i++)
    {
        for(int j = 0; j < c.nXvertices; j++)
        {
            int cur = i*c.nY + j; 
            vec3 curpos = c.intermediatePositions[cur];
            vec3 spherepos = s.center;            
            vec3 diff = curpos - spherepos;

            float dist = glm::length(diff);
            vec3 normal = diff/dist;
            if(dist <= s.collisionRadius)
            {
                //we have a collision. now we will compute the velocities along the normal direction. 
                float sphereSpeed = glm::dot(s.velocity, normal);  //we dont consider the angular velocity here as that is perpendicular to the normal.
                float clothSpeed = glm::dot(c.velocities[cur], normal);
                float relativeSpeed = sphereSpeed - clothSpeed;  //the relative speed along the normal.  
                if(relativeSpeed > 0)
                { //assume no collision here if velocities are going away in the normal direction.
                    float newRelativeSpeed = relativeSpeed * coeff; //this is the new relative velocity that we should have. 
                    //therefore the new cloth speed should be.
                    float newClothSpeed = sphereSpeed + newRelativeSpeed;
                    c.velocities[cur] += (-clothSpeed + newClothSpeed) * normal; 
                    //this accounts for hte normal force that is instantaneously applied by the ball on this cloth. 
                
                    //we also need to update finally the frictional force that it would feel. 
                }
                float offset = 0.001f;
                c.intermediatePositions[cur] += (s.collisionRadius - dist + offset) * normal; //this is the new position of the cloth vertex.
            }
        }
    }

}

void handlePlaneCollisions(Cloth &c, Plane &p, float coeff)
{
     //detect collisions between c and s and appropriately change the positions of the cloth vertices. 
    for(int i = 0; i < c.nYvertices; i++)
    {
        for(int j = 0; j < c.nXvertices; j++)
        {
            int cur = i * c.nY + j;
            vec3 curpos = c.intermediatePositions[cur];
            
            float prevPlanePos = glm::dot(p.normal, c.positions[cur]); //positions is the unupdated position so it is previous timestep position.
            float planePos = glm::dot(p.normal, curpos);
            
            if(planePos <= p.offset) //then we assume it is in contact. it shoudl always be above p.offset since it is an infinite plane not a finite one.
            {
                //we have a collision. now we will compute the velocities along the normal direction. 
                float clothSpeed = glm::dot(c.velocities[cur], p.normal);
                float newClothSpeed = -clothSpeed * coeff; //this is the new relative velocity that we should have. 
                c.velocities[cur] += (-clothSpeed + newClothSpeed) * p.normal; 
                //this accounts for hte normal force that is instantaneously applied by the ball on this cloth. 
                c.intermediatePositions[cur] += (p.offset - planePos) * p.normal; //this is the new position of the cloth vertex.
            }
        }
    }
}

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
    // std::cout << "done this " << std::endl;
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

void Cloth::constrain(int a, int b, int x, int y, float kDash, float deflen, vector<Sphere*> spheres) //k is for spring constant, len is defaultLen
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

// void Cloth::updateConstraints(int solverIterations, float constrain_K, float deltaT, vector<Sphere*> spheres, Plane &p)
void Cloth::updateConstraints(vector<Sphere*> spheres,Plane &p, int solverIterations, float constrain_K, float deltaT)
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
                    constrain(i, j, i + 1, j, kDash, structlenX, spheres);
                }
                if(j != nYvertices - 1)
                {
                    constrain(i, j, i, j+1, kDash, structlenY, spheres); 
                }
            }
        }
        // we handle collisions as well as do this
        //we will also now handle the collisions in this loop itself. 
        for(int i = 0; i < spheres.size(); i++)
        {
            handleCollisions(*this, *spheres[i], spheres[i]->coeff_restitution);
        
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

void Cloth::handleCollisionForces(int i, int j, vector<Sphere*> spheres, Plane &p)
{
    int cur = i*nY + j; 
    vec3 curpos = intermediatePositions[cur];
    for(int cursphere = 0; cursphere < spheres.size(); cursphere++)
    {
        Sphere *s = spheres[cursphere];
        vec3 spherepos = s->center;            
        vec3 diff = curpos - spherepos;

        float dist = glm::length(diff);
        vec3 normal = diff/dist;

        if(dist <= s->collisionRadius + 0.001)
        {
            //then we need to apply 2 types of forces. 1 is the normal force. 
            //the actual normal force depends upon our forces in this direction. 
            //we will simply take the component of them along the normal and reduce it to 0.
            vec3 normalForce = - glm::dot(forces[cur], normal) * normal;
            forces[cur] += normalForce; //removes the force along this component. 
            
            //now if there is relative velocity, then we also need to apply frictional force. 

            //we also consider the angular velocity of the sphere. 
            vec3 relvel = s->velocity + glm::cross(s->angularVelocity, s->radius * normal) - velocities[cur];
            relvel = relvel - glm::dot(relvel, normal) * normal; //the tangential component of the relative velocity
            if(relvel != vec3(0))
            {
                vec3 relvel_dir = glm::normalize(relvel);
                // cout << "relvel is : " << relvel.x << " , " << relvel.y << " , " << relvel.z << endl; 
                // //Frictional force is
                // cout << "frictional coeffecient is " << s->coeff_friction << endl;
                vec3 frictional_force = s->coeff_friction * relvel_dir * glm::length(normalForce); 
                // cout << "frictional force is hence : " << frictional_force.x << " , " << frictional_force.y << " , " << frictional_force.z << endl; 
                forces[cur] += frictional_force;
            }
            //else    
            //relative velocity tangential to normal is 0 so we do not apply any frictional force.
        }

    }

    //now we check collisions with the plane. 
    float planepos = glm::dot(p.normal, curpos); 
    float prevplanepos = glm::dot(p.normal, positions[cur]); //positions is the unupdated position so it is previous timestep position.
    if(-p.gap <= planepos && planepos < p.gap)  //then we consider it a collision with the plane.
    {
        //if we were previously above it and in this frame we are down, then we reset it to 0.
 //we will simply take the component of them along the normal and reduce it to 0.
        
        float forceAlongNormal = glm::dot(forces[cur], p.normal);
        vec3 normalForce = vec3(0);
        if(forceAlongNormal < 0) //if the force is inwards on the plane.
            normalForce = - glm::dot(forces[cur], p.normal) * p.normal;
        forces[cur] += normalForce; //removes the force along this component. 

        vec3 relvel = -velocities[cur]; //assuming plane is stationary.
        relvel = relvel - glm::dot(relvel, p.normal) * p.normal; //the tangential component of the relative velocity
        if(relvel != vec3(0))
        {
            vec3 relvel_dir = glm::normalize(relvel);
            vec3 frictional_force = p.coeff_friction * relvel_dir * glm::length(normalForce); 
            // cout << "frictional force is hence : " << frictional_force.x << " , " << frictional_force.y << " , " << frictional_force.z << endl; 
            forces[cur] += frictional_force;
        }
    }

}

void Cloth::calculateForces(float g, vector<Sphere*> spheres, Plane &p)
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
            if(true) //calculating structural forces ONLY if we are not using constraints to solve this as k = 0 when constraints are used. 
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

    for(int i = 0; i < nXvertices; i++)
    {
        for(int j = 0; j < nYvertices; j++)
        {
            handleCollisionForces(i, j, spheres, p); //now we handle the collision forces at the end. 
        }
    }
}

void Cloth::update(float dt, float g, vector<Sphere*> spheres, Plane &p)
{
    calculateForces(g, spheres, p);
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
    if(usingConstrains) updateConstraints(spheres, p, 25, 1, dt);
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
    std::set<pair<int,int>> edgeSet;
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

            edgeSet.insert(make_pair(std::min(currRow + i,nextRow + i), std::max(currRow+i, nextRow + i)));
            edgeSet.insert(make_pair(std::min(nextRow + i,nextRow + nextI), std::max(nextRow+i, nextRow + nextI)));
            edgeSet.insert(make_pair(std::min(currRow + nextI,nextRow + nextI), std::max(currRow+nextI, nextRow + nextI)));
            edgeSet.insert(make_pair(std::min(currRow + nextI,currRow+ i), std::max(currRow+nextI, currRow + i)));
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
        edgeSet.insert({std::min(northPoleIndex, i), std::max(northPoleIndex,i)});
        edgeSet.insert({std::min(northPoleIndex, nextI), std::max(northPoleIndex,nextI)});
        edgeSet.insert({std::min(i, nextI), std::max(i,nextI)});
    }

    int bottomStart = (n - 2) * m;
    for (int i = 0; i < m; i++) {
        int nextI = (i + 1) % m;
        sphereMesh.triangles.push_back({
            southPoleIndex,
            bottomStart + nextI,
            bottomStart + i
        });
        edgeSet.insert({std::min(southPoleIndex, bottomStart + i), std::max(southPoleIndex,bottomStart + i)});
        edgeSet.insert({std::min(southPoleIndex, bottomStart + nextI), std::max(southPoleIndex,bottomStart + nextI)});
        edgeSet.insert({std::min(bottomStart + i, bottomStart + nextI), std::max(bottomStart + i,bottomStart + nextI)});
    }

    for(auto x : edgeSet)
    {
        sphereMesh.edges.push_back(ivec2(x.first, x.second));
    }

}

Sphere::Sphere( int m, int n, float radius, vec3 center, vec3 color, float coeff_restitution, float coeff_friction)
{
    this->radius = radius;
    this->collisionRadius = radius + std::min(0.01, radius * 0.02);
    this->center = center;
    this->color = color;
    this->coeff_friction = coeff_friction;
    this->coeff_restitution = coeff_restitution;
    generateSphere(m, n, *this, center, radius);
    this->velocity = vec3(0); //initially.
}

COL781::OpenGL::Object Sphere::setupObject(COL781::OpenGL::Rasterizer &r)
{
    COL781::OpenGL::Object object = r.createObject();   
    vertexBuf = r.createVertexAttribs(object, 0, positions.size(), positions.data()); 
    normalBuf = r.createVertexAttribs(object, 1, normals.size(), normals.data()); 
    r.createTriangleIndices(object, triangles.size(), triangles.data());
    r.createEdgeIndices(object, edges.size(), edges.data());
    // std::cout << "done this " << std::endl;
    return object;
}

void Sphere::update(float dt, bool shadeSphereEdges)
{

	vec3 offset = this->velocity * dt;
    
	//then we must also update all the vertexpositions.
	for(int j = 0; j < this->positions.size(); j++)
	{
        if(shadeSphereEdges)
        {
            vec3 radiusVector = positions[j] - center;
            vec3 angularOffset = glm::cross(angularVelocity, radiusVector) * dt; 
            this->positions[j] += angularOffset;
            this->positions[j] = center + glm::normalize(positions[j] - center) * radius;
            normals[j] = glm::normalize(positions[j] - center);
        }
        this->positions[j] += offset; //linear velocity offset
	}
    this->center += offset; //update the center later by the linear velocity offset. 
    
    //this updates all vertex positions.

}