#include "VLKUtils.h"
#include "Cube.h"
#include "Camera.h"

VLKPipeline* createFontPipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader) {
	VLKPipeline* pipeline = (VLKPipeline*)malloc(sizeof(VLKPipeline));

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &shader->setLayout;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = NULL;

	VLKCheck(vkCreatePipelineLayout(device->device, &layoutCreateInfo, NULL, &pipeline->pipelineLayout),
		"Failed to create pipeline layout");

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};
	shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[0].module = shader->vertexShaderModule;
	shaderStageCreateInfo[0].pName = "main";
	shaderStageCreateInfo[0].pSpecializationInfo = NULL;

	shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[1].module = shader->fragmentShaderModule;
	shaderStageCreateInfo[1].pName = "main";
	shaderStageCreateInfo[1].pSpecializationInfo = NULL;

	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(float) * 4;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescritpion[2];
	vertexAttributeDescritpion[0].location = 0;
	vertexAttributeDescritpion[0].binding = 0;
	vertexAttributeDescritpion[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescritpion[0].offset = 0;

	vertexAttributeDescritpion[1].location = 1;
	vertexAttributeDescritpion[1].binding = 0;
	vertexAttributeDescritpion[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescritpion[1].offset = 2 * sizeof(float);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescritpion;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = swapChain->width;
	viewport.height = swapChain->height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;

	VkRect2D scissors = {};
	scissors.offset.x = 0;
	scissors.offset.y = 0;
	scissors.extent.width = swapChain->width;
	scissors.extent.height = swapChain->height;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissors;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0;
	rasterizationState.depthBiasClamp = 0;
	rasterizationState.depthBiasSlopeFactor = 0;
	rasterizationState.lineWidth = 1;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 0;
	multisampleState.pSampleMask = NULL;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;

	VkStencilOpState noOPStencilState = {};
	noOPStencilState.failOp = VK_STENCIL_OP_KEEP;
	noOPStencilState.passOp = VK_STENCIL_OP_KEEP;
	noOPStencilState.depthFailOp = VK_STENCIL_OP_KEEP;
	noOPStencilState.compareOp = VK_COMPARE_OP_ALWAYS;
	noOPStencilState.compareMask = 0;
	noOPStencilState.writeMask = 0;
	noOPStencilState.reference = 0;

	VkPipelineDepthStencilStateCreateInfo depthState = {};
	depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthState.depthTestEnable = VK_TRUE;
	depthState.depthWriteEnable = VK_TRUE;
	depthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthState.depthBoundsTestEnable = VK_FALSE;
	depthState.stencilTestEnable = VK_FALSE;
	depthState.front = noOPStencilState;
	depthState.back = noOPStencilState;
	depthState.minDepthBounds = 0;
	depthState.maxDepthBounds = 0;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	colorBlendState.blendConstants[0] = 0.0;
	colorBlendState.blendConstants[1] = 0.0;
	colorBlendState.blendConstants[2] = 0.0;
	colorBlendState.blendConstants[3] = 0.0;

	VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicState;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStageCreateInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pTessellationState = NULL;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = pipeline->pipelineLayout;
	pipelineCreateInfo.renderPass = swapChain->renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = NULL;
	pipelineCreateInfo.basePipelineIndex = 0;

	VLKCheck(vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline->pipeline),
		"Failed to create graphics pipeline");

	return pipeline;
}

void destroyFontPipeline(VLKDevice* device, VLKPipeline* pipeline) {
	vkDestroyPipeline(device->device, pipeline->pipeline, NULL);
	vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);

	free(pipeline);
}

VLKModel* drawText(VLKDevice* device,
	std::string text, float xOff, float yOff, float size,
	VLKShader* fontShader, VLKPipeline* fontPipeline) {

	int tsz = text.size();
	float* verts = new float[tsz * 4 * 6];

	for (int i = 0; i < tsz; i++) {
		uint8_t chid = (uint8_t)text.at(i);

		float sx = chid % 16;
		float sy = (chid - sx) / 16;

		float x1 = xOff + size * i;
		float y1 = yOff;
		float x2 = xOff + size * (i + 1);
		float y2 = xOff + size;

		float tx1 = (sx + 0.0f) / 16.0f;
		float ty1 = (sy + 0.0f) / 16.0f;
		float tx2 = (sx + 1.0f) / 16.0f;
		float ty2 = (sy + 1.0f) / 16.0f;

		verts[i * 24 + 0] = x1;
		verts[i * 24 + 1] = y1;

		verts[i * 24 + 2] = tx1;
		verts[i * 24 + 3] = ty1;

		verts[i * 24 + 4] = x1;
		verts[i * 24 + 5] = y2;

		verts[i * 24 + 6] = tx1;
		verts[i * 24 + 7] = ty2;

		verts[i * 24 + 8] = x2;
		verts[i * 24 + 9] = y1;

		verts[i * 24 + 10] = tx2;
		verts[i * 24 + 11] = ty1;

		verts[i * 24 + 12] = x1;
		verts[i * 24 + 13] = y2;

		verts[i * 24 + 14] = tx1;
		verts[i * 24 + 15] = ty2;

		verts[i * 24 + 16] = x2;
		verts[i * 24 + 17] = y1;

		verts[i * 24 + 18] = tx2;
		verts[i * 24 + 19] = ty1;

		verts[i * 24 + 20] = x2;
		verts[i * 24 + 21] = y2;

		verts[i * 24 + 22] = tx2;
		verts[i * 24 + 23] = ty2;
	}

	VLKModel * fontModel = vlkCreateModel(device, verts, 6 * 4 * sizeof(float) * tsz);

	VkDeviceSize offsets = 0;

	vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipeline->pipeline);
	vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &fontModel->vertexInputBuffer, &offsets);
	vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		fontPipeline->pipelineLayout, 0, 1, &fontShader->descriptorSet, 0, NULL);

	vkCmdDraw(device->drawCmdBuffer, 6 * tsz, 1, 0, 0);

	delete[] verts;

	return fontModel;
}
