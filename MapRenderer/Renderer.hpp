#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Types.hpp"

using namespace std;

struct FrameStats
{
    size_t visibleTiles = 0;
    size_t drawCalls = 0;
    size_t renderedVertices = 0;
};

class Renderer
{
public:
    Renderer(const vector<MapTile>& tiles);
    ~Renderer();

    FrameStats draw(const vector<MapTile>& tiles, const glm::mat4& view, const glm::mat4& projection);

private:
    struct GpuMesh
    {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;
        int indexCount = 0;
        size_t vertexCount = 0;
    };


    unsigned int shaderProgram;
    int mvpLocation;

    vector<GpuMesh> meshes;

    unsigned int compileShader(unsigned int type, const char* source);
    void uploadMesh(GpuMesh& gpuMesh, const MeshData& mesh);
};