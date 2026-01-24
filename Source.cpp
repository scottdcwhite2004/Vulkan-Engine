#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>

#include "ObjLoader.h"
#include "CameraManager.h"
#include "InputManager.h"
#include "Vertex.h"
#include "Shape.h"
#include "Mesh.h"
#include "Cube.h"
#include "Sphere.h"
#include "Cylinder.h"
#include "Material.h"
#include "textureManager.h"
#include "Light.h"
#include "Cubemap.h"
#include "GraphicsPipelineBuilder.h"
#include "particleSystem.h"
#include "GlobeScene.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct TimeUBO
{
	float time;
    float pad0;
    float pad1;
    float pad2;
};

struct ShadowUBO {
    alignas(16) glm::mat4 lightView;
    alignas(16) glm::mat4 lightProj;
};

std::vector<Vertex> vertices = {
    // -X -Y +Z
    { {-1.0f, -1.0f,  1.0f}, {0,0,0}, {0,0} },
    // +X -Y +Z
    { { 1.0f, -1.0f,  1.0f}, {0,0,0}, {0,0} },
    // +X +Y +Z
    { { 1.0f,  1.0f,  1.0f}, {0,0,0}, {0,0} },
    // -X +Y +Z
    { {-1.0f,  1.0f,  1.0f}, {0,0,0}, {0,0} },

    // -X -Y -Z
    { {-1.0f, -1.0f, -1.0f}, {0,0,0}, {0,0} },
    // +X -Y -Z
    { { 1.0f, -1.0f, -1.0f}, {0,0,0}, {0,0} },
    // +X +Y -Z
    { { 1.0f,  1.0f, -1.0f}, {0,0,0}, {0,0} },
    // -X +Y -Z
    { {-1.0f,  1.0f, -1.0f}, {0,0,0}, {0,0} },
};

std::vector<uint16_t> indices = {
    // +Z (front)
    0, 2, 1, 0, 3, 2,
    // -Z (back)
    4, 5, 6, 4, 6, 7,
    // +X (right)
    1, 2, 6, 1, 6, 5,
    // -X (left)
    0, 7, 3, 0, 4, 7,
    // +Y (top)
    3, 7, 6, 3, 6, 2,
    // -Y (bottom)
    0, 1, 5, 0, 5, 4,
};

