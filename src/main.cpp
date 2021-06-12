#include <VKL/VKL.h>

#include "master.h"
#include <chrono>
#include <thread>
#include "lodepng.h"

#include "Cursor.h"

void createCubeShader(VKLDevice* device, VKLShader** pShader) {
	char* shaderPaths[2];
	shaderPaths[0] = "res/cube-vert.spv";
	shaderPaths[1] = "res/cube-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[2] = { 0, sizeof(float) * 4 };
	VkFormat formats[2] = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32_SFLOAT };

	VkDescriptorSetLayoutBinding bindings[2];
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = NULL;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = NULL;

	VKLShaderCreateInfo shaderCreateInfo;
	memset(&shaderCreateInfo, 0, sizeof(VKLShaderCreateInfo));
	shaderCreateInfo.shaderPaths = shaderPaths;
	shaderCreateInfo.shaderStages = stages;
	shaderCreateInfo.shaderCount = 2;
	shaderCreateInfo.bindings = bindings;
	shaderCreateInfo.bindingsCount = 2;
	shaderCreateInfo.vertexInputAttributeStride = sizeof(float) * 6;
	shaderCreateInfo.vertexInputAttributesCount = 2;
	shaderCreateInfo.vertexInputAttributeOffsets = offsets;
	shaderCreateInfo.vertexInputAttributeFormats = formats;
	
	vklCreateShader(device, pShader, &shaderCreateInfo);
}

