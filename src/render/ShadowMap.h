#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

struct ShadowMap {
    unsigned int depth_map_fbo;
    unsigned int depth_map_texture;
    unsigned int width;
    unsigned int height;
};

ShadowMap createShadowMap(unsigned int width, unsigned int height);
#endif
