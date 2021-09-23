#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_RICONS
#include "raygui.h"
#include "raymath.h"

#define DARK_RED     (Color){204, 36, 29, 255}
#define LIGHT_RED    (Color){251, 73, 52, 255}
#define DARK_GREEN   (Color){152, 151, 26, 255}
#define LIGHT_GREEN  (Color){184, 187, 38, 255}
#define DARK_YELLOW  (Color){215, 153, 33, 255}
#define LIGHT_YELLOW (Color){250, 189, 47, 255}
#define DARK_BLUE    (Color){69, 133, 136, 255}
#define LIGHT_BLUE   (Color){131, 165, 152, 255}
#define DARK_PURPLE  (Color){177, 98, 134, 255}
#define LIGHT_PURPLE (Color){211, 134, 155, 255}
#define DARK_CYAN    (Color){104, 157, 106, 255}
#define LIGHT_CYAN   (Color){142, 192, 124, 255}
#define DARK_ORANGE  (Color){214, 93, 14, 255}
#define LIGHT_ORANGE (Color){254, 128, 25, 255}
#define DARK_GRAY    (Color){146, 131, 116, 255}
#define LIGHT_GRAY   (Color){168, 153, 132, 255}
#define BG           (Color){40, 40, 40, 255}
#define FG           (Color){235, 219, 178, 255}
#define BG0          BG
#define BG0_H        (Color){29, 32, 33, 255}
#define BG0_S        (Color){50, 4, 47, 255}
#define BG1          (Color){60, 56, 54, 255}
#define BG2          (Color){80, 73, 69, 255}
#define BG3          (Color){102, 92, 84, 255}
#define BG4          (Color){124, 111, 100, 255}
#define FG0          (Color){251, 241, 199, 255}
#define FG1          FG
#define FG2          (Color){213, 196, 161, 255}
#define FG3          (Color){189, 174, 147, 255}
#define FG4          (Color){168, 153, 132, 255}


#define G 3000
#define PLAYER_JUMP_SPD 850.0f
#define PLAYER_HOR_SPD 400.0f

typedef struct Player {
    Vector2 position;
    float speed;
    bool singleJump, doubleJump;
    int jumpCounter;
    int lives;

} Player;

typedef struct EnvItem {
    Rectangle rect;
    int blocking;
    Color color;

} EnvItem;

static const int screenWidth = 1000;
static const int screenHeight = 800;
static const int maxJump = 2;

void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta);

void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------

    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "");
    int mw =  GetMonitorWidth(0),
        mh = GetMonitorHeight(0);
    HideCursor();

    GuiLoadStyle("gruvbox.rgs");


    Vector2 mousePosition = { 0 };
    Vector2 windowPosition = { mw/8, mh/16 };
    Vector2 panOffset = mousePosition;
    bool dragWindow = false;
    
    SetWindowPosition(windowPosition.x, windowPosition.y);
    
    bool exitWindow = false;

    Player player = { 0 };
    player.position = (Vector2){ 400, 280 };
    player.speed = 0;
    player.singleJump = false;
    EnvItem envItems[] = {
        {{ 0, 400, 1000, 200 }, 1, FG3 },
        {{ 300, 200, 400, 10 }, 1, FG3 },
        {{ 250, 300, 100, 10 }, 1, FG3 },
        {{ 650, 300, 100, 10 }, 1, FG3 }
    };

    int envItemsLength = sizeof(envItems)/sizeof(envItems[0]);

    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Store pointers to the multiple update camera functions
    void (*cameraUpdaters[])(Camera2D*, Player*, EnvItem*, int, float, int, int) = {
        UpdateCameraCenter,
        UpdateCameraCenterInsideMap,
        UpdateCameraCenterSmoothFollow,
        UpdateCameraEvenOutOnLanding,
        UpdateCameraPlayerBoundsPush
    };

    int cameraOption = 0;
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(cameraUpdaters[0]);

    char *cameraDescriptions[] = {
        "[ Follow player center ]",
        "[ Follow player center, but clamp to map edges ]",
        "[ Follow player center; smoothed ]",
        "[ Follow player center horizontally; updateplayer center vertically after landing ]",
        "[ Player push camera on getting too close to screen edge ]"
    };

    // Main game loop
    while (!WindowShouldClose() && !exitWindow){
        // Update
        //----------------------------------------------------------------------------------
        float deltaTime = GetFrameTime();
        mousePosition = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if (CheckCollisionPointRec(mousePosition, (Rectangle){ 0, 0, screenWidth, 20 })){
                dragWindow = true;
                panOffset = mousePosition;
            }
        }

        if (dragWindow){            
            windowPosition.x += (mousePosition.x - panOffset.x);
            windowPosition.y += (mousePosition.y - panOffset.y);
            
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) dragWindow = false;

            SetWindowPosition(windowPosition.x, windowPosition.y);
        }

        UpdatePlayer(&player, envItems, envItemsLength, deltaTime);

        camera.zoom += ((float)GetMouseWheelMove()*0.05f);

        if      (camera.zoom > 3.0f)  camera.zoom = 3.0f;
        else if (camera.zoom < 0.25f) camera.zoom = 0.25f;

        if (IsKeyPressed(KEY_V)){
            camera.zoom = 1.0f;
        }

        if (IsKeyPressed(KEY_C)) cameraOption = (cameraOption + 1)%cameraUpdatersLength;
        if (IsKeyPressed(KEY_R)){
          player.position = (Vector2){ 400, 280 };
        }
        if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))){
          ToggleFullscreen();
          /* if (IsWindowFullscreen()){} */
        }


        // Call update camera function by its pointer
        cameraUpdaters[cameraOption](&camera, &player, envItems, envItemsLength, deltaTime, screenWidth, screenHeight);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            /* ClearBackground(LIGHTGRAY); */
            exitWindow = GuiWindowBox((Rectangle){ 0, 0, screenWidth, screenHeight }, "#198# CHAOS FACTION CLONE");
            
            /* DrawText(TextFormat("Mouse Position: [ %.0f, %.0f ]", mousePosition.x, mousePosition.y), 10, 40, 10, LIGHTGRAY); */

            BeginMode2D(camera);

                for (int i = 0; i < envItemsLength; i++) DrawRectangleRec(envItems[i].rect, envItems[i].color);

                Rectangle playerRect = { player.position.x - 20, player.position.y - 40, 40, 40 };
                DrawRectangleRec(playerRect, DARK_BLUE);

            EndMode2D();

            DrawText(TextFormat("[ %.0f, %.0f ]", player.position.x, player.position.y), 20, 30, 20, FG);
            DrawText(cameraDescriptions[cameraOption], 20, 55, 10, FG);


        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta){
    if (IsKeyDown(KEY_LEFT)) player->position.x -= PLAYER_HOR_SPD*delta;
    if (IsKeyDown(KEY_RIGHT)) player->position.x += PLAYER_HOR_SPD*delta;
    if (IsKeyPressed(KEY_UP) && player->jumpCounter>0){
        player->speed = -PLAYER_JUMP_SPD;
        --player->jumpCounter;
    }

    int hitObstacle = 0;
    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        Vector2 *p = &(player->position);
        if (ei->blocking &&
            ei->rect.x <= p->x &&
            ei->rect.x + ei->rect.width >= p->x &&
            ei->rect.y >= p->y &&
            ei->rect.y < p->y + player->speed*delta)
        {
            hitObstacle = 1;
            player->speed = 0.0f;
            p->y = ei->rect.y;
        }
    }

    if (!hitObstacle)
    {
        player->position.y += player->speed*delta;
        player->speed += G*delta;
    }
    else {
      player->jumpCounter=maxJump;
    }

    if (player->position.y > screenHeight){
      player->position = (Vector2){ 400, 280 };
    }
}