void createBGShaderAndPipeline(VKLDevice* device, VKLShader** pShader, VKLPipeline** pPipeline, VKLSwapChain* swapChain) {
	char* shaderPaths[2];
	shaderPaths[0] = "res/bg-vert.spv";
	shaderPaths[1] = "res/bg-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[2] = { 0, sizeof(float) * 2 };
	VkFormat formats[2] = { VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT };

	VkDescriptorSetLayoutBinding bindings[1];
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = NULL;

	VKLShaderCreateInfo shaderCreateInfo;
	memset(&shaderCreateInfo, 0, sizeof(VKLShaderCreateInfo));
	shaderCreateInfo.shaderPaths = shaderPaths;
	shaderCreateInfo.shaderStages = stages;
	shaderCreateInfo.shaderCount = 2;
	shaderCreateInfo.bindings = bindings;
	shaderCreateInfo.bindingsCount = 1;
	shaderCreateInfo.vertexInputAttributeStride = sizeof(float) * 4;
	shaderCreateInfo.vertexInputAttributesCount = 2;
	shaderCreateInfo.vertexInputAttributeOffsets = offsets;
	shaderCreateInfo.vertexInputAttributeFormats = formats;
	
	vklCreateShader(device, pShader, &shaderCreateInfo);
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = *pShader;
	pipelineCreateInfo.renderPass = swapChain->backBuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = swapChain->width;
	pipelineCreateInfo.extent.height = swapChain->height;

	vklCreateGraphicsPipeline(device, pPipeline, &pipelineCreateInfo);
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "VKraft", NULL, NULL);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowFocusCallback(window, window_focus_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	VkBool32 debug = 0;

#ifdef _DEBUG
	debug = 1;
#endif

	uint32_t extensionCountGLFW = 0;
	char** extensionsGLFW = (char**)glfwGetRequiredInstanceExtensions(&extensionCountGLFW);

	VKLInstance* instance;
	vklCreateInstance(&instance, NULL, extensionsGLFW[1], glfwGetInstanceProcAddress, debug);

	VKLSurface* surface = (VKLSurface*) malloc(sizeof(VKLSurface));
	glfwCreateWindowSurface(instance->instance, window, NULL, &surface->surface);

	surface->width = 1280;
	surface->height = 720;

	VKLDevice* device;
	VKLDeviceGraphicsContext** deviceContexts;
	vklCreateDevice(instance, &device, &surface, 1, &deviceContexts, 0, NULL);

	VKLDeviceGraphicsContext* devCon = deviceContexts[0];

	VKLSwapChain* swapChain;
	VKLFrameBuffer* backBuffer;
	vklCreateSwapChain(devCon, &swapChain, VK_TRUE);
	vklGetBackBuffer(swapChain, &backBuffer);
	
	CubeUniformBuffer uniformBufferData;
	memcpy(uniformBufferData.proj, getPerspective(16.0f / 9.0f, 90), sizeof(float) * 16);

	VKLShader* shader;
	createCubeShader(device, &shader);

	VKLUniformObject* uniform;
	vklCreateUniformObject(device, &uniform, shader);

	VKLBuffer* uniformBuffer;
	vklCreateBuffer(device, &uniformBuffer, VK_FALSE, sizeof(CubeUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	vklSetUniformBuffer(device, uniform, uniformBuffer, 0);

	VKLFrameBuffer* frameBuffer;
	vklCreateFrameBuffer(devCon, &frameBuffer, swapChain->width * 2, swapChain->height * 2, VK_FORMAT_R8G8B8A8_UNORM, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = shader;
	pipelineCreateInfo.renderPass = frameBuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = swapChain->width;
	pipelineCreateInfo.extent.height = swapChain->height;

	VKLPipeline* pipeline;
	vklCreateGraphicsPipeline(device, &pipeline, &pipelineCreateInfo);

	Vec3 pos = Vec3(0, 0, 1);
	Vec3 rot = Vec3(0, 0, 0);
	double prev_x = 0;
	double prev_y = 0;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	prev_x = xpos;
	prev_y = ypos;

	
	/*
	VulkanRenderContext renderContext = {};
	renderContext.device = device;
	renderContext.swapChain = swapChain;
	renderContext.uniformBuffer = &uniformBuffer;
	renderContext.shader = shader;
	renderContext.pipeline = pipeline;
	renderContext.framebuffer = frameBuffer;

	Camera::init(window, &uniformBuffer, context);

	Chunk::init(1337, window, &renderContext);
	*/

	VKLTextureCreateInfo textureCreateInfo;
	textureCreateInfo.width = 4;
	textureCreateInfo.height = 4;
	textureCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	textureCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	textureCreateInfo.filter = VK_FILTER_NEAREST;
	textureCreateInfo.colorSize = sizeof(char);
	textureCreateInfo.colorCount = 4;

	uint32_t tex_width, tex_height;

	std::vector<unsigned char> imageData;

	unsigned int error = lodepng::decode(imageData, tex_width, tex_height, "res/pack.png");

	if (error != 0) {
		std::cout << "Error loading image: " << error << "\n";
		return NULL;
	}

	VKLTexture* texture;
	vklCreateStagedTexture(devCon, &texture, &textureCreateInfo, imageData.data());

	vklSetUniformTexture(device, uniform, texture, 1);
	
	float backGroundVerts[] = {
		-1, -1, 0, 0,
		 1, -1, 1, 0,
		-1,  1, 0, 1,
		-1,  1, 0, 1,
		 1, -1, 1, 0,
		 1,  1, 1, 1 };
	
	VKLBuffer* bgModel;
	vklCreateStagedBuffer(devCon, &bgModel, backGroundVerts, 6 * 4 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	VKLShader* bgShader;
	VKLPipeline* bgPipeline;
	createBGShaderAndPipeline(device, &bgShader, &bgPipeline, swapChain);

	VKLUniformObject* bgUniform;
	vklCreateUniformObject(device, &bgUniform, bgShader);

	vklSetUniformFramebuffer(device, bgUniform, frameBuffer, 0);
	
	Cursor cursor = Cursor(device, swapChain, frameBuffer);
	
	/*

	VLKShader* cursorShader = createCursorShader(device, "res/cursor-vert.spv", "res/cursor-frag.spv", cursorUniform, sizeof(float) * 18);
	VLKPipeline* cursorPipeline = createCursorPipeline(device, swapChain, cursorShader);
	VLKTexture* cursorTexture = createCursorTexture(device, "res/Cursor.png");

	VkWriteDescriptorSet writeDescriptors[2];

	writeDescriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptors[1].pNext = NULL;
	writeDescriptors[1].dstSet = cursorShader->descriptorSet;
	writeDescriptors[1].dstBinding = 1;
	writeDescriptors[1].dstArrayElement = 0;
	writeDescriptors[1].descriptorCount = 1;
	writeDescriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptors[1].pImageInfo = &frameBuffer->descriptorImageInfo;
	writeDescriptors[1].pBufferInfo = NULL;
	writeDescriptors[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device->device, 2, writeDescriptors, 0, NULL);

	VLKShader* fontShader = vlkCreateShader(device, "res/font-vert.spv", "res/font-frag.spv", cursorUniform, 16 * sizeof(float));
	VLKPipeline* fontPipeline = createFontPipeline(device, swapChain, fontShader);
	VLKTexture* font = vlkCreateTexture(device, "res/font.png", VK_FILTER_LINEAR);
	vlkBindTexture(device, fontShader, font);
	 
	 */

	double ct = glfwGetTime();
	double dt = ct;

	float accDT = 0;
	uint32_t frames = 0;
	uint32_t fps = 0;

	int width, height;
	int pwidth, pheight;

	glfwGetWindowSize(window, &width, &height);
	pwidth = width;
	pheight = height;
	
	VkCommandBuffer cmdBuffer;
	vklAllocateCommandBuffer(devCon, &cmdBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	
	vklSetClearColor(frameBuffer, 0.25f, 0.45f, 1.0f, 1.0f );
	vklSetClearColor(backBuffer, 1.0f, 0.0f, 1.0f, 1.0f );

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		
		glfwGetWindowSize(window, &width, &height);

		if (pwidth != width || pheight != height) {
			//vlkRecreateSwapchain(device, &swapChain, false);
			
			cursor.updateProjection(width, height);
		}
		
		pwidth = width;
		pheight = height;
		 

		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		accDT += dt;
		frames++;

		if (accDT > 1) {
			fps = frames;
			printf("frames: %d\n", frames);
			frames = 0;
			accDT = 0;
		}
		
		//Camera::update(dt);
		
		vklBeginCommandBuffer(device, cmdBuffer);
		
		vklBeginRender(device, frameBuffer, cmdBuffer);
		
		//Chunk::render(device, swapChain);
		
		vklEndRender(device, frameBuffer, cmdBuffer);
		
		vklBeginRender(device, backBuffer, cmdBuffer);
		
		VkViewport viewport = { 0, 0, swapChain->width, swapChain->height, 0, 1 };
		VkRect2D scissor = { 0, 0, swapChain->width, swapChain->height };
		
		device->pvkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		device->pvkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		
		VkDeviceSize vertexOffsets = 0;
		device->pvkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bgPipeline->pipeline);
		device->pvkCmdBindVertexBuffers(cmdBuffer, 0, 1, &bgModel->buffer, &vertexOffsets);

		device->pvkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			bgPipeline->pipelineLayout, 0, 1, &bgUniform->descriptorSet, 0, NULL);

		device->pvkCmdDraw(cmdBuffer, 6, 1, 0, 0);
		
		cursor.render(cmdBuffer);
		
		//VLKModel* fontModel = drawText(device, std::string("FPS:") + std::to_string(fps), 10, 10, 16, fontShader, fontPipeline);
		
		vklEndRender(device, backBuffer, cmdBuffer);
		
		vklEndCommandBuffer(device, cmdBuffer);
		
		vklExecuteCommandBuffer(devCon, cmdBuffer);

		vklPresent(swapChain);
		
		//vlkDestroyModel(device, fontModel);
	}
	
	cursor.destroy();
	
	vklDestroyPipeline(device, pipeline);
	vklDestroyPipeline(device, bgPipeline);
	
	vklDestroyTexture(device, texture);
	
	vklDestroyFrameBuffer(device, frameBuffer);
	
	vklDestroyBuffer(device, bgModel);
	vklDestroyBuffer(device, uniformBuffer);
	
	vklDestroyUniformObject(device, bgUniform);
	vklDestroyUniformObject(device, uniform);

	vklDestroyShader(device, bgShader);
	vklDestroyShader(device, shader);
	
	vklDestroySwapChain(swapChain);
	vklDestroyDevice(device);
	vklDestroySurface(instance, surface);
	vklDestroyInstance(instance);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
