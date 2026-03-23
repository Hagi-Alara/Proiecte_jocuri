#include "Tema1.h"
#include "lab_m1/lab3/transform2D.h"
#include "lab_m1/lab3/object2D.h"
#include "components/text_renderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <queue>
#include <time.h> 

using namespace std;
using namespace m1;

//    Mapeaza tipul de bloc (enum) la numele mesh-ului pentru editor/paleta
string Tema1::GetMeshNameFromBlockType(BlockType type)
{
    // Scop: Asigura ca blocurile din editor si paleta sunt desenate in culoarea corecta
    switch (type)
    {
    case RED_BLOCK: return "block_red";
    case YELLOW_BLOCK: return "block_yellow";
    case GREEN_BLOCK: return "block_green";
    case GREY_BLOCK: return "block_grey";
    default: return "block_grey"; // Fallback
    }
}

//    Determina mesh-ul caramizii in joc in functie de HP-ul ramas
string Tema1::GetMeshNameFromHp(int hp)
{
    // Scop: Realizeaza feedback vizual in joc (culoarea = rezistenta)
    if (hp == 3) return "block_red";
    if (hp == 2) return "block_yellow";
    if (hp == 1) return "block_green";
    return "block_grey"; // Caramida fara HP / fallback
}

// Constructor: initializeaza pointer-ul text renderer
Tema1::Tema1()
{
    textRenderer = nullptr;
}

// Destructor: elibereaza memoria alocata pentru text renderer
Tema1::~Tema1()
{
    if (textRenderer)
    {
        delete textRenderer;
        textRenderer = nullptr;
    }
}

// Setup initial scena (camera, mesh-uri, stare)
void Tema1::Init()
{
    // Setup camera 2D (Ortografica)
    auto res = window->GetResolution();
    auto camera = GetSceneCamera();
    camera->SetOrthographic(0, (float)res.x, 0, (float)res.y, 0.01f, 400.0f);
    camera->SetPosition(glm::vec3(0, 0, 50));
    camera->SetRotation(glm::vec3(0));
    camera->Update();
    GetCameraInput()->SetActive(false); // Blocam miscarea camerei
    glDisable(GL_DEPTH_TEST); // Randare 2D

    // Creare mesh-uri pentru blocurile colorate (editor/joc)
    AddMeshToList(object2D::CreateSquare("block_red", glm::vec3(0), 1.0f, glm::vec3(0.9f, 0.2f, 0.2f), true));
    AddMeshToList(object2D::CreateSquare("block_yellow", glm::vec3(0), 1.0f, glm::vec3(0.9f, 0.9f, 0.2f), true));
    AddMeshToList(object2D::CreateSquare("block_green", glm::vec3(0), 1.0f, glm::vec3(0.2f, 0.8f, 0.2f), true));
    AddMeshToList(object2D::CreateSquare("block_grey", glm::vec3(0), 1.0f, glm::vec3(0.6f, 0.6f, 0.6f), true));

    // Incarcare mesh-uri UI (borduri, grid, indicatori)
    AddMeshToList(object2D::CreateSquare("grid_cell_blue", glm::vec3(0), CELL, glm::vec3(0.15f, 0.25f, 0.7f), true));
    AddMeshToList(object2D::CreateSquare("border_red", glm::vec3(0), 1.0f, glm::vec3(1, 0, 0), true));
    AddMeshToList(object2D::CreateSquare("border_blue", glm::vec3(0), 1.0f, glm::vec3(0.1f, 0.1f, 0.4f), true));
    AddMeshToList(object2D::CreateSquare("green_cell", glm::vec3(0), TOPBAR_CELL, glm::vec3(0, 1, 0), true));
    AddMeshToList(object2D::CreateSquare("green_active", glm::vec3(0), TOPBAR_CELL + 20, glm::vec3(0.2f, 0.8f, 0.2f), true));

    // Mesh-uri joc (bila, paleta, sageti)
    CreateCircle("ball", 1.0f, glm::vec3(1, 1, 1));
    vector<VertexFormat> v1 = { VertexFormat(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)), VertexFormat(glm::vec3(START_SIZE, START_SIZE / 2, 0), glm::vec3(0, 1, 0)), VertexFormat(glm::vec3(0, START_SIZE, 0), glm::vec3(0, 1, 0)) };
    vector<unsigned int> ind = { 0u, 1u, 2u };
    Mesh* a = new Mesh("arrow_start"); a->InitFromData(v1, ind); AddMeshToList(a);
    vector<VertexFormat> v2 = { VertexFormat(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0)), VertexFormat(glm::vec3(START_SIZE, START_SIZE / 2, 0), glm::vec3(1, 0, 0)), VertexFormat(glm::vec3(0, START_SIZE, 0), glm::vec3(1, 0, 0)) };
    Mesh* b = new Mesh("arrow_stop"); b->InitFromData(v2, ind); AddMeshToList(b);

    // Initializare stare joc
    shipGrid.assign((size_t)GRID_H, vector<GridCell>((size_t)GRID_W));
    paddleH = 20.0f; paddleSpeed = 450.0f; ballRadius = 14.0f; ballSpeed = 420.0f;
    lives = 3; score = 0; gameMode = false; ballLaunched = false;

    // Initializare Text renderer
    auto res2 = window->GetResolution();
    string exePath = window->props.selfDir;
    textRenderer = new gfxc::TextRenderer(exePath, res2.x, res2.y);
    textRenderer->Load("assets/fonts/Hack-Bold.ttf", 32);
}