void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height){
    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    camera->target = player->position;
}

void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height){
    camera->target = player->position;
    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000;

    for (int i = 0; i < envItemsLength; i++){
        EnvItem *ei = envItems + i;
        minX = fminf(ei->rect.x, minX);
        maxX = fmaxf(ei->rect.x + ei->rect.width, maxX);
        minY = fminf(ei->rect.y - height/2, minY); // mod
        maxY = fmaxf(ei->rect.y + ei->rect.height, maxY);
    }

    Vector2 max = GetWorldToScreen2D((Vector2){ maxX, maxY }, *camera);
    Vector2 min = GetWorldToScreen2D((Vector2){ minX, minY }, *camera);

    if (max.x < width) camera->offset.x = width - (max.x - width/2);
    if (max.y < height) camera->offset.y = height - (max.y - height/2);
    if (min.x > 0) camera->offset.x = width/2 - min.x;
    if (min.y > 0) camera->offset.y = height/2 - min.y;
}

void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height){
    static float minSpeed = 30;
    static float minEffectLength = 10;
    static float fractionSpeed = 0.8f;

    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    Vector2 diff = Vector2Subtract(player->position, camera->target);
    float length = Vector2Length(diff);

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed*length, minSpeed);
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed*delta/length));
    }
}

void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height){
    static float evenOutSpeed = 700;
    static int eveningOut = false;
    static float evenOutTarget;

    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    camera->target.x = player->position.x;

    if (eveningOut){
        if (evenOutTarget > camera->target.y){
            camera->target.y += evenOutSpeed*delta;

            if (camera->target.y > evenOutTarget){
                camera->target.y = evenOutTarget;
                eveningOut = 0;
            }
        }
        else{
            camera->target.y -= evenOutSpeed*delta;

            if (camera->target.y < evenOutTarget)
            {
                camera->target.y = evenOutTarget;
                eveningOut = 0;
            }
        }
    }
    else{
        if (player->singleJump && (player->speed == 0) && (player->position.y != camera->target.y)){
            eveningOut = 1;
            evenOutTarget = player->position.y;
        }
    }
}

void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height){
    static Vector2 bbox = { 0.2f, 0.2f };

    Vector2 bboxWorldMin = GetScreenToWorld2D((Vector2){ (1 - bbox.x)*0.5f*width, (1 - bbox.y)*0.5f*height }, *camera);
    Vector2 bboxWorldMax = GetScreenToWorld2D((Vector2){ (1 + bbox.x)*0.5f*width, (1 + bbox.y)*0.5f*height }, *camera);
    camera->offset = (Vector2){ (1 - bbox.x)*0.5f * width, (1 - bbox.y)*0.5f*height };

    if (player->position.x < bboxWorldMin.x) camera->target.x = player->position.x;
    if (player->position.y < bboxWorldMin.y) camera->target.y = player->position.y;
    if (player->position.x > bboxWorldMax.x) camera->target.x = bboxWorldMin.x + (player->position.x - bboxWorldMax.x);
    if (player->position.y > bboxWorldMax.y) camera->target.y = bboxWorldMin.y + (player->position.y - bboxWorldMax.y);
}
