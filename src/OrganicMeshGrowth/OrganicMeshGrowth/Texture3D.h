
#include <vulkan/vulkan.h>
#include "Device.h"
#include "Instance.h"

#pragma once
class Texture3D
{
public:
	Texture3D(Device* device, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkSamplerCreateInfo samplerInfo);
	~Texture3D();
	
	bool Create();
	bool CreateView();
	bool CreateSampler();

	VkImage GetImage();
	VkImageView GetImageView();
	VkSampler GetSampler();
	
protected:
	uint32_t width;
	uint32_t height;
	uint32_t depth;

	VkFormat format;
	VkImageUsageFlags usage;
	VkImageTiling tiling;
	VkSamplerCreateInfo samplerInfo;

	VkDeviceMemory imageMemory;
	VkMemoryPropertyFlags memoryProperties;
	
	Device* device;

	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
};