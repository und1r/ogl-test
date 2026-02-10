#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include <vector>
#include "../math/GeometryUtils.h" // For Vertex struct
#include "../render/ShaderProgram.h"

namespace RenderUtils {
    unsigned int createVaoFromVertices(const std::vector<Vertex>& vertices);
    unsigned int createAxisVAO();
    void drawAxis(unsigned int vao, const ShaderProgram& shader);
}

#endif
