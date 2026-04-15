#include "Engine3D.h"
#include <iostream>

#define scene engine->getScene()
#define workspace scene->GetWorkspace().lock()

const long long int sampleSize = 64; // defines a 2N by 2N area of cubes
const float funcGridSize = 4.0f;
const int gridSize = 16; //16
int f_div = 1; // Divisor for our function values
float pos_scalar = 0.5f;

int graphFunction3D_t0(int x, int z) {
    //outputs y (UP)
    return (int)(sinf((float)x / 32.0f * glm::pi<float>()) * cosf((float)z / 32.0f * glm::pi<float>()) * 60.0f);
}
int graphFunction3D_t1(int x, int z) {
    //outputs y (UP)
    return (int)(sinf((float)z*z / 512.0f * glm::pi<float>()) * cosf((float)x*x / 512.0f * glm::pi<float>()) * 60.0f);
}
int graphFunction3D(int x, int z, const float t) {
    return t * graphFunction3D_t0(x, z) + (1 - t) * graphFunction3D_t1(x, z);
}

const float vectorWidth = 4.0f;

Engine3D* engine = Engine3D::GetEngine3D();
auto vectB = scene->CreateUnitVector();

class MyVector {
    UserRef<Instance> v;
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
        return *this;
    }
};

const AColor3 getClr(const int x, const int y, const int z) {
    //return AColor3((int)((sinf((float)x / 16.0f) + 1.0f)/2.0f * 255.0f), y * 2, (int)((cosf((float)z / 16.0f) + tanf((float)z/16.0f) + 1.0f) / 2.0f * 255.0f), 255);
    return AColor3(
        y*4 + 20,

        sinf((float)x / 64.0f * cosf((float)z / 16.0f)) * 
        cosf((float)z / 64.0f * sinf((float)x / 16.0f)) * 128 + 255, 

        255, 255
    );
}
const AVector3 getClrV(const int x, const int y, const int z, const float t) {
    //return AColor3((int)((sinf((float)x / 16.0f) + 1.0f)/2.0f * 255.0f), y * 2, (int)((cosf((float)z / 16.0f) + tanf((float)z/16.0f) + 1.0f) / 2.0f * 255.0f), 255);
    return AVector3(
        y*4 + 20,

        sinf((float)x / 64.0f * cosf((float)z / 16.0f)) * 
        cosf((float)z / 64.0f * sinf((float)x / 16.0f)) * 128 + 255, 

        255
    ) * t + 
    AVector3(
        sinf((float)x / 64.0f * cosf((float)z / 16.0f)) *
        cosf((float)z / 64.0f * sinf((float)x / 16.0f)) * 128 + 255, 
        
        y * 4 + 20,

        255
    ) * (1 - t);
}

