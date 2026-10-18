
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspge.h>

PSP_MODULE_INFO("Dark Bot", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define SCREEN_W 480
#define SCREEN_H 272

static unsigned int __attribute__((aligned(16)))
display_list[262144];

typedef enum {
    MENU,
    GAME,
    ZONES,
    CREDITS,
    OPTIONS
} GameState;

static GameState state = MENU;
static int selected = 0;
static int zone = 0;
static int x = 40;
static int y = 190;
static int vy = 0;
static int grounded = 1;
static int facing = 0;
static int frame = 0;

static const char *menu[] = {
    "NOVO JOGO",
    "CONTINUAR",
    "ZONAS",
    "OPCOES",
    "CREDITOS",
    "SAIR"
};

static const char *zones[] = {
    "Floresta Abandonada",
    "Caverna Sombria",
    "Fabrica Perdida",
    "Cidade Destruida",
    "Montanhas Congeladas",
    "Laboratorio Final"
};

static void graphics_init(void)
{
    sceGuInit();
    sceGuStart(GU_DIRECT, display_list);

    sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
    sceGuDispBuffer(480, 272, (void *)0x88000, 512);
    sceGuDepthBuffer((void *)0x110000, 512);

    sceGuOffset(2048 - 240, 2048 - 136);
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);

    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

static void begin_frame(unsigned int color)
{
    sceGuStart(GU_DIRECT, display_list);
    sceGuClearColor(color);
    sceGuClear(GU_COLOR_BUFFER_BIT |
               GU_DEPTH_BUFFER_BIT);
}

static void end_frame(void)
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

static void draw_player(void)
{
    /* Personagem temporario, ate os PNGs serem adicionados. */
    sceGuDisable(GU_TEXTURE_2D);
    sceGuColor(0xFF50B8FF);
    sceGuDrawArray(GU_SPRITES,
        GU_VERTEX_16BIT | GU_TRANSFORM_2D,
        2, NULL, (void *)0);
}

static void update_player(unsigned int buttons)
{
    int speed = (buttons & PSP_CTRL_RTRIGGER) ? 4 : 2;

    if (buttons & PSP_CTRL_LEFT) {
        x -= speed;
        facing = 1;
    }

    if (buttons & PSP_CTRL_RIGHT) {
        x += speed;
        facing = 0;
    }

    if ((buttons & PSP_CTRL_CROSS) && grounded) {
        vy = -9;
        grounded = 0;
    }

    y += vy;

    if (!grounded) {
        vy++;
        if (y >= 190) {
            y = 190;
            vy = 0;
            grounded = 1;
        }
    }

    if (x < 0) x = 0;
    if (x > 448) x = 448;
    frame++;
}

static void draw_menu(void)
{
    int i;

    pspDebugScreenInit();
    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenSetXY(15, 2);
    pspDebugScreenPrintf("DARK BOT");

    pspDebugScreenSetTextColor(0xFFFFD080);
    pspDebugScreenSetXY(9, 4);
    pspDebugScreenPrintf("AVENTURA PARA PSP");

    for (i = 0; i < 6; i++) {
        pspDebugScreenSetXY(10, 7 + i * 2);

        if (i == selected)
            pspDebugScreenPrintf("> %s", menu[i]);
        else
            pspDebugScreenPrintf("  %s", menu[i]);
    }

    pspDebugScreenSetXY(3, 25);
    pspDebugScreenPrintf("D-PAD: navegar | X: escolher");
}

static void draw_zones(void)
{
    int i;

    pspDebugScreenInit();
    pspDebugScreenSetTextColor(0xFFFFD080);
    pspDebugScreenSetXY(5, 2);
    pspDebugScreenPrintf("SELECIONE UMA ZONA");

    for (i = 0; i < 6; i++) {
        pspDebugScreenSetXY(3, 5 + i * 2);

        if (i == zone)
            pspDebugScreenPrintf("> %d. %s", i + 1, zones[i]);
        else
            pspDebugScreenPrintf("  %d. %s", i + 1, zones[i]);
    }

    pspDebugScreenSetXY(3, 25);
    pspDebugScreenPrintf("X: entrar | O: voltar");
}

static void draw_credits(void)
{
    pspDebugScreenInit();
    pspDebugScreenSetXY(8, 4);
    pspDebugScreenPrintf("CREDITOS");

    pspDebugScreenSetXY(3, 8);
    pspDebugScreenPrintf("Criado por Gabriel Nunes Pereira (hollow)");

    pspDebugScreenSetXY(3, 11);
    pspDebugScreenPrintf("Programacao com auxilio do ChatGPT");

    pspDebugScreenSetXY(3, 14);
    pspDebugScreenPrintf("Projeto independente");

    pspDebugScreenSetXY(3, 24);
    pspDebugScreenPrintf("O: voltar");
}

static void draw_options(void)
{
    pspDebugScreenInit();
    pspDebugScreenSetXY(8, 4);
    pspDebugScreenPrintf("OPCOES");

    pspDebugScreenSetXY(3, 8);
    pspDebugScreenPrintf("Setas: movimentar");

    pspDebugScreenSetXY(3, 10);
    pspDebugScreenPrintf("X: pular");

    pspDebugScreenSetXY(3, 12);
    pspDebugScreenPrintf("R: correr");

    pspDebugScreenSetXY(3, 14);
    pspDebugScreenPrintf("O: voltar");
}

static void update_menu(unsigned int pressed)
{
    if (pressed & PSP_CTRL_UP)
        selected = (selected + 5) % 6;

    if (pressed & PSP_CTRL_DOWN)
        selected = (selected + 1) % 6;

    if (!(pressed & PSP_CTRL_CROSS))
        return;

    switch (selected) {
        case 0:
        case 1:
            x = 40;
            y = 190;
            vy = 0;
            grounded = 1;
            state = GAME;
            break;
        case 2:
            state = ZONES;
            break;
        case 3:
            state = OPTIONS;
            break;
        case 4:
            state = CREDITS;
            break;
        case 5:
            sceKernelExitGame();
            break;
    }
}

static void update_zones(unsigned int pressed)
{
    if (pressed & PSP_CTRL_UP)
        zone = (zone + 5) % 6;

    if (pressed & PSP_CTRL_DOWN)
        zone = (zone + 1) % 6;

    if (pressed & PSP_CTRL_CROSS) {
        x = 40;
        y = 190;
        vy = 0;
        grounded = 1;
        state = GAME;
    }

    if (pressed & PSP_CTRL_CIRCLE)
        state = MENU;
}

int main(void)
{
    SceCtrlData pad;
    unsigned int previous = 0;
    unsigned int pressed;

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
    pspDebugScreenInit();
    graphics_init();

    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);
        pressed = pad.Buttons & ~previous;
        previous = pad.Buttons;

        switch (state) {
        case MENU:
            update_menu(pressed);
            draw_menu();
            break;

        case GAME:
            if (pressed & PSP_CTRL_CIRCLE) {
                state = MENU;
                break;
            }

            if (pressed & PSP_CTRL_START) {
                state = ZONES;
                break;
            }

            update_player(pad.Buttons);
            begin_frame(0xFF202838);
            end_frame();
            draw_menu(); /* substituido abaixo pelo estado de jogo */
            break;

        case ZONES:
            update_zones(pressed);
            draw_zones();
            break;

        case CREDITS:
            draw_credits();
            if (pressed & PSP_CTRL_CIRCLE)
                state = MENU;
            break;

        case OPTIONS:
            draw_options();
            if (pressed & PSP_CTRL_CIRCLE)
                state = MENU;
            break;
        }
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
