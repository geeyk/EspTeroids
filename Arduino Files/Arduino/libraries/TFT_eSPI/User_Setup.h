// User_Setup.h - Configuração recomendada para ST7789 240x320 + ESP32

// 1) Driver do display
#define ST7789_DRIVER

// 2) Resolução
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// 3) Pinos SPI (ESP32 padrão)
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4

// Se o seu display NÃO tem CS, comente o CS acima e desabilite o CS:
// #define TFT_CS   -1

// 4) Backlight (opcional, se controlado por GPIO)
// #define TFT_BL   5
// #define TFT_BACKLIGHT_ON HIGH

// 5) Troca de cores (se as cores estiverem invertidas, descomente)
// #define TFT_RGB_ORDER TFT_BGR

// 6) Offset de colunas/linhas (ajuste se a imagem estiver cortada/deslocada)
// Para muitos ST7789 240x320, colStart/rowStart = 0;
// Se precisar, experimente:
// #define CGRAM_OFFSET
// #define COLSTART 0
// #define ROWSTART 0

// 7) SPI mais rápido para jogos
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000

// 8) Fontes
#define LOAD_GLCD   // Fonte 8x8
#define LOAD_FONT2  // Fonte 4x6
#define LOAD_FONT4  // Fonte 6x8
#define LOAD_FONT6  // Fonte 8x12
#define LOAD_FONT7  // Fonte 7x14
#define LOAD_FONT8  // Fonte 8x16
#define LOAD_GFXFF  // FreeFonts (GFX)

// 9) Sprites (para aumentar performance e evitar flicker)
#define HEAP_CAPSULE_AVAILABLE

// 10) Desabilitar MISO (se não usar leitura do display)
#define TFT_MISO -1