//  : Inceputul cadrului (curatare buffer)
void Tema1::FrameStart()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Setam culoarea de fundal (negru)
    glClear(GL_COLOR_BUFFER_BIT); // Curatam ecranul
    auto res = window->GetResolution();
    glViewport(0, 0, res.x, res.y);
}

//  : Loop principal (Editor vs Joc)
void Tema1::Update(float dt)
{
    if (gameMode) // MOD JOC
    {
        UpdateBreakout(dt); // Fizica, coliziuni
        UpdateFragments(dt); // Animatie particule
        DrawBreakoutScene(); // Randare joc
        DrawOuterBorder();
        return;
    }

    // MOD EDITOR
    constraintsMet = CheckConstraints(); // Verifica validitatea paletei (Rosu/Verde)

    DrawLeftPanel(); // Panou selectie blocuri
    DrawLeftPanelDividers();
    DrawContainer(); // Chenar grila
    DrawGreenBar(); // Indicator blocuri ramase
    DrawStartButton(); // Buton Start (Rosu/Verde)
    DrawGridBackground(); // Fundal grila
    DrawGridForeground(); // Blocurile plasate

    if (isDragging) // Deseneaza blocul tras (D&D vizual)
    {
        modelMatrix = glm::mat3(1.0f);
        modelMatrix *= transform2D::Translate(draggedPosition.x - currentDragSize / 2.0f, draggedPosition.y - currentDragSize / 2.0f);
        modelMatrix *= transform2D::Scale(currentDragSize, currentDragSize);
        RenderMesh2D(meshes[draggedMeshName], shaders["VertexColor"], modelMatrix);
    }

    DrawOuterBorder(); // Rama rosie exterioara
}

void Tema1::FrameEnd() {} // Functie lasata nefolosita

//    Verifica constrangerile paletei (linie unica si continua, 1-10 blocuri)
bool Tema1::CheckConstraints()
{
    vector<pair<int, int>> blocks;
    // Colectam blocurile plasate
    for (int r = 0; r < GRID_H; r++)
        for (int c = 0; c < GRID_W; c++)
            if (shipGrid[(size_t)r][(size_t)c].type != NONE)
                blocks.emplace_back(r, c);

    int total = (int)blocks.size();
    if (total == 0 || total > 10) return false; // Conditie 1: Numar blocuri (1-10)

    // Conditie 2: Toate blocurile trebuie sa fie pe acelasi rand
    int row = blocks[0].first;
    for (const auto& p : blocks)
        if (p.first != row)
            return false;

    // Conditie 3: Continuitate (fara spatii)
    int minC = blocks[0].second;
    int maxC = blocks[0].second;

    for (const auto& p : blocks) { if (p.second < minC) minC = p.second; if (p.second > maxC) maxC = p.second; }

    return total == maxC - minC + 1; // Verifica daca totalul acopera span-ul (fara gauri)
}

//    Verifica daca mouse-ul este in zona butonului START
bool Tema1::IsInsideStartButton(float x, float y)
{
    // Returneaza true daca coordonatele sunt in limitele butonului
    return x >= START_X && x <= START_X + START_SIZE &&
        y >= START_Y && y <= START_Y + START_SIZE;
}

//    Deseneaza rama rosie a ferestrei
void Tema1::DrawOuterBorder()
{
    auto r = window->GetResolution();
    float W = (float)r.x, H = (float)r.y, t = 3.0f; // W, H = rezolutie, t = grosime bordura

    // Desenarea celor 4 linii de bordura
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(0, H - t); modelMatrix *= transform2D::Scale(W, t); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Scale(W, t); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Scale(t, H); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(W - t, 0); modelMatrix *= transform2D::Scale(t, H); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
}