int main() {

    int success = engine->setupWindow(1200, 900, "window");
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

    double t0 = glfwGetTime();
    std::cout << "A) Timer started \n";

    Blueprint* cube = scene->CreateCube(funcGridSize);
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
        float w = (i == 0) ? 0.5f : 0.25f;
        lineZ->SetSize(AVector3(w, w, (float)gridSize * 10.0f));
        lineZ->SetPosition(AVector3(5.0f * (float)i, 0.0f, 0.0f));
        lineZ->SetColor(
            150, 150 - std::abs(i) * mul, 150 - std::abs(i) * mul, 255);
    }
    

    double t1 = glfwGetTime();
    std::cout << "Loading time A: " << t1 - t0 << " seconds \n";
    t0 = glfwGetTime();
    std::cout << "B) Timer started \n";

    auto plane = scene->CreateInstance(cube, "Plane");
    plane->SetColor(100, 100, 100, 255);
    plane->SetSize(AVector3(10.0f, 10.0f, 0.25f));

    // VECTOR

    AVector3 vec3_1 = AVector3(30.0f, 0.0f, 20.0f);
    AVector3 vec3_2 = AVector3(10.0f, 0.0f, -30.0f);

    MyVector v1(vec3_1, AColor3(255, 0, 0, 255));
    MyVector v2(vec3_2, AColor3(0, 255, 0, 255));

    AVector3 vec3_add = vec3_1 + vec3_2;
    MyVector v3(vec3_add, AColor3(255, 255, 0, 255));

    AVector3 vec3_cross = vec3_1 ^ vec3_2;
    MyVector v4(vec3_cross.Normalize() * 10.0f, AColor3(0, 255, 255, 255));

    // Define our cubes //////////

    UserRef<Instance> graph[sampleSize * 2][sampleSize * 2];

    for (int x = -sampleSize; x < sampleSize; x++) {
        for (int z = -sampleSize; z < sampleSize; z++) {
            const int y = graphFunction3D(x, z, 0.5f ) / f_div;
            //val[x + sampleSize][z + sampleSize] = y;
            graph[x + sampleSize][z + sampleSize] = scene->CreateInstance(cube, "t_" + std::to_string(x) + "_" + std::to_string(z));
            graph[x + sampleSize][z + sampleSize]->SetPosition(AVector3(x * funcGridSize, y + 60.0f, z * funcGridSize) * pos_scalar);
            graph[x + sampleSize][z + sampleSize]->SetColor(
                getClr(x,y,z)
            );
        }
    }

    //////////////////////////////
    engine->DEBUG_ArrayOrganizers();
    scene->GetInstanceOrganizer().print();
    scene->GetMatrixOrganizer().print();

    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);

    t1 = glfwGetTime();
    std::cout << "Loading time B: " << t1 - t0 << " seconds \n";

    std::cout << "Printing Instance VBO dimensions: \n";
    scene->GetInstanceOrganizer().print();

    float rot = 0.0f;
    int frci = 0;

    //scene->DEBUG_PrintInstanceHierarchy(workspace, 0, 1, true);

    struct Measurement {
        double tset = 0.0f;
        double tdraw = 0.0f;
        int tk = 0;
    } Mt;

    int fsi = 0;
    int sign = 1;
    while (!engine->windowShouldClose()) {

        double tst = glfwGetTime() * 1000.0f;

        AVector3 vec3_1copy = vec3_1.Normalize().Rotate(AVector3(rot * 0.75f, 0.0f, 0.0f)) * vec3_1.Magnitude();
        AVector3 vec3_2copy = vec3_2.Normalize().Rotate(AVector3(0.0f, 0.0f, rot * 2.0f)) * vec3_2.Magnitude();
        vec3_add = vec3_1copy + vec3_2copy;
        vec3_cross = vec3_1copy ^ vec3_2copy;
        v1.Update(vec3_1copy);
        v2.Update(vec3_2copy);
        v3.Update(vec3_add);
        v4.Update(vec3_cross.Normalize() * 10.0f);

        rot += 0.025f;

        plane->LookAt(AVector3(0.0f, 0.0f, 0.0f), vec3_cross);

        for (int x = -sampleSize; x < sampleSize; x++) {
            for (int z = -sampleSize; z < sampleSize; z++) {
                const int y = graphFunction3D(
                    (x + sampleSize + frci) % (sampleSize * 2),
                    (z + sampleSize + frci) % (sampleSize * 2), 
                    (float)fsi / 64.0f
                );
                graph[x + sampleSize][z + sampleSize]->SetPosition(AVector3(x * funcGridSize, y + 60.0f, z * funcGridSize));

                AVector3 sampleClrV = AVector3(0.0f, 0.0f, 0.0f);
                for (int xi = -1; xi < 1; xi++) {
                    for (int zi = -1; zi < 1; zi++) {
                        const int yi = graphFunction3D(
                            (x + xi + sampleSize + frci) % (sampleSize * 2),
                            (z + zi + sampleSize + frci) % (sampleSize * 2),
                            (float)fsi / 64.0f
                        );
                        sampleClrV += getClrV(
                            (x + xi - frci) * 8, 
                            yi + 60, (z + zi + frci) * 8,
                            (float)fsi / 64.0f
                        );
                    }
                }
                sampleClrV = sampleClrV * (1.0f / 9.0f);

                graph[x + sampleSize][z + sampleSize]->SetColor(
                    AColor3(sampleClrV.x, sampleClrV.y, sampleClrV.z, 255)
                    //getClr((x - frci)*8, y + 60, (z + frci)*8)
                );
            }
        }
        frci++;
        fsi += sign;
        if (fsi % 64 == 0) sign = -sign;

        double dt = glfwGetTime() * 1000.0f - tst;
        Mt.tset += dt;

        tst = glfwGetTime() * 1000.0f;

        // GAME-LOOP CODE HERE
        engine->RenderInstances( 12.0f
            //(float)fsi / 64.0f * 24.0f  
        );

        dt = glfwGetTime() * 1000.0f - tst;
        Mt.tdraw += dt;

        Mt.tk++;

        if (Mt.tk % 256 == 0) {
            std::cout << "Frame Set Time: " << Mt.tset / (float)Mt.tk << "ms | Draw Time: " << Mt.tdraw / (float)Mt.tk << "ms \n";
        }
    }

    //scene->DEBUG_PrintInstanceHierarchy(workspace,0,1,true);

    engine->EngineTerminate();

    return 0;
}