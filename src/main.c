
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspge.h>
#include <psputils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "png_loader.h"

PSP_MODULE_INFO("Dark Bot", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define SCREEN_W 480
#define SCREEN_H 272
#define MAX_SPRITES 14
#define MAX_ZONES 6
#define DISPLAY_STRIDE 512
#define DISPLAY_BUFFER ((void *)0x88000)
#define DEPTH_BUFFER ((void *)0x110000)

#define COLOR_BG       0xFF202838
#define COLOR_WHITE    0xFFFFFFFF
#define COLOR_GOLD     0xFFFFD080
#define COLOR_BLUE     0xFF50B8FF
#define COLOR_RED      0xFF5050FF

static unsigned int __attribute__((aligned(16)))
display_list[262144];

typedef struct {
    int x, y;
    int w, h;
    int loaded;
    PngImage image;
} Sprite;

typedef struct {
    float x, y;
    float vx, vy;
    int grounded;
    int facing;
    int frame;
    int anim_timer;
} Player;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_ZONES,
    STATE_CREDITS,
    STATE_OPTIONS
} GameState;

static Sprite sprites[MAX_SPRITES];
static Player player;
static GameState state = STATE_MENU;

static int menu_choice = 0;
static int zone_choice = 0;
static int current_zone = 0;
static int running = 0;
static int frame_counter = 0;

static const char *menu_items[] = {
    "NOVO JOGO",
    "CONTINUAR",
    "ZONAS",
    "OPCOES",
    "CREDITOS",
    "SAIR"
};

static const char *zone_names[MAX_ZONES] = {
    "Floresta Abandonada",
    "Caverna Sombria",
    "Fabrica Perdida",
    "Cidade Destruida",
    "Montanhas Congeladas",
    "Laboratorio Final"
};

static int load_sprite(int index)
{
    char path[64];

    if (index < 0 || index >= MAX_SPRITES)
        return 0;

    snprintf(path, sizeof(path), "assets/sprite_%02d.png",
             index + 1);

    memset(&sprites[index], 0, sizeof(Sprite));

    if (!psp_png_load(path, &sprites[index].image))
        return 0;

    sprites[index].w = sprites[index].image.width;
    sprites[index].h = sprites[index].image.height;
    sprites[index].loaded = 1;

    return 1;
}

static void load_assets(void)
{
    int i;

    for (i = 0; i < MAX_SPRITES; i++)
        load_sprite(i);
}

static void free_assets(void)
{
    int i;

    for (i = 0; i < MAX_SPRITES; i++) {
        if (sprites[i].loaded) {
            psp_png_free(&sprites[i].image);
            sprites[i].loaded = 0;
        }
    }
}

static void graphics_init(void)
{
    sceGuInit();

    sceGuStart(GU_DIRECT, display_list);

    sceGuDrawBuffer(GU_PSM_8888, (void *)0,
                    DISPLAY_STRIDE);
    sceGuDispBuffer(SCREEN_W, SCREEN_H,
                    DISPLAY_BUFFER, DISPLAY_STRIDE);
    sceGuDepthBuffer(DEPTH_BUFFER, DISPLAY_STRIDE);

    sceGuOffset(2048 - SCREEN_W / 2,
                2048 - SCREEN_H / 2);
    sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);

    sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_LIGHTING);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

static void clear_screen(unsigned int color)
{
    sceGuStart(GU_DIRECT, display_list);
    sceGuClearColor(color);
    sceGuClear(GU_COLOR_BUFFER_BIT |
               GU_DEPTH_BUFFER_BIT);
}

typedef struct {
    unsigned short u, v;
    short x, y, z;
} SpriteVertex;

static void draw_sprite(int index, int x, int y,
                        int flip)
{
    Sprite *s;
    SpriteVertex *vertices;
    int w, h;

    if (index < 0 || index >= MAX_SPRITES)
        return;

    s = &sprites[index];

    if (!s->loaded || !s->image.pixels)
        return;

    w = s->w;
    h = s->h;

    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);

    sceGuTexImage(0, w, h, w, s->image.pixels);

    sceGuTexSync();

    vertices = (SpriteVertex *)
        sceGuGetMemory(2 * sizeof(SpriteVertex));

    if (flip) {
        vertices[0].u = w;
        vertices[1].u = 0;
    } else {
        vertices[0].u = 0;
        vertices[1].u = w;
    }

    vertices[0].v = 0;
    vertices[1].v = h;

    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0;

    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0;

    sceGuColor(COLOR_WHITE);

    sceGuDrawArray(
        GU_SPRITES,
        GU_TEXTURE_16BIT |
        GU_VERTEX_16BIT |
        GU_TRANSFORM_2D,
        2, NULL, vertices
    );

    sceGuDisable(GU_TEXTURE_2D);
}

