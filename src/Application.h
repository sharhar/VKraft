#ifndef Application_hpp
#define Application_hpp

#include "Base.h"

#include <string>

class Timer {
private:
	double m_startTime;
	double m_time;
	int m_count;
	std::string m_name;
public:
	Timer(const std::string& name) {
		m_startTime = -1;
		m_name = name;
		
		reset();
	}
	
	void reset() {
		m_count = 0;
		m_time = 0;
	}
	
	void start() {
		m_startTime = glfwGetTime();
	}
	
	void stop() {
		m_time = m_time + glfwGetTime() - m_startTime;
		m_count++;
	}
	
	double getLapTime() {
		return m_time/m_count;
	}
	
	const std::string& getName() {
		return m_name;
	}
	
	void printLapTime() {
		printf("%s: %g ms\n", m_name.c_str(), getLapTime() * 1000);
	}
	
	void printLapTimeAndFPS() {
		printf("%s: %g ms (%g FPS)\n", m_name.c_str(), getLapTime() * 1000, 1.0 / getLapTime());
	}
};

class Application {
public:
	Application(uint32_t width, uint32_t height, const char* title);
	
	void mainLoop();
	
	void destroy();
	
	GLFWwindow* window;
	
	VKLInstance instance;
	VKLSurface surface;
	VKLDevice device;
	
	const VKLQueue* graphicsQueue;
	const VKLQueue* computeQueue;
	const VKLQueue* transferQueue;
	
	VKLSwapChain swapChain;
	VKLRenderPass renderPass;
	
	VKLImage backBuffers[2];
	VKLImageView backBufferViews[2];
	
	VKLFramebuffer framebuffer;
	
	VKLCommandBuffer* cmdBuffer;
	
	Cursor* cursor;
	
};

#endif /* Application_hpp */
