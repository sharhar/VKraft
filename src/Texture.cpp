#include "Texture.h"
#include "lodepng.h"

#include <iostream>
#include <string>

Texture::Texture(const VKLDevice* device, const VKLQueue* queue, const char* path, VkFilter filter) {
	m_device = device;
	
	uint32_t tex_width, tex_height;
	
	std::vector<unsigned char> imageData;
	
	std::string f_name;

#ifdef UTIL_DIR_PRE
	f_name.append(UTIL_DIR_PRE);
#endif

	f_name.append(path);
	
	unsigned int error = lodepng::decode(imageData, tex_width, tex_height, f_name);
	
	if (error != 0) {
		std::cout << "Error loading image: " << error << "\n";
	}
	
	m_image.create(VKLImageCreateInfo()
						.device(m_device)
						.extent(tex_width, tex_height, 1)
						.format(VK_FORMAT_R8G8B8A8_UNORM)
						.usage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
						.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	m_image.transition(queue, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	
	m_image.uploadData(queue, imageData.data(), imageData.size(), sizeof(char) * 4);
	
	m_image.transition(queue, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	
	m_imageView.create(VKLImageViewCreateInfo().image(&m_image));
	
	VkSamplerCreateInfo samplerCreateInfo;
	memset(&samplerCreateInfo, 0, sizeof(VkSamplerCreateInfo));
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = filter;
	samplerCreateInfo.minFilter = filter;
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

	VK_CALL(m_device->vk.CreateSampler(m_device->handle(), &samplerCreateInfo, m_device->allocationCallbacks(), &m_sampler));
}

const VKLImage* Texture::image() const {
	return &m_image;
}

const VKLImageView* Texture::view() const {
	return &m_imageView;
}

VkSampler Texture::sampler() const {
	return m_sampler;
}

void Texture::destroy() {
	VK_CALL(m_device->vk.DestroySampler(m_device->handle(), m_sampler, m_device->allocationCallbacks()));
	m_imageView.destroy();
	m_image.destroy();
}
