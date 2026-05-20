#include "Engine3D.h"
#include <iostream>
#include <chrono>

#include "LightingModel.h"
#include "Image.h"

#define scene engine->getScene()
#define workspace scene->GetWorkspace().lock()

Engine3D* engine = Engine3D::GetEngine3D();

int main() {

    std::cout << "5!! = " << DoubleFactorial(5) << "\n";

    std::cout << "Legendre Polynomial L=2, M=1 at x=0.5 ---> " << LegendrePolynomial<2, 1>().Compute(0.5f) << "\n";

    std::cout << "SH basis function L=2, M=1 at theta=0.5, phi=0.2 ---> " << Y<2, 1>().Compute(0.5f, 0.2f) << "\n";

    SH<3> s;

    auto res = s.EvaluateAll(0.5f, 0.2f);
    int index = 0;
    for (int l = 0; l <= 3; l++) {
        for (int m = -l; m <= l; m++) {
            std::cout << "SH basis evaluation : L = " << l << ", M = " << m << " | " << res[index] << "\n";
            index++;
        }
    }


    int success = engine->setupWindow(1200, 900, "window");
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

    // SH soft shadows

    SHLM shlm(engine, engine->getCFG(), 
        AVector3(512,32,512), AVector3(-1024.0f, -16.0f, -1024.0f), AVector3(1024.0f, 48.0f, 1024.0f));

    //

    Blueprint* cube = scene->CreateCube(1.0f);
    cube->SetColor(0, 255, 0, 255);

    // PLANE
    auto c = scene->CreateInstance(cube, "c");
    c->SetPosition(AVector3(0.0f, 0.0f, 0.0f));
    c->SetSize(AVector3(500.0f, 6.0f, 500.0f));

    auto c2 = scene->CreateInstance(cube, "c");
    c2->SetPosition(AVector3(0.0f, 30.0f, 0.0f));
    c2->SetSize(AVector3(30.0f, 60.0f, 30.0f));
    c2->SetColor(255, 0, 255, 255);

    auto c3 = scene->CreateInstance(cube, "c");
    c3->SetPosition(AVector3(-30.0f, 10.0f, -20.0f));
    c3->SetSize(AVector3(20.0f, 20.0f, 20.0f));
    c3->SetColor(0, 255, 255, 255);

    //////////////////////////////
    engine->DEBUG_ArrayOrganizers();
    scene->GetInstanceOrganizer().print();
    scene->GetMatrixOrganizer().print();

    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "Printing Instance VBO dimensions: \n";
    scene->GetInstanceOrganizer().print();

    float t = 8.0f;

    // PRE-GAME LOOP ---> ACTIVATE SHLM
    shlm.BindToEngine(45.0f, 0.01f, 1000.0f);

    shlm.Load_Cubemap_GPU_ComputeShader_Extended();

    auto ims = ImageService::GetService();

    auto IMG = ims->CreateImage(IMG_TYPE::PNG, "resources/cat_coding.png");

    // UP-scaling, and converting to TIFF at the same time
    auto IMG2 = ims->CreateImage(IMG_TYPE::TIFF, 1000, 1200, "resources/example.tiff");

    ims->TexAdapter->Resample(*IMG, *IMG2, ims);

    int widwn = IMG.get()->GetWidth();
    std::cout << "Width img: " << widwn << "\n";

    bool screenshot = false;

    auto IMG3 = ims->CreateImage(IMG_TYPE::TIFF, "resources/screenshot.tiff");
    Request<IMG_TYPE, int, int, int, int, std::string>* screenshot_request =
        ims->GetScreenshotRequest(IMG_TYPE::TIFF, IMG3->GetWidth(), IMG3->GetHeight(), 1200, 900, "resources/screenshot.tiff");
    engine->getCFG()->PostRenderRequest = screenshot_request;

    while (!engine->windowShouldClose()) {

        // GAME-LOOP CODE HERE
        engine->RenderInstances( 
            t / 64.0f * 12.0f  + 8.0f
        );

        t += 0.01f;

        //std::cout << t / 64.0f * 24.0f << " TIME \n";
        if (t > 16.0f) {
            if (!screenshot) {
                engine->getCFG()->PostRenderRequest->Activate();
                std::cout << "SCREENSHOT ACTIVATED!  \n";
                screenshot = true;
            }
        }
        if (t > 64.0f) {
            t = 0.0f;
        }
    }

    delete screenshot_request;

    engine->EngineTerminate();

    return 0;
}