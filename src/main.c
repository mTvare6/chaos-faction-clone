#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_RICONS
#include "raygui.h"
#include "raymath.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#define DARK_RED     CLITERAL(Color){204, 36, 29, 255}
#define LIGHT_RED    CLITERAL(Color){251, 73, 52, 255}
#define DARK_GREEN   CLITERAL(Color){152, 151, 26, 255}
#define LIGHT_GREEN  CLITERAL(Color){184, 187, 38, 255}
#define DARK_YELLOW  CLITERAL(Color){215, 153, 33, 255}
#define LIGHT_YELLOW CLITERAL(Color){250, 189, 47, 255}
#define DARK_BLUE    CLITERAL(Color){69, 133, 136, 255}
#define LIGHT_BLUE   CLITERAL(Color){131, 165, 152, 255}
#define DARK_PURPLE  CLITERAL(Color){177, 98, 134, 255}
#define LIGHT_PURPLE CLITERAL(Color){211, 134, 155, 255}
#define DARK_CYAN    CLITERAL(Color){104, 157, 106, 255}
#define LIGHT_CYAN   CLITERAL(Color){142, 192, 124, 255}
#define DARK_ORANGE  CLITERAL(Color){214, 93, 14, 255}
#define LIGHT_ORANGE CLITERAL(Color){254, 128, 25, 255}
#define DARK_GRAY    CLITERAL(Color){146, 131, 116, 255}
#define LIGHT_GRAY   CLITERAL(Color){168, 153, 132, 255}
#define BG           CLITERAL(Color){40, 40, 40, 255}
#define FG           CLITERAL(Color){235, 219, 178, 255}
#define BG0          BG
#define BG0_H        CLITERAL(Color){29, 32, 33, 255}
#define BG0_S        CLITERAL(Color){50, 4, 47, 255}
#define BG1          CLITERAL(Color){60, 56, 54, 255}
#define BG2          CLITERAL(Color){80, 73, 69, 255}
#define BG3          CLITERAL(Color){102, 92, 84, 255}
#define BG4          CLITERAL(Color){124, 111, 100, 255}
#define FG0          CLITERAL(Color){251, 241, 199, 255}
#define FG1          FG
#define FG2          CLITERAL(Color){213, 196, 161, 255}
#define FG3          CLITERAL(Color){189, 174, 147, 255}
#define FG4          CLITERAL(Color){168, 153, 132, 255}




typedef struct Player {
    Vector2 position, size;
    float jumpVel;
    float xVel;
    int jumpCount;
    int lives, maxLives;
    int health;
    struct timespec jump, boost;

} Player;

typedef struct EnvItem {
    Rectangle rect;
    int blocking;
    Color color;

} EnvItem;


void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta);

void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);

struct timespec current;


#define G                       (3000.f)
#define PLAYER_JUMP_SPD         (850.f)
#define PLAYER_BOOST_SPD        (650.f)
#define PLAYER_HOR_SPD          (550.f)
#define CAMROT                  (.15f)
#define PLAYER_JUMP_TIM         (210*1000)
#define FRICTION                (.94f)
static const int screenWidth  = 1000;
static const int screenHeight = 800;
static const int maxJump      = 2;

