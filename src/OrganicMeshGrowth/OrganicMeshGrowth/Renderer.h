#pragma once

#include "Device.h"
#include "SwapChain.h"
#include "Scene.h"
#include "Camera.h"

class Texture3D;

class Renderer {
public:
    Renderer() = delete;
    Renderer(Device* device, SwapChain* swapChain, Scene* scene, Camera* camera);
    ~Renderer();

    void CreateCommandPools();

    void CreateRenderPass();

    void CreateCameraDescriptorSetLayout();
    void CreateModelDescriptorSetLayout();
    void CreateTimeDescriptorSetLayout();
    void CreateSceneSDFDescriptorSetLayout();
	void CreateVectorFieldDescriptorSetLayout();
	void CreateGeneratorDescriptorSetLayout();

    void CreateDescriptorPool();

    void CreateCameraDescriptorSet();
    void CreateModelDescriptorSets(bool primary);
    void CreateTimeDescriptorSet();
    void CreateSceneSDFDescriptorSet();
	void CreateVectorFieldDescriptorSet();
	void CreateGeneratorDescriptorSet();

    void CreateRaymarchingPipeline();
    void CreateKernelComputePipeline();
	void CreateGeneratorComputePipeline();

    void CreateFrameResources();
    void DestroyFrameResources();
    void RecreateFrameResources();

    void RecordCommandBuffers(bool primary);
    void RecordKernelComputeCommandBuffer();
	void RecordGeneratorComputeCommandBuffer();

	void GenerateSceneSDF();
    void Frame();

private:
    Device* device;
    VkDevice logicalDevice;
    SwapChain* swapChain;
    Scene* scene;
    Camera* camera;

	int currentFrameIndex;
	
    VkCommandPool raymarchingCommandPool;
    VkCommandPool computeCommandPool;

    VkRenderPass renderPass;

	VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout cameraDescriptorSetLayout;
    VkDescriptorSetLayout modelDescriptorSetLayout;
	VkDescriptorSetLayout timeDescriptorSetLayout;
    VkDescriptorSetLayout sceneSDFDescriptorSetLayout;
	VkDescriptorSetLayout vectorFieldDescriptorSetLayout;
	VkDescriptorSetLayout generatorDescriptorSetLayout;

	VkDescriptorSet generatorDescriptorSet;
    VkDescriptorSet cameraDescriptorSet;
    VkDescriptorSet timeDescriptorSet;
	VkDescriptorSet primarySceneSDFDescriptorSet;
	VkDescriptorSet secondarySceneSDFDescriptorSet;
	VkDescriptorSet vectorFieldDescriptorSet;

    std::vector<VkDescriptorSet> primaryModelDescriptorSets;
	std::vector<VkDescriptorSet> secondaryModelDescriptorSets;

    VkPipelineLayout raymarchingPipelineLayout;
    VkPipelineLayout kernelComputePipelineLayout;
	VkPipelineLayout generatorComputePipelineLayout;

    VkPipeline raymarchingPipeline;
    VkPipeline kernelComputePipeline;
	VkPipeline generatorComputePipeline;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

	VkCommandBuffer generatorCommandBuffer;

    VkCommandBuffer primaryKernelCommandBuffer;
	VkCommandBuffer secondaryKernelCommandBuffer;
    
	std::vector<VkCommandBuffer> primaryCommandBuffers;
	std::vector<VkCommandBuffer> secondaryCommandBuffers;
};
