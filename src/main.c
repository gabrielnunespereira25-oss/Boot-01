#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("Dark Bot", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define MENU_COUNT 4
#define ZONE_COUNT 6

static const char *menu_items[MENU_COUNT] = {
    "Jogar Dark Bot",
    "Menu Beta (referencia)",
    "Selecionar zona",
    "Creditos"
};

static const char *zones[ZONE_COUNT] = {
    "Floresta Abandonada",
    "Caverna Sombria",
    "Fabrica Perdida",
    "Cidade Destruida",
    "Montanhas Congeladas",
    "Laboratorio Final"
};

static int menu_index = 0;
static int zone_index = 0;
static int current_zone = 0;
static int screen = 0; /* 0 menu, 1 game, 2 zones, 3 credits, 4 beta info */
static int player_x = 20;
static int player_y = 22;
static int jump_ticks = 0;

static unsigned int previous_buttons = 0;

static int pressed(unsigned int buttons, unsigned int key)
{
    return (buttons & key) && !(previous_buttons & key);
}

static void draw_header(const char *subtitle)
{
    pspDebugScreenSetXY(0, 0);
    printf("============================================================\n");
    printf("                     DARK BOT PSP\n");
    printf("============================================================\n");
    printf("%s\n\n", subtitle);
}

static void draw_menu(void)
{
    int i;
    pspDebugScreenClear();
    draw_header("Projeto integrado - menu reconstruido");
    printf("Use CIMA/BAIXO para navegar e X para selecionar.\n\n");
    for (i = 0; i < MENU_COUNT; ++i) {
        printf("%s %s\n", (i == menu_index) ? ">" : " ", menu_items[i]);
    }
    printf("\n\nInspiracao visual/textual: Hollow Knight Beta Menu.\n");
    printf("Este menu foi refeito em C; nao e codigo extraido do EBOOT.\n");
    printf("\nControles: X selecionar | O voltar | START iniciar jogo\n");
}

static void draw_zones(void)
{
    int i;
    pspDebugScreenClear();
    draw_header("SELECAO DE ZONA");
    printf("CIMA/BAIXO: navegar   X: selecionar   O: voltar\n\n");
    for (i = 0; i < ZONE_COUNT; ++i) {
        printf("%s %d. %s\n", (i == zone_index) ? ">" : " ", i + 1, zones[i]);
    }
}

static void draw_credits(void)
{
    pspDebugScreenClear();
    draw_header("CREDITOS");
    printf("Criado por Gabriel Nunes Pereira (hollow)\n\n");
    printf("Programacao com auxilio do ChatGPT\n");
    printf("Projeto independente\n\n");
    printf("O menu Beta foi usado apenas como referencia.\n");
    printf("\nPressione O para voltar.");
}

static void draw_beta_info(void)
{
    pspDebugScreenClear();
    draw_header("HOLLOW KNIGHT - BETA MENU (REFERENCIA)");
    printf("Texto de referencia encontrado no EBOOT enviado:\n\n");
    printf("HOLLOW NEST / PS202\n");
    printf("HOLLOW KNIGHT\n");
    printf("BETA EDITION\n\n");
    printf("Este e um painel informativo reconstruido.\n");
    printf("Nao carrega nem executa o outro EBOOT.PBP.\n");
    printf("\nPressione O para voltar.");
}

static void draw_game(void)
{
    int i;
    pspDebugScreenClear();
    draw_header(zones[current_zone]);
    printf("D-PAD: mover | X: pular | R: correr | O: menu | START: zonas\n\n");

    /* Area de jogo textual simples para validar compilacao e controles. */
    for (i = 0; i < 8; ++i) printf("\n");
    pspDebugScreenSetXY(0, 15);
    printf("+----------------------------------------------------------+\n");
    for (i = 0; i < 8; ++i) printf("|                                                          |\n");
    printf("+----------------------------------------------------------+\n");

    if (player_y < 15) player_y = 15;
    if (player_y > 22) player_y = 22;
    if (player_x < 1) player_x = 1;
    if (player_x > 57) player_x = 57;

    pspDebugScreenSetXY(player_x, player_y);
    printf("@");
    pspDebugScreenSetXY(0, 27);
    printf("Zona %d/%d: %s\n", current_zone + 1, ZONE_COUNT, zones[current_zone]);
    printf("Prototipo textual. Os sprites PNG podem ser ligados depois.");
}

int main(void)
{
    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    pspDebugScreenInit();

    for (;;) {
        sceCtrlReadBufferPositive(&pad, 1);

        if (screen == 0) {
            if (pressed(pad.Buttons, PSP_CTRL_UP)) menu_index = (menu_index + MENU_COUNT - 1) % MENU_COUNT;
            if (pressed(pad.Buttons, PSP_CTRL_DOWN)) menu_index = (menu_index + 1) % MENU_COUNT;
            if (pressed(pad.Buttons, PSP_CTRL_START)) screen = 1;
            if (pressed(pad.Buttons, PSP_CTRL_CROSS)) {
                if (menu_index == 0) screen = 1;
                else if (menu_index == 1) screen = 4;
                else if (menu_index == 2) screen = 2;
                else if (menu_index == 3) screen = 3;
            }
        } else if (screen == 1) {
            int speed = (pad.Buttons & PSP_CTRL_RTRIGGER) ? 2 : 1;
            if (pad.Buttons & PSP_CTRL_LEFT) player_x -= speed;
            if (pad.Buttons & PSP_CTRL_RIGHT) player_x += speed;
            if (pressed(pad.Buttons, PSP_CTRL_CROSS)) jump_ticks = 8;
            if (jump_ticks > 0) {
                player_y = 18;
                --jump_ticks;
            } else {
                player_y = 22;
            }
            if (pressed(pad.Buttons, PSP_CTRL_START)) screen = 2;
            if (pressed(pad.Buttons, PSP_CTRL_CIRCLE)) screen = 0;
        } else if (screen == 2) {
            if (pressed(pad.Buttons, PSP_CTRL_UP)) zone_index = (zone_index + ZONE_COUNT - 1) % ZONE_COUNT;
            if (pressed(pad.Buttons, PSP_CTRL_DOWN)) zone_index = (zone_index + 1) % ZONE_COUNT;
            if (pressed(pad.Buttons, PSP_CTRL_CROSS)) {
                current_zone = zone_index;
                screen = 1;
            }
            if (pressed(pad.Buttons, PSP_CTRL_CIRCLE)) screen = 0;
        } else {
            if (pressed(pad.Buttons, PSP_CTRL_CIRCLE)) screen = 0;
        }

        if (screen == 0) draw_menu();
        else if (screen == 1) draw_game();
        else if (screen == 2) draw_zones();
        else if (screen == 3) draw_credits();
        else draw_beta_info();

        previous_buttons = pad.Buttons;
        sceKernelDelayThread(50000);
    }
    sceKernelExitGame();
    return 0;
}
