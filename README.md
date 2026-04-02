# CRender (3D Renderer in C++ realizat cu OpenGL 3.3)

## Informatii generale

Dezvoltarea proiectului a inceput la finalul lunii februarie.
Acest repo a fost inital clonat din un repo ceva mai vechi de al meu https://github.com/AndreiCd27/3D-Renderer

CRender este atat un vizualizator 3D, cat si un enviroment in care utilizatorul poate crea jocuri simple 3D, avand functii implementate pentru translatii (schimbare pozitie, rotatie, dimensiuni) cat si o ierarhie de instante care este esentiala pentru scalabilitate. Proiectul contine destul de multe abstractizari pentru a face crearea de obiecte foarte usoara.

### Un workflow tipic ar fi urmatorul:
- Configuram engine-ul / renderer-ul
- Cream niste *Blueprint-uri* pentru viitoarele *Instante*
- Instantiem obiecte si apelam metodele pentru a pozitiona, roti, scala
(Putem eventual sa cream o ierarhie de instante folosind SetParent(), AddChild() etc.)
- Cream un game-loop, si apelam metodele engine-ului de render
- Verificam daca fereasta trebuie inchisa si oprim game-loop-ul
- Apelam OBLIGATORIU engine.EngineTerminate() dupa finalizare

### Un exemplu de program 

Exemple de cod folosind clasa Engine3D puteti gasi in directorul examples.
Cel mai explicativ este "Cube.cpp", care creaza un cub si il randeaza pe ecran.

Gasiti in directorul *src* fisierul *Main.cpp*, care e codul "default", si incearca sa arate toate functionalitatiile clasei.

Fisierul *Main.cpp* initializeaza Singleton-ul Engine3D, apoi creaza niste *Blueprint-uri* folosind metode precum CreateCube(), CreatePrism() si LoadSTLGeomFile(). Apoi creaza cateva instante ale acestora si aplica asupra lor niste translatii.

Pe ecran ar trebui sa apara un plan verde, 10 cuburi pozitionate diagonal, o prisma triunghiulara si 4 modele de oameni (modelul il gasiti in Resources). In game-loop, experimentam cu metodele de translatie, si observam ca instantele copil se translateaza odata cu parintii. 

Se afiseaza apoi in consola ierarhia instantelor prin intermediul metodelor cu prefixul 'DEBUG'.

   Rezultat
![Main.cpp](https://github.com/AndreiCd27/CRender/blob/main/Gallery/Screenshot0.png)

### Documentatie

O documentatie completa va fi incarcata pana la data de 6 aprilie.

Codul este comentat destul de des, deci puteti folosi repo-ul pentru a va crea proprile implementari. Cu toate acestea, unele clase mai necesita niste redactare. In curand voi adauga niste comentarii mai "riguroase".
Comentariile din codul sursa au fost redactate in limba engleza pentru a putea fi intelese si de vitorii citiori.

## Instalare / Rularea proiectului

### Windows

Librariile necesare pentru proiect sunt deja incluse in directorul Libraries, iar Windows vine nativ cu OpenGL, trebuie doar sa creati build-ul ruland cmake.
Puteti sa rulati repo-ul fie in Microsoft Visual Studio, fie instalati CMake (https://cmake.org/download/) si il rulati acolo.
Executabilul cel mai probabil se va genera in directorul build care va fi creat automat de CMake.

### MacOS

Librariile necesare pentru proiect sunt deja incluse in directorul Libraries, iar MacOS vine nativ cu OpenGL, trebuie doar sa rulati cmake.
Puteti sa rulati fisierul in terminal, dar trebuie sa va asigurati ca aveti CMake pe care puteti sa il descarcati folosind un package-manager precum Homebrew (https://formulae.brew.sh/formula/cmake#default).
```
brew install cmake
```
Apoi, puteti sa rulati urmatoarele comenzii in directorul unde se afla repo-ul.
```
cd CRender/build
cmake ..
cmake --build .
```
Alternativ, instalati CMake (https://cmake.org/download/) si introduceti fisierul.
Executabilul cel mai probabil se va genera in directorul build care va fi creat automat de CMake.

### Linux

Aveti in directorul repo-ului un script shell care contine toate comenzile necesare pentru a instala OpenGL si GLFW
Aici gasiti dependintele Linux pentru OpenGL si GLFW
https://medium.com/geekculture/a-beginners-guide-to-setup-opengl-in-linux-debian-2bfe02ccd1e

Puteti instala CMake folosind package-manager-ul vostru default.

Pentru distributii Debian-based:
Instalati CMake si librariile folosite de OpenGL
```
sudo apt-get install cmake pkg-config
sudo apt-get install mesa-utils libglu1-mesa-dev freeglut3-dev mesa-common-dev
sudo apt-get install libglew-dev libglfw3-dev libglm-dev
sudo apt-get install libao-dev libmpg123-dev
```
Instalati wayland-scanner (daca nu il aveti deja)
```
sudo apt install libwayland-bin libwayland-dev
```
Instalati GLFW
```
cd /usr/local/lib/
sudo git clone https://github.com/glfw/glfw.git
cd glfw
sudo cmake .
sudo make
sudo make install
```

Apoi, puteti sa rulati urmatoarele comenzii in directorul unde se afla repo-ul.
```
cd CRender/build
cmake ..
cmake --build .
```

## Arhitectura

### Diagrama Claselor
![Diagrama Claselor (Fara detalii)](https://github.com/AndreiCd27/CRender/blob/main/Gallery/AllClassesDiagram.png)
### Diagrama Claselor (Detaliat) - 1
![Diagrama Claselor (Top Level)](https://github.com/AndreiCd27/CRender/blob/main/Gallery/ClassDiagramTopLevel.png)
### Diagrama Claselor (Detaliat) - 2
![Diagrama Claselor (Low Level)](https://github.com/AndreiCd27/CRender/blob/main/Gallery/ClassDiagramLowLevel.png)
