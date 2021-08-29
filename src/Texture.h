//
//  Texture.h
//  VKraft
//
//  Created by Shahar Sandhaus on 8/25/21.
//

#ifndef Texture_h
#define Texture_h

#include <VKL/VKL.h>

class Texture {
public:
	Texture(const VKLDevice* device, const VKLQueue* queue, const char* path, VkFilter filter);
	
	const VKLImage* image() const;
	const VKLImageView* view() const;
	VkSampler sampler() const;
	
	void destroy();
	
private:
	const VKLDevice* m_device;
	
	VKLImage m_image;
	VKLImageView m_imageView;
	VkSampler m_sampler;
};

#endif /* Texture_h */