void loadModel()
{
    

}

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    Mesh _mesh;
    Material _material;
    Material _sphereMaterial;
	Material _cylinderMaterial;
	Cube _cube;
	Sphere _sphere;
	Cylinder _cylinder;
    Sphere _globe;
    Cylinder _plane;
    GlobeScene _scene;


    std::vector<VkSemaphore> imagePresentSemaphores;

	std::vector<Shape*> _shapes;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

	VkPipeline particlePipeline;
	VkPipelineLayout particlePipelineLayout;

    VkCommandPool commandPool;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage shadowImage;
	VkDeviceMemory shadowImageMemory;
	VkImageView shadowImageView;
	VkSampler shadowSampler;

	VkRenderPass shadowRenderPass = VK_NULL_HANDLE;
	VkFramebuffer shadowFrameBuffer = VK_NULL_HANDLE;

	VkPipeline shadowPipeline = VK_NULL_HANDLE;
	VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout shadowDescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkBuffer> shadowUniformBuffers;
	std::vector<VkDeviceMemory> shadowUniformBuffersMemory;
	std::vector<void*> shadowUniformBuffersMapped;
	std::vector<VkDescriptorSet> shadowDescriptorSets;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

	std::vector<VkBuffer> lightUniformBuffers;
	std::vector<VkDeviceMemory> lightUniformBuffersMemory;
	std::vector<void*> lightUniformBuffersMapped;

	std::vector<VkBuffer> timeBuffer;
	std::vector<VkDeviceMemory> timeBufferMemory;
	std::vector<void*> timeBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

	CameraManager cameraManager;
    Camera camera1;
	Camera camera2;
	Camera camera3;

    textureManager texManager;

    std::vector<Light> _lights;
    Light pt;
	Light dir;

    VkPipeline skyboxPipeline = VK_NULL_HANDLE;
    VkPipelineLayout skyboxPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout skyboxDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet skyboxDescriptorSet = VK_NULL_HANDLE;
    Cubemap skybox;

    RenderContext _ctx{};

	VkPipeline globePipeline = VK_NULL_HANDLE;
	VkPipelineLayout globePipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSet globeDescriptorSet = VK_NULL_HANDLE;

	VkPipeline outlinePipeline = VK_NULL_HANDLE;

	VkPipeline postProcessPipeline = VK_NULL_HANDLE;
	VkPipelineLayout postProcessPipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool postProcessDescriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> postProcessDescriptorSets;
	VkDescriptorSetLayout postProcessDescriptorSetLayout = VK_NULL_HANDLE;

	VkImage offscreenImage = VK_NULL_HANDLE;
	VkDeviceMemory offscreenImageMemory = VK_NULL_HANDLE;
	VkImageView offscreenImageView = VK_NULL_HANDLE;
	VkSampler offscreenSampler = VK_NULL_HANDLE;

	bool offscreenInitialized = false;

	VkRenderPass offscreenRenderPass = VK_NULL_HANDLE;
	VkFramebuffer offscreenFramebuffer = VK_NULL_HANDLE;

	VkPipeline sceneOffscreenPipeline = VK_NULL_HANDLE;

	VkPipeline phongPipeline = VK_NULL_HANDLE;
	VkPipelineLayout phongPipelineLayout = VK_NULL_HANDLE;

	VkPipeline gouraudPipeline = VK_NULL_HANDLE;
	VkPipelineLayout gouraudPipelineLayout = VK_NULL_HANDLE;
    
    std::chrono::steady_clock::time_point _lastFrameTime;
    float _deltaTime = 0.0f;

    std::vector<particleSystem> _particleSystems;

    bool framebufferResized = false;

    float _burstTimer = 0.0f;
    float _burstInterval = 0.75f;

    float _timeScale = 1.0f;

	VkBuffer particleQuadVB = VK_NULL_HANDLE;
	VkDeviceMemory particleQuadVBMemory = VK_NULL_HANDLE;
	VkBuffer particleQuadIB = VK_NULL_HANDLE;
	VkDeviceMemory particleQuadIBMemory = VK_NULL_HANDLE;
	uint32_t particleQuadIndexCount = 0;

    // Simple rain settings
    glm::vec2 _rainHalfSizeXZ{ 50.0f, 50.0f };  // half-extent over XZ
    float _rainTopY = 30.0f;                    // spawn Y height
    float _rainEmissionRate = 2000.0f;          // particles per second
    glm::vec2 _rainWindXZ{ 1.0f, 0.2f };        // wind along X/Z
    float _rainSpeedMin = 18.0f;
    float _rainSpeedMax = 26.0f;
    float _rainLifeMin = 1.0f;
    float _rainLifeMax = 2.5f;

    VkImage maskImage = VK_NULL_HANDLE;
    VkDeviceMemory maskImageMemory = VK_NULL_HANDLE;
    VkImageView maskImageView = VK_NULL_HANDLE;
    VkSampler maskSampler = VK_NULL_HANDLE;

    VkRenderPass maskRenderPass = VK_NULL_HANDLE;
    VkFramebuffer maskFramebuffer = VK_NULL_HANDLE;

    VkPipeline maskPipeline = VK_NULL_HANDLE;
    VkPipelineLayout maskPipelineLayout = VK_NULL_HANDLE;


    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        InputManager::Init(window);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {

		camera1 = Camera(glm::vec3(0.0f, 10.0f, -150.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, WIDTH / (float)HEIGHT, 0.1f, 300.0f);
		camera2 = Camera(glm::vec3(0.0f, 4.0f, -40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, WIDTH / (float)HEIGHT, 0.1f, 300.0f);
		camera3 = Camera(glm::vec3(0.0f, 4.0f, 40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, WIDTH / (float)HEIGHT, 0.1f, 300.0f);
		cameraManager.addCamera(camera1);
		cameraManager.addCamera(camera2);
		cameraManager.addCamera(camera3);


        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
		createShadowDescriptorSetLayoutOnly();
		createDescriptorPool();
        createShadowResources();
        createGraphicsPipeline();
		createPhongPipeline();
		createGouraudPipeline();
        createCommandPool();
        texManager.initialize(device, physicalDevice, commandPool, graphicsQueue);
        texManager.addTexture("sand", "textures/sand.jpg");
		texManager.addTexture("camel", "textures/CamelTexture.png");
		texManager.addTexture("candle", "textures/CandleTexture.jpg");
		texManager.addTexture("cabin", "textures/WoodCabinTexture.jpg");
		texManager.addTexture("rock", "textures/rock.png");
		texManager.addTexture("cactus", "textures/cactus.png");
        createDepthResources();
        createFramebuffers();
		createPerImageSemaphores();
        
        createTextureImage();
        createTextureImageView();
        createTextureSampler(textureSampler);
		
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
		createParticleQuadGeometry();
        createUniformBuffers();
        setupPostProcess();

        _material = Material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, 0.5f, texManager.getTexture("cabin"));
		_sphereMaterial = Material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, 0.5f, texManager.getTexture("sand"));
        _mesh = Mesh(glm::vec3(0.0f, 4.0f, 0.0f), _material, "models/Cabin.obj");
		_cylinder = Cylinder(glm::vec3(0.0f, 0.0f, 0.0f), _sphereMaterial, 100.0f, 5.0, 32);
		_globe = Sphere(glm::vec3(0.0f, 0.0, 0.0f), Material(glm::vec4(1.0f), 0.5f, 0.5f, texManager.getTexture("earth")), 100.0f);
		_shapes.push_back(&_mesh);
		_shapes.push_back(&_cylinder);
        _lights.clear();

        Light sun;
        sun.setType(LightType::Directional);
        sun.setColor(glm::vec3(1.0f, 0.95f, 0.9f)); // warm sunlight
        sun.setAmbient(0.2f);
        sun.setSpecular(12.0f);

        Light moon;
        moon.setType(LightType::Directional);
        moon.setColor(glm::vec3(0.75f, 0.8f, 1.0f)); // cool moonlight
        moon.setAmbient(0.05f);
        moon.setSpecular(6.0f);

        _lights.push_back(sun);
        _lights.push_back(moon);

		std::vector<VkDescriptorBufferInfo> lightinBufferInfos(MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
			lightinBufferInfos[i].buffer = lightUniformBuffers[i];
			lightinBufferInfos[i].offset = 0;
			lightinBufferInfos[i].range = sizeof(LightingUBOCPU);
        }

		createParticlePipeline();

        createSkyboxDescriptorSetLayout();
        createSkyboxPipelineLayout();
		skybox = Cubemap(device, physicalDevice, commandPool, graphicsQueue);
        
        int texWidth, texHeight, texChannels;

        const std::array<std::string, 6> filenames{ "textures/sky_right.tga","textures/sky_left.tga","textures/sky_up.tga","textures/sky_down.tga","textures/sky_back.tga","textures/sky_front.tga" };
        std::vector<stbi_uc*> pixels(6);
        VkDeviceSize layerSize = 0;

        for (int i = 0; i < 6; i++)
        {
            pixels[i] = stbi_load(filenames[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels[i])
            {
                throw std::runtime_error("Failed to load cubemap face: " + filenames[i]);
            }
            layerSize = texWidth * texHeight * 4;
        }

        std::array<const void*, 6> faces{ pixels[0],pixels[1],pixels[2],pixels[3],pixels[4],pixels[5] };

        skybox.create(static_cast<uint32_t>(texWidth), VK_FORMAT_R8G8B8A8_SRGB, faces);
        for (int i = 0; i < 6; ++i) {
            stbi_image_free(pixels[i]);
        }
        allocateSkyboxDescriptorSet(skybox.view(), skybox.sampler());
		createGlobePipeline();
		createGlobeOutlinePipeline();
        createSkyboxPipeline();


        _ctx.device = device;
        _ctx.physicalDevice = physicalDevice;
        _ctx.graphicsQueue = graphicsQueue;
        _ctx.commandPool = commandPool;
        _ctx.descriptorSetLayout = descriptorSetLayout;
        _ctx.descriptorPool = descriptorPool;
		_scene = GlobeScene(texManager, cameraManager);
		_scene.initializeScene();
		_scene.loadScene();
        _scene.uploadScene(_ctx, MAX_FRAMES_IN_FLIGHT, textureImageView, textureSampler, lightinBufferInfos);
		auto candleLights = _scene.getCandleLights();
        for (const auto& light : candleLights)
        {
            _lights.push_back(light);
        }


        for (Shape* shape : _shapes)
        {
            shape->create();
			shape->upload(_ctx, MAX_FRAMES_IN_FLIGHT, shape->getMaterial().getTextureImageView(), shape->getMaterial().getTextureSampler(), lightinBufferInfos);
		}

		_globe.create();
		_globe.upload(_ctx, MAX_FRAMES_IN_FLIGHT, _globe.getMaterial().getTextureImageView(), _globe.getMaterial().getTextureSampler(), lightinBufferInfos);

        createDescriptorSets();
        
        _particleSystems.clear();
        _particleSystems.push_back(particleSystem(glm::vec3(0.0f, 0.0f, 0.0f), 20000));
        for (auto& ps : _particleSystems) {
            ps.create(_ctx);
            // Do not burst; rain will emit continuously
            ps.uploadDescriptors(_ctx);
        }
		_scene.setRainParticleSystem(&_particleSystems[0]);

        createCommandBuffers();
        createSyncObjects();

		_lastFrameTime = std::chrono::steady_clock::now();
    }

    void createShadowResources()
    {
        // 1) Depth image used as sampled shadow map
        VkFormat shadowFormat = VK_FORMAT_D32_SFLOAT;
        createImage(swapChainExtent.width, swapChainExtent.height, shadowFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowImage, shadowImageMemory);

        shadowImageView = createImageView(shadowImage, shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        // 2) sampler (use compare sampler for sampler2DShadow)
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // common for shadow maps
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // for sampler2DShadow
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow sampler!");
        }

        // 3) Shadow render pass (depth-only)
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = shadowFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // we want depth written so we can sample
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 0;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rpci.attachmentCount = 1;
        rpci.pAttachments = &depthAttachment;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dep;

        if (vkCreateRenderPass(device, &rpci, nullptr, &shadowRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow render pass!");
        }

        // 4) Framebuffer
        VkFramebufferCreateInfo fbci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbci.renderPass = shadowRenderPass;
        fbci.attachmentCount = 1;
        fbci.pAttachments = &shadowImageView;
        fbci.width = swapChainExtent.width;
        fbci.height = swapChainExtent.height;
        fbci.layers = 1;
        if (vkCreateFramebuffer(device, &fbci, nullptr, &shadowFrameBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow framebuffer!");
        }

        // 5) Shadow descriptor set layout (single UBO for light matrices at set=1)
        if (shadowDescriptorSetLayout == VK_NULL_HANDLE)
        {
            VkDescriptorSetLayoutBinding shUboBinding{};
            shUboBinding.binding = 0;
            shUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            shUboBinding.descriptorCount = 1;
            shUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            shUboBinding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding shSamplerBinding{};
            shSamplerBinding.binding = 1;
            shSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            shSamplerBinding.descriptorCount = 1;
            shSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            shSamplerBinding.pImmutableSamplers = nullptr;

            std::array<VkDescriptorSetLayoutBinding, 2> shBindings = { shUboBinding, shSamplerBinding };
            VkDescriptorSetLayoutCreateInfo shLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            shLayoutInfo.bindingCount = static_cast<uint32_t>(shBindings.size());
            shLayoutInfo.pBindings = shBindings.data();
            if (vkCreateDescriptorSetLayout(device, &shLayoutInfo, nullptr, &shadowDescriptorSetLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shadow descriptor set layout!");
            }
        }

        // 6) Shadow uniform buffers (per-frame)
        VkDeviceSize shUBOSize = sizeof(ShadowUBO);
        shadowUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        shadowUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        shadowUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            createBuffer(shUBOSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                shadowUniformBuffers[i], shadowUniformBuffersMemory[i]);
            if (vkMapMemory(device, shadowUniformBuffersMemory[i], 0, shUBOSize, 0, &shadowUniformBuffersMapped[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to map shadow UBO");
            }
        }

        // 7) Allocate descriptor sets for shadow (re-using existing descriptorPool)
        std::vector<VkDescriptorSetLayout> shLayouts(MAX_FRAMES_IN_FLIGHT, shadowDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = shLayouts.data();

        shadowDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, shadowDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate shadow descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = shadowUniformBuffers[i];
            bufInfo.offset = 0;
            bufInfo.range = sizeof(ShadowUBO);

            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // we will transition after render
            imgInfo.imageView = shadowImageView;
            imgInfo.sampler = shadowSampler;

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = shadowDescriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &bufInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = shadowDescriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[1].descriptorCount = 1;
            writes[1].pImageInfo = &imgInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }

        // 8) shadow pipeline - very simple vertex shader that transforms by light matrices and no color output
        auto vsCode = readFile("shaders/shadow.vert.spv");
        auto fsCode = readFile("shaders/shadow.frag.spv");
        VkShaderModule vs = createShaderModule(vsCode);
        VkShaderModule fs = createShaderModule(fsCode);

        // Use only position attribute (location = 0) to match shadow.vert which consumes only position.
        auto bindingDesc = Vertex::getBindingDescription(); // reuse stride/binding
        VkVertexInputAttributeDescription posAttr{};
        posAttr.location = 0;
        posAttr.binding = bindingDesc.binding;
        posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
        posAttr.offset = static_cast<uint32_t>(offsetof(Vertex, pos));

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bindingDesc;
        vi.vertexAttributeDescriptionCount = 1;
        vi.pVertexAttributeDescriptions = &posAttr;

        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1; vp.scissorCount = 1;
        // pViewports/pScissors intentionally left NULL because we'll declare them dynamic below.

        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_BACK_BIT;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;

        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Make viewport and scissor dynamic to avoid providing pViewports/pScissors here.
        std::vector<VkDynamicState> dyn{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dsi{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dsi.dynamicStateCount = static_cast<uint32_t>(dyn.size());
        dsi.pDynamicStates = dyn.data();

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vs;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fs;
        stages[1].pName = "main";

        // Pipeline layout: set 0 = existing descriptorSetLayout (model/view/proj UBO) ; set 1 = shadowDescriptorSetLayout
        VkDescriptorSetLayout setLayoutsArr[2] = { descriptorSetLayout, shadowDescriptorSetLayout };

        VkPipelineLayoutCreateInfo pli{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        if (shadowDescriptorSetLayout != VK_NULL_HANDLE) {
            pli.setLayoutCount = 2;
            pli.pSetLayouts = setLayoutsArr;
        }
        else {
            pli.setLayoutCount = 1;
            pli.pSetLayouts = &descriptorSetLayout;
        }

        if (vkCreatePipelineLayout(device, &pli, nullptr, &shadowPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo gpci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gpci.stageCount = 2;
        gpci.pStages = stages;
        gpci.pVertexInputState = &vi;
        gpci.pInputAssemblyState = &ia;
        gpci.pViewportState = &vp;
        gpci.pRasterizationState = &rs;
        gpci.pDepthStencilState = &ds;
        gpci.pMultisampleState = &ms;
        gpci.pDynamicState = &dsi; // viewport & scissor are dynamic
        gpci.layout = shadowPipelineLayout;
        gpci.renderPass = shadowRenderPass;
        gpci.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, nullptr, &shadowPipeline) != VK_SUCCESS) {
            vkDestroyShaderModule(device, fs, nullptr);
            vkDestroyShaderModule(device, vs, nullptr);
            throw std::runtime_error("failed to create shadow pipeline!");
        }

        vkDestroyShaderModule(device, fs, nullptr);
        vkDestroyShaderModule(device, vs, nullptr);
	}

    void createShadowDescriptorSetLayoutOnly()
    {
        if (shadowDescriptorSetLayout != VK_NULL_HANDLE) return;

        VkDescriptorSetLayoutBinding shUboBinding{};
        shUboBinding.binding = 0;
        shUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shUboBinding.descriptorCount = 1;
        shUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        shUboBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding shSamplerBinding{};
        shSamplerBinding.binding = 1;
        shSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shSamplerBinding.descriptorCount = 1;
        shSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        shSamplerBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> shBindings = { shUboBinding, shSamplerBinding };
        VkDescriptorSetLayoutCreateInfo shLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        shLayoutInfo.bindingCount = static_cast<uint32_t>(shBindings.size());
        shLayoutInfo.pBindings = shBindings.data();
        if (vkCreateDescriptorSetLayout(device, &shLayoutInfo, nullptr, &shadowDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow descriptor set layout!");
        }
    }

    void createMaskImage()
    {
        VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8_UNORM;
        imageInfo.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageInfo, nullptr, &maskImage) != VK_SUCCESS) {
            throw std::runtime_error("failed to create mask image!");
        }

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(device, maskImage, &memReq);

        VkMemoryAllocateInfo alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc.allocationSize = memReq.size;
        alloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &alloc, nullptr, &maskImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate mask image memory!");
        }
        vkBindImageMemory(device, maskImage, maskImageMemory, 0);

        // view + sampler
        maskImageView = createImageView(maskImage, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        createTextureSampler(maskSampler);
    }

    void createMaskRenderPass()
    {
        VkAttachmentDescription color{};
        color.format = VK_FORMAT_R8_UNORM;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rpci.attachmentCount = 1;
        rpci.pAttachments = &color;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dep;

        if (vkCreateRenderPass(device, &rpci, nullptr, &maskRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create mask render pass!");
        }
    }

    void createMaskFramebuffer()
    {
        VkImageView attachments[] = { maskImageView };
        VkFramebufferCreateInfo fci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fci.renderPass = maskRenderPass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = swapChainExtent.width;
        fci.height = swapChainExtent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(device, &fci, nullptr, &maskFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create mask framebuffer!");
        }
	}

    void createMaskPipeine()
    {
        auto vertShaderCode = readFile("shaders/Gouraud.vert.spv");
        auto fragShaderCode = readFile("shaders/mask.frag.spv");
        VkShaderModule v = createShaderModule(vertShaderCode);
        VkShaderModule f = createShaderModule(fragShaderCode);

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bindingDescription;
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vi.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineColorBlendAttachmentState cbAtt{};
        cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
        cbAtt.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cb.attachmentCount = 1;
        cb.pAttachments = &cbAtt;

        std::vector<VkDynamicState> dyn{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dsi{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dsi.dynamicStateCount = static_cast<uint32_t>(dyn.size());
        dsi.pDynamicStates = dyn.data();

        // Reuse your existing pipelineLayout (UBO(0), sampler(1), lighting(2)) so Mesh::draw binds correctly
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, v, "main" };
        stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, f, "main" };

        VkGraphicsPipelineCreateInfo gpci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gpci.stageCount = 2;
        gpci.pStages = stages;
        gpci.pVertexInputState = &vi;
        gpci.pInputAssemblyState = &ia;
        gpci.pViewportState = &vp;
        gpci.pRasterizationState = &rs;
        gpci.pMultisampleState = &ms;
        gpci.pDepthStencilState = &ds;
        gpci.pColorBlendState = &cb;
        gpci.pDynamicState = &dsi;
        gpci.layout = pipelineLayout;           // reuse existing layout
        gpci.renderPass = maskRenderPass;       // mask pass
        gpci.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, nullptr, &maskPipeline) != VK_SUCCESS) {
            vkDestroyShaderModule(device, f, nullptr);
            vkDestroyShaderModule(device, v, nullptr);
            throw std::runtime_error("failed to create mask pipeline!");
        }

        vkDestroyShaderModule(device, f, nullptr);
        vkDestroyShaderModule(device, v, nullptr);
    }

    

    void createParticleQuadGeometry()
    {
        // A thin vertical quad centered at origin (world-aligned)
        const float halfWidth = 0.03f;
        const float halfHeight = 0.3f;

        std::vector<Vertex> verts{
            // pos                         color          uv      normal
            { { -halfWidth, -halfHeight, 0.0f }, {1,1,1}, {0,0}, {0,0,1} },
            { {  halfWidth, -halfHeight, 0.0f }, {1,1,1}, {1,0}, {0,0,1} },
            { {  halfWidth,  halfHeight, 0.0f }, {1,1,1}, {1,1}, {0,0,1} },
            { { -halfWidth,  halfHeight, 0.0f }, {1,1,1}, {0,1}, {0,0,1} },
        };

        std::vector<uint16_t> idx{ 0,1,2, 2,3,0 };
        particleQuadIndexCount = static_cast<uint32_t>(idx.size());

        // VB
        VkDeviceSize vbSize = sizeof(Vertex) * verts.size();
        VkBuffer stagingVB; VkDeviceMemory stagingVBMem;
        createBuffer(vbSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingVB, stagingVBMem);
        void* mapped = nullptr;
        vkMapMemory(device, stagingVBMem, 0, vbSize, 0, &mapped);
        std::memcpy(mapped, verts.data(), static_cast<size_t>(vbSize));
        vkUnmapMemory(device, stagingVBMem);

        createBuffer(vbSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, particleQuadVB, particleQuadVBMemory);
        copyBuffer(stagingVB, particleQuadVB, vbSize);
        vkDestroyBuffer(device, stagingVB, nullptr);
        vkFreeMemory(device, stagingVBMem, nullptr);

        // IB
        VkDeviceSize ibSize = sizeof(uint16_t) * idx.size();
        VkBuffer stagingIB; VkDeviceMemory stagingIBMem;
        createBuffer(ibSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingIB, stagingIBMem);
        vkMapMemory(device, stagingIBMem, 0, ibSize, 0, &mapped);
        std::memcpy(mapped, idx.data(), static_cast<size_t>(ibSize));
        vkUnmapMemory(device, stagingIBMem);

        createBuffer(ibSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, particleQuadIB, particleQuadIBMemory);
        copyBuffer(stagingIB, particleQuadIB, ibSize);
        vkDestroyBuffer(device, stagingIB, nullptr);
        vkFreeMemory(device, stagingIBMem, nullptr);
    }

    void createsceneOffscreenPipeline()
    {
        auto vertShaderCode = readFile("shaders/Gouraud.vert.spv");
        auto fragShaderCode = readFile("shaders/Gouraud.frag.spv");
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertShaderModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragShaderModule;
        stages[1].pName = "main";

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = offscreenRenderPass; // offscreen pass
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &sceneOffscreenPipeline) != VK_SUCCESS) {
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            throw std::runtime_error("failed to create scene offscreen pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

    void createPostProcessImage()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageInfo, nullptr, &offscreenImage) != VK_SUCCESS) {
            throw std::runtime_error("failed to create post-process image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, offscreenImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &offscreenImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate offscreen image memory!");
        }

        vkBindImageMemory(device, offscreenImage, offscreenImageMemory, 0);

        // create view + sampler
        createPostProcessImageView();
        createPostProcessSampler();
    }

    void createPostProcessImageView()
    {
        if (offscreenImage == VK_NULL_HANDLE) return;
        offscreenImageView = createImageView(offscreenImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createPostProcessSampler()
    {
        createTextureSampler(offscreenSampler);
    }

    void createPostProcessPipeline()
    {
        auto vertShaderCode = readFile("shaders/fullscreen.vert.spv");
        auto fragShaderCode = readFile("shaders/fullscreen.frag.spv");
        VkShaderModule v = createShaderModule(vertShaderCode);
        VkShaderModule f = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = v;
        stages[0].pName = "main";
        stages[0].pSpecializationInfo = nullptr;
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = f;
        stages[1].pName = "main";
        stages[1].pSpecializationInfo = nullptr;

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 0;
        vi.pVertexBindingDescriptions = nullptr;
        vi.vertexAttributeDescriptionCount = 0;
        vi.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ia.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo vp{};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1;
        vp.scissorCount = 1;
        vp.pViewports = nullptr;
        vp.pScissors = nullptr;

        VkPipelineRasterizationStateCreateInfo rs{};
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.depthBiasEnable = VK_FALSE;
        rs.lineWidth = 1.f;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        ms.sampleShadingEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState cbAtt{};
        cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cbAtt.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.logicOpEnable = VK_FALSE;
        cb.logicOp = VK_LOGIC_OP_COPY;
        cb.attachmentCount = 1;
        cb.pAttachments = &cbAtt;
        cb.blendConstants[0] = 0.0f;
        cb.blendConstants[1] = 0.0f;
        cb.blendConstants[2] = 0.0f;
        cb.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dyn{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dsi{};
        dsi.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dsi.dynamicStateCount = static_cast<uint32_t>(dyn.size());
        dsi.pDynamicStates = dyn.data();

        VkPushConstantRange pushRange{};
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.offset = 0;
		pushRange.size = sizeof(bool);

        VkPipelineLayoutCreateInfo pli{};
        pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pli.setLayoutCount = 1;
        pli.pSetLayouts = &postProcessDescriptorSetLayout;
        pli.pushConstantRangeCount = 0;
		pli.pPushConstantRanges = &pushRange;
        if (vkCreatePipelineLayout(device, &pli, nullptr, &postProcessPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create postprocess pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo gpci{};
        gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gpci.stageCount = 2;
        gpci.pStages = stages;
        gpci.pVertexInputState = &vi;
        gpci.pInputAssemblyState = &ia;
        gpci.pViewportState = &vp;
        gpci.pRasterizationState = &rs;
        gpci.pMultisampleState = &ms;
        gpci.pDepthStencilState = &ds;
        gpci.pColorBlendState = &cb;
        gpci.pDynamicState = &dsi;
        gpci.layout = postProcessPipelineLayout;
        gpci.renderPass = renderPass;
        gpci.subpass = 0;
        gpci.basePipelineHandle = VK_NULL_HANDLE;
        gpci.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, nullptr, &postProcessPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create postprocess pipeline!");
        }

        vkDestroyShaderModule(device, f, nullptr);
        vkDestroyShaderModule(device, v, nullptr);
    }

    void createPostProcessDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 1;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // NEW: mask sampler at binding 2
        VkDescriptorSetLayoutBinding maskLayoutBinding{};
        maskLayoutBinding.binding = 2;
        maskLayoutBinding.descriptorCount = 1;
        maskLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        maskLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = { samplerLayoutBinding, uboLayoutBinding, maskLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &postProcessDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create post-process descriptor set layout!");
        }
    }

    void createPostProcessDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        // 1 UBO per set
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // 2 samplers per set: scene + mask
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(2 * MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &postProcessDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createPostProcessDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, postProcessDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = postProcessDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        postProcessDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, postProcessDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = timeBuffer[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(TimeUBO);

            VkDescriptorImageInfo sceneImage{};
            sceneImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            sceneImage.imageView = offscreenImageView;
            sceneImage.sampler = offscreenSampler;

            // NEW: mask image info
            VkDescriptorImageInfo maskImageInfo{};
            maskImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            maskImageInfo.imageView = maskImageView;
            maskImageInfo.sampler = maskSampler;

            std::array<VkWriteDescriptorSet, 3> writes{};

            // (0) Scene sampler
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = postProcessDescriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &sceneImage;

            // (1) Time UBO
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = postProcessDescriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufferInfo;

            // NEW (2) Mask sampler
            writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[2].dstSet = postProcessDescriptorSets[i];
            writes[2].dstBinding = 2;
            writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[2].descriptorCount = 1;
            writes[2].pImageInfo = &maskImageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }

    void setupPostProcess()
    {
        createPostProcessImage();
        createOffscreenRenderPass();
        createOffscreenFramebuffer();
        createMaskImage();
        createMaskRenderPass();
        createMaskFramebuffer();
        createPostProcessDescriptorSetLayout();
        createPostProcessDescriptorPool();
        createPostProcessDescriptorSets();
        createsceneOffscreenPipeline();
        // create the mask pipeline before we try to use it
        createMaskPipeine();
        createPostProcessPipeline();
    }

    void createParticlePipeline()
    {
        auto vertCode = readFile("shaders/particle.vert.spv");
        auto fragCode = readFile("shaders/particle.frag.spv");
        VkShaderModule v = createShaderModule(vertCode);
        VkShaderModule f = createShaderModule(fragCode);

        auto bindingDesc0 = Vertex::getBindingDescription();
        auto attrArray0 = Vertex::getAttributeDescriptions();
        std::vector<VkVertexInputAttributeDescription> allAttrs(attrArray0.begin(), attrArray0.end());

        VkVertexInputBindingDescription bindingDesc1{};
        bindingDesc1.binding = 1;
        bindingDesc1.stride = sizeof(Particle);
        bindingDesc1.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        VkVertexInputAttributeDescription instAttr{};
        instAttr.location = 5;
        instAttr.binding = 1;
        instAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
        instAttr.offset = static_cast<uint32_t>(offsetof(Particle, position)); // position.z
        allAttrs.push_back(instAttr);

        std::array<VkVertexInputBindingDescription, 2> bindings{ bindingDesc0, bindingDesc1 };

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vi.pVertexBindingDescriptions = bindings.data();
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(allAttrs.size());
        vi.pVertexAttributeDescriptions = allAttrs.data();

        GraphicsPipelineBuilder b;
        b.setDevice(device)
            .setRenderPass(renderPass)
            .setPipelineLayout(pipelineLayout)
            .setVertexInput(vi)
            .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE) // disable culling
            .setMultisample(VK_SAMPLE_COUNT_1_BIT)
            .enableDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE)
            .addColorBlendAttachment(
                VK_TRUE,
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // standard alpha
                VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
            .setColorBlendLogic(VK_FALSE)
            .setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .setShaderStages(v, f);

        particlePipeline = b.build();

        vkDestroyShaderModule(device, f, nullptr);
        vkDestroyShaderModule(device, v, nullptr);
    }
        // Create outline pipeline after createGlobePipeline()
        void createGlobeOutlinePipeline()
        {
            auto vertCode = readFile("shaders/transluscent_outline.vert.spv");
            auto fragCode = readFile("shaders/transluscent_outline.frag.spv");
            VkShaderModule v = createShaderModule(vertCode);
            VkShaderModule f = createShaderModule(fragCode);

            auto bindingDesc = Vertex::getBindingDescription();
            auto attrDescs = Vertex::getAttributeDescriptions();

            VkPipelineVertexInputStateCreateInfo vi{};
            vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vi.vertexBindingDescriptionCount = 1;
            vi.pVertexBindingDescriptions = &bindingDesc;
            vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
            vi.pVertexAttributeDescriptions = attrDescs.data();

            GraphicsPipelineBuilder b;
            b.setDevice(device)
                .setRenderPass(renderPass)
                .setPipelineLayout(pipelineLayout) // reuse layout: UBO(0), sampler(1), lighting(2)
                .setVertexInput(vi)
                .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .setMultisample(VK_SAMPLE_COUNT_1_BIT)
                .enableDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE)
                .addColorBlendAttachment(VK_TRUE)
                .setColorBlendLogic(VK_FALSE)
                .setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
                .setShaderStages(v, f);

            outlinePipeline = b.build();

            vkDestroyShaderModule(device, f, nullptr);
            vkDestroyShaderModule(device, v, nullptr);
        }

    void createSkyboxDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding cubemap{};
        cubemap.binding = 0;
        cubemap.descriptorCount = 1;
        cubemap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubemap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = &cubemap;

        if (vkCreateDescriptorSetLayout(device, &info, nullptr, &skyboxDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create skybox descriptor set layout!");
        }
    }

    void createSkyboxPipelineLayout()
    {
		VkDescriptorSetLayout setLayouts[2] = { descriptorSetLayout, skyboxDescriptorSetLayout };
        VkPipelineLayoutCreateInfo pli{};
        pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pli.setLayoutCount = 2;
        pli.pSetLayouts = setLayouts;
        if (vkCreatePipelineLayout(device, &pli, nullptr, &skyboxPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create skybox pipeline layout!");
        }
    }

    void allocateSkyboxDescriptorSet(VkImageView cubeView, VkSampler cubeSampler) {
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = descriptorPool;                 // reuse existing pool
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &skyboxDescriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &ai, &skyboxDescriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate skybox descriptor set!");
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = cubeView;
        imageInfo.sampler = cubeSampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = skyboxDescriptorSet;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    void createSkyboxPipeline() {
        auto skyVertCode = readFile("shaders/skybox.vert.spv");
        auto skyFragCode = readFile("shaders/skybox.frag.spv");
        VkShaderModule skyVert = createShaderModule(skyVertCode);
        VkShaderModule skyFrag = createShaderModule(skyFragCode);

        // Position-only binding/attributes for skybox
        VkVertexInputBindingDescription skyBinding{};
        skyBinding.binding = 0;
        skyBinding.stride = sizeof(Vertex); // if your VB uses Vertex; otherwise stride of position-only struct
        skyBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription skyAttr{};
        skyAttr.location = 0;
        skyAttr.binding = 0;
        skyAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
        skyAttr.offset = offsetof(Vertex, pos); // adjust to your position field name

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &skyBinding;
        vi.vertexAttributeDescriptionCount = 1;
        vi.pVertexAttributeDescriptions = &skyAttr;

        GraphicsPipelineBuilder b;
        b.setDevice(device)
            .setRenderPass(renderPass)
            .setPipelineLayout(skyboxPipelineLayout)
            .setVertexInput(vi)
            .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .setMultisample(VK_SAMPLE_COUNT_1_BIT)
            .enableDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE)
            .addColorBlendAttachment(VK_FALSE)
            .setColorBlendLogic(VK_FALSE)
            .setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .setShaderStages(skyVert, skyFrag);

        skyboxPipeline = b.build();

        vkDestroyShaderModule(device, skyFrag, nullptr);
        vkDestroyShaderModule(device, skyVert, nullptr);
    }
    
    void allocateGlobeDescriptorSet(VkImageView view, VkSampler sampler)
    {
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = descriptorPool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &descriptorSetLayout; // use the same layout: UBO(0), sampler(1), lighting(2)

        if (vkAllocateDescriptorSets(device, &ai, &globeDescriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate globe descriptor set!");
        }

        // Per-frame UBO and lighting remain bound via frame descriptorSets[currentFrame],
        // but for simplicity we also bind texture here using a dedicated set if needed.
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = view;
        imageInfo.sampler = sampler;

        // We still need to provide UBO + Lighting for this set if we intend to use it alone.
        // Alternatively, bind the frame descriptor set for UBO+Lighting, and bind this globe set
        // only for its sampler at binding 1. That requires the same set layout. Here we write all three.
        VkDescriptorBufferInfo uboInfo{};
        uboInfo.buffer = uniformBuffers[0]; // any valid buffer; we overwrite each frame at bind time using frame set
        uboInfo.offset = 0;
        uboInfo.range = sizeof(UniformBufferObject);

        VkDescriptorBufferInfo lightInfo{};
        lightInfo.buffer = lightUniformBuffers[0];
        lightInfo.offset = 0;
        lightInfo.range = sizeof(LightingUBO);

        std::array<VkWriteDescriptorSet, 3> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = globeDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &uboInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = globeDescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &imageInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = globeDescriptorSet;
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &lightInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void createGlobePipeline()
    {
        auto vertCode = readFile("shaders/transluscent.vert.spv");
        auto fragCode = readFile("shaders/transluscent.frag.spv");
        VkShaderModule v = createShaderModule(vertCode);
        VkShaderModule f = createShaderModule(fragCode);

        // Vertex input matches your Vertex type
        auto bindingDesc = Vertex::getBindingDescription();
        auto attrDescs = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bindingDesc;
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vi.pVertexAttributeDescriptions = attrDescs.data();

        // Enable blending for translucency and disable depth writes
        GraphicsPipelineBuilder b;
        b.setDevice(device)
            .setRenderPass(renderPass)
            .setPipelineLayout(pipelineLayout) // reuse existing layout with descriptorSetLayout
            .setVertexInput(vi)
            .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .setMultisample(VK_SAMPLE_COUNT_1_BIT)
            .enableDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE) // test on, writes off
            .addColorBlendAttachment(
                VK_TRUE,
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
            // logicOp must be disabled unless deviceFeatures.logicOp is enabled
            .setColorBlendLogic(VK_FALSE)
            .setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .setShaderStages(v, f);

        globePipeline = b.build();

        vkDestroyShaderModule(device, f, nullptr);
        vkDestroyShaderModule(device, v, nullptr);
    }

    void createPerImageSemaphores()
    {
        imagePresentSemaphores.resize(swapChainImages.size());
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imagePresentSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores for a swap chain image!");
            }
		}
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
			int camera1Index = 0;
			int camera2Index = 1;
			int camera3Index = 2;

            if(InputManager::isKeyPressed(GLFW_KEY_ESCAPE))
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                continue;
			}

            if(InputManager::isKeyPressed(GLFW_KEY_R))
            {
                
			}



            if (InputManager::isKeyPressed(GLFW_KEY_F1))
            {
				cameraManager.switchToCamera(camera1Index);
            }
			else if (InputManager::isKeyPressed(GLFW_KEY_F2))
            {
				cameraManager.switchToCamera(camera2Index);
            }
			else if (InputManager::isKeyPressed(GLFW_KEY_F3))
            {
				cameraManager.switchToCamera(camera3Index);
            }

            const float yawSpeed = glm::radians(90.0f);   // deg/s
            const float pitchSpeed = glm::radians(90.0f); // deg/s
            const float panSpeed = 5.0f;                  // units/s

            const bool ctrlDown =
                InputManager::isKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
                InputManager::isKeyPressed(GLFW_KEY_RIGHT_CONTROL);


            float yaw = 0.0f;
            float pitch = 0.0f;

            float panRight = 0.0f;
            float panForward = 0.0f;
            float panUp = 0.0f;

            if (!ctrlDown) {
                // Rotation with arrows
                if (InputManager::isKeyPressed(GLFW_KEY_LEFT)) {
                    yaw += yawSpeed * _deltaTime;
                }
                if (InputManager::isKeyPressed(GLFW_KEY_RIGHT)) {
                    yaw -= yawSpeed * _deltaTime;
                }
                if (InputManager::isKeyPressed(GLFW_KEY_UP)) {
                    pitch += pitchSpeed * _deltaTime;
                }
                if (InputManager::isKeyPressed(GLFW_KEY_DOWN)) {
                    pitch -= pitchSpeed * _deltaTime;
                }
                if (yaw != 0.0f || pitch != 0.0f) {
					cameraManager.rotateCurrentCamera(yaw, pitch);
                }
            }
            else {
                if (InputManager::isKeyPressed(GLFW_KEY_LEFT)) {
                    panRight -= panSpeed * _deltaTime;   // left
                }
                if (InputManager::isKeyPressed(GLFW_KEY_RIGHT)) {
                    panRight += panSpeed * _deltaTime;   // right
                }
                if (InputManager::isKeyPressed(GLFW_KEY_UP)) {
                    panForward += panSpeed * _deltaTime; // forward
                }
                if (InputManager::isKeyPressed(GLFW_KEY_DOWN)) {
                    panForward -= panSpeed * _deltaTime; // backward
                }
                if (InputManager::isKeyPressed(GLFW_KEY_PAGE_UP)) {
                    panUp += panSpeed * _deltaTime;      // up
                }
                if (InputManager::isKeyPressed(GLFW_KEY_PAGE_DOWN)) {
                    panUp -= panSpeed * _deltaTime;      // down
                }
                if (panRight != 0.0f || panForward != 0.0f || panUp != 0.0f) {
                    cameraManager.panCurrentCamera(panRight, panForward, panUp);
                }
            }

            if (InputManager::isKeyPressed(GLFW_KEY_T)) {
                const bool shiftDown =
                    InputManager::isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                    InputManager::isKeyPressed(GLFW_KEY_RIGHT_SHIFT);

                const float step = 0.1f;
                if (shiftDown) {
                    _timeScale += step;
                }
                else {
                    _timeScale -= step;
                }


                _timeScale = std::clamp(_timeScale, 0.0f, 10.0f);
            }
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanupSwapChain() {
		cleanupPostProcess();
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        for (auto& shape : _shapes) {
            shape->destroy({ device, physicalDevice, graphicsQueue, commandPool, descriptorSetLayout, descriptorPool });
        }

		_globe.destroy({ device, physicalDevice, graphicsQueue, commandPool, descriptorSetLayout, descriptorPool });

        _scene.destroyScene({ device, physicalDevice, graphicsQueue, commandPool, descriptorSetLayout, descriptorPool });

        for(auto sem: imagePresentSemaphores)
        {
            vkDestroySemaphore(device, sem, nullptr);
		}
		imagePresentSemaphores.clear();

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
            commandBuffers.clear();
        }

        if (skyboxPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, skyboxPipeline, nullptr);
            skyboxPipeline = VK_NULL_HANDLE;
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void cleanupPostProcess()
    {
        if (postProcessPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, postProcessPipeline, nullptr);
            postProcessPipeline = VK_NULL_HANDLE;
        }
        if (postProcessPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, postProcessPipelineLayout, nullptr);
            postProcessPipelineLayout = VK_NULL_HANDLE;
        }
        if (postProcessDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, postProcessDescriptorPool, nullptr);
            postProcessDescriptorPool = VK_NULL_HANDLE;
            postProcessDescriptorSets.clear();
        }

        if (offscreenFramebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, offscreenFramebuffer, nullptr);
            offscreenFramebuffer = VK_NULL_HANDLE;
        }
        if (offscreenRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, offscreenRenderPass, nullptr);
            offscreenRenderPass = VK_NULL_HANDLE;
        }
        if (offscreenSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, offscreenSampler, nullptr);
            offscreenSampler = VK_NULL_HANDLE;
        }
        if (offscreenImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, offscreenImageView, nullptr);
            offscreenImageView = VK_NULL_HANDLE;
        }
        if (offscreenImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, offscreenImage, nullptr);
            offscreenImage = VK_NULL_HANDLE;
        }
        if (offscreenImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, offscreenImageMemory, nullptr);
            offscreenImageMemory = VK_NULL_HANDLE;
        }

        offscreenInitialized = false;
    }

    void cleanup() {
        cleanupSwapChain();

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        if (particleQuadIB != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, particleQuadIB, nullptr);
            particleQuadIB = VK_NULL_HANDLE;
        }
        if (particleQuadIBMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, particleQuadIBMemory, nullptr);
            particleQuadIBMemory = VK_NULL_HANDLE;
        }
        if (particleQuadVB != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, particleQuadVB, nullptr);
            particleQuadVB = VK_NULL_HANDLE;
        }
        if (particleQuadVBMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, particleQuadVBMemory, nullptr);
            particleQuadVBMemory = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);

            vkDestroyBuffer(device, lightUniformBuffers[i], nullptr);
            vkFreeMemory(device, lightUniformBuffersMemory[i], nullptr);
        }

        if (outlinePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, outlinePipeline, nullptr);
            outlinePipeline = VK_NULL_HANDLE;
        }
        if (skyboxPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, skyboxPipeline, nullptr);
            skyboxPipeline = VK_NULL_HANDLE;
        }
        if (skyboxPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
            skyboxPipelineLayout = VK_NULL_HANDLE;
        }
        if (skyboxDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, skyboxDescriptorSetLayout, nullptr);
            skyboxDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (particlePipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, particlePipeline, nullptr);
            particlePipeline = VK_NULL_HANDLE;
        }
        if (particlePipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, particlePipelineLayout, nullptr);
            particlePipelineLayout = VK_NULL_HANDLE;
        }

        RenderContext ctx{};
        ctx.device = device;
        ctx.physicalDevice = physicalDevice;
        ctx.graphicsQueue = graphicsQueue;
        ctx.commandPool = commandPool;
        ctx.descriptorSetLayout = descriptorSetLayout;
        ctx.descriptorPool = descriptorPool;
        for (auto& sys : _particleSystems) {
            sys.destroy(ctx);
        }
        _particleSystems.clear();

        skybox.destroy();
        texManager.destroy();

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);


        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createDepthResources();
        createFramebuffers();
        createPerImageSemaphores();
        createCommandBuffers();
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        createDescriptorPool();
        createDescriptorSets();
        createPostProcessImage();
		createOffscreenRenderPass();
		createOffscreenFramebuffer();
        createPostProcessDescriptorPool();
        createPostProcessDescriptorSets();
		createsceneOffscreenPipeline();
        createPostProcessPipeline();
        offscreenInitialized = false;



        allocateSkyboxDescriptorSet(skybox.view(), skybox.sampler());
        createSkyboxPipeline();


        // Re-upload shape descriptor sets from the new pool
        _ctx.device = device;
        _ctx.physicalDevice = physicalDevice;
        _ctx.graphicsQueue = graphicsQueue;
        _ctx.commandPool = commandPool;
        _ctx.descriptorSetLayout = descriptorSetLayout;
        _ctx.descriptorPool = descriptorPool;

        std::vector<VkDescriptorBufferInfo> lightinBufferInfos(MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            lightinBufferInfos[i] = { lightUniformBuffers[i], 0, sizeof(LightingUBOCPU) };
        }
        for (auto& shape : _shapes) {
            shape->create();
            shape->upload(_ctx, MAX_FRAMES_IN_FLIGHT,
                shape->getMaterial().getTextureImageView(),
                shape->getMaterial().getTextureSampler(),
                lightinBufferInfos);
        }

		_globe.create();
		_globe.upload(_ctx, MAX_FRAMES_IN_FLIGHT, _globe.getMaterial().getTextureImageView(), _globe.getMaterial().getTextureSampler(), lightinBufferInfos);
        _scene.uploadScene(_ctx, MAX_FRAMES_IN_FLIGHT, textureImageView, textureSampler, lightinBufferInfos);
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;


        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &features13;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (uint32_t i = 0; i < swapChainImages.size(); i++) {
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createOffscreenRenderPass() {
        VkAttachmentDescription color{};
        color.format = VK_FORMAT_R16G16B16A16_SFLOAT;            // matches offscreenImage
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // store for sampling later
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // transition via subpass

        VkAttachmentDescription depth{};
        depth.format = findDepthFormat();
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{ };
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{ };
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        // External dependency to ensure writes are visible for sampling later
        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = 0;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> atts = { color, depth };
        VkRenderPassCreateInfo rpci{ };
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = static_cast<uint32_t>(atts.size());
        rpci.pAttachments = atts.data();
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dep;

        if (vkCreateRenderPass(device, &rpci, nullptr, &offscreenRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreen render pass!");
        }
    }

    void createOffscreenFramebuffer() {
        std::array<VkImageView, 2> attachments = { offscreenImageView, depthImageView };

        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = offscreenRenderPass;
        fb.attachmentCount = static_cast<uint32_t>(attachments.size());
        fb.pAttachments = attachments.data();
        fb.width = swapChainExtent.width;
        fb.height = swapChainExtent.height;
        fb.layers = 1;

        if (vkCreateFramebuffer(device, &fb, nullptr, &offscreenFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreen framebuffer!");
        }
    }



    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding lightingLayoutBinding{};
		lightingLayoutBinding.binding = 2;
		lightingLayoutBinding.descriptorCount = 1;
		lightingLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightingLayoutBinding.pImmutableSamplers = nullptr;
		lightingLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding timeLayoutBinding{};
		timeLayoutBinding.binding = 3;
		timeLayoutBinding.descriptorCount = 1;
		timeLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		timeLayoutBinding.pImmutableSamplers = nullptr;
		timeLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding shadowLayoutBinding{};
		shadowLayoutBinding.binding = 4;
		shadowLayoutBinding.descriptorCount = 1;
		shadowLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		shadowLayoutBinding.pImmutableSamplers = nullptr;
		shadowLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uboLayoutBinding, samplerLayoutBinding, lightingLayoutBinding, timeLayoutBinding, shadowLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/Phong.vert.spv");
        auto fragShaderCode = readFile("shaders/Phong.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        if (shadowDescriptorSetLayout != VK_NULL_HANDLE) {
            VkDescriptorSetLayout setLayoutsArr[2] = { descriptorSetLayout, shadowDescriptorSetLayout };
            pipelineLayoutInfo.setLayoutCount = 2;
            pipelineLayoutInfo.pSetLayouts = setLayoutsArr;
        }
        else {
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        }

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createTextureSampler(VkSampler& sampler) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            VkResult mapRes = vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
            if (mapRes != VK_SUCCESS || uniformBuffersMapped[i] == nullptr) {
                throw std::runtime_error("vkMapMemory failed for uniform buffer " + std::to_string(i));
            }
        }

        VkDeviceSize lightBufferSize = sizeof(LightingUBOCPU);

		lightUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		lightUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		lightUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(lightBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightUniformBuffers[i], lightUniformBuffersMemory[i]);
            void* p = nullptr;
            if (vkMapMemory(device, lightUniformBuffersMemory[i], 0, lightBufferSize, 0, &p) != VK_SUCCESS || p == nullptr)
            {
                throw std::runtime_error("vkMapMemory failed for light uniform buffer " + std::to_string(i));
            }
            lightUniformBuffersMapped[i] = p;
        }

		VkDeviceSize timeBufferSize = sizeof(TimeUBO);

		timeBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		timeBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
		timeBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(timeBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer[i], timeBufferMemory[i]);
            void* p = nullptr;
            if (vkMapMemory(device, timeBufferMemory[i], 0, timeBufferSize, 0, &p) != VK_SUCCESS || p == nullptr)
            {
                throw std::runtime_error("vkMapMemory failed for time uniform buffer " + std::to_string(i));
            }
            timeBuffersMapped[i] = p;
        }
    }

    void createDescriptorPool() {
        // Per-frame descriptor sets (frame UBOs)
        const uint32_t frameSets = MAX_FRAMES_IN_FLIGHT;

        // Use conservative upper bounds because the pool is created before shapes/scene are populated.
        // Adjust these if your scene may require more descriptor sets.
        const uint32_t maxShapes = 32;         // conservative max number of Shapes (including globe)
        const uint32_t maxSceneObjects = 256;  // conservative max number of scene objects
        const uint32_t shapeSets = maxShapes * MAX_FRAMES_IN_FLIGHT;
        const uint32_t sceneSets = maxSceneObjects * MAX_FRAMES_IN_FLIGHT;

        // Skybox: single set
        const uint32_t skyboxSets = 1;

        // Particle compute sets (if any)
        const uint32_t computeSets = 2;

        // Shadow descriptor sets (per-frame)
        const uint32_t shadowSets = MAX_FRAMES_IN_FLIGHT;

        // Total sets we must support
        const uint32_t totalSets = frameSets + shapeSets + sceneSets + skyboxSets + computeSets + shadowSets;

        // Layout bindings per main set:
        // - UBOs at bindings 0,2,3 (3 UBOS)
        // - Combined image samplers at bindings 1 and 4 (texture + shadow)
        const uint32_t ubosPerSet = 3;      // bindings 0, 2, 3
        const uint32_t samplersPerSet = 2; // bindings 1 and 4

        // Aggregate descriptor counts
        const uint32_t totalUboDescriptors =
            (frameSets + shapeSets + sceneSets) * ubosPerSet
            // shadow sets also contain a uniform buffer (shadow UBO)
            + shadowSets * 1
            // compute parameter UBOs
            + computeSets * 1;

        const uint32_t totalSamplerDescriptors =
            // main sets have two samplers each (texture + shadow)
            (frameSets + shapeSets + sceneSets) * samplersPerSet
            // skybox has one cubemap sampler
            + skyboxSets * 1
            // shadow descriptor sets include one sampler each
            + shadowSets * 1;

        const uint32_t totalStorageDescriptors =
            computeSets * 2; // storage buffers for compute

        // Safety margin for fragmentation and future growth
        const uint32_t safety = 64;

        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  totalUboDescriptors + safety };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalSamplerDescriptors + safety };
        poolSizes[2] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  totalStorageDescriptors + safety };

        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = totalSets + safety;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

			VkDescriptorBufferInfo lightBufferInfo{};
			lightBufferInfo.buffer = lightUniformBuffers[i];
			lightBufferInfo.offset = 0;
			lightBufferInfo.range = sizeof(LightingUBOCPU);

			VkDescriptorBufferInfo timeBufferInfo{};
			timeBufferInfo.buffer = timeBuffer[i];
			timeBufferInfo.offset = 0;
			timeBufferInfo.range = sizeof(TimeUBO);

            std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &lightBufferInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &timeBufferInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void createGouraudPipeline()
    {
        auto vertCode = readFile("shaders/Gouraud.vert.spv");
        auto fragCode = readFile("shaders/Gouraud.frag.spv");
        VkShaderModule v = createShaderModule(vertCode);
        VkShaderModule f = createShaderModule(fragCode);

        auto bindingDesc = Vertex::getBindingDescription();
        auto attrDescs = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bindingDesc;
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vi.pVertexAttributeDescriptions = attrDescs.data();

        GraphicsPipelineBuilder b;
        b.setDevice(device)
            .setRenderPass(renderPass)
            .setPipelineLayout(pipelineLayout)
            .setVertexInput(vi)
            .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .setMultisample(VK_SAMPLE_COUNT_1_BIT)
            .enableDepthTest(VK_COMPARE_OP_LESS, VK_TRUE)
            .addColorBlendAttachment(VK_FALSE)
            .setColorBlendLogic(VK_FALSE)
            .setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .setShaderStages(v, f);

        gouraudPipeline = b.build();

        vkDestroyShaderModule(device, f, nullptr);
        vkDestroyShaderModule(device, v, nullptr);
    }

    void createPhongPipeline()
    {
        auto vertCode = readFile("shaders/Phong.vert.spv");
        auto fragCode = readFile("shaders/Phong.frag.spv");
        VkShaderModule v = createShaderModule(vertCode);
        VkShaderModule f = createShaderModule(fragCode);

        auto bindingDesc = Vertex::getBindingDescription();
        auto attrDescs = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bindingDesc;
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vi.pVertexAttributeDescriptions = attrDescs.data();

        GraphicsPipelineBuilder b;
        b.setDevice(device)
            .setRenderPass(renderPass)
            .setPipelineLayout(pipelineLayout)
            .setVertexInput(vi)
            .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .setMultisample(VK_SAMPLE_COUNT_1_BIT)
            .enableDepthTest(VK_COMPARE_OP_LESS, VK_TRUE)
            .addColorBlendAttachment(VK_FALSE)
            .setColorBlendLogic(VK_FALSE)
            .setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .setShaderStages(v, f);

        phongPipeline = b.build();

        vkDestroyShaderModule(device, f, nullptr);
        vkDestroyShaderModule(device, v, nullptr);
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence = VK_NULL_HANDLE;
        if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence for one-time submit!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
            vkDestroyFence(device, fence, nullptr);
            throw std::runtime_error("failed to submit one-time command buffer!");
        }

        // Wait only for this work, not the whole queue
        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device, fence, nullptr);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }



    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

        VkCommandBufferBeginInfo beginInfo{ };
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // Begin shadow pass
        std::array<VkClearValue, 1> shadowClear{};
        shadowClear[0].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo shadowRp{};
        shadowRp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadowRp.renderPass = shadowRenderPass;
        shadowRp.framebuffer = shadowFrameBuffer;
        shadowRp.renderArea.extent = swapChainExtent;
        shadowRp.clearValueCount = static_cast<uint32_t>(shadowClear.size());
        shadowRp.pClearValues = shadowClear.data();

        vkCmdBeginRenderPass(commandBuffer, &shadowRp, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport shadowVp{ 0.f, 0.f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.f, 1.f };
        VkRect2D shadowSc{ {0,0}, swapChainExtent };
        vkCmdSetViewport(commandBuffer, 0, 1, &shadowVp);
        vkCmdSetScissor(commandBuffer, 0, 1, &shadowSc);

        // bind shadow pipeline and descriptor sets (set0 = frame UBOs, set1 = shadow set)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
        VkDescriptorSet setsShadow[] = { descriptorSets[currentFrame], shadowDescriptorSets[currentFrame] };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 2, setsShadow, 0, nullptr);

        // draw shapes similarly to your other passes (mesh/shape draw accept pipeline + layout)
        _mesh.draw(commandBuffer, shadowPipeline, shadowPipelineLayout, currentFrame);
        _cylinder.draw(commandBuffer, shadowPipeline, shadowPipelineLayout, currentFrame);
        _scene.drawScene(commandBuffer, shadowPipelineLayout, shadowPipeline, currentFrame); // adjust if signature differs
        _globe.draw(commandBuffer, shadowPipeline, shadowPipelineLayout, currentFrame);

        vkCmdEndRenderPass(commandBuffer);

        // After render, transition shadowImage to SHADER_READ_ONLY_OPTIMAL for sampling
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = shadowImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // ----- Pass 1: scene to offscreen framebuffer -----
        std::array<VkClearValue, 2> offscreenClears{};
        offscreenClears[0].color = { {0.f, 0.f, 0.f, 1.f} };
        offscreenClears[1].depthStencil = { 1.f, 0 };

        VkRenderPassBeginInfo rpBegin1{ };
        rpBegin1.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin1.renderPass = offscreenRenderPass;
        rpBegin1.framebuffer = offscreenFramebuffer;
        rpBegin1.renderArea.offset = { 0, 0 };
        rpBegin1.renderArea.extent = swapChainExtent;
        rpBegin1.clearValueCount = static_cast<uint32_t>(offscreenClears.size());
        rpBegin1.pClearValues = offscreenClears.data();

        VkImageMemoryBarrier toColor{};
        toColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toColor.srcAccessMask = 0;
        toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toColor.oldLayout = offscreenInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toColor.image = offscreenImage;
        toColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toColor.subresourceRange.baseMipLevel = 0;
        toColor.subresourceRange.levelCount = 1;
        toColor.subresourceRange.baseArrayLayer = 0;
        toColor.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            commandBuffer,
            offscreenInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &toColor);

        vkCmdBeginRenderPass(commandBuffer, &rpBegin1, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vp{ 0.f,0.f,(float)swapChainExtent.width,(float)swapChainExtent.height,0.f,1.f };
        VkRect2D sc{ {0,0}, swapChainExtent };
        vkCmdSetViewport(commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(commandBuffer, 0, 1, &sc);

        VkDescriptorSet sets[] = { descriptorSets[currentFrame], shadowDescriptorSets[currentFrame] };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            0, 2, sets, 0, nullptr);

        _scene.drawPostProcessables(commandBuffer, pipelineLayout, phongPipeline, currentFrame);

        vkCmdEndRenderPass(commandBuffer);

        offscreenInitialized = true;

        // ----- Pass 2: post-process to swapchain framebuffer -----
        std::array<VkClearValue, 2> swapClears{};
        swapClears[0].color = { {0.f, 0.f, 0.f, 1.f} };
        swapClears[1].depthStencil = { 1.f, 0 };

        VkRenderPassBeginInfo rpBegin2{ };
        rpBegin2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin2.renderPass = renderPass; // your swapchain render pass
        rpBegin2.framebuffer = swapChainFramebuffers[imageIndex];
        rpBegin2.renderArea.offset = { 0, 0 };
        rpBegin2.renderArea.extent = swapChainExtent;
        rpBegin2.clearValueCount = static_cast<uint32_t>(swapClears.size());
        rpBegin2.pClearValues = swapClears.data();

        vkCmdBeginRenderPass(commandBuffer, &rpBegin2, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(commandBuffer, 0, 1, &sc);

        // Bind post-process pipeline + descriptor set BEFORE drawing fullscreen quad
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postProcessPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            postProcessPipelineLayout, 0, 1, &postProcessDescriptorSets[currentFrame], 0, nullptr);

        // Draw fullscreen triangle/quad (now that the pipeline is bound)
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);

        if (_globe.WithinBounds(cameraManager.getCurrentCamera().getEye()))
        {
            // Bind skybox pipeline and descriptor sets
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);

            // Bind both descriptor sets: frame UBO set (set 0) and skybox cubemap set (set 1)
            VkDescriptorSet sets[] = { descriptorSets[currentFrame], skyboxDescriptorSet };
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout,
                0, 2, sets, 0, nullptr);

            // Bind skybox vertex/index buffers (use your cube geometry)
            VkBuffer vertexBuffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            // Draw the cube (36 indices for a box)
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gouraudPipeline);
        _mesh.draw(commandBuffer, gouraudPipeline, pipelineLayout, currentFrame);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, phongPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            0, 2, sets, 0, nullptr);

        _cylinder.draw(commandBuffer, phongPipeline, pipelineLayout, currentFrame);
        _scene.drawScene(commandBuffer, pipelineLayout, phongPipeline, currentFrame);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particlePipeline);

        for (const auto& sys : _particleSystems) {
            sys.recordDraw(commandBuffer, particlePipeline, particleQuadVB, particleQuadIB, particleQuadIndexCount);
        }

        // NOTE: post-process draw already executed earlier
        // bind outline and draw globe outline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipeline);
        _globe.draw(commandBuffer, outlinePipeline, pipelineLayout, currentFrame);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        uint32_t idx = currentFrame;

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.view = cameraManager.getCurrentCamera().getViewMatrix();
        ubo.proj = cameraManager.getCurrentCamera().getProjectionMatrix();
        ubo.proj[1][1] *= -1;

        for(Shape* shape : _shapes)
        {
            shape->updateUniformBuffer(idx,ubo.model,ubo.view,ubo.proj);
		}

		_globe.updateUniformBuffer(idx, ubo.model, ubo.view, ubo.proj);
		_scene.updateSceneUniformBuffers(idx, ubo.model, ubo.view, ubo.proj);

        void* dst = uniformBuffersMapped[currentImage];
        if (!dst) {
            throw std::runtime_error("uniformBuffersMapped[currentImage] is null after resize/recreate.");
        }
        std::memcpy(dst, &ubo, sizeof(ubo));

        for (auto& sys : _particleSystems)
        {
            sys.update(_deltaTime);
        }

		glm::mat4 invView = glm::inverse(ubo.view);
		glm::vec3 camPos(invView[3]);

        // Day-night cycle from GlobeScene
        const float secondsPerCycle = _scene.getDayNightCycleDuration();
        const float timeOfDay = _scene.getTimeOfDay();
        const float t = secondsPerCycle > 0.0f ? std::fmod(timeOfDay, secondsPerCycle) / secondsPerCycle : 0.0f;

        // Vertical plane (YZ)
        const float theta = t * glm::two_pi<float>();
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.0f, std::sin(theta), -std::cos(theta)));
        glm::vec3 moonDir = -sunDir;

        // Twilight widening + gamma lift
        const float twilightWidth = 0.45f;   // wider near horizon
        const float sunGamma = 0.75f;   // lift mids for broader daylight
        const float moonGamma = 0.95f;   // slightly flatter nights
        const float twilightBias = 0.08f;   // small upward shift for more twilight time

        const float sunUp = glm::smoothstep(-twilightWidth, +twilightWidth, sunDir.y + twilightBias);
        const float moonUp = glm::smoothstep(-twilightWidth, +twilightWidth, -sunDir.y + twilightBias);

        const float sunIntensity = std::pow(sunUp, sunGamma);
        const float moonIntensity = std::pow(moonUp, moonGamma);

        auto mix3 = [](const glm::vec3& a, const glm::vec3& b, float s) { return a + (b - a) * s; };

        const glm::vec3 sunWarm = { 1.0f, 0.85f, 0.70f }; // sunrise/sunset
        const glm::vec3 sunNeutral = { 1.0f, 0.95f, 0.90f }; // midday
        const glm::vec3 moonCool = { 0.75f, 0.80f, 1.00f };

        const float warmBlend = 1.0f - std::abs(sunDir.y); // more warm near horizon
        glm::vec3 sunColor = mix3(sunNeutral, sunWarm, glm::clamp(warmBlend, 0.0f, 1.0f));
        sunColor *= glm::mix(0.0f, 1.0f, sunIntensity);

        glm::vec3 moonColor = moonCool * glm::mix(0.0f, 0.6f, moonIntensity);

        const float dayAmbientMin = 0.03f;
        const float nightAmbientMin = 0.02f;

        const float sunAmbient = glm::max(glm::mix(0.06f, 0.25f, sunIntensity), dayAmbientMin);
        const float moonAmbient = glm::max(glm::mix(0.02f, 0.08f, moonIntensity), nightAmbientMin);

        // Specular falls off near horizon to avoid harsh rim lighting
        const float sunSpecular = glm::mix(4.0f, 16.0f, sunIntensity) * glm::clamp(sunDir.y * 0.8f + 0.2f, 0.0f, 1.0f);
        const float moonSpecular = glm::mix(1.0f, 8.0f, moonIntensity) * glm::clamp((-sunDir.y) * 0.6f + 0.2f, 0.0f, 1.0f);

        // Apply to your Light objects
        if (_lights.size() >= 2) {
            // Sun
            _lights[0].setDirectionWS(sunDir);
            _lights[0].setColor(sunColor);
            _lights[0].setAmbient(sunAmbient);
            _lights[0].setSpecular(sunSpecular);

            // Moon
            _lights[1].setDirectionWS(moonDir);
            _lights[1].setColor(moonColor);
            _lights[1].setAmbient(moonAmbient);
            _lights[1].setSpecular(moonSpecular);
        }

        if (_lights.size() > 0)
        {
            // example for directional light: use an orthographic projection that covers scene extents
            glm::vec3 lightDir = glm::normalize(_lights[0].toGPULight().direction); // or _lights[0].getDirectionWS()
            glm::vec3 center = glm::vec3(0.0f); // scene center - pick appropriate bounds
            glm::vec3 lightPos = center - lightDir * 150.0f; // place light back along direction

            ShadowUBO sh{};
            sh.lightView = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

            const float orthoSize = 120.0f; // adjust to scene
            sh.lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 400.0f);
            sh.lightProj[1][1] *= -1; // if your clip-space flips Y for Vulkan

            std::memcpy(shadowUniformBuffersMapped[currentImage], &sh, sizeof(sh));
        }

        LightingUBOCPU l{};
        l.viewPosWorld = camPos;
        l.shininess = 32.0f;

        int count = 0;
        if (!_lights.empty())
        {
			count = static_cast<int>(std::min<std::size_t>(_lights.size(), LightingUBO::MaxLights));
			l.lightCount = count;
            for (int i = 0; i < count; ++i)
            {
				l.lights[i] = _lights[i].toGPULight();
            }
        }
        else
        {
			l.lightCount = 0;
        }

        std::memcpy(lightUniformBuffersMapped[currentImage], &l, sizeof(LightingUBOCPU));
		TimeUBO ti{};
		ti.time = time;
		std::memcpy(timeBuffersMapped[currentImage], &ti, sizeof(TimeUBO));
    }

    void drawFrame() {
        
		auto now = std::chrono::high_resolution_clock::now();
		_deltaTime = std::chrono::duration<float>(now - _lastFrameTime).count();
        _lastFrameTime = now;

		_scene.updateScene(_deltaTime * _timeScale);

        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame);

        for (auto& sys : _particleSystems)
        {
            sys.uploadInstances(_ctx);
		}

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { imagePresentSemaphores[imageIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitRes = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        if (submitRes != VK_SUCCESS) {
            std::cerr << "vkQueueSubmit failed with VkResult = " << static_cast<int>(submitRes) << std::endl;
            throw std::runtime_error("failed to submit draw command buffer! VkResult=" + std::to_string(static_cast<int>(submitRes)));
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}