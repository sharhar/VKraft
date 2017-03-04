#pragma once

#include "VLKUtils.h"
#include "Utils.h"
#include <thread>

typedef struct VLKComputeContext {
	VkQueue computeQueue;

	VkBuffer inBuffer;
	VkBuffer outBuffer;
	VkBuffer posBuffer;

	VkDeviceMemory inMemory;
	VkDeviceMemory outMemory;
	VkDeviceMemory posMemory;

	VkShaderModule shader;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkCommandBuffer computeCmdBuffer;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	VkFence fence;
} VLKComputeContext;

class Camera {
private:
	static double prev_x, prev_y;
	static std::thread* cameraThread;
public:
	static int fence;
	static bool grounded;
	static float* worldviewMat;
	static GLFWwindow* window;
	static VLKComputeContext context;

	static Vec3* poss;
	static int possSize;

	static float yVel;
	static Vec3 pos;
	static Vec3 renderPos;
	static Vec3 rot;

	static void init(GLFWwindow* win, float* viewMat, VulkanRenderContext* vrc);
	static void update(float dt);
	static void destroy(VLKDevice device);
};