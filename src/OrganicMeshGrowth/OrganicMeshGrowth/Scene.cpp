#include "Scene.h"
#include "BufferUtils.h"
#include <iostream>
#include <stack>
#include <glm/gtc/constants.hpp>

#define SAH_SUBDIV 15

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Scene::Scene(Device* device) : device(device) {
    BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));
}

const std::vector<Model*>& Scene::GetModels() const {
    return models;
}

void Scene::AddModel(Model* model) {
    models.push_back(model);
}

void Scene::UpdateTime() {
    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    duration<float> nextDeltaTime = duration_cast<duration<float>>(currentTime - startTime);
    startTime = currentTime;

	time.deltaTime = .016;// nextDeltaTime.count();
    time.totalTime += time.deltaTime;

    memcpy(mappedData, &time, sizeof(Time));
}

void Scene::CreateSceneSDF()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Interpolation of texels that are magnified or minified
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	// Addressing mode
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	// Anisotropic filtering
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;

	// Border color
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Choose coordinate system for addressing texels --> [0, 1) here
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	// Comparison function used for filtering operations
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// Mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	for (int i = 0; i < 2; ++i) {
		sceneSDF.push_back(new Texture3D(device, 512, 512, 512, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, samplerInfo));
	}
}

void Scene::LoadMesh(const std::string filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string error;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &error, filename.c_str());

	if (!loaded)
		throw std::exception(error.c_str());

	this->meshTriangleCount = 0;

	glm::vec3 minBounds = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 maxBounds = glm::vec3(-std::numeric_limits<float>::max());

	for (int i = 0; i < shapes.size(); ++i)
	{
		for (int f = 0; f < shapes[i].mesh.num_face_vertices.size(); ++f)
			if (shapes[i].mesh.num_face_vertices[f] == 3)
				meshTriangleCount++;
	}

	for (int i = 0; i < attrib.vertices.size(); i += 3)
	{
		glm::vec3 v;
		v.x = attrib.vertices[i + 0];
		v.y = attrib.vertices[i + 1];
		v.z = attrib.vertices[i + 2];

		minBounds = glm::min(minBounds, v);
		maxBounds = glm::max(maxBounds, v);
	}

	glm::vec3 centerPivot = (maxBounds + minBounds) * .5f;
	glm::vec3 meshSize = glm::abs((maxBounds - minBounds) * .75f);
	float meshUniformSize = glm::max(meshSize.x, glm::max(meshSize.y, meshSize.z)) + .00001;
	
	int currentOffset = 0;
	this->meshBufferObject = new TriangleData[meshTriangleCount];

	for (int m = 0; m < shapes.size(); ++m)
	{
		int indexOffset = 0;

		for (int f = 0; f < shapes[m].mesh.num_face_vertices.size(); ++f)
		{
			if (shapes[m].mesh.num_face_vertices[f] == 3)
			{
				int i1 = shapes[m].mesh.indices[indexOffset + 0].vertex_index;
				int i2 = shapes[m].mesh.indices[indexOffset + 1].vertex_index;
				int i3 = shapes[m].mesh.indices[indexOffset + 2].vertex_index;
				indexOffset += 3;

				TriangleData tri;

				// v1
				tri.v1.x = attrib.vertices[i1 * 3 + 0];
				tri.v1.y = attrib.vertices[i1 * 3 + 1];
				tri.v1.z = attrib.vertices[i1 * 3 + 2];

				// v2
				tri.v2.x = attrib.vertices[i2 * 3 + 0];
				tri.v2.y = attrib.vertices[i2 * 3 + 1];
				tri.v2.z = attrib.vertices[i2 * 3 + 2];

				// v3
				tri.v3.x = attrib.vertices[i3 * 3 + 0];
				tri.v3.y = attrib.vertices[i3 * 3 + 1];
				tri.v3.z = attrib.vertices[i3 * 3 + 2];

				// Transform
				tri.v1 = (tri.v1 - centerPivot) / meshUniformSize;
				tri.v2 = (tri.v2 - centerPivot) / meshUniformSize;
				tri.v3 = (tri.v3 - centerPivot) / meshUniformSize;

				// Offsets
				tri.v21 = glm::vec4(tri.v2 - tri.v1, 0.f);
				tri.v32 = glm::vec4(tri.v3 - tri.v2, 0.f);
				tri.v13 = glm::vec4(tri.v1 - tri.v3, 0.f);

				// Magnitudes
				tri.v21.w = 1.f / glm::dot(tri.v21, tri.v21);
				tri.v32.w = 1.f / glm::dot(tri.v32, tri.v32);
				tri.v13.w = 1.f / glm::dot(tri.v13, tri.v13);

				// Unnormalized normal
				tri.normal = glm::vec4(glm::cross(glm::vec3(tri.v21), glm::vec3(tri.v13)), 0.f);
				tri.normal.w = 1.f / glm::dot(tri.normal, tri.normal);

				tri.t21 = glm::cross(glm::vec3(tri.v21), glm::vec3(tri.normal));
				tri.t32 = glm::cross(glm::vec3(tri.v32), glm::vec3(tri.normal));
				tri.t13 = glm::cross(glm::vec3(tri.v13), glm::vec3(tri.normal));

				this->meshBufferObject[currentOffset] = tri;
				currentOffset++;
			}
			else
			{
				// Ignore this face
				indexOffset += shapes[m].mesh.num_face_vertices[f];
			}
		}
	}

	this->meshBufferSize = sizeof(TriangleData) * meshTriangleCount;

	//std::cout << centerPivot.x << ", " << centerPivot.y << ", " << centerPivot.z << std::endl;
	//std::cout << meshUniformSize << std::endl;
	//std::cout << "sizeof(TriangleData) " << sizeof(TriangleData) << std::endl;
	std::cout << "Loaded " << filename << " with " << meshTriangleCount << " triangles" << std::endl;

	BufferUtils::CreateBuffer(device, meshBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, meshBuffer, meshBufferMemory);
	vkMapMemory(device->GetVkDevice(), meshBufferMemory, 0, meshBufferSize, 0, &meshMappedData);
	memcpy(meshMappedData, meshBufferObject, meshBufferSize);

	BufferUtils::CreateBuffer(device, sizeof(int), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, meshAttributeBuffer, meshAttributeBufferMemory);
	vkMapMemory(device->GetVkDevice(), meshAttributeBufferMemory, 0, sizeof(int), 0, &meshAttributeMappedData);
	memcpy(meshAttributeMappedData, &this->meshTriangleCount, sizeof(int));
}

