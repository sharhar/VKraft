#ifndef Application_hpp
#define Application_hpp

#include "Base.h"
#include "Texture.h"

class Application {
public:
	Application(int width, int height, const char* title);
	
	void mainLoop();
	
	void destroy();
	
	GLFWwindow* window;
	
	VKLInstance instance;
	VKLSurface surface;
	VKLDevice device;
	
	int winWidth;
	int winHeight;
	
	const VKLQueue* graphicsQueue;
	const VKLQueue* computeQueue;
	const VKLQueue* transferQueue;
	
	VKLSwapChain swapChain;
	VKLRenderPass renderPass;
	
	VKLImageCreateInfo backBuffersCreateInfo;
	
	VKLImage backBuffers[3];
	VKLImageView backBufferViews[3];
	
	VKLFramebuffer framebuffer;
	
	VKLCommandBuffer* cmdBuffer;
	
	Cursor* cursor;
	Camera* camera;
	
	ChunkManager* chunkManager;
	ChunkRenderer* chunkRenderer;
	
	struct TextRenderingData {
		VKLBuffer vertBuffer;
		VKLPipelineLayout layout;
		VKLPipeline pipeline;
		
		Texture* texture;
		
		VKLDescriptorSet* descriptorSet;
	};
	
	TextRenderingData textRenderingData;
	
	TextObject* fpsText;
	TextObject* posText;
	
private:
	void createBackBuffer(uint32_t width, uint32_t height);
	void pollWindowEvents();
	void render();
	
	void setupTextRenderingData();                                  // These functions are both implemented in TextObject.cpp for orginizational purposes
	void perpareTextRendering(const VKLCommandBuffer* cmdBuff);     //
	void cleanUpTextRenderingData();                                //
};

#endif /* Application_h */
