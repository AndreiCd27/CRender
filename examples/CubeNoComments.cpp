#include "Engine3D.h"

#define Scene engine->getScene()
#define workspace Scene->GetWorkspace().lock()

int main() {
    Engine3D* engine = Engine3D::GetEngine3D();

    int success = engine->setupWindow(900, 600, "window");
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

    engine->setBackground(0.2f, 0.3f, 0.8f, 1.0f);

    Blueprint* cube = Scene->CreateCube(1.0f);
    auto MyCube = Scene->CreateInstance(cube, "Cube 1");
    MyCube->SetSize(AVector3(20.0f, 20.0f, 20.0f));
    MyCube->SetPosition(AVector3(1.0f, 10.0f, 20.0f)); MyCube->SetColor(255, 0, 0, 0);

    auto Plane = Scene->CreateInstance(cube, "Plane");
    Plane->SetSize(AVector3(100.0f, 0.1f, 100.0f)); Plane->SetColor(0, 200, 0, 0);

    engine->SetupFull("static");

    int a = 18;

    while (!engine->windowShouldClose()) {

        // GAME-LOOP CODE HERE

        engine->RenderInstances(a % 24);

    }
    engine->EngineTerminate();

    return 0;
}