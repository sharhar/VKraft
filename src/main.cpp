#include "Application.h"

int main() {
	Application app(800, 600, "VKraft");
	
	app.mainLoop();
	
	app.destroy();
	
	return 0;
}
