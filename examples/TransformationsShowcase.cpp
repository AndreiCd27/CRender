#include "Engine3D.h"
#include <iostream>

#define scene engine->getScene()
#define workspace scene->GetWorkspace().lock()

const long long int sampleSize = 63; // defines a 2N by 2N area of cubes
const int gridSize = 16;

const float vectorWidth = 4.0f;

Engine3D* engine = Engine3D::GetEngine3D();
auto vectB = scene->CreateUnitVector();

class MyVector {
    std::shared_ptr<Instance> v = nullptr;
    AVector3 vec3 = AVector3(0.0f, 1.0f, 0.0f);
    AVector3 applyPoint = AVector3(0.0f, 0.0f, 0.0f);
public:
    MyVector(AVector3 _vec3, AColor3 CLR) : vec3(_vec3) {
        v = scene->CreateInstance(vectB, "Vector");
        v->SetColor(CLR);
        v->SetSize(AVector3(vectorWidth, vectorWidth, vec3.Magnitude()));
        v->LookAt(AVector3(0.0f, 0.0f, 0.0f), vec3);
    }
    void Update(AVector3 _vec3) {
        vec3 = _vec3;
        v->SetSize(AVector3(vectorWidth, vectorWidth, vec3.Magnitude()));
        v->LookAt(AVector3(0.0f, 0.0f, 0.0f), vec3);
        v->SetPosition(applyPoint);
    }
    void SetApplyPoint(AVector3 _applyPoint) {
        applyPoint = _applyPoint;
        v->SetPosition(applyPoint);
    }
    MyVector(const MyVector& dr) {
        v = scene->CreateInstance(vectB, "Vector");
        vec3 = dr.vec3;
        applyPoint = dr.applyPoint;
        v->SetSize(AVector3(vectorWidth, vectorWidth, vec3.Magnitude()));
        v->LookAt(AVector3(0.0f, 0.0f, 0.0f), vec3);
        v->SetPosition(applyPoint);
    }
    MyVector& operator=(const MyVector& dr) {
        if (this == &dr) {
            return *this;
        }
        v = scene->CreateInstance(vectB, "Vector");
        vec3 = dr.vec3;
        applyPoint = dr.applyPoint;
        v->SetSize(AVector3(vectorWidth, vectorWidth, vec3.Magnitude()));
        v->LookAt(AVector3(0.0f, 0.0f, 0.0f), vec3);
        v->SetPosition(applyPoint);
    }
};

int main() {

    int success = engine->setupWindow(900, 600, "window");
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

    double t0 = glfwGetTime();
    std::cout << "A) Timer started \n";

    Blueprint* cube = scene->CreateCube(1.0f);
    cube->SetColor(0, 255, 0, 255);

    const int mul = 150 / gridSize;

    for (int i = -gridSize; i < gridSize; i++) {
        auto lineX = scene->CreateInstance(cube, "LineX");
        float w = (i == 0) ? 0.5f : 0.25f;
        lineX->SetSize(AVector3((float)gridSize * 10.0f, w, w));
        lineX->SetPosition(AVector3(0.0f, 0.0f, 5.0f * (float)i));
        lineX->SetColor(
            150 - std::abs(i) * mul, 150, 150 - std::abs(i) * mul, 255);
    }

    auto lineY = scene->CreateInstance(cube, "LineY");
    lineY->SetSize(AVector3(0.5f, (float)gridSize * 10.0f, 0.5f));
    lineY->SetColor(150, 150, 150, 255);

    for (int i = -gridSize; i < gridSize; i++) {
        auto lineZ = scene->CreateInstance(cube, "LineZ");
        float w = (i == 0) ? 1.0f : 0.25f;
        lineZ->SetSize(AVector3(w, w, (float)gridSize * 10.0f));
        lineZ->SetPosition(AVector3(5.0f * (float)i, 0.0f, 0.0f));
        lineZ->SetColor(
            150, 150 - std::abs(i) * mul, 150 - std::abs(i) * mul, 255);
    }

    double t1 = glfwGetTime();
    std::cout << "Loading time A: " << t1 - t0 << " seconds \n";
    t0 = glfwGetTime();
    std::cout << "B) Timer started \n";

    auto showcaseCUBE = scene->CreateInstance(cube, "showcase");
    showcaseCUBE->SetPosition(AVector3(10.0f, 5.0f, 8.0f));

    // VECTOR

    //////////////////////////////

    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);

    t1 = glfwGetTime();
    std::cout << "Loading time B: " << t1 - t0 << " seconds \n";

    //FOR FPS COUNTER
    double PREV_TIME = 0.0f;
    int frameCounter = 0;
    const double FPSsampleTime = 1.0f / 20.0f;

    float rot = 0.0f;

    int t = 0;

    while (!engine->windowShouldClose()) {

        double CURRENT_TIME = glfwGetTime();
        double timeDifference = CURRENT_TIME - PREV_TIME;
        frameCounter++;
        if (timeDifference >= FPSsampleTime) {
            std::string FPS = std::to_string((1.0f / timeDifference) * frameCounter);
            std::string msPerFrame = std::to_string((timeDifference / frameCounter) * 1000);
            std::string winTitle = "WINDOW | " + FPS + " FPS | " + msPerFrame + " ms/frame";
            engine->setWindowTitle(winTitle);
            PREV_TIME = CURRENT_TIME;
            frameCounter = 0;

            t++;
        }

        

        // GAME-LOOP CODE HERE
        engine->RenderInstances(13);
    }

    engine->EngineTerminate();

    return 0;
}