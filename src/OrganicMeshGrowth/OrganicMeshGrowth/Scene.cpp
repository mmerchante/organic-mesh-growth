#include "Scene.h"
#include "BufferUtils.h"

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
		sceneSDF.push_back(new Texture3D(device, 256, 256, 256, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, samplerInfo));
	}
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

	this->vectorFieldTexture = new Texture3D(device, 256, 256, 256, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, samplerInfo);
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

	for (Texture3D* t : sceneSDF)
		delete t;

	delete vectorFieldTexture;
}
