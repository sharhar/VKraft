#ifndef TextObject_h
#define TextObject_h

#include "Base.h"

class TextObject {
public:
	TextObject(Application* application, int maxCharNum);
	
	void render(VKLCommandBuffer* cmdBuffer);
	void setText(const std::string& str);
	void setCoords(float xPos, float yPos, float size);
	
	void destroy();
	
private:
	Application* m_application;
	
	float m_pcBuff[5];
	
	VKLBuffer m_instanceBuffer;
	int m_charNum;
};

#endif /* TextObject_h */
