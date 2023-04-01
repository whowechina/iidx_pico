/*
 * IIDX Controller Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_IIDX_PICO
/* A button consists of a switch and an LED
   7 main buttons + E1, E2, E3, E4 */
#define BUTTON_DEF { \
    {8, -1}, \
    {7, -1}, \
    {6, -1}, \
    {5, -1}, \
    {4, -1}, \
    {3, -1}, \
    {2, -1}, \
    {12, -1}, \
    {11, -1}, \
    {10, -1}, \
    {9, -1}, \
    {1, -1}, \
    {0, -1}, \
}

#define BUTTON_RGB_PIN 13
#define BUTTON_RGB_NUM 11
#define BUTTON_RGB_MAP { 6, 0, 5, 1, 4, 2, 3, 7, 8, 9, 10}

/* Color 0xc1c2c3 */
//#define BUTTON_RGB_ORDER RGB
#define BUTTON_RGB_ORDER GRB

#define TT_RGB_PIN 28
#define TT_RGB_NUM 52
#define TT_RGB_ORDER GRB

#define TT_RGB_START 0
#define TT_RGB_SIZE 52
#define TT_RGB_REVERSED true

#define TT_AS5600_SCL 27
#define TT_AS5600_SDA 26
#define TT_AS5600_I2C i2c1
#else

#endif
