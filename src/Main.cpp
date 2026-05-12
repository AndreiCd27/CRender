#include "Engine3D.h"
#include <iostream>
#include <chrono>

#include "LightingModel.h"

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
        AVector3(512,16,512), AVector3(-1024.0f, -32.0f, -1024.0f), AVector3(1024.0f, 32.0f, 1024.0f));

    auto b_compute_t0 = std::chrono::high_resolution_clock::now();

    /*
    for (int i = 0; i < 500; i++) {
        float x = (i % 360) / 180.0f * __PI;
        float y = (i % 180) / 180.0f * __PI;
        float z = (float)(i % 128 + 120);
        float r = (float)(i % 8 + 1);
        shlm.SetBlockerOBJ(x, y, z, r);
    }
    */

    // CUBE c2
    shlm.SetBlockerOBJ(-5.0f, 5.0f, -5.0f,5.0f, 2);
    shlm.SetBlockerOBJ(5.0f, 5.0f, -5.0f,5.0f, 2);
    shlm.SetBlockerOBJ(-5.0f, 5.0f, 5.0f,5.0f, 2);
    shlm.SetBlockerOBJ(5.0f, 5.0f, 5.0f,5.0f, 2);

    shlm.SetBlockerOBJ(-5.0f, 15.0f, -5.0f,5.0f,2);
    shlm.SetBlockerOBJ(5.0f, 15.0f, -5.0f,5.0f,2);
    shlm.SetBlockerOBJ(-5.0f, 15.0f, 5.0f,5.0f,2);
    shlm.SetBlockerOBJ(5.0f, 15.0f, 5.0f,5.0f,2);

    shlm.SetBlockerOBJ(-5.0f, 25.0f, -5.0f,5.0f,2);
    shlm.SetBlockerOBJ(5.0f, 25.0f, -5.0f,5.0f,2);
    shlm.SetBlockerOBJ(-5.0f, 25.0f, 5.0f,5.0f,2);
    shlm.SetBlockerOBJ(5.0f, 25.0f, 5.0f,5.0f,2);

    shlm.SetBlockerOBJ(-5.0f, 35.0f, -5.0f,5.0f,2);
    shlm.SetBlockerOBJ(5.0f, 35.0f, -5.0f,5.0f,2);
    shlm.SetBlockerOBJ(-5.0f, 35.0f, 5.0f,5.0f,2);
    shlm.SetBlockerOBJ(5.0f, 35.0f, 5.0f,5.0f,2);

    // CUBE c3
    shlm.SetBlockerOBJ(-25.0f, 5.0f, -25.0f,5.0f,3);
    shlm.SetBlockerOBJ(-15.0f, 5.0f, -25.0f,5.0f,3);
    shlm.SetBlockerOBJ(-25.0f, 5.0f, -15.0f,5.0f,3);
    shlm.SetBlockerOBJ(-15.0f, 5.0f, -15.0f,5.0f,3);

    shlm.SetBlockerOBJ(-25.0f, 15.0f, -25.0f,5.0f,3);
    shlm.SetBlockerOBJ(-15.0f, 15.0f, -25.0f,5.0f,3);
    shlm.SetBlockerOBJ(-25.0f, 15.0f, -15.0f,5.0f,3);
    shlm.SetBlockerOBJ(-15.0f, 15.0f, -15.0f,5.0f,3);


    shlm.SetBlockerOBJ(50.0f, 25.0f, 50.0f,15.0f,1);

    shlm.Load_Cubemap_GPU_ComputeShader();

    shlm.BindToEngine(45.0f, 0.01f, 1000.0f);

    auto b_compute_t1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = b_compute_t1 - b_compute_t0;

    std::cout << "\nTotal Visbility Function COMPUTE TIME: " << duration.count() << " ms\n\n";

    //

    Blueprint* cube = scene->CreateCube(1.0f);
    cube->SetColor(0, 255, 0, 255);

    // PLANE
    auto c = scene->CreateInstance(cube, "c");
    c->SetPosition(AVector3(0.0f, 0.0f, 0.0f));
    c->SetSize(AVector3(1024.0f, 0.1f, 1024.0f));

    auto c2 = scene->CreateInstance(cube, "c");
    c2->SetPosition(AVector3(0.0f, 20.0f, 0.0f));
    c2->SetSize(AVector3(20.0f, 40.0f, 20.0f));
    c2->SetColor(255,0,255,255);

    auto c3 = scene->CreateInstance(cube, "c");
    c3->SetPosition(AVector3(-20.0f, 10.0f, -20.0f));
    c3->SetSize(AVector3(20.0f, 20.0f, 20.0f));
    c3->SetColor(0,255,255,255);

    //

    //////////////////////////////
    engine->DEBUG_ArrayOrganizers();
    scene->GetInstanceOrganizer().print();
    scene->GetMatrixOrganizer().print();

    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "Printing Instance VBO dimensions: \n";
    scene->GetInstanceOrganizer().print();

    float t = 8.0f;

    while (!engine->windowShouldClose()) {

        // GAME-LOOP CODE HERE
        engine->RenderInstances( 
            t / 64.0f * 12.0f  + 8.0f
        );

        t += 0.01f;
        //std::cout << t / 64.0f * 24.0f << " TIME \n";
        if (t > 64.0f) {
            t = 0.0f;
        }
    }

    //scene->DEBUG_PrintInstanceHierarchy(workspace,0,1,true);

    engine->EngineTerminate();

    return 0;
}