int main(void){


    // Initialization
    //--------------------------------------------------------------------------------------

    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE);
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
    clock_gettime(CLOCK_MONOTONIC_RAW, &player.jump);
    clock_gettime(CLOCK_MONOTONIC_RAW, &player.boost);
    player.position = (Vector2){ 400, 280 };
    player.size = (Vector2){40, 40};
    player.lives = 5;
    player.maxLives = 5;
    player.health = 100;



    EnvItem envItems[] = {
        {{ 0, 400, 1000, 200 }, 1, FG3 },
        {{ 300, 200, 400, 10 }, 1, FG3 },
        {{ 250, 300, 100, 10 }, 1, FG3 },
        {{ 650, 300, 100, 10 }, 1, FG3 }
    };

    int envItemsLength = sizeof(envItems)/sizeof(*envItems);

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
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(*cameraUpdaters);

    char *cameraDescriptions[] = {
        "[ Follow player center ]",
        "[ Follow player center, but clamp to map edges ]",
        "[ Follow player center; smoothed ]",
        "[ Follow player center horizontally; updateplayer center vertically after landing ]",
        "[ Player push camera on getting too close to screen edge ]"
    };
    cameraOption = (cameraOption + 1)%cameraUpdatersLength;


    // Main game loop
    while (!WindowShouldClose() && !exitWindow){
        // Update
        //----------------------------------------------------------------------------------
        clock_gettime(CLOCK_MONOTONIC_RAW, &current);
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
        if (IsKeyPressed(KEY_V)) camera.zoom = 1.0f;

        if(IsKeyDown(KEY_RIGHT_BRACKET)) camera.rotation+=CAMROT;
        if(IsKeyDown(KEY_LEFT_BRACKET))  camera.rotation-=CAMROT;
        if(IsKeyPressed(KEY_EQUAL)){
          if(camera.rotation!=.0f)
            camera.rotation=.0f;
          else
            camera.rotation=180.f;
        }

        if (IsKeyPressed(KEY_C)) cameraOption = (cameraOption + 1)%cameraUpdatersLength;
        if (IsKeyPressed(KEY_R)){
          player.position = (Vector2){ 400, 280 };
        }
        if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))){
          ToggleFullscreen();
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

                int start = player.position.x - ((player.maxLives==player.lives)?player.size.x-17:(player.size.x/player.maxLives - player.lives))-15;
                for(int i=0;i<player.lives;++i){
                  Rectangle heart = { start, player.position.y - player.size.y - 20, 10, 10 };
                  DrawRectangleRec(heart, LIGHT_RED);
                  start+=15;
                }
                Rectangle playerRect = { player.position.x - player.size.x/2, player.position.y - player.size.y, player.size.x, player.size.y };
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

  double boostdiff = ((current.tv_sec - player->boost.tv_sec) * 1e6 + (current.tv_nsec - player->boost.tv_nsec) / 1e3);

  if(IsKeyDown(KEY_X) && player->jumpCount!=maxJump && boostdiff > 2000 * 1000){
    player->boost = current;
  }
  if(boostdiff<150*1000){
    if(IsKeyDown(KEY_LEFT)){
      player->position.x -= 2*PLAYER_BOOST_SPD*delta;
      player->jumpVel=0;
    }
    if(IsKeyDown(KEY_RIGHT)){
      player->position.x += 2*PLAYER_BOOST_SPD*delta;
      player->jumpVel=0;
    }

    
  }
  if (IsKeyDown(KEY_LEFT)){
      player->xVel = (-PLAYER_HOR_SPD)*delta;
  }
  if (IsKeyDown(KEY_RIGHT)){
      player->xVel = PLAYER_HOR_SPD*delta;
  }

  player->xVel *= FRICTION;
  player->position.x += player->xVel;

  if (IsKeyDown(KEY_UP) && player->jumpCount>0 && ((current.tv_sec - player->jump.tv_sec) * 1e6 + (current.tv_nsec - player->jump.tv_nsec) / 1e3) > PLAYER_JUMP_TIM ){
    player->jumpVel = -PLAYER_JUMP_SPD*(maxJump==player->jumpCount?1:maxJump-player->jumpCount);
    player->jump = current;
    --player->jumpCount;
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
            ei->rect.y < p->y + player->jumpVel*delta)
        {
            hitObstacle = 1;
            player->jumpVel = 0.0f;
            p->y = ei->rect.y;
        }
    }

    if (!hitObstacle)
    {
        player->position.y += player->jumpVel*delta;
        player->jumpVel += G*delta;
    }
    else {
      player->jumpCount=maxJump;
    }

    if (player->position.y > screenHeight){
      player->position = (Vector2){ 400, 280 };
      --player->lives;
      player->health=100;
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
        if (player->jumpCount!=0 && (player->jumpVel == 0) && (player->position.y != camera->target.y)){
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