//    Deseneaza panoul de selectie si cele 4 blocuri sursa
void Tema1::DrawLeftPanel()
{
    float t = 3.0f;

    // Desenare chenar exterior
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(PANEL_X, PANEL_Y + PANEL_H - t); modelMatrix *= transform2D::Scale(PANEL_W, t); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(PANEL_X, PANEL_Y); modelMatrix *= transform2D::Scale(PANEL_W, t); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(PANEL_X, PANEL_Y); modelMatrix *= transform2D::Scale(t, PANEL_H); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(PANEL_X + PANEL_W - t, PANEL_Y); modelMatrix *= transform2D::Scale(t, PANEL_H); RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);

    // Desenare blocuri sursa (cu culorile lor)
    float section = PANEL_H / 4.0f;
    float sq = 80.0f;
    string mesh[4] = { "block_grey", "block_green", "block_yellow", "block_red" };

    for (int i = 0; i < 4; i++)
    {
        float cx = PANEL_X + PANEL_W / 2.0f;
        float cy = PANEL_Y + section * (i + 0.5f);

        modelMatrix = glm::mat3(1.0f);
        modelMatrix *= transform2D::Translate(cx - sq / 2.0f, cy - sq / 2.0f);
        modelMatrix *= transform2D::Scale(sq, sq);

        RenderMesh2D(meshes[mesh[i]], shaders["VertexColor"], modelMatrix);
    }
}

// Deseneaza liniile separatoare din panoul lateral
void Tema1::DrawLeftPanelDividers()
{
    float section = PANEL_H / 4.0f;
    float t = 2.0f;

    for (int i = 1; i <= 3; i++)
    {
        float y = PANEL_Y + section * i;

        modelMatrix = glm::mat3(1.0f);
        modelMatrix *= transform2D::Translate(PANEL_X, y - t / 2.0f);
        modelMatrix *= transform2D::Scale(PANEL_W, t);

        RenderMesh2D(meshes["border_red"], shaders["VertexColor"], modelMatrix);
    }
}

