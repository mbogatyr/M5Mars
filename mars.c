/*
 * MARS Simulation - Voxel Terrain Flight Engine (16-bit MS-DOS)
 * Inspired by Tim Clarke's MARS.EXE (1993)
 * Target: IBM PC 286 / 386, VGA Mode 0x13 (320x200, 256 colors)
 */

#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <mem.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define MAP_SIZE      256
#define MAP_MASK      255

/* Указатель на видеопамять VGA Mode 13h */
unsigned char far *VGA = (unsigned char far *)0xA0000000L;

/* Буферы высот и цветов ландшафта */
unsigned char height_map[MAP_SIZE][MAP_SIZE];
unsigned char color_map[MAP_SIZE][MAP_SIZE];

/* Установка видеорежима VGA 320x200x256 */
void set_mode_13h(void) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x13;
    int86(0x10, &regs, &regs);
}

/* Возврат в текстовый режим */
void set_text_mode(void) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int86(0x10, &regs, &regs);
}

/* Настройка марсианской палитры (градиенты красного, оранжевого и коричневого) */
void set_mars_palette(void) {
    int i;
    outp(0x03C8, 0);
    for (i = 0; i < 256; i++) {
        /* Формирование оттенков марсианского грунта */
        int r = i / 4 + 16;
        int g = i / 8;
        int b = i / 16;
        
        if (r > 63) r = 63;
        if (g > 63) g = 63;
        if (b > 63) b = 63;

        outp(0x03C9, r);
        outp(0x03C9, g);
        outp(0x03C9, b);
    }
}

/* Простая генерация марсианского фрактального ландшафта */
void generate_mars_terrain(void) {
    int x, y;
    for (y = 0; y < MAP_SIZE; y++) {
        for (x = 0; x < MAP_SIZE; x++) {
            /* Простая комбинация синусоид для имитации кратеров и каньонов */
            int h = (int)(32 + 20 * (x % 32) / 32.0 + 15 * (y % 64) / 64.0);
            h += (rand() % 8);
            if (h > 255) h = 255;
            if (h < 0) h = 0;

            height_map[y][x] = (unsigned char)h;
            /* Цвет зависит от высоты + небольшая затененность */
            color_map[y][x]  = (unsigned char)(h / 2 + 64);
        }
    }
}

/* Воксельный рендеринг кадра (Raycasting / Heightfield Projection) */
void render_frame(int cam_x, int cam_y, int cam_alt, int angle) {
    int x, z;
    
    /* Очистка экрана (небо) */
    _fmemset(VGA, 10, SCREEN_WIDTH * SCREEN_HEIGHT);

    /* Рендеринг столбцов экрана слева направо */
    for (x = 0; x < SCREEN_WIDTH; x++) {
        int highest_y = SCREEN_HEIGHT;
        
        /* Прорисовка луча от ближнего плана к горизонту */
        for (z = 1; z < 150; z++) {
            /* Вычисление координат карты с учетом перспективы */
            int map_x = (cam_x + ((x - SCREEN_WIDTH / 2) * z) / 128) & MAP_MASK;
            int map_y = (cam_y + z) & MAP_MASK;

            /* Получение высоты и цвета точки */
            int h = height_map[map_y][map_x];
            int col = color_map[map_y][map_x];

            /* Проекция 3D высоты на 2D экран */
            int screen_y = SCREEN_HEIGHT - ((h - cam_alt) * 100 / z + 100);

            if (screen_y < 0) screen_y = 0;
            if (screen_y > SCREEN_HEIGHT) screen_y = SCREEN_HEIGHT;

            /* Рисуем вертикальный отрезок, если он выше предыдущих */
            if (screen_y < highest_y) {
                int py;
                for (py = screen_y; py < highest_y; py++) {
                    VGA[py * SCREEN_WIDTH + x] = (unsigned char)col;
                }
                highest_y = screen_y;
            }

            if (highest_y <= 0) break;
        }
    }
}

int main(void) {
    int cam_x = 0;
    int cam_y = 0;
    int cam_alt = 50;

    generate_mars_terrain();

    set_mode_13h();
    set_mars_palette();

    /* Цикл полёта над Марсом */
    while (!kbhit()) {
        render_frame(cam_x, cam_y, cam_alt, 0);
        
        /* Движение вперед */
        cam_y = (cam_y + 1) & MAP_MASK;
        cam_x = (cam_x + 1) & MAP_MASK;
    }

    getch(); /* Сбросить нажатую клавишу */
    set_text_mode();

    return 0;
}