static void graphics_end(void)
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

static void player_reset(void)
{
    player.x = 40;
    player.y = 170;
    player.vx = 0;
    player.vy = 0;
    player.grounded = 1;
    player.facing = 0;
    player.frame = 0;
    player.anim_timer = 0;
    running = 0;
}

static void start_game(void)
{
    current_zone = 0;
    player_reset();
    state = STATE_PLAYING;
}

static void update_player(unsigned int buttons)
{
    float speed = running ? 3.2f : 1.8f;

    player.vx = 0;
    running = (buttons & PSP_CTRL_RTRIGGER) != 0;

    if (buttons & PSP_CTRL_LEFT) {
        player.vx = -speed;
        player.facing = 1;
    }

    if (buttons & PSP_CTRL_RIGHT) {
        player.vx = speed;
        player.facing = 0;
    }

    if ((buttons & PSP_CTRL_CROSS) && player.grounded) {
        player.vy = -7.5f;
        player.grounded = 0;
    }

    player.x += player.vx;
    player.y += player.vy;

    if (!player.grounded)
        player.vy += 0.32f;

    if (player.y >= 170) {
        player.y = 170;
        player.vy = 0;
        player.grounded = 1;
    }

    if (player.x < 0)
        player.x = 0;

    if (player.x > SCREEN_W - 32)
        player.x = SCREEN_W - 32;

    if (player.anim_timer++ >= 8) {
        player.anim_timer = 0;
        player.frame++;
    }

    if (player.frame > 3)
        player.frame = 0;
}

static int player_sprite_index(void)
{
    /*
       Organizacao inicial dos 14 arquivos:
       01-02: parado
       03-06: andando
       07-10: correndo
       11-14: pulando

       Ajuste os indices se seus PNGs estiverem
       em outra ordem.
    */

    if (!player.grounded)
        return 10 + (player.frame % 4);

    if (running && player.vx != 0)
        return 6 + (player.frame % 4);

    if (player.vx != 0)
        return 2 + (player.frame % 4);

    return player.frame % 2;
}

static void draw_game(void)
{
    int sprite_index;

    clear_screen(COLOR_BG);

    /* Linha simples representando o chao. */
    sceGuDisable(GU_TEXTURE_2D);
    sceGuFinish();
    sceGuSync(0, 0);

    sprite_index = player_sprite_index();

    draw_sprite(sprite_index, (int)player.x,
                (int)player.y, player.facing);

    graphics_end();
}

static void draw_menu(void)
{
    int i;

    pspDebugScreenInit();
    pspDebugScreenSetTextColor(COLOR_WHITE);
    pspDebugScreenSetXY(14, 2);
    pspDebugScreenPrintf("DARK BOT");

    pspDebugScreenSetTextColor(COLOR_BLUE);
    pspDebugScreenSetXY(8, 4);
    pspDebugScreenPrintf("JOGO INDEPENDENTE PARA PSP");

    for (i = 0; i < 6; i++) {
        pspDebugScreenSetXY(10, 7 + i * 2);

        if (i == menu_choice) {
            pspDebugScreenSetTextColor(COLOR_GOLD);
            pspDebugScreenPrintf("> %s", menu_items[i]);
        } else {
            pspDebugScreenSetTextColor(COLOR_WHITE);
            pspDebugScreenPrintf("  %s", menu_items[i]);
        }
    }

    pspDebugScreenSetTextColor(COLOR_WHITE);
    pspDebugScreenSetXY(3, 25);
    pspDebugScreenPrintf("D-PAD: mover  X: selecionar");
}

static void draw_zones(void)
{
    int i;

    pspDebugScreenInit();
    pspDebugScreenSetTextColor(COLOR_GOLD);
    pspDebugScreenSetXY(5, 2);
    pspDebugScreenPrintf("SELECIONE UMA ZONA");

    for (i = 0; i < MAX_ZONES; i++) {
        pspDebugScreenSetXY(4, 5 + i * 2);

        if (i == zone_choice) {
            pspDebugScreenSetTextColor(COLOR_GOLD);
            pspDebugScreenPrintf("> %d. %s",
                                 i + 1, zone_names[i]);
        } else {
            pspDebugScreenSetTextColor(COLOR_WHITE);
            pspDebugScreenPrintf("  %d. %s",
                                 i + 1, zone_names[i]);
        }
    }

    pspDebugScreenSetTextColor(COLOR_WHITE);
    pspDebugScreenSetXY(3, 25);
    pspDebugScreenPrintf("X: entrar   O: voltar");
}

