#include "TextObject.h"

#include "lodepng.h"
#include <iostream>

#include "Utils.h"

#include "Application.h"

void Application::setupTextRenderingData() {
	float verts[24] = {
		0, 0, 0, 0,
		1, 0, 1, 0,
		0, 1, 0, 1,
		
		0, 1, 0, 1,
		1, 0, 1, 0,
		1, 1, 1, 1
	};

	textRenderingData.vertBuffer.create(VKLBufferCreateInfo()
										.device(&device).size(6 * 4 * sizeof(float))
										.usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
										.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));

	textRenderingData.vertBuffer.uploadData(transferQueue, verts, 6 * 4 * sizeof(float), 0);

	size_t vertSize = 0;
	uint32_t* vertCode = (uint32_t*)FileUtils::readBinaryFile("res/font-vert.spv", &vertSize);

	size_t fragSize = 0;
	uint32_t* fragCode = (uint32_t*)FileUtils::readBinaryFile("res/font-frag.spv", &fragSize);

	textRenderingData.shader.create(VKLShaderCreateInfo()
						.device(&device)
						.addShaderModule(vertCode, vertSize, VK_SHADER_STAGE_VERTEX_BIT, "main")
						.addShaderModule(fragCode, fragSize, VK_SHADER_STAGE_FRAGMENT_BIT, "main")
						.addDescriptorSet()
							.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
						.end()
						.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 5));

	textRenderingData.pipeline.create(VKLPipelineCreateInfo()
									  .shader(&textRenderingData.shader)
									  .renderPass(&renderPass, 0)
									  .vertexInput
										  .addBinding(0, sizeof(float) * 4)
											  .addAttrib(0, VK_FORMAT_R32G32_SFLOAT, 0)
											  .addAttrib(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2)
										  .end()
										  .addBinding(1, sizeof(int32_t))
											  .inputRate(VK_VERTEX_INPUT_RATE_INSTANCE)
											  .addAttrib(2, VK_FORMAT_R32_SINT, 0)
										  .end()
										.end());

	textRenderingData.texture = new Texture(&device, transferQueue, "res/font.png", VK_FILTER_LINEAR);
	
	textRenderingData.descriptorSet = new VKLDescriptorSet(&textRenderingData.shader, 0);
	
	textRenderingData.descriptorSet->writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textRenderingData.texture->view()->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textRenderingData.texture->sampler());
}

void Application::perpareTextRendering(const VKLCommandBuffer* cmdBuff) {
	cmdBuff->bindVertexBuffer(textRenderingData.vertBuffer, 0, 0);
	cmdBuff->bindPipeline(textRenderingData.pipeline);
	cmdBuff->bindDescriptorSet(textRenderingData.descriptorSet);
}

void Application::cleanUpTextRenderingData() {
	textRenderingData.descriptorSet->destroy();
	delete textRenderingData.descriptorSet;
	
	textRenderingData.texture->destroy();
	delete textRenderingData.texture;
	
	textRenderingData.pipeline.destroy();
	textRenderingData.shader.destroy();
	textRenderingData.vertBuffer.destroy();
}

TextObject::TextObject(Application* application, int maxCharNum) {
	m_application = application;
	
	m_instanceBuffer.create(VKLBufferCreateInfo()
							.device(&application->device)
							.memoryUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
							.size(sizeof(int) * maxCharNum)
							.usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
}

void TextObject::render(VKLCommandBuffer* cmdBuffer) {
	m_pcBuff[0] = m_application->winWidth;
	m_pcBuff[1] = m_application->winHeight;
	
	cmdBuffer->bindVertexBuffer(m_instanceBuffer, 1, 0);
	cmdBuffer->pushConstants(m_application->textRenderingData.pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 5, m_pcBuff);
	
	cmdBuffer->draw(6, m_charNum, 0, 0);
	
}

void TextObject::setText(const std::string& str) {
	int* ids = new int[str.length()];
	
	for(int i = 0; i < str.length(); i++) {
		ids[i] = (int) str[i];
	}
	
	m_instanceBuffer.setData(ids, sizeof(int) * str.length(), 0);
	
	delete[] ids;
	
	m_charNum = str.length();
}

void TextObject::setCoords(float xPos, float yPos, float size) {
	float buff[3] = {xPos, yPos, size};
	
	memcpy(&m_pcBuff[2], buff, sizeof(float) * 3);
}

void TextObject::destroy() {
	m_instanceBuffer.destroy();
}
