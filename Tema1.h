#pragma once

#include "components/simple_scene.h"
#include "lab_m1/lab3/object2D.h"
#include "lab_m1/lab3/transform2D.h"
#include "components/text_renderer.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace m1
{
    class Tema1 : public gfxc::SimpleScene
    {
    public:
        Tema1();
        ~Tema1();
        void Init() override;

    private:
        // Functii cadru
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        // Input
        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;

        // Tipurile de bloc folosite in editor, paleta si caramizi
        enum BlockType
        {
            NONE = 0,
            RED_BLOCK,
            YELLOW_BLOCK,
            GREEN_BLOCK,
            GREY_BLOCK
        };

        // Culorile paletei copiate din editor
        std::vector<BlockType> paddleBlocks;

        // Render text
        gfxc::TextRenderer* textRenderer = nullptr;

        // Celula din grila editorului
        struct GridCell
        {
            BlockType type = NONE;
        };

        // Structura unei caramizi din joc
        struct Brick
        {
            float x, y;
            float w, h;
            bool alive;
            bool breaking;
            float breakScale;
            int hp;
            int maxHp;
            BlockType type;
        };

        // Fragmente vizuale generate cand o caramida se sparge
        struct Fragment
        {
            float x, y;
            float vx, vy;
            float size;
            float life;
            glm::vec3 color;
        };

        std::vector<Fragment> fragments;

        // Functii editor
        bool CheckConstraints();// verifica forma paletei
        void DrawOuterBorder();
        void DrawLeftPanel();
        void DrawLeftPanelDividers();
        void DrawContainer();
        void DrawGridBackground();
        void DrawGridForeground();
        void DrawGreenBar();
        void DrawStartButton();

        glm::vec2 ConvertPixelToLogical(int mouseX, int mouseY);
        BlockType GetBlockTypeFromPanelY(float y);
        std::string GetMeshNameFromBlockType(BlockType type);
        std::string GetMeshNameFromHp(int hp);
        bool IsInsideStartButton(float x, float y);

        // Functii gameplay breakout
        void InitBreakoutLevel(); // genereaza caramizile
        void StartGameFromEditor(); // porneste jocul cu paleta editorului
        void UpdateBreakout(float dt); // fizica + coliziuni
        void UpdateFragments(float dt);// efectele de spart
        void DrawBreakoutScene(); // desen gameplay
        void ResetBallAndPaddle();
        void LoseLife();

        void CreateCircle(const std::string& name, float radius, const glm::vec3& color);

        bool AllBricksDestroyed() const;
        bool CheckCollisionCircleAABB(glm::vec2 center, float radius,
            float rx, float ry, float rw, float rh);
        void CreateArcMesh(const std::string&, float, float, float, const glm::vec3&, bool);
        void CreateRectangleMesh(const std::string&, float, float, const glm::vec3&, bool);
        void HandleClick(int mouseX, int mouseY, int button);

        // Layout editor
        int GRID_W = 17;
        int GRID_H = 9;
        float CELL = 38.0f;
        float CELL_SPACING = 15.0f;

        bool isDragging = false;
        bool isStartButtonHovered = false;
        bool constraintsMet = false;

        BlockType draggedType = NONE;
        glm::vec2 draggedPosition = glm::vec2(0.0f);
        float currentDragSize = 80.0f;

        float PLACED_BLOCK_OFFSET = 7.5f;

        float PANEL_X = 20.0f;
        float PANEL_Y = 60.0f;
        float PANEL_W = 200.0f;
        float PANEL_H = 600.0f;

        float GRID_X = 260.0f;
        float GRID_Y = 100.0f;

        float TOPBAR_Y = 600.0f;
        float TOPBAR_X = 260.0f;
        float TOPBAR_CELL = 40.0f;
        float TOPBAR_SPACING = 50.0f;

        float START_SIZE = 55.0f;
        float START_X = 1200.0f;
        float START_Y = 605.0f;

        int remainingBlocks = 10;

        glm::mat3 modelMatrix;
        std::string draggedMeshName;

        std::vector<std::vector<GridCell>> shipGrid;

        // Starea gameplay-ului
        bool gameMode = false;
        std::vector<Brick> bricks;

        float paddleX = 0.0f;
        float paddleY = 0.0f;
        float paddleW = 0.0f;
        float paddleH = 20.0f;
        float paddleSpeed = 450.0f;

        float paddleStartX = 0.0f;
        float paddleStartY = 0.0f;
        float paddleStartW = 0.0f;

        glm::vec2 ballPos = glm::vec2(0.0f);
        glm::vec2 ballVel = glm::vec2(0.0f);

        float ballRadius = 14.0f;
        float ballSpeed = 420.0f;
        bool ballLaunched = false;

        int lives = 3;
        int score = 0;
    };
}
