#include "master.h"

float* Camera::worldviewMat = 0;
GLFWwindow* Camera::window = 0;
GLFWwindow* Camera::gameWindow = 0;
Vec3 Camera::pos = { 0, 3.1, 0 };
Vec3 Camera::renderPos = {0, 0, 0};
Vec3 Camera::rot = { 0, 0, 0 };
double Camera::prev_x = 0;
double Camera::prev_y = 0;
float Camera::yVel = 0;
std::thread* Camera::cameraThread = 0;
Vec3* Camera::poss = 0;
int Camera::possSize = 0;
int Camera::fence = 0;
bool Camera::grounded = false;

static void cameraThreadRun(GLFWwindow* win) {
	glfwMakeContextCurrent(win);
	
	std::vector<Cube*> closeCubes;
	Vec3* pvecs = NULL;

	//Setup framebuffer

	while (!glfwWindowShouldClose(win)) {
		while (Chunk::m_fence == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		Chunk::m_fence = Chunk::m_fence + 2;

		closeCubes.clear();
		for (int i = 0; i < Chunk::rcubesSize; i++) {
			Cube* cube = Chunk::rcubes[i];
			if (Camera::pos.dist(cube->m_pos) <= 8) {
				closeCubes.push_back(new Cube(cube));
			}
		}

		Chunk::m_fence = Chunk::m_fence - 2;

		int cbsz = closeCubes.size();
		Vec3* vecs = new Vec3[cbsz];

		for (int i = 0; i < cbsz; i++) {
			vecs[i] = { closeCubes[i]->m_pos.x, closeCubes[i]->m_pos.y, closeCubes[i]->m_pos.z };
		}

		/*
		GLfloat* verts = new GLfloat[3 * cbsz];

		for (int i = 0; i < cbsz; i++) {
			verts[i * 3 + 0] = vecs[i].x;
			verts[i * 3 + 1] = vecs[i].y - 0.5f;
			verts[i * 3 + 2] = vecs[i].z;
		}

		GLfloat* infs = new GLfloat[cbsz * 4];

		for (int i = 0; i < cbsz; i++) {
			infs[i * 4 + 0] = closeCubes[i]->tex->x;
			infs[i * 4 + 1] = closeCubes[i]->tex->y;
			infs[i * 4 + 2] = closeCubes[i]->tex->z;
			infs[i * 4 + 3] = closeCubes[i]->vid;
		}

		GLfloat* ids = new GLfloat[cbsz];

		for (int i = 0; i < cbsz; i++) {
			ids[i] = i + 1;
		}

		*/

		//Load and render cubes

		while (Camera::fence > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}

		Camera::fence = 1;

		Camera::poss = vecs;
		Camera::possSize = cbsz;

		Camera::fence = 0;

		/*
		if (ids != NULL) {
			delete[] ids;
		}

		if (infs != NULL) {
			delete[] infs;
		}

		if (verts != NULL) {
			delete[] verts;
		}
		*/

		if (pvecs != NULL) {
			delete[] pvecs;
		}

		pvecs = vecs;

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

void Camera::init(GLFWwindow* win, float* viewMat) {
	window = glfwCreateWindow(1, 1, "Camera Off Screen Context", NULL, NULL);
	glfwHideWindow(window);

	gameWindow = win;

	renderPos.x = pos.x;
	renderPos.y = -pos.y;
	renderPos.z = pos.z;

	worldviewMat = viewMat;
	getWorldview(worldviewMat, renderPos, rot);

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	prev_x = xpos;
	prev_y = ypos;

	cameraThread = new std::thread(cameraThreadRun, window);
}

bool hittingCube(Vec3 pos, Vec3 other) {
	if (pos.x + 0.1f < other.x - 0.5f || other.x + 0.5f  < pos.x - 0.1f) {
		return false;
	}

	if (pos.z + 0.1f < other.z - 0.5f || other.z + 0.5f  < pos.z - 0.1f) {
		return false;
	}

	if (pos.y < other.y - 0.5f || other.y + 0.5f  < pos.y - 1) {
		return false;
	}

	return true;
}

bool hittingCubes(Vec3 pos) {
	for (int i = 0; i < Camera::possSize; i++) {
		if (hittingCube(pos, Camera::poss[i])) {
			return true;
		}
	}

	return false;
}

void Camera::update(float dt) {
	bool focused = glfwGetInputMode(gameWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

	double xpos, ypos;
	glfwGetCursorPos(gameWindow, &xpos, &ypos);

	if (focused) {
		float speed = 3.5f;

		if (glfwGetKey(gameWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			speed *= 2;
		}

		float xVel = 0;
		float zVel = 0;

		if (glfwGetKey(gameWindow, GLFW_KEY_W) == GLFW_PRESS) {
			zVel -= speed * cos(rot.y*DEG_TO_RAD) * dt;
			xVel += speed * sin(rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(gameWindow, GLFW_KEY_S) == GLFW_PRESS) {
			zVel += speed * cos(rot.y*DEG_TO_RAD) * dt;
			xVel -= speed * sin(rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(gameWindow, GLFW_KEY_A) == GLFW_PRESS) {
			zVel -= speed * sin(rot.y*DEG_TO_RAD) * dt;
			xVel -= speed * cos(rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(gameWindow, GLFW_KEY_D) == GLFW_PRESS) {
			zVel += speed * sin(rot.y*DEG_TO_RAD) * dt;
			xVel += speed * cos(rot.y*DEG_TO_RAD) * dt;
		}

		//T = 3/5, H = 1.2

		if (grounded && glfwGetKey(gameWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
			yVel = 10;
			grounded = false;
		}

		yVel -= 33.3333f * dt;

		while (fence == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		fence = fence + 2;

		pos.x += xVel;

		if (hittingCubes(pos)) {
			pos.x -= xVel;
		}

		pos.y += yVel * dt;

		if (hittingCubes(pos)) {
			pos.y -= yVel * dt;
			yVel = 0;
			grounded = true;
		}

		pos.z += zVel;

		if (hittingCubes(pos)) {
			pos.z -= zVel;
		}

		fence = fence - 2;

		if (glfwGetKey(gameWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetInputMode(gameWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		float xdiff = (prev_x - xpos);

		if (xdiff != 0) {
			rot.y -= 5 * xdiff * 0.085f;// *2 * dt;
		}

		float ydiff = (prev_y - ypos);

		if (ydiff != 0) {
			rot.x += 5 * ydiff * 0.085f;// *2 * dt;
		}

		if (rot.x > 90) {
			rot.x = 90;
		}

		if (rot.x < -90) {
			rot.x = -90;
		}
	}

	prev_x = xpos;
	prev_y = ypos;

	renderPos.x = pos.x;
	renderPos.y = -pos.y;
	renderPos.z = pos.z;

	getWorldview(worldviewMat, renderPos, rot);
}