// Deseneaza chenarul albastru al zonei de proiectare (grila)
void Tema1::DrawContainer()
{
    float gw = GRID_W * (CELL + CELL_SPACING) - CELL_SPACING; // latimea totala a celulelor
    float gh = GRID_H * (CELL + CELL_SPACING) - CELL_SPACING; // inaltimea totala a celulelor

    float x = GRID_X - CELL_SPACING + 1.0f;
    float y = GRID_Y - CELL_SPACING + 1.0f;

    float w = gw + 2.0f * CELL_SPACING - 2.0f;
    float h = gh + 2.0f * CELL_SPACING - 2.0f;

    float t = 3.0f;

    // ... (Logica de desenare a celor 4 linii de chenar albastru) ...
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(x, y + h - t); modelMatrix *= transform2D::Scale(w, t); RenderMesh2D(meshes["border_blue"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(x, y); modelMatrix *= transform2D::Scale(w, t); RenderMesh2D(meshes["border_blue"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(x, y); modelMatrix *= transform2D::Scale(t, h); RenderMesh2D(meshes["border_blue"], shaders["VertexColor"], modelMatrix);
    modelMatrix = glm::mat3(1.0f); modelMatrix *= transform2D::Translate(x + w - t, y); modelMatrix *= transform2D::Scale(t, h); RenderMesh2D(meshes["border_blue"], shaders["VertexColor"], modelMatrix);
}

// Deseneaza fundalul celulelor grilei (patratele albastre)
void Tema1::DrawGridBackground()
{
    for (int r = 0; r < GRID_H; r++)
        for (int c = 0; c < GRID_W; c++)
        {
            float x = GRID_X + c * (CELL + CELL_SPACING);
            float y = GRID_Y + r * (CELL + CELL_SPACING);

            modelMatrix = glm::mat3(1.0f);
            modelMatrix *= transform2D::Translate(x, y);

            RenderMesh2D(meshes["grid_cell_blue"], shaders["VertexColor"], modelMatrix);
        }
}

// Deseneaza blocurile plasate de utilizator (prim-plan)
void Tema1::DrawGridForeground()
{
    for (int r = 0; r < GRID_H; r++)
        for (int c = 0; c < GRID_W; c++)
        {
            if (shipGrid[(size_t)r][(size_t)c].type != NONE) // Daca celula are un bloc
            {
                float x = GRID_X + c * (CELL + CELL_SPACING);
                float y = GRID_Y + r * (CELL + CELL_SPACING);

                float s = CELL + PLACED_BLOCK_OFFSET * 2.0f; // Marimea cu offset-ul vizual

                modelMatrix = glm::mat3(1.0f);
                modelMatrix *= transform2D::Translate(x - PLACED_BLOCK_OFFSET, y - PLACED_BLOCK_OFFSET);
                modelMatrix *= transform2D::Scale(s, s);

                // Deseneaza blocul in culoarea sa
                RenderMesh2D(meshes[GetMeshNameFromBlockType(shipGrid[(size_t)r][(size_t)c].type)], shaders["VertexColor"], modelMatrix);
            }
        }
}

// Deseneaza indicatorul verde (blocuri ramase)
void Tema1::DrawGreenBar()
{
    for (int i = 0; i < remainingBlocks; i++) // Deseneaza cate un patrat pentru fiecare bloc ramas
    {
        float x = TOPBAR_X + i * (TOPBAR_CELL + TOPBAR_SPACING);
        float y = TOPBAR_Y;

        modelMatrix = glm::mat3(1.0f);
        modelMatrix *= transform2D::Translate(x, y);
        RenderMesh2D(meshes["green_cell"], shaders["VertexColor"], modelMatrix);
    }
}

// Deseneaza sageata Start/Stop (Verde/Rosu)
void Tema1::DrawStartButton()
{
    const char* mesh = constraintsMet ? "arrow_start" : "arrow_stop"; // Alege culoarea in functie de validitate

    modelMatrix = glm::mat3(1.0f);
    modelMatrix *= transform2D::Translate(START_X, START_Y);
    RenderMesh2D(meshes[mesh], shaders["VertexColor"], modelMatrix);
}

// Converte?te coordonatele mouse-ului (pixeli) la sistemul de coordonate al jocului (Y inversat)
glm::vec2 Tema1::ConvertPixelToLogical(int mx, int my)
{
    auto r = window->GetResolution();
    return glm::vec2((float)mx, (float)(r.y - my));
}

// Identifica tipul de bloc apasat in panoul lateral
Tema1::BlockType Tema1::GetBlockTypeFromPanelY(float y)
{
    float h = PANEL_H / 4.0f;
    if (y < PANEL_Y || y > PANEL_Y + PANEL_H) return NONE;

    int s = (int)((y - PANEL_Y) / h); // Afla in ce sectiune (0-3) s-a dat click

    if (s == 0) return GREY_BLOCK; // Jos
    if (s == 1) return GREEN_BLOCK;
    if (s == 2) return YELLOW_BLOCK;
    if (s == 3) return RED_BLOCK; // Sus

    return NONE;
}

// ----------------- Input -----------------

// Gestioneaza click-urile de mouse (pornire D&D, pornire joc, stergere)
void Tema1::OnMouseBtnPress(int mx, int my, int btn, int mods)
{
    glm::vec2 p = ConvertPixelToLogical(mx, my);

    if (!gameMode && btn == GLFW_MOUSE_BUTTON_2) // Click Dreapta (folosit aici pentru D&D sau Start)
    {
        if (IsInsideStartButton(p.x, p.y)) // Click pe butonul Start
        {
            if (constraintsMet) StartGameFromEditor(); // Porneste jocul doar daca e valid
            return;
        }
    }

    if (gameMode) return;

    if (btn == GLFW_MOUSE_BUTTON_2) // Incepe D&D
    {
        if (p.x >= PANEL_X && p.x <= PANEL_X + PANEL_W && p.y >= PANEL_Y && p.y <= PANEL_Y + PANEL_H)
        {
            draggedType = GetBlockTypeFromPanelY(p.y);
            if (draggedType != NONE)
            {
                isDragging = true;
                draggedPosition = p;
                currentDragSize = 80.0f;
                draggedMeshName = GetMeshNameFromBlockType(draggedType); // Salveaza culoarea pentru desen
            }
        }
        return;
    }

    if (btn == GLFW_MOUSE_BUTTON_3) // Click Mijlociu (Stergere)
    {
        float gw = GRID_W * (CELL + CELL_SPACING) - CELL_SPACING;
        float gh = GRID_H * (CELL + CELL_SPACING) - CELL_SPACING;

        if (p.x >= GRID_X && p.x <= GRID_X + gw && p.y >= GRID_Y && p.y <= GRID_Y + gh)
        {
            // Calculeaza celula exacta pe grila
            int col = (int)((p.x - GRID_X) / (CELL + CELL_SPACING));
            int row = (int)((p.y - GRID_Y) / (CELL + CELL_SPACING));

            if (row >= 0 && row < GRID_H && col >= 0 && col < GRID_W)
            {
                auto& cell = shipGrid[row][col];
                if (cell.type != NONE)
                {
                    cell.type = NONE; // Sterge blocul
                    remainingBlocks++;
                    constraintsMet = CheckConstraints(); // Revalideaza paleta
                }
            }
        }
    }
}

void Tema1::OnMouseMove(int mx, int my, int dx, int dy)
{
    if (gameMode) return;

    if (isDragging) // Actualizeaza pozitia blocului tras
        draggedPosition = ConvertPixelToLogical(mx, my);

    glm::vec2 p = ConvertPixelToLogical(mx, my);

    // Verifica hover pe butonul Start
    isStartButtonHovered = (p.x >= START_X && p.x <= START_X + START_SIZE && p.y >= START_Y && p.y <= START_Y + START_SIZE);
}

void Tema1::OnMouseBtnRelease(int mx, int my, int btn, int mods)
{
    if (gameMode) return;

    if (btn == GLFW_MOUSE_BUTTON_2 && isDragging) // Finalizare Drag and Drop
    {
        isDragging = false;
        glm::vec2 p = ConvertPixelToLogical(mx, my);

        float gw = GRID_W * (CELL + CELL_SPACING) - CELL_SPACING;
        float gh = GRID_H * (CELL + CELL_SPACING) - CELL_SPACING;

        if (p.x >= GRID_X && p.x <= GRID_X + gw && p.y >= GRID_Y && p.y <= GRID_Y + gh)
        {
            int col = (int)((p.x - GRID_X) / (CELL + CELL_SPACING));
            int row = (int)((p.y - GRID_Y) / (CELL + CELL_SPACING));

            if (row >= 0 && row < GRID_H && col >= 0 && col < GRID_W)
            {
                GridCell& cell = shipGrid[(size_t)row][(size_t)col];
                if (remainingBlocks > 0 && cell.type == NONE)
                {
                    // Plaseaza blocul si scade contorul
                    cell.type = draggedType;
                    remainingBlocks--;
                }
            }
        }

        constraintsMet = CheckConstraints(); // Verifica noile constrangeri
        draggedType = NONE;
    }
}

void Tema1::OnInputUpdate(float dt, int mods)
{
    if (!gameMode) return;
    // Logica miscarii paletei in modul joc (tineti apasat LEFT/RIGHT)
    float move = paddleSpeed * dt;

    if (window->KeyHold(GLFW_KEY_LEFT)) paddleX -= move;
    if (window->KeyHold(GLFW_KEY_RIGHT)) paddleX += move;

    // Limitare paleta la marginea ecranului
    auto res = window->GetResolution();
    float W = (float)res.x;
    float half = paddleW / 2.0f;

    if (paddleX - half < 0.0f) paddleX = half;
    if (paddleX + half > W) paddleX = W - half;
}

void Tema1::OnKeyPress(int key, int mods)
{
    if (!gameMode) return;

    // Lansarea bilei la apasarea Space
    if (key == GLFW_KEY_SPACE && !ballLaunched)
    {
        const float PI = 3.1415926f;
        float angle = 45.0f * PI / 180.0f; // Unghi initial

        ballLaunched = true;
        ballVel.x = ballSpeed * cosf(angle); // Calculeaza componentele vitezei
        ballVel.y = ballSpeed * sinf(angle);
    }
}

void Tema1::OnKeyRelease(int key, int mods) {}
void Tema1::OnMouseScroll(int x, int y, int ox, int oy) {}
void Tema1::OnWindowResize(int a, int b) {}

void Tema1::CreateArcMesh(const string&, float, float, float, const glm::vec3&, bool) {}
void Tema1::CreateRectangleMesh(const string&, float, float, const glm::vec3&, bool) {}
void Tema1::HandleClick(int, int, int) {}

// Doar Breakout elements below

//    Genereaza layout-ul nivelului Breakout (caramizi, culori, HP, pozitii)
// Scop: Creeaza o matrice de caramizi cu latime adaptata la ecran si culori semi-aleatoare
void Tema1::InitBreakoutLevel()
{
    bricks.clear(); // Curata nivelul anterior (daca exista)

    auto res = window->GetResolution();
    float W = (float)res.x;
    float H = (float)res.y;

    int rows = 5;   // Numar de randuri de caramizi
    int cols = 14;  // Numar caramizi pe rand

    float spacing = 2.0f;  // Spatiul dintre caramizi
    float brickH = 45.0f;  // Inaltime fixa caramida
    float topOffset = H - 120.0f; // Pozitia Y de start (de sus)

    // Calculeaza automat latimea caramizii pentru tot ecranul
    float brickW = (W - (cols - 1) * spacing) / cols;

    srand((unsigned)time(NULL)); // Porneste RNG

    // Lista de culori posibile
    BlockType palette[] = { RED_BLOCK, YELLOW_BLOCK, GREEN_BLOCK, GREY_BLOCK };

    // Culoare dominanta pe rand (pentru aspect frumos ordonat)
    BlockType dominantColor[5] =
    {
        RED_BLOCK, YELLOW_BLOCK, GREEN_BLOCK, GREY_BLOCK, GREEN_BLOCK
    };

    // Alege o culoare aleatoare din paleta
    auto randomColor = [&]() -> BlockType {
        return palette[rand() % 4];
        };

    // Atribuie HP pe baza culorii
    auto assignHp = [&](BlockType t) -> int {
        switch (t)
        {
        case RED_BLOCK:    return 3;
        case YELLOW_BLOCK: return 2;
        case GREEN_BLOCK:  return 1;
        case GREY_BLOCK:   return 1;
        }
        return 1;
        };

    // Genereaza toate caramizile rand cu rand
    for (int r = 0; r < rows; r++)
    {
        BlockType base = dominantColor[r]; // Culoare principala pe rand

        for (int c = 0; c < cols; c++)
        {
            Brick b;

            // Pozitie si dimensiuni
            b.w = brickW;
            b.h = brickH;
            b.x = c * (brickW + spacing);
            b.y = topOffset - r * (brickH + spacing);

            // Probabilitati pentru culori variate
            int roll = rand() % 100;

            if (roll < 70)
                b.type = base;          // 70% culoarea dominanta
            else if (roll < 90)
                b.type = randomColor(); // 20% aleator
            else
                b.type = GREY_BLOCK;    // 10% gri special

            // HP pe baza culorii
            b.hp = assignHp(b.type);
            b.maxHp = b.hp;

            // Stare initiala
            b.alive = true;
            b.breaking = false;
            b.breakScale = 1.0f;

            bricks.push_back(b);
        }
    }
}



//    Porneste jocul folosind forma paletei proiectate in editor
// Scop: Construieste paleta, seteaza pozitia, scorul, vietile si initializeaza nivelul
void Tema1::StartGameFromEditor()
{
    vector<int> cols;

    // Determina coloanele ocupate de blocuri
    for (int r = 0; r < GRID_H; r++)
        for (int c = 0; c < GRID_W; c++)
            if (shipGrid[(size_t)r][(size_t)c].type != NONE)
                cols.push_back(c);

    if (cols.empty()) return;

    // Gaseste extremele paletei (stanga/dreapta)
    int minC = cols[0];
    int maxC = cols[0];
    for (int v : cols)
    {
        if (v < minC) minC = v;
        if (v > maxC) maxC = v;
    }

    // Latimea paletei proportionala cu numarul de celule ocupate
    int lenCells = maxC - minC + 1;
    paddleW = lenCells * 60.0f;

    if (paddleW <= 0.0f)
        paddleW = (float)cols.size() * 60.0f;

    // Pozitionare paleta
    auto res = window->GetResolution();
    float W = (float)res.x;

    paddleX = W / 2.0f;
    paddleY = 80.0f;

    // Salvam valorile de reset (folosite la pierderea vietii)
    paddleStartX = paddleX;
    paddleStartY = paddleY;
    paddleStartW = paddleW;

    // Resetare scor/vieti
    lives = 3;
    score = 0;
    ballLaunched = false;

    // Copiem culorile paletei proiectate
    paddleBlocks.clear();

    for (int c = minC; c <= maxC; c++)
    {
        for (int r = 0; r < GRID_H; r++)
        {
            if (shipGrid[(size_t)r][(size_t)c].type != NONE)
            {
                paddleBlocks.push_back(shipGrid[(size_t)r][(size_t)c].type);
                break;
            }
        }
    }

    InitBreakoutLevel(); // Creeaza caramizile
    ResetBallAndPaddle(); // Pozitioneaza mingea

    gameMode = true;  // Intra in modul joc
}

//    Reseteaza paleta si mingea dupa pierderea unei vieti
// Scop: Reaseaza obiectele la pozitia initiala (fara a rupe paleta)
void Tema1::ResetBallAndPaddle()
{
    paddleX = paddleStartX;
    paddleY = paddleStartY;
    paddleW = paddleStartW;

    ballLaunched = false;

    // Minge asezata deasupra paletei
    ballPos.x = paddleX;
    ballPos.y = paddleY + paddleH / 2.0f + ballRadius + 1.0f;

    ballVel = glm::vec2(0.0f);
}


//    Scade o viata si verifica Game Over
// Scop: Controleaza reintrarea in editor sau resetarea partiala a jocului
void Tema1::LoseLife()
{
    lives--;

    if (lives <= 0)
    {
        gameMode = false;  // Revenim in editor
        lives = 3;
        score = 0;
        ballLaunched = false;
        ballPos = glm::vec2(0.0f);
    }
    else
    {
        ResetBallAndPaddle();
    }
}

//    Verifica daca toate caramizile au fost eliminate
// Scop: Declanseaza victoria
bool Tema1::AllBricksDestroyed() const
{
    for (const auto& b : bricks)
        if (b.alive || b.breaking)
            return false;
    return true;
}

//    Detecteaza coliziunea dintre cerc (minge) si dreptunghi (AABB)
// Scop: Detectie precisa folosita pentru caramizi, paleta si margini
bool Tema1::CheckCollisionCircleAABB(glm::vec2 center, float radius,
    float rx, float ry, float rw, float rh)
{
    float closestX = std::max(rx, std::min(center.x, rx + rw));
    float closestY = std::max(ry, std::min(center.y, ry + rh));

    float dx = center.x - closestX;
    float dy = center.y - closestY;

    return dx * dx + dy * dy <= radius * radius;
}


//    Fizica jocului Breakout (misca mingea, verifica coliziuni, sparge caramizi)
// Scop: Logica centrala a gameplay-ului
void Tema1::UpdateBreakout(float dt)
{
    auto res = window->GetResolution();
    float W = (float)res.x;
    float H = (float)res.y;

    // Misca mingea
    if (ballLaunched)
    {
        ballPos += ballVel * dt;

        // Reflexii margini
        if (ballPos.x - ballRadius < 0) { ballPos.x = ballRadius; ballVel.x *= -1; }
        if (ballPos.x + ballRadius > W) { ballPos.x = W - ballRadius; ballVel.x *= -1; }
        if (ballPos.y + ballRadius > H) { ballPos.y = H - ballRadius; ballVel.y *= -1; }

        if (ballPos.y - ballRadius < 0)  // Minge pierduta
        {
            LoseLife();
            return;
        }
    }
    else
    {
        // Minge asezata pe paleta inainte de lansare
        ballPos.x = paddleX;
        ballPos.y = paddleY + paddleH / 2.0f + ballRadius + 1.0f;
    }

    // Coliziune cu caramizi
    if (ballLaunched)
    {
        for (auto& b : bricks)
        {
            if (!b.alive) continue;
            if (b.breaking) continue;

            if (CheckCollisionCircleAABB(ballPos, ballRadius, b.x, b.y, b.w, b.h))
            {
                // Determinam partea lovita
                float cx = b.x + b.w / 2.0f;
                float cy = b.y + b.h / 2.0f;

                float dx = ballPos.x - cx;
                float dy = ballPos.y - cy;

                // Scadem HP
                if (b.hp > 0)
                {
                    b.hp--;
                    score++;

                    if (b.hp <= 0)
                    {
                        b.alive = false;
                        b.breaking = true;
                        b.breakScale = 1.0f;
                    }
                }

                // Genereaza particule
                for (int i = 0; i < 4; i++)
                {
                    Fragment f;
                    f.x = b.x + b.w / 2.0f;
                    f.y = b.y + b.h / 2.0f;

                    float speed = 120.0f;
                    float angle = i * 3.14159f / 2.0f;

                    f.vx = cos(angle) * speed;
                    f.vy = sin(angle) * speed;

                    f.size = b.w * 0.7f;
                    f.life = 0.35f;

                    // Culoare particule
                    if (b.type == RED_BLOCK)    f.color = glm::vec3(0.9f, 0.2f, 0.2f);
                    if (b.type == YELLOW_BLOCK) f.color = glm::vec3(0.9f, 0.9f, 0.2f);
                    if (b.type == GREEN_BLOCK)  f.color = glm::vec3(0.2f, 0.8f, 0.2f);
                    if (b.type == GREY_BLOCK)   f.color = glm::vec3(0.6f, 0.6f, 0.6f);

                    fragments.push_back(f);
                }

                // Reflexie
                if (fabsf(dx) > fabsf(dy))
                    ballVel.x *= -1;
                else
                    ballVel.y *= -1;

                break;
            }
        }

        // Coliziune cu paleta
        float prx = paddleX - paddleW / 2.0f;
        float pry = paddleY - paddleH / 2.0f;

        if (CheckCollisionCircleAABB(ballPos, ballRadius,
            prx, pry, paddleW, paddleH)
            && ballVel.y < 0)
        {
            float rel = (ballPos.x - paddleX) / (paddleW / 2.0f);
            rel = std::max(-1.0f, std::min(1.0f, rel));

            const float PI = 3.1415926f;
            float maxAngle = 60.0f * PI / 180.0f;

            float angle = rel * maxAngle;

            ballVel.x = ballSpeed * sinf(angle);
            ballVel.y = ballSpeed * cosf(angle);
        }

        // Toate caramizile distruse
        if (AllBricksDestroyed())
        {
            cout << "You win! Score: " << score << "\n";
            gameMode = false;
        }
    }

    // Animatie spargere caramizi
    for (auto& b : bricks)
    {
        if (b.breaking)
        {
            b.breakScale -= dt * 7.0f;

            if (b.breakScale <= 0.0f)
            {
                b.breakScale = 0.0f;
                b.breaking = false;
            }
        }
    }
}


//    Actualizeaza fragmentele de particule (spargere)
// Scop: Miscare, scaderea dimensiunii si stergerea fragmentelor expirate
void Tema1::UpdateFragments(float dt)
{
    for (auto& f : fragments)
    {
        f.x += f.vx * dt;
        f.y += f.vy * dt;
        f.life -= dt;
        f.size *= 0.92f;
    }

    // Sterge particulele al caror timp a expirat
    fragments.erase(
        std::remove_if(fragments.begin(), fragments.end(),
            [](const Fragment& f) { return f.life <= 0; }),
        fragments.end()
    );
}



//    Creeaza mesh pentru un cerc (mingea)
// Scop: Genereaza vertecsi + indici pentru randarea bilei 2D
void Tema1::CreateCircle(const std::string& name, float radius, const glm::vec3& color)
{
    std::vector<VertexFormat> vertices;
    std::vector<unsigned int> indices;

    int segments = 64;
    float step = 2 * (float)M_PI / segments;

    vertices.push_back(VertexFormat(glm::vec3(0, 0, 0), color)); // Centru cerc

    for (int i = 0; i <= segments; i++)
    {
        float angle = i * step;
        float x = radius * cosf(angle);
        float y = radius * sinf(angle);

        vertices.push_back(VertexFormat(glm::vec3(x, y, 0), color));
        indices.push_back(i + 1);
    }

    Mesh* circle = new Mesh(name);
    circle->SetDrawMode(GL_TRIANGLE_FAN);
    circle->InitFromData(vertices, indices);
    AddMeshToList(circle);
}


//    Deseneaza tot ce tine de modul Breakout (caramizi, paleta, minge, fragmente, text)
// Scop: Este functia principala de randare din modul joc
void Tema1::DrawBreakoutScene()
{
    auto res = window->GetResolution();
    float H = (float)res.y;
    float W = (float)res.x;

    // Caramizile
    for (const auto& b : bricks)
    {
        // Animatie spargere
        if (b.breaking && b.breakScale > 0.0f)
        {
            float drawW = b.w * b.breakScale;
            float drawH = b.h * b.breakScale;

            float drawX = b.x + (b.w - drawW) / 2.0f;
            float drawY = b.y + (b.h - drawH) / 2.0f;

            modelMatrix = glm::mat3(1.0f);
            modelMatrix *= transform2D::Translate(drawX, drawY);
            modelMatrix *= transform2D::Scale(drawW, drawH);

            RenderMesh2D(meshes["block_grey"], shaders["VertexColor"], modelMatrix);
            continue;
        }

        if (!b.alive) continue;

        std::string meshName = GetMeshNameFromHp(b.hp);

        modelMatrix = glm::mat3(1.0f);
        modelMatrix *= transform2D::Translate(b.x, b.y);
        modelMatrix *= transform2D::Scale(b.w, b.h);

        RenderMesh2D(meshes[meshName], shaders["VertexColor"], modelMatrix);
    }

    // Deseneaza paleta colorata
    if (!paddleBlocks.empty())
    {
        float segmentW = paddleW / paddleBlocks.size();

        for (int i = 0; i < (int)paddleBlocks.size(); i++)
        {
            float x = (paddleX - paddleW / 2.0f) + i * segmentW;

            modelMatrix = glm::mat3(1.0f);
            modelMatrix *= transform2D::Translate(x, paddleY - paddleH / 2.0f);
            modelMatrix *= transform2D::Scale(segmentW, paddleH);

            RenderMesh2D(
                meshes[GetMeshNameFromBlockType(paddleBlocks[i])],
                shaders["VertexColor"],
                modelMatrix
            );
        }
    }
    else
    {
        // fallback (paleta gri)
        modelMatrix = glm::mat3(1.0f);
        modelMatrix *= transform2D::Translate(paddleX - paddleW / 2.0f,
            paddleY - paddleH / 2.0f);
        modelMatrix *= transform2D::Scale(paddleW, paddleH);
        RenderMesh2D(meshes["block_grey"], shaders["VertexColor"], modelMatrix);
    }

    // Minge
    modelMatrix = glm::mat3(1.0f);
    modelMatrix *= transform2D::Translate(ballPos.x, ballPos.y);
    modelMatrix *= transform2D::Scale(ballRadius, ballRadius);
    RenderMesh2D(meshes["ball"], shaders["VertexColor"], modelMatrix);

    // Fragmente de spargere
    for (auto& f : fragments)
    {
        glm::mat3 m = glm::mat3(1.0f);
        m *= transform2D::Translate(f.x - f.size / 2, f.y - f.size / 2);
        m *= transform2D::Scale(f.size, f.size);

        Mesh* sq = object2D::CreateSquare(
            "fragment_temp",
            glm::vec3(0, 0, 0),
            1.0f,
            f.color,
            true
        );

        RenderMesh2D(sq, shaders["VertexColor"], m);
    }

    // Text score + lives
    if (textRenderer)
    {
        textRenderer->RenderText(
            "Score: " + std::to_string(score),
            40.0f,
            H - 700.0f,
            1.0f,
            glm::vec3(1, 1, 1)
        );

        textRenderer->RenderText(
            "Lives: " + std::to_string(lives),
            W - 200.0f,
            H - 700.0f,
            1.0f,
            glm::vec3(1, 1, 1)
        );
    }
}
