
#include <iostream>

#include "Engine3D.h"
// Alte librarii utilizate

// Niste define-uri pentru simplificarea codului
// Schimba linia asta daca modifici numele singleton-ului
#define Scene engine->getScene()
#define workspace Scene->GetWorkspace().lock()

// Variabile pentru fereastra noastra
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const char* WINDOW_TITLE = "window";

int main() {
    Engine3D* engine = Engine3D::GetEngine3D();

    int success = engine->setupWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE); // 1 = success, altceva = EROARE
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

    engine->setBackground(0.2f, 0.3f, 0.8f, 1.0f); // Setam culoarea de background

    // AICI CREAM BLUEPRINT-URI SI INSTANTE /////////////
    Blueprint* cube = Scene->CreateCube(10.0f);
    // Cream o INSTANTA (auto devine shared_ptr<Instance>)
    auto MyCube = Scene->CreateInstance(cube, AVector3(0.0f, 10.0f, -5.0f), "Cube 1");
    // Automat instanta devine parinte al Workspace
    // Daca variabila MyCube se schimba, putem sa o gasim usor in ierarhie
    // Sau putem stoca intr-o variabila o referinta catre aceasta instanta
    auto MyCube2 = workspace->FirstChild("Cube 1");
    // Putem translata obiectul
    MyCube->SetPosition(AVector3(1.0f, 5.0f, 2.0f));
    MyCube->SetRotation(AVector3(0.0f, 45.0f, 0.0f));
    MyCube->SetColor(0, 255, 0, 0); // Setam culoarea
    /////////////////////////////////////////////////////

    // Apelam metodele engine-ului
    engine->setCamera(0.0f, 40.0f, 120.0f); // Camera player-ului
    engine->setSunCamera(0.0f, 100.0f, 1.0f); // Camera soarelui

    // Configuram shaderele, puteti adauga alte shadere fata de cele 2 default
    engine->setupShaders();

    // Pasul acesta trebuie facut neaparat
    // Aici configuram obiecte esentiale pentru OpenGL:
    // 1) VAO (Vertex Array Object) - stocam aici "layout-urile" si necesita sa fie legat de un VBO, EBO si instanceVBO in cazul nostru
    // 2) VBO (Vertex Buffer Object) - stocam aici varfurile Blueprint-urilor, trimitem "punctele 3D" catre OpenGL
    // 3) EBO (Entity Buffer Object) - stocam aici indicii varfurilor, asa determina OpenGL cum trebuie sa conecteze varfurile
    // 4) instanceVBO - stocam aici matrici 4x4 de translatie
    engine->setupGeometryArrayObjects(engine->getDrawStyle("static"));
    engine->setupInstanceVBO();

    Scene->DEBUG_PrintInstanceHierarchy(workspace, 0, 10, true); // OPTIONAL, daca vreti sa vedeti ierarhia

    // Intram in GAME-LOOP si iesim cand se inchide fereastra
    while (!engine->windowShouldClose()) {
        engine->initGameFrame(); // Metoda recoloreaza background-ul in caz ca s-a schimbat, aici puteti adauga si alt cod care sa se execute inainte de desenarea instantelor

        // COD GAME LOOP

        // Apelam metodele de render, primul parametru e false pentru ca nu vrem sa folosim shader-ul default care doar ne randeaza blueprint-urile
        engine->shadowPass(false); // Pasul care genereaza un SHADOW MAP folosit pentru determinarea umbrelor
        engine->renderPass(false, 45.0f, 0.1f, 1000.0f); // Pasul care randeaza instantele pe ecran
    }

    // OBLIGATORIU: dupa iesirea din GAME-LOOP, inchidem procesul de randare si curatam memoria
    engine->EngineTerminate();

    return 0;
}