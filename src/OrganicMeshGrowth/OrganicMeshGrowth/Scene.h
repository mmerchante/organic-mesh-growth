#pragma once

#include <glm/glm.hpp>
#include <chrono>

#include "Model.h"
#include "Texture3D.h"

using namespace std::chrono;

struct TriangleData {
	GLM_ALIGN(16) glm::vec3 v1, v2, v3;
	GLM_ALIGN(16) glm::vec4 v21, v32, v13;
	GLM_ALIGN(16) glm::vec4 normal;
	GLM_ALIGN(16) glm::vec3 t21, t32, t13;
};

struct Time {
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

struct CompactNode
{
	int leftNode;	// The index of the left node
	int rightNode;	// The index of the right node

	int axis;		// The axis for this node
	float split;	// The offset on this axis

	int primitiveCount;			// The size of this leaf
	int primitiveStartOffset;	// The offset where the triangles are
};

class AABB
{
public:
	AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max), center((min + max) * .5f) {}
	AABB() : min(glm::vec3(0.f)), max(glm::vec3(0.f)), center((min + max) * .5f) {}

	// Intersection is handled in device
	AABB Encapsulate(AABB bounds);
	AABB Transform(const glm::mat4x4& transform);

	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 center;

private:
	static glm::vec3 aabb[];
};

struct Triangle
{
	// Data
	glm::vec3 p1;
	glm::vec3 p2;
	glm::vec3 p3;

	// Normals
	glm::vec3 n1;
	glm::vec3 n2;
	glm::vec3 n3;

	AABB bounds;
};

// kd-tree implementation for meshes
class Mesh
{
public:
	Mesh(int maxDepth, int maxLeafSize, std::vector<Triangle*>& triangles);
	~Mesh();

	void Build();

	int maxDepth;
	AABB meshBounds;
	int maxLeafSize;
	
	CompactNode * compactNodes;
	int compactNodeSize;

	TriangleData * compactTriangles;
	int compactTriangleSize;

protected:
	struct MeshNode
	{
	public:
		MeshNode(const std::vector<Triangle *> &triangles, glm::vec3 min, glm::vec3 max, int depth, int maxDepth, int threshold);
		~MeshNode();

		void BuildNode(const std::vector<Triangle *> &triangles, const glm::vec3& minVector, const glm::vec3& maxVector, int depth, int maxDepth, int threshold);
		float CostFunction(float split, const std::vector<Triangle *> &triangles, float minAxis, float maxAxis);
		float GetSplitPoint(const std::vector<Triangle *> &triangles, float minAxis, float maxAxis);

		bool IsLeaf();
		int GetNodeCount();
		int TriangleCount();
		int GetDepth();

	public:
		std::vector<Triangle *> nodeTriangles;
		MeshNode * left;
		MeshNode * right;

		float split;
		int axis;
		int parentOffset; // For compaction
	};

	std::vector<Triangle*> triangles;
	MeshNode * root;

	AABB CalculateAABB();
	void Compact();

};

class Scene {
private:
    Device* device;
    
    VkBuffer timeBuffer;
    VkDeviceMemory timeBufferMemory;
    Time time;
	std::vector<Texture3D*> sceneSDF;
	Texture3D* vectorFieldTexture;

	TriangleData * meshBufferObject;
	VkBuffer meshBuffer;
	VkDeviceMemory meshBufferMemory;
	void * meshMappedData;
	int meshTriangleCount;

	CompactNode * indexBufferObject;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	void * indexMappedData;
	int indexCount;

	int meshBufferSize;
	VkBuffer meshAttributeBuffer;
	VkDeviceMemory meshAttributeBufferMemory;
	void * meshAttributeMappedData;
    
    void* mappedData;

    std::vector<Model*> models;

	high_resolution_clock::time_point startTime = high_resolution_clock::now();

public:
    Scene() = delete;
    Scene(Device* device);
    ~Scene();

    const std::vector<Model*>& GetModels() const;
    
    void AddModel(Model* model);


    VkBuffer GetTimeBuffer() const;

	Texture3D* GetSceneSDF(int index);
	void CreateSceneSDF();

	void LoadMesh(std::string filename);

	VkBuffer GetMeshIndexBuffer();
	VkBuffer GetMeshBuffer();
	VkBuffer GetMeshAttributeBuffer();
	int GetMeshBufferSize();

	void CreateVectorField();
	Texture3D* GetVectorField();

    float UpdateTime();
};
