#ifndef TextObject_h
#define TextObject_h

#include <VKL/VKL.h>
#include <string>

class TextObject {
public:
	void updateProjection();
	
	TextObject(int maxCharNum);
	void render(VKLCommandBuffer* cmdBuffer);
	void setText(const std::string& str);
	void setCoords(float xPos, float yPos, float size);
	
private:
	VKLBuffer m_instanceBuffer;
	VKLBuffer m_uniformBuffer;
	int m_charNum;
};

#endif /* TextObject_h */