VkBuffer Scene::GetMeshBuffer()
{
	return meshBuffer;
}

VkBuffer Scene::GetMeshAttributeBuffer()
{
	return meshAttributeBuffer;
}

int Scene::GetMeshBufferSize()
{
	return meshBufferSize;
}

void Scene::CreateVectorField()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Interpolation of texels that are magnified or minified
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	// Addressing mode
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	// Anisotropic filtering
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;

	// Border color
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Choose coordinate system for addressing texels --> [0, 1) here
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	// Comparison function used for filtering operations
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// Mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	this->vectorFieldTexture = new Texture3D(device, 512, 512, 512, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, samplerInfo);
}

Texture3D * Scene::GetVectorField()
{
	return vectorFieldTexture;
}

VkBuffer Scene::GetTimeBuffer() const {
    return timeBuffer;
}

Texture3D * Scene::GetSceneSDF(int index)
{
	return sceneSDF[index];
}

Scene::~Scene() {
    vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
    vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);

	vkUnmapMemory(device->GetVkDevice(), meshBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), meshBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), meshBufferMemory, nullptr);

	vkUnmapMemory(device->GetVkDevice(), meshAttributeBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), meshAttributeBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), meshAttributeBufferMemory, nullptr);

	for (Texture3D* t : sceneSDF)
		delete t;

	delete vectorFieldTexture;
}

glm::vec3 AABB::aabb[] = { glm::vec3(1, 1, 1),glm::vec3(1, -1, -1), glm::vec3(1, 1, -1), glm::vec3(1, -1, 1),
glm::vec3(-1, 1, 1), glm::vec3(-1, -1, -1), glm::vec3(-1, 1, -1), glm::vec3(-1, -1, 1) };

