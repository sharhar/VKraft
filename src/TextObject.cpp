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

	uint32_t tex_width, tex_height;
	
	std::vector<unsigned char> imageData;
	
	std::string f_name;

#ifdef UTIL_DIR_PRE
	f_name.append(UTIL_DIR_PRE);
#endif

	f_name.append("res/font.png");
	
	unsigned int error = lodepng::decode(imageData, tex_width, tex_height, f_name);
	
	if (error != 0) {
		std::cout << "Error loading image: " << error << "\n";
	}
	
	textRenderingData.fontImage.create(VKLImageCreateInfo()
										.device(&device)
										.extent(tex_width, tex_height, 1)
										.format(VK_FORMAT_R8G8B8A8_UNORM)
										.usage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
										.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	textRenderingData.fontImage.transition(transferQueue,
										   VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	
	textRenderingData.fontImage.uploadData(transferQueue, imageData.data(), imageData.size(), sizeof(char) * 4);
	
	textRenderingData.fontImage.transition(transferQueue,
										   VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	
	textRenderingData.fontImageView.create(VKLImageViewCreateInfo().image(&textRenderingData.fontImage));
	
	textRenderingData.descriptorSet = new VKLDescriptorSet(&textRenderingData.shader, 0);
	
	VkSamplerCreateInfo samplerCreateInfo;
	memset(&samplerCreateInfo, 0, sizeof(VkSamplerCreateInfo));
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 0;
	samplerCreateInfo.minLod = 0;
	samplerCreateInfo.maxLod = 0;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VK_CALL(device.vk.CreateSampler(device.handle(), &samplerCreateInfo, device.allocationCallbacks(), &textRenderingData.sampler));
	
	textRenderingData.descriptorSet->writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
												textRenderingData.fontImageView.handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textRenderingData.sampler);
}

void Application::perpareTextRendering(const VKLCommandBuffer* cmdBuff) {
	cmdBuff->bindVertexBuffer(textRenderingData.vertBuffer, 0, 0);
	cmdBuff->bindPipeline(textRenderingData.pipeline);
	cmdBuff->bindDescriptorSet(textRenderingData.descriptorSet);
}

void Application::cleanUpTextRenderingData() {
	device.vk.DestroySampler(device.handle(), textRenderingData.sampler, device.allocationCallbacks());
	
	textRenderingData.descriptorSet->destroy();
	
	delete textRenderingData.descriptorSet;
	
	textRenderingData.fontImageView.destroy();
	textRenderingData.fontImage.destroy();
	
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
