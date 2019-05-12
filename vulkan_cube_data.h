#pragma once

#include <cassert>

struct Position {
    static constexpr size_t dim = 4;

    union {
        struct {
            float x, y, z, w;
        };
        float pos[dim];
    };

    float operator[](size_t idx) { assert(idx < dim); return pos[idx]; }
};

struct ColorRGBA {
    static constexpr size_t dim = 4;

    union {
        struct {
            float r, g, b, a;
        };
        float col[dim];
    };

    float operator[](size_t idx) { assert(idx < dim); return col[idx]; }
};

struct TextureCoord {
    static constexpr size_t dim = 2;
    union {
        struct {
            float u, v;
        };
        float coord[dim];
    };

    float operator[](size_t idx) { assert(idx < dim); return coord[idx]; }
};

struct Vertex {
    Position pos;
    ColorRGBA col;
};

struct VertexUV {
    Position pos;
    TextureCoord tex;
};

#define XYZ1(X, Y, Z) (X), (Y), (Z), 1.0f
#define UV(U, V) (U), (V)

namespace Cube_Model
{
    static constexpr Vertex vertex_buffer_data[] = {
        {XYZ1(-1, -1, -1), XYZ1(0.f, 0.f, 0.f)}, {XYZ1(1, -1, -1), XYZ1(1.f, 0.f, 0.f)}, {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},  {XYZ1(1, -1, -1), XYZ1(1.f, 0.f, 0.f)}, {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},

        {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},  {XYZ1(-1, 1, 1), XYZ1(0.f, 1.f, 1.f)},  {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 1.f)},   {XYZ1(-1, 1, 1), XYZ1(0.f, 1.f, 1.f)},  {XYZ1(1, 1, 1), XYZ1(1.f, 1.f, 1.f)},