AABB AABB::Encapsulate(AABB bounds)
{
	glm::vec3 min = glm::min(this->min, bounds.min);
	glm::vec3 max = glm::max(this->max, bounds.max);
	return AABB(min, max);
}

AABB AABB::Transform(const glm::mat4x4 &transform)
{
	// If infinite box, prevent overflowing
	if (min.x == -std::numeric_limits<float>::infinity() || min.y == -std::numeric_limits<float>::infinity() || min.z == -std::numeric_limits<float>::infinity()
		|| max.x == std::numeric_limits<float>::infinity() || max.y == std::numeric_limits<float>::infinity() || max.z == std::numeric_limits<float>::infinity())
	{
		return *this;
	}

	float maxX = -std::numeric_limits<float>::infinity();
	float maxY = -std::numeric_limits<float>::infinity();
	float maxZ = -std::numeric_limits<float>::infinity();

	float minX = std::numeric_limits<float>::infinity();
	float minY = std::numeric_limits<float>::infinity();
	float minZ = std::numeric_limits<float>::infinity();

	glm::vec3 halfSize = (max - center);

	for (int i = 0; i < 8; i++)
	{
		glm::vec3 v = center + (AABB::aabb[i] * halfSize);
		glm::vec3 tPoint = glm::vec3(transform * glm::vec4(v.x, v.y, v.z, 1.));

		maxX = glm::max(tPoint.x, maxX);
		maxY = glm::max(tPoint.y, maxY);
		maxZ = glm::max(tPoint.z, maxZ);

		minX = glm::min(tPoint.x, minX);
		minY = glm::min(tPoint.y, minY);
		minZ = glm::min(tPoint.z, minZ);
	}

	glm::vec3 newMin = glm::vec3(minX, minY, minZ);
	glm::vec3 newMax = glm::vec3(maxX, maxY, maxZ);

	return AABB(newMin, newMax);
}

glm::vec3 axisPlaneNormals[] = { glm::vec3(1.f,0,0), glm::vec3(0,1.f,0), glm::vec3(0,0,1.f) };

Mesh::MeshNode::MeshNode(const std::vector<Triangle *>& originalTriangles, glm::vec3 min, glm::vec3 max, int depth, int maxDepth, int threshold)
{
	glm::vec3 extent = glm::abs(max - min);

	if (extent.x > extent.y && extent.x > extent.z)
		this->axis = 0;
	else if (extent.y > extent.x && extent.y > extent.z)
		this->axis = 1;
	else
		this->axis = 2;

	this->left = nullptr;
	this->right = nullptr;
	this->split = 0;
	this->BuildNode(originalTriangles, min, max, depth, maxDepth, threshold);
	this->parentOffset = -1;
}

Mesh::MeshNode::~MeshNode()
{
	if (this->left != nullptr)
		delete left;

	if (this->right != nullptr)
		delete right;
}

void Mesh::MeshNode::BuildNode(const std::vector<Triangle *>& triangles, const glm::vec3 &minVector, const glm::vec3 &maxVector, int depth, int maxDepth, int threshold)
{
	if (triangles.size() > threshold && depth < maxDepth)
	{
		glm::vec3 axisNormal = axisPlaneNormals[axis];

		float minAxis = minVector[axis];
		float maxAxis = maxVector[axis];

		// If axis cannot be subdivided, we stop, to prevent jumping
		// between axis indefinitely
		if (glm::abs(minAxis - maxAxis) < glm::epsilon<float>())
		{
			this->nodeTriangles = triangles;
			this->left = nullptr;
			this->right = nullptr;
			return;
		}

		split = GetSplitPoint(triangles, minVector[axis], maxVector[axis]);

		std::vector<Triangle*> leftShapes;
		std::vector<Triangle*> rightShapes;

		for (int i = 0; i < triangles.size(); i++)
		{
			Triangle * tri = triangles[i];
			AABB bounds = tri->bounds;

			float p = bounds.center[axis];

			// If shape position is on right, surely its on right node
			if (p > split) {
				rightShapes.push_back(tri);

				// But if bounding box collides with plane, add on left
				// node
				float min = bounds.min[axis];

				if (min <= split)
					leftShapes.push_back(tri);

			}
			else {
				leftShapes.push_back(tri);

				// But if bounding box collides with plane, add on right
				// node
				float max = bounds.max[axis];

				if (max >= split)
					rightShapes.push_back(tri);
			}
		}

		glm::vec3 leftMax = maxVector - (axisNormal * glm::abs(maxVector[axis] - split));
		glm::vec3 rightMin = minVector + (axisNormal * glm::abs(split - minVector[axis]));

		this->left = new MeshNode(leftShapes, minVector, leftMax, depth + 1, maxDepth, threshold);
		this->right = new MeshNode(rightShapes, rightMin, maxVector, depth + 1, maxDepth, threshold);
	}
	else
	{
		this->nodeTriangles = triangles;
		this->left = nullptr;
		this->right = nullptr;
	}
}

