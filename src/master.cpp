#include "VLKUtils.h"
#include "Cube.h"
#include "Camera.h"

VLKModel* drawText(VLKDevice* device,
	std::string text, float xOff, float yOff, float size,
	VLKShader* fontShader, VLKPipeline* fontPipeline) {

	int tsz = text.size();
	float* verts = new float[tsz * 4 * 6];

	for (int i = 0; i < tsz; i++) {
		uint8_t chid = (uint8_t)text.at(i);

		float sx = chid % 16;
		float sy = (chid - sx) / 16;

		float x1 = xOff + size * i;
		float y1 = yOff;
		float x2 = xOff + size * (i + 1);
		float y2 = xOff + size;

		float tx1 = (sx + 0.0f) / 16.0f;
		float ty1 = (sy + 0.0f) / 16.0f;
		float tx2 = (sx + 1.0f) / 16.0f;
		float ty2 = (sy + 1.0f) / 16.0f;

		verts[i * 24 + 0] = x1;
		verts[i * 24 + 1] = y1;

		verts[i * 24 + 2] = tx1;
		verts[i * 24 + 3] = ty1;

		verts[i * 24 + 4] = x1;
		verts[i * 24 + 5] = y2;

		verts[i * 24 + 6] = tx1;
		verts[i * 24 + 7] = ty2;

		verts[i * 24 + 8] = x2;
		verts[i * 24 + 9] = y1;

		verts[i * 24 + 10] = tx2;
		verts[i * 24 + 11] = ty1;

		verts[i * 24 + 12] = x1;
		verts[i * 24 + 13] = y2;

		verts[i * 24 + 14] = tx1;
		verts[i * 24 + 15] = ty2;

		verts[i * 24 + 16] = x2;
		verts[i * 24 + 17] = y1;

		verts[i * 24 + 18] = tx2;
		verts[i * 24 + 19] = ty1;

		verts[i * 24 + 20] = x2;
		verts[i * 24 + 21] = y2;

		verts[i * 24 + 22] = tx2;
		verts[i * 24 + 23] = ty2;
	}

	VLKModel * fontModel = vlkCreateModel(device, verts, 6 * 4 * sizeof(float) * tsz);

	VkDeviceSize offsets = 0;

	vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipeline->pipeline);
	vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &fontModel->vertexInputBuffer, &offsets);
	vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		fontPipeline->pipelineLayout, 0, 1, &fontShader->descriptorSet, 0, NULL);

	vkCmdDraw(device->drawCmdBuffer, 6 * tsz, 1, 0, 0);

	delete[] verts;

	return fontModel;
}
