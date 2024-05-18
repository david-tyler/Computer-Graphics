//
//  Component.hpp
//  A2
//
//  Created by David-Tyler Ighedosa on 2/20/24.
//

#ifndef Component_hpp
#define Component_hpp

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

class Component {
public:
    Component(glm::vec3 jointTranslateWRTParent, glm::vec3 meshTranslateWRTJoint, glm::vec3 currJointAngles, glm::vec3 scaleXYZ);
    void draw(MatrixStack& MV, map<string,GLint> unifIDs, int indCount, double t);
    void addChild(Component* child);
    void setJointAngles(glm::vec3 angles);
    void setScalingFactors(glm::vec3 scales);
    glm::vec3 getJointAngles();
    void setSelected(bool selected);
    bool isSelected() const;
    std::vector<Component*> getChildren();
    glm::vec3 getJointTranslations();
    void setPermaRotate(bool perma);

private:
    glm::vec3 jointTranslation;
    glm::vec3 jointAngles;
    glm::vec3 meshTranslation;
    glm::vec3 scalingFactors;
    std::vector<Component*> children;
    bool selected;
    bool permaRotate;
    
};

#endif /* Component_hpp */
