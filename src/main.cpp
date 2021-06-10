#include <VKL/VKL.h>

#include "master.h"
#include <chrono>
#include <thread>
#include "lodepng.h"

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
	pipelineCreateInfo.cullMode = VK_CULL_MODE_NONE;
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
	pipelineCreateInfo.cullMode = VK_CULL_MODE_NONE;
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

	//vklSetClearColor(backBuffer, 0.25f, 0.45f, 1.0f, 1.0f);
	
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
	
	float cursorVerts[] = {
		-25, -2.5f,
		 25, -2.5f,
		-25,  2.5f,
		-25,  2.5f,
		 25, -2.5f,
		 25,  2.5f,

		-2.5f, -25,
		 2.5f, -25,
		-2.5f,  25,
		-2.5f,  25,
		 2.5f, -25,
		 2.5f,  25 };

	float r = swapChain->width;
	float l = 0;
	float t = swapChain->height;
	float b = 0;
	float f = 1;
	float n = -1;

	float cursorUniform[] = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, -2 / (f - n), 0,
		-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1,

		640, 360
	};
	
	VKLBuffer* cursorModel;
	vklCreateStagedBuffer(devCon, &cursorModel, cursorVerts, 12 * 2 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	
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

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		/*
		glfwGetWindowSize(window, &width, &height);

		if (pwidth != width || pheight != height) {
			vlkRecreateSwapchain(device, &swapChain, false);

			float r2 = swapChain->width;
			float l2 = 0;
			float t2 = swapChain->height;
			float b2 = 0;
			float f2 = 1;
			float n2 = -1;

			float aspect = ((float)swapChain->width) / ((float)swapChain->height);

			float cursorUniform2[] = {
				2 / (r2 - l2), 0, 0, 0,
				0, 2 / (t2 - b2), 0, 0,
				0, 0, -2 / (f2 - n2), 0,
				-(r2 + l2) / (r2 - l2), -(t2 + b2) / (t2 - b2), -(f2 + n2) / (f2 - n2), 1,

				swapChain->width/2, swapChain->height/2
			};
			
			vlkUniforms(device, fontShader, cursorUniform2, sizeof(float) * 16);
			vlkUniforms(device, cursorShader, cursorUniform2, sizeof(float) * 18);

			memcpy(uniformBuffer.proj, getPerspective(aspect, 90), sizeof(float) * 16);

			vlkDestroyFramebuffer(device, frameBuffer);
			frameBuffer = vlkCreateFramebuffer(device, swapChain->imageCount, swapChain->width * 2, swapChain->height * 2);

			writeDescriptors[0].pImageInfo = &frameBuffer->descriptorImageInfo;
			writeDescriptors[1].pImageInfo = &frameBuffer->descriptorImageInfo;
			
			vkUpdateDescriptorSets(device->device, 2, writeDescriptors, 0, NULL);
		}
		
		pwidth = width;
		pheight = height;

		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		accDT += dt;
		frames++;

		if (accDT > 1) {
			fps = frames;
			frames = 0;
			accDT = 0;
		}
		
		Camera::update(dt);
		
		vlkClear(device, &swapChain);

		vlkStartFramebuffer(device, frameBuffer);

		//Chunk::render(device, swapChain);
	
		vlkEndFramebuffer(device, frameBuffer);
		
		VkClearValue clearValue[] = {
			{ 0.25f, 0.45f, 1.0f, 1.0f },
			{ 1.0, 0.0 } };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = swapChain->renderPass;
		renderPassBeginInfo.framebuffer = swapChain->frameBuffers[swapChain->nextImageIdx];
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = swapChain->width;
		renderPassBeginInfo.renderArea.extent.height = swapChain->height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValue;
		vkCmdBeginRenderPass(device->drawCmdBuffer, &renderPassBeginInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = { 0, 0, swapChain->width, swapChain->height, 0, 1 };
		VkRect2D scissor = { 0, 0, swapChain->width, swapChain->height };
		VkDeviceSize offsets =0;

		vkCmdSetViewport(device->drawCmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(device->drawCmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bgPipeline->pipeline);
		vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &bgModel->vertexInputBuffer, &offsets);
		vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			bgPipeline->pipelineLayout, 0, 1, &bgShader->descriptorSet, 0, NULL);

		vkCmdDraw(device->drawCmdBuffer, 6, 1, 0, 0);

		vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cursorPipeline->pipeline);
		vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &cursorModel->vertexInputBuffer, &offsets);
		vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			cursorPipeline->pipelineLayout, 0, 1, &cursorShader->descriptorSet, 0, NULL);

		vkCmdDraw(device->drawCmdBuffer, 12, 1, 0, 0);
		
		VLKModel* fontModel = drawText(device, std::string("FPS:") + std::to_string(fps), 10, 10, 16, fontShader, fontPipeline);

		vkCmdEndRenderPass(device->drawCmdBuffer);

		vlkSwap(device, &swapChain);

		vlkDestroyModel(device, fontModel);
		*/
	}
	vklDestroyPipeline(device, pipeline);
	vklDestroyPipeline(device, bgPipeline);
	
	vklDestroyTexture(device, texture);
	
	vklDestroyFrameBuffer(device, frameBuffer);
	
	vklDestroyBuffer(device, bgModel);
	vklDestroyBuffer(device, uniformBuffer);
	vklDestroyBuffer(device, cursorModel)
	
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
	
	//system("PAUSE");

	return 0;
}
