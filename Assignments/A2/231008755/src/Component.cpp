//
//  Component.cpp
//  A2
//
//  Created by David-Tyler Ighedosa on 2/20/24.
//

#include "Component.hpp"

#include "GLSL.h"
#include "MatrixStack.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <map>

using namespace std;
using namespace glm;

Component::Component(glm::vec3 jointTranslateWRTParent, glm::vec3 meshTranslateWRTJoint, glm::vec3 currJointAngles, glm::vec3 scaleXYZ)
{
    jointTranslation = jointTranslateWRTParent;
    jointAngles = currJointAngles;
    meshTranslation = meshTranslateWRTJoint;
    scalingFactors = scaleXYZ;
}

void Component::draw(MatrixStack& MV, map<string,GLint> unifIDs, int indCount, double t) {
    float RotationSpeed = float(M_PI/2); // degrees per second
    float RotationAngle = RotationSpeed * t;
    
    MV.pushMatrix();
        
    
        MV.translate(jointTranslation); // Where is the components's joint with respect to the parent's joint?
        
        
        MV.rotate(jointAngles.x, glm::vec3(1.0f, 0.0f, 0.0f)); // This rotation applies to component
        MV.rotate(jointAngles.y, glm::vec3(0.0f, 1.0f, 0.0f)); // This rotation applies to component
        MV.rotate(jointAngles.z, glm::vec3(0.0f, 0.0f, 1.0f)); // This rotation applies to component
    
        
        MV.pushMatrix();
            if(permaRotate == true){
                MV.rotate(RotationAngle, glm::vec3(1.0f, 0.0f, 0.0f)); //
                
            }
            
            
            
            MV.pushMatrix();
                
                MV.scale(glm::vec3(0.75f, 0.75f, 0.75f));
                glUniformMatrix4fv(unifIDs["MV"], 1, GL_FALSE, value_ptr(MV.topMatrix()));
                glDrawArrays(GL_TRIANGLES, 0, indCount);
            MV.popMatrix();
    
            MV.translate(meshTranslation); // Where is the components's mesh with respect to the components's joint?
            if (selected == true) {
                float scaleChange = 1.0 + float(0.08/2) + (float(0.08/2)*(sin(2 * M_PI * 2 * t)));
                glm::vec3 dynamicScalingFactors = scalingFactors * (scaleChange);

                MV.scale(dynamicScalingFactors);
            }
            else{
                MV.scale(scalingFactors);
            }
            
            glUniformMatrix4fv(unifIDs["MV"], 1, GL_FALSE, value_ptr(MV.topMatrix()));
            glDrawArrays(GL_TRIANGLES, 0, indCount);
        MV.popMatrix();
    
        
    
        // Draw children
    
        for (Component* child : children) {
               child->draw(MV, unifIDs, indCount, t);
        }
    
    MV.popMatrix();
    
}

void Component::addChild(Component* child) {
    children.push_back(child);
}

void Component::setJointAngles(glm::vec3 angles) {
    jointAngles = angles;
}

glm::vec3 Component::getJointAngles() {
    return jointAngles;
}

glm::vec3 Component::getJointTranslations() {
    return jointTranslation;
}

std::vector<Component*> Component::getChildren() {
    return children;
}

void Component::setScalingFactors(glm::vec3 scales) {
    scalingFactors = scales;
}

void Component::setSelected(bool isSelected) {
    selected = isSelected;
}

bool Component::isSelected() const {
    return selected;
}

void Component::setPermaRotate(bool perma) {
    permaRotate = perma;
}
