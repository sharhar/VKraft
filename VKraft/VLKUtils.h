#pragma once

#include <iostream>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <assert.h>

void VLKCheck(VkResult result, char *msg);

typedef struct Vertex {
	float x, y, z;
	float u, v;
} Vertex;

typedef struct VLKContext {
	VkInstance instance;
	VkDebugReportCallbackEXT callback;

	int layerCount;
	char** layers;
} VLKContext;

typedef struct VLKDevice {
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	uint32_t presentQueueIdx;
	VkDevice device;
	VkQueue presentQueue;
	VkCommandBuffer setupCmdBuffer;
	VkCommandBuffer drawCmdBuffer;
	VkCommandPool commandPool;
} VLKDevice;

typedef struct VLKSwapchain {
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	VkImage* presentImages;
	VkImageView *presentImageViews;
	VkImage depthImage;
	VkDeviceMemory imageMemory;
	VkImageView depthImageView;
	VkRenderPass renderPass;
	VkFramebuffer* frameBuffers;
	VkSemaphore presentCompleteSemaphore, renderingCompleteSemaphore;

	uint32_t width, height, nextImageIdx, imageCount;
} VLKSwapchain;

typedef struct VLKModel {
	VkBuffer vertexInputBuffer;
	VkDeviceMemory vertexBufferMemory;
} VLKModel;

typedef struct VLKShader {
	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;

	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDescriptorSetLayout setLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
} VLKShader;

typedef struct VLKPipeline {
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
} VLKPipeline;

typedef struct VLKTexture {
	uint32_t width;
	uint32_t height;
	unsigned char* data;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureView;
	VkSampler sampler;
	VkDescriptorImageInfo descriptorImageInfo;
} VLKTexture;

VLKContext* vlkCreateContext();
void vlkDestroyContext(VLKContext* context);

void vlkCreateDeviceAndSwapchain(GLFWwindow* window, VLKContext* context, VLKDevice** device, VLKSwapchain** swapChain);
void vlkDestroyDeviceAndSwapchain(VLKContext* context, VLKDevice* device, VLKSwapchain* swapChain);

VLKModel* vlkCreateModel(VLKDevice* device, Vertex* verts, uint32_t num);
void vlkDestroyModel(VLKDevice* device, VLKModel* model);

VLKShader* vlkCreateShader(VLKDevice* device, char* vertPath, char* fragPath, void* uniformBuffer, uint32_t uniformSize);
void vlkUniforms(VLKDevice* device, VLKShader* shader, void* uniformBuffer, uint32_t uniformSize);
void vlkDestroyShader(VLKDevice* device, VLKShader* shader);

VLKPipeline* vlkCreatePipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader);
void vlkDestroyPipeline(VLKDevice* device, VLKPipeline* pipeline);

void vlkClear(VLKContext* context, VLKDevice* device, VLKSwapchain* swapChain);
void vlkSwap(VLKContext* context, VLKDevice* device, VLKSwapchain* swapChain);

VLKTexture* vlkCreateTexture(VLKDevice* device, char* path);
void vlkBindTexture(VLKDevice* device, VLKShader* shader, VLKTexture* texture);
void vlkDestroyTexture(VLKDevice* device, VLKTexture* texture);