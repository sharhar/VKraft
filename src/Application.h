#ifndef Application_hpp
#define Application_hpp

#include "Base.h"

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
	
	VKLImage backBuffers[2];
	VKLImageView backBufferViews[2];
	
	VKLFramebuffer framebuffer;
	
	VKLCommandBuffer* cmdBuffer;
	
	Cursor* cursor;
	
private:
	void createBackBuffer(uint32_t width, uint32_t height);
	void pollWindowEvents();
	void render();
};

#endif /* Application_h */