static void draw_credits(void)
{
    pspDebugScreenInit();
    pspDebugScreenSetTextColor(COLOR_GOLD);
    pspDebugScreenSetXY(8, 4);
    pspDebugScreenPrintf("CREDITOS");

    pspDebugScreenSetTextColor(COLOR_WHITE);
    pspDebugScreenSetXY(3, 8);
    pspDebugScreenPrintf("Criado por Gabriel Nunes");
    pspDebugScreenSetXY(3, 9);
    pspDebugScreenPrintf("Pereira (hollow)");

    pspDebugScreenSetXY(3, 12);
    pspDebugScreenPrintf("Programacao com auxilio");
    pspDebugScreenSetXY(3, 13);
    pspDebugScreenPrintf("do ChatGPT");

    pspDebugScreenSetXY(3, 16);
    pspDebugScreenPrintf("Projeto independente");

    pspDebugScreenSetXY(3, 24);
    pspDebugScreenPrintf("O: voltar ao menu");
}

static void draw_options(void)
{
    pspDebugScreenInit();
    pspDebugScreenSetTextColor(COLOR_GOLD);
    pspDebugScreenSetXY(8, 4);
    pspDebugScreenPrintf("OPCOES");

    pspDebugScreenSetTextColor(COLOR_WHITE);
    pspDebugScreenSetXY(3, 8);
    pspDebugScreenPrintf("D-PAD: movimentar");
    pspDebugScreenSetXY(3, 10);
    pspDebugScreenPrintf("X: pular");
    pspDebugScreenSetXY(3, 12);
    pspDebugScreenPrintf("R: correr");
    pspDebugScreenSetXY(3, 14);
    pspDebugScreenPrintf("O: voltar");

    pspDebugScreenSetXY(3, 24);
    pspDebugScreenPrintf("O: voltar ao menu");
}

static void menu_update(unsigned int buttons)
{
    if (buttons & PSP_CTRL_UP) {
        menu_choice--;
        if (menu_choice < 0)
            menu_choice = 5;
    }

    if (buttons & PSP_CTRL_DOWN) {
        menu_choice++;
        if (menu_choice > 5)
            menu_choice = 0;
    }

    if (!(buttons & PSP_CTRL_CROSS))
        return;

    switch (menu_choice) {
        case 0:
            start_game();
            break;

        case 1:
            player_reset();
            state = STATE_PLAYING;
            break;

        case 2:
            zone_choice = current_zone;
            state = STATE_ZONES;
            break;

        case 3:
            state = STATE_OPTIONS;
            break;

        case 4:
            state = STATE_CREDITS;
            break;

        case 5:
            sceKernelExitGame();
            break;
    }
}

static void zones_update(unsigned int buttons)
{
    if (buttons & PSP_CTRL_UP) {
        zone_choice--;
        if (zone_choice < 0)
            zone_choice = MAX_ZONES - 1;
    }

    if (buttons & PSP_CTRL_DOWN) {
        zone_choice++;
        if (zone_choice >= MAX_ZONES)
            zone_choice = 0;
    }

    if (buttons & PSP_CTRL_CROSS) {
        current_zone = zone_choice;
        player_reset();
        state = STATE_PLAYING;
    }

    if (buttons & PSP_CTRL_CIRCLE)
        state = STATE_MENU;
}

int main(void)
{
    SceCtrlData pad;
    unsigned int old_buttons = 0;
    unsigned int buttons;
    unsigned int pressed;

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    pspDebugScreenInit();
    graphics_init();
    load_assets();
    player_reset();

    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);

        buttons = pad.Buttons;
        pressed = buttons & ~old_buttons;
        old_buttons = buttons;

        switch (state) {
            case STATE_MENU:
                if (pressed & (PSP_CTRL_UP | PSP_CTRL_DOWN))
                    menu_update(pressed);
                else if (pressed & PSP_CTRL_CROSS)
                    menu_update(PSP_CTRL_CROSS);

                draw_menu();
                break;

            case STATE_PLAYING:
                if (pressed & PSP_CTRL_CIRCLE) {
                    state = STATE_MENU;
                    break;
                }

                if (pressed & PSP_CTRL_START) {
                    state = STATE_ZONES;
                    break;
                }

                update_player(buttons);
                draw_game();
                break;

            case STATE_ZONES:
                zones_update(pressed);
                draw_zones();
                break;

            case STATE_CREDITS:
                draw_credits();
                if (pressed & PSP_CTRL_CIRCLE)
                    state = STATE_MENU;
                break;

            case STATE_OPTIONS:
                draw_options();
                if (pressed & PSP_CTRL_CIRCLE)
                    state = STATE_MENU;
                break;
        }

        sceDisplayWaitVblankStart();
    }

    free_assets();
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