float Mesh::MeshNode::CostFunction(float split, const std::vector<Triangle *>& triangles, float minAxis, float maxAxis)
{
	int leftCount = 0;
	int rightCount = 0;

	for (int i = 0; i < triangles.size(); i++)
	{
		Triangle * tri = triangles[i];
		AABB bounds = tri->bounds;

		float p = bounds.center[axis];

		// If shape position is on right, surely its on right node
		if (p > split) {
			rightCount++;

			// But if bounding box collides with plane, add on left
			// node
			float min = bounds.min[axis];

			if (min <= split)
				leftCount++;

		}
		else {
			leftCount++;

			// But if bounding box collides with plane, add on right
			// node
			float max = bounds.max[axis];

			if (max >= split)
				rightCount++;
		}
	}

	// Here we simplify Surface area by using just the size on the split
	// axis
	float leftSize = split - minAxis;
	float rightSize = maxAxis - split;

	return (leftSize * leftCount) + (rightSize * rightCount);
}

float Mesh::MeshNode::GetSplitPoint(const std::vector<Triangle *> &triangles, float minAxis, float maxAxis)
{
	// Spatial median
	float center = (maxAxis + minAxis) * .5f;

	// Object median
	float objMedian = 0;

	for (int i = 0; i < triangles.size(); i++)
		objMedian += triangles[i]->bounds.center[axis];

	objMedian /= triangles.size();

	float step = (center - objMedian) / SAH_SUBDIV;

	float minCost = std::numeric_limits<float>::infinity();
	float result = objMedian;

	if (glm::abs(step) > glm::epsilon<float>())
	{
		// i is the proposed split point
		for (float i = objMedian; i < center; i += step)
		{
			float cost = CostFunction(i, triangles, minAxis, maxAxis);

			if (minCost > cost)
			{
				minCost = cost;
				result = i;
			}
		}
	}

	return result;
}

int Mesh::MeshNode::GetNodeCount()
{
	if (IsLeaf())
		return 1;
	else
		return 1 + left->GetNodeCount() + right->GetNodeCount();
}

int Mesh::MeshNode::TriangleCount()
{
	if (IsLeaf())
		return this->nodeTriangles.size();
	else
		return left->TriangleCount() + right->TriangleCount();
}

int Mesh::MeshNode::GetDepth()
{
	if (IsLeaf())
		return 1;

	return glm::max(left->GetDepth(), right->GetDepth()) + 1;
}

bool Mesh::MeshNode::IsLeaf()
{
	return left == nullptr && right == nullptr;
}

Mesh::Mesh(int maxDepth, int maxLeafSize, std::vector<Triangle*>& triangles) : maxDepth(maxDepth), maxLeafSize(maxLeafSize), root(nullptr), compactNodes(nullptr), triangles(triangles)
{
}

Mesh::~Mesh()
{
	if (this->root != nullptr)
		delete this->root;

	if (this->compactNodes != nullptr)
		delete[] this->compactNodes;
}

AABB Mesh::CalculateAABB()
{
	AABB bounds;

	if (triangles.size() > 0)
		bounds = triangles[0]->bounds;

	for (int i = 1; i < triangles.size(); i++)
		bounds = bounds.Encapsulate(triangles[i]->bounds);

	return bounds;
}

