#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include "Instance.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Image.h"
#include <iostream>

Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;

namespace {
    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;

        vkDeviceWaitIdle(device->GetVkDevice());
        swapChain->Recreate();
        renderer->RecreateFrameResources();
    }

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    double previousX = 0.0;
    double previousY = 0.0;

    void mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                leftMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                leftMouseDown = false;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                rightMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                rightMouseDown = false;
            }
        }
    }

    void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition) {
        if (leftMouseDown) {
            double sensitivity = 0.5;
            float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
            float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);

            camera->UpdateOrbit(deltaX, deltaY, 0.0f);

            previousX = xPosition;
            previousY = yPosition;
        } else if (rightMouseDown) {
            double deltaZ = static_cast<float>((previousY - yPosition) * 0.05);

            camera->UpdateOrbit(0.0f, 0.0f, deltaZ);

            previousY = yPosition;
        }
    }
}

int main() {

	system("compiler.bat");
	
    static constexpr char* applicationName = "Organic Mesh Growth";
    InitializeWindow(1280, 720, applicationName);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    Instance* instance = new Instance(applicationName, glfwExtensionCount, glfwExtensions);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->GetVkInstance(), GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    instance->PickPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, surface);
/*
	// Uncomment and break for debugging purposes
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(instance->GetPhysicalDevice(), &deviceProperties);
*/
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.tessellationShader = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    device = instance->CreateDevice(QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, deviceFeatures);

    swapChain = device->CreateSwapChain(surface, 5);

    camera = new Camera(device, 1280.f / 720.f);

    VkCommandPoolCreateInfo transferPoolInfo = {};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Transfer];
    transferPoolInfo.flags = 0;

    VkCommandPool transferCommandPool;
    if (vkCreateCommandPool(device->GetVkDevice(), &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    VkImage grassImage;
    VkDeviceMemory grassImageMemory;
    Image::FromFile(device,
        transferCommandPool,
        "images/sphere_lit_1.png",
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        grassImage,
        grassImageMemory
    );

    float planeDim = 1.f;
    float halfWidth = planeDim * 0.5f;
    Model* cube = new Model(device, transferCommandPool,
        {
            
			// UP face
			{ { -halfWidth, halfWidth, halfWidth }},
            { { halfWidth, halfWidth, halfWidth } },
            { { halfWidth, halfWidth, -halfWidth } },
			{ { -halfWidth, halfWidth, -halfWidth } },
			
			// DOWN face
			{ { -halfWidth, -halfWidth, halfWidth } },
			{ { halfWidth, -halfWidth, halfWidth } },
			{ { halfWidth, -halfWidth, -halfWidth } },
			{ { -halfWidth, -halfWidth, -halfWidth } },

			// RIGHT face
			{ { halfWidth, -halfWidth, halfWidth } },
			{ { halfWidth, halfWidth, halfWidth } },
			{ { halfWidth, halfWidth, -halfWidth } },
			{ { halfWidth, -halfWidth, -halfWidth } },

			// LEFT face
			{ { -halfWidth, -halfWidth, halfWidth } },
			{ { -halfWidth, halfWidth, halfWidth } },
			{ { -halfWidth, halfWidth, -halfWidth } },
			{ { -halfWidth, -halfWidth, -halfWidth } },

			// FRONT face
			{ { -halfWidth, halfWidth, halfWidth } },
			{ { halfWidth, halfWidth, halfWidth } },
			{ { halfWidth, -halfWidth, halfWidth } },
			{ { -halfWidth, -halfWidth, halfWidth } },

			// BACK face
			{ { -halfWidth, halfWidth, -halfWidth } },
			{ { halfWidth, halfWidth, -halfWidth } },
			{ { halfWidth, -halfWidth, -halfWidth } },
			{ { -halfWidth, -halfWidth, -halfWidth } },
        },
        {
			0, 1, 2, 2, 3, 0, 
			6, 5, 4, 4, 7, 6,
			10, 9, 8, 8, 11, 10,
			12, 13, 14, 14, 15, 12,
			18, 17, 16, 16, 19, 18,
			20, 21, 22, 22, 23, 20
		}
    );
    cube->SetTexture(grassImage);

    vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

    Scene* scene = new Scene(device);
    scene->AddModel(cube);
	scene->CreateSceneSDF();
	scene->CreateVectorField();
	scene->LoadMesh("meshes/dragon.obj");

    renderer = new Renderer(device, swapChain, scene, camera);
	renderer->GenerateSceneSDF();

	float delta = scene->UpdateTime();

	std::cout << "SDF generated in " << delta << " seconds " << std::endl;
	
    glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
    glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
    glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);
	
    while (!ShouldQuit()) {
        glfwPollEvents();
        scene->UpdateTime();
        renderer->Frame();
    }

    vkDeviceWaitIdle(device->GetVkDevice());

    vkDestroyImage(device->GetVkDevice(), grassImage, nullptr);
    vkFreeMemory(device->GetVkDevice(), grassImageMemory, nullptr);

    delete scene;
    delete cube;
    delete camera;
    delete renderer;
    delete swapChain;
    delete device;
    delete instance;
    DestroyWindow();
    return 0;
}
