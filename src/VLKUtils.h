#pragma once

#include <iostream>
#include "vexl.h"
#include <assert.h>
#include <vector>

void VLKCheck(VkResult result, char *msg);
std::vector<char> readFile(const std::string& filename);

typedef struct Vertex {
	float x, y, z, w;
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
	uint32_t queueIdx;
	VkDevice device;
	VkQueue queue;
	VkCommandBuffer setupCmdBuffer;
	VkCommandBuffer drawCmdBuffer;
	VkCommandPool commandPool;

	VkSurfaceKHR surface;
} VLKDevice;

typedef struct VLKSwapchain {
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

	GLFWwindow* window;
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

typedef struct VLKFramebuffer {
	VkImage colorImage;
	VkImageView colorImageView;
	VkDeviceMemory colorImageMemory;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkRenderPass renderPass;
	VkFramebuffer frameBuffer;

	VkSampler sampler;
	VkDescriptorImageInfo descriptorImageInfo;
	VkCommandBuffer drawCmdBuffer;

	uint32_t imageCount, width, height;
} VLKFramebuffer;

VLKContext* vlkCreateContext();
void vlkDestroyContext(VLKContext* context);

VLKDevice* vlkCreateRenderDevice(VLKContext* context, GLFWwindow* window, uint32_t queueCount);
void vlkDestroyRenderDevice(VLKContext* context, VLKDevice* device);

VLKDevice* vlkCreateComputeDevice(VLKContext* context, uint32_t queueCount);
void vlkDestroyComputeDevice(VLKContext* context, VLKDevice* device);

VLKSwapchain* vlkCreateSwapchain(VLKDevice* device, GLFWwindow* window, bool vSync);
void vlkRecreateSwapchain(VLKDevice* device, VLKSwapchain** pSwapChain, bool vSync);
void vlkDestroySwapchain(VLKDevice* device, VLKSwapchain* swapChain);

VLKModel* vlkCreateModel(VLKDevice* device, void* verts, uint32_t vertsSize);
void vlkDestroyModel(VLKDevice* device, VLKModel* model);

VLKShader* vlkCreateShader(VLKDevice* device, char* vertPath, char* fragPath, void* uniformBuffer, uint32_t uniformSize);
void vlkUniforms(VLKDevice* device, VLKShader* shader, void* uniformBuffer, uint32_t uniformSize);
void vlkDestroyShader(VLKDevice* device, VLKShader* shader);

VLKPipeline* vlkCreatePipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader);
VLKPipeline* vlkCreatePipeline(VLKDevice* device, VLKFramebuffer* frameBuffer, VLKShader* shader);
void vlkDestroyPipeline(VLKDevice* device, VLKPipeline* pipeline);

void vlkClear(VLKDevice* device, VLKSwapchain** pSwapChain);
void vlkSwap(VLKDevice* device, VLKSwapchain** pSwapChain);

VLKTexture* vlkCreateTexture(VLKDevice* device, char* path, VkFilter filter);
void vlkBindTexture(VLKDevice* device, VLKShader* shader, VLKTexture* texture);
void vlkDestroyTexture(VLKDevice* device, VLKTexture* texture);

VLKFramebuffer* vlkCreateFramebuffer(VLKDevice* device, uint32_t imageCount, uint32_t width, uint32_t height);
void vlkDestroyFramebuffer(VLKDevice* device, VLKFramebuffer* frameBuffer);

void vlkStartFramebuffer(VLKDevice* device, VLKFramebuffer* frameBuffer);
void vlkEndFramebuffer(VLKDevice* device, VLKFramebuffer* frameBuffer);