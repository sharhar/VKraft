#pragma once

#include "VLKUtils.h"
#include "Cube.h"
#include "Camera.h"

void window_focus_callback(GLFWwindow* window, int focused);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
VLKShader* createBGShader(VLKDevice* device, char* vertPath, char* fragPath);
void destroyBGShader(VLKDevice* device, VLKShader* shader);
VLKShader* createCursorShader(VLKDevice* device, char* vertPath, char* fragPath, void* uniformBuffer, uint32_t uniformSize);
void destroyCursorShader(VLKDevice* device, VLKShader* shader);
VLKPipeline* createBGPipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader);
void destroyBGPipeline(VLKDevice* device, VLKPipeline* pipeline);
VLKPipeline* createCursorPipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader);
void destroyCursorPipeline(VLKDevice* device, VLKPipeline* pipeline);
VLKTexture* createCursorTexture(VLKDevice* device, char* path);
void destroyCursorTexture(VLKDevice* device, VLKTexture* texture);
VLKPipeline* createFontPipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader);
void destroyFontPipeline(VLKDevice* device, VLKPipeline* pipeline);
VLKModel* drawText(VLKDevice* device,
	std::string text, float xOff, float yOff, float size,
	VLKShader* fontShader, VLKPipeline* fontPipeline);