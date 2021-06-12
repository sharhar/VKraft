#pragma once

#include "VLKUtils.h"
#include "Utils.h"
#include "Cube.h"
#include <thread>

typedef struct VLKComputeContext {
	VLKContext* context;
	VLKDevice* device;

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
} VLKComputeContext;

class Camera_old {
private:
	static double prev_x, prev_y;
	static std::thread* cameraThread;
public:
	static int fence;
	static bool grounded;
	static CubeUniformBuffer* uniforms;
	static GLFWwindow* window;
	static VLKComputeContext* computeContext;

	static Vec3* poss;
	static int possSize;

	static float yVel;
	static Vec3 pos;
	static Vec3 renderPos;
	static Vec3 rot;

	static void init(GLFWwindow* win, CubeUniformBuffer* puniforms, VLKContext* context);
	static void update(float dt);
	static void destroy(VLKDevice* device);
};