        {XYZ1(1, 1, 1), XYZ1(1.f, 1.f, 1.f)},    {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},  {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 1.f)},   {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},  {XYZ1(1, -1, -1), XYZ1(1.f, 0.f, 0.f)},

        {XYZ1(-1, 1, 1), XYZ1(0.f, 1.f, 1.f)},   {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)}, {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},  {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)}, {XYZ1(-1, -1, -1), XYZ1(0.f, 0.f, 0.f)},

        {XYZ1(1, 1, 1), XYZ1(1.f, 1.f, 1.f)},    {XYZ1(-1, 1, 1), XYZ1(0.f, 1.f, 1.f)},  {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},
        {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},   {XYZ1(-1, 1, 1), XYZ1(0.f, 1.f, 1.f)},  {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},

        {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 1.f)},   {XYZ1(1, -1, -1), XYZ1(1.f, 0.f, 0.f)}, {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},
        {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},  {XYZ1(1, -1, -1), XYZ1(1.f, 0.f, 0.f)}, {XYZ1(-1, -1, -1), XYZ1(0.f, 0.f, 0.f)},
    };

    static constexpr Vertex vertex_buffer_solid_face_colors_data[] = {
        // red face
        {XYZ1(-1, -1, 1), XYZ1(1.f, 0.f, 0.f)},
        {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 0.f)},
        {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 0.f)},
        {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 0.f)},
        {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 0.f)},
        {XYZ1(1, 1, 1), XYZ1(1.f, 0.f, 0.f)},

        // green face
        {XYZ1(-1, -1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 0.f)},
        {XYZ1(1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
        
        // blue face
        {XYZ1(-1, 1, 1), XYZ1(0.f, 0.f, 1.f)},
        {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},
        {XYZ1(-1, 1, -1), XYZ1(0.f, 0.f, 1.f)},
        {XYZ1(-1, 1, -1), XYZ1(0.f, 0.f, 1.f)},
        {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},
        {XYZ1(-1, -1, -1), XYZ1(0.f, 0.f, 1.f)},
        
        // yellow face
        {XYZ1(1, 1, 1), XYZ1(1.f, 1.f, 0.f)},
        {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},
        {XYZ1(1, -1, 1), XYZ1(1.f, 1.f, 0.f)},
        {XYZ1(1, -1, 1), XYZ1(1.f, 1.f, 0.f)},
        {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},
        {XYZ1(1, -1, -1), XYZ1(1.f, 1.f, 0.f)},
        
        // magenta face
        {XYZ1(1, 1, 1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(1, 1, -1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(1, 1, -1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 1.f)},
        {XYZ1(-1, 1, -1), XYZ1(1.f, 0.f, 1.f)},
        
        // cyan face
        {XYZ1(1, -1, 1), XYZ1(0.f, 1.f, 1.f)},
        {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 1.f)},
        {XYZ1(-1, -1, 1), XYZ1(0.f, 1.f, 1.f)},
        {XYZ1(-1, -1, 1), XYZ1(0.f, 1.f, 1.f)},
        {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 1.f)},
        {XYZ1(-1, -1, -1), XYZ1(0.f, 1.f, 1.f)},
    };

    static constexpr VertexUV vertex_buffer_texture_data[] = {
        // left face
        {XYZ1(-1, -1, -1), UV(1.f, 0.f)},  // lft-top-front
        {XYZ1(-1, 1, 1), UV(0.f, 1.f)},    // lft-btm-back
        {XYZ1(-1, -1, 1), UV(0.f, 0.f)},   // lft-top-back
        {XYZ1(-1, 1, 1), UV(0.f, 1.f)},    // lft-btm-back
        {XYZ1(-1, -1, -1), UV(1.f, 0.f)},  // lft-top-front
        {XYZ1(-1, 1, -1), UV(1.f, 1.f)},   // lft-btm-front
        
        // front face
        {XYZ1(-1, -1, -1), UV(0.f, 0.f)},  // lft-top-front
        {XYZ1(1, -1, -1), UV(1.f, 0.f)},   // rgt-top-front
        {XYZ1(1, 1, -1), UV(1.f, 1.f)},    // rgt-btm-front
        {XYZ1(-1, -1, -1), UV(0.f, 0.f)},  // lft-top-front
        {XYZ1(1, 1, -1), UV(1.f, 1.f)},    // rgt-btm-front
        {XYZ1(-1, 1, -1), UV(0.f, 1.f)},   // lft-btm-front
        
        // top face
        {XYZ1(-1, -1, -1), UV(0.f, 1.f)},  // lft-top-front
        {XYZ1(1, -1, 1), UV(1.f, 0.f)},    // rgt-top-back
        {XYZ1(1, -1, -1), UV(1.f, 1.f)},   // rgt-top-front
        {XYZ1(-1, -1, -1), UV(0.f, 1.f)},  // lft-top-front
        {XYZ1(-1, -1, 1), UV(0.f, 0.f)},   // lft-top-back
        {XYZ1(1, -1, 1), UV(1.f, 0.f)},    // rgt-top-back
        
        // bottom face
        {XYZ1(-1, 1, -1), UV(0.f, 0.f)},  // lft-btm-front
        {XYZ1(1, 1, 1), UV(1.f, 1.f)},    // rgt-btm-back
        {XYZ1(-1, 1, 1), UV(0.f, 1.f)},   // lft-btm-back
        {XYZ1(-1, 1, -1), UV(0.f, 0.f)},  // lft-btm-front
        {XYZ1(1, 1, -1), UV(1.f, 0.f)},   // rgt-btm-front
        {XYZ1(1, 1, 1), UV(1.f, 1.f)},    // rgt-btm-back
        
        // right face
        {XYZ1(1, 1, -1), UV(0.f, 1.f)},   // rgt-btm-front
        {XYZ1(1, -1, 1), UV(1.f, 0.f)},   // rgt-top-back
        {XYZ1(1, 1, 1), UV(1.f, 1.f)},    // rgt-btm-back
        {XYZ1(1, -1, 1), UV(1.f, 0.f)},   // rgt-top-back
        {XYZ1(1, 1, -1), UV(0.f, 1.f)},   // rgt-btm-front
        {XYZ1(1, -1, -1), UV(0.f, 0.f)},  // rgt-top-front

        // back face
        {XYZ1(-1, 1, 1), UV(1.f, 1.f)},   // lft-btm-back
        {XYZ1(1, 1, 1), UV(0.f, 1.f)},    // rgt-btm-back
        {XYZ1(-1, -1, 1), UV(1.f, 0.f)},  // lft-top-back
        {XYZ1(-1, -1, 1), UV(1.f, 0.f)},  // lft-top-back
        {XYZ1(1, 1, 1), UV(0.f, 1.f)},    // rgt-btm-back
        {XYZ1(1, -1, 1), UV(0.f, 0.f)}, // rgt-top-back
    };
};