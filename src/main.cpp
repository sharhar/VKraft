#include "Application.h"

int main() {
	Application app(1600, 900, "VKraft");
	
	app.mainLoop();
	
	app.destroy();
	
	return 0;
}
