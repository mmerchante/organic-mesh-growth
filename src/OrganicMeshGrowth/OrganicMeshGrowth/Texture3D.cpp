#include "Texture3D.h"

Texture3D::Texture3D(Device * device, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSamplerCreateInfo samplerInfo)
	: device(device), width(width), height(height), depth(depth), format(format), tiling(tiling), usage(usage), memoryProperties(properties), samplerInfo(samplerInfo)
{
	this->Create();
}

Texture3D::~Texture3D()
{
	if (image != VK_NULL_HANDLE)
		vkDestroyImage(device->GetVkDevice(), image, nullptr);

	if (imageMemory != VK_NULL_HANDLE)
		vkFreeMemory(device->GetVkDevice(), imageMemory, nullptr);

	if (view != VK_NULL_HANDLE)
		vkDestroyImageView(device->GetVkDevice(), view, nullptr);

	if (textureSampler != VK_NULL_HANDLE)
		vkDestroySampler(device->GetVkDevice(), textureSampler, nullptr);
}

bool Texture3D::Create()
{
	// Create Vulkan image
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_3D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = depth;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device->GetVkDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create 3D texture");
	}

	// Allocate memory for the image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device->GetVkDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->GetInstance()->GetMemoryTypeIndex(memRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(device->GetVkDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory");
	}

	// Bind the image
	vkBindImageMemory(device->GetVkDevice(), image, imageMemory, 0);

	CreateView();
	CreateSampler();

	return true;
}

bool Texture3D::CreateView()
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	viewInfo.format = format;

	// Describe the image's purpose and which part of the image should be accessed
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device->GetVkDevice(), &viewInfo, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("Failed to texture 3D texture view");
	}

	return true;
}

bool Texture3D::CreateSampler()
{
	if (textureSampler != VK_NULL_HANDLE)
		return false;
	
	if (vkCreateSampler(device->GetVkDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler");
	}

	return true;
}

VkImage Texture3D::GetImage()
{
	return image;
}

VkImageView Texture3D::GetImageView()
{
	return view;
}

VkSampler Texture3D::GetSampler()
{
	return textureSampler;
}