void Mesh::Build()
{
	for (int i = 0; i < triangles.size(); i++)
	{
		Triangle * t = triangles[i];
		glm::vec3 min = glm::min(t->p1, glm::min(t->p2, t->p3));
		glm::vec3 max = glm::max(t->p1, glm::max(t->p2, t->p3));
		t->bounds = AABB(min - glm::vec3(glm::epsilon<float>()), max + glm::vec3(glm::epsilon<float>()));
	}

	meshBounds = this->CalculateAABB();
	this->root = new MeshNode(triangles, meshBounds.min, meshBounds.max, 0, this->maxDepth, this->maxLeafSize);

	std::cout << this->root->GetDepth() << " tree depth" << std::endl;
	std::cout << this->root->GetNodeCount() << " tree nodes" << std::endl;
	std::cout << this->root->TriangleCount() << " total elements in tree" << std::endl;

	this->Compact();
}

void Mesh::Compact()
{
	int nodeCount = this->root->GetNodeCount();
	int elements = this->root->TriangleCount();

	// Left, right, split, axis, primitiveCount
	int nodeSize = 4 + 4 + 4 + 4 + 4;
	int elementSize = sizeof(CompactTriangle);

	this->compactDataSize = (nodeCount * nodeSize) + (elements * elementSize);
	this->compactNodes = (int*)std::malloc(compactDataSize);

	int mb = compactDataSize / (1024 * 1024);
	std::cout << "Compact mesh kd-tree memory size: " << mb << " megabytes" << std::endl;

	if (this->compactNodes == nullptr)
		std::cerr << "Could not allocate " << mb << " megabytes" << std::endl;

	std::stack<MeshNode*> stack;
	stack.push(this->root);

	int offset = 0;

	// For compaction, we need to remember the original placement of nodes in parent
	// nodes because we can't really know the offset until we build everything.
	while (!stack.empty())
	{
		MeshNode * node = stack.top();
		stack.pop();

		if (node == nullptr)
			continue;

		// If this node is the child of a parent, let's set the current offset
		if (node->parentOffset != -1)
			compactNodes[node->parentOffset] = offset;

		int baseOffset = offset;

		CompactNode * cNode = (CompactNode*)(compactNodes + offset);
		cNode->leftNode = -1;
		cNode->rightNode = -1;
		cNode->split = node->split;
		cNode->axis = node->axis;

		if (node->IsLeaf())
		{
			cNode->primitiveCount = node->nodeTriangles.size();
			offset += 5;

			float * triData = (float*)compactNodes;
			int triCount = node->nodeTriangles.size();

			for (int i = 0; i < triCount; i++)
			{
				Triangle * triangle = node->nodeTriangles[i];

				glm::vec3 e1 = triangle->p2 - triangle->p1;
				glm::vec3 e2 = triangle->p3 - triangle->p1;
				glm::vec3 p = triangle->p1;

				// e1
				triData[offset++] = e1.x;
				triData[offset++] = e1.y;
				triData[offset++] = e1.z;

				// e2
				triData[offset++] = e2.x;
				triData[offset++] = e2.y;
				triData[offset++] = e2.z;

				// p1
				triData[offset++] = triangle->p1.x;
				triData[offset++] = triangle->p1.y;
				triData[offset++] = triangle->p1.z;

				// Normals
				triData[offset++] = triangle->n1.x;
				triData[offset++] = triangle->n1.y;
				triData[offset++] = triangle->n1.z;

				triData[offset++] = triangle->n2.x;
				triData[offset++] = triangle->n2.y;
				triData[offset++] = triangle->n2.z;

				triData[offset++] = triangle->n3.x;
				triData[offset++] = triangle->n3.y;
				triData[offset++] = triangle->n3.z;
			}
		}
		else
		{
			cNode->primitiveCount = 0;
			offset += 5;

			node->left->parentOffset = baseOffset;
			node->right->parentOffset = baseOffset + 1;

			stack.push(node->left);
			stack.push(node->right);
		}
	}

	// Now that everything is copied and compacted, we can delete our root
	delete this->root;
	this->root = nullptr;
}
