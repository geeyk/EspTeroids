// Jogo simples com TFT_eSPI + ESP32 + ST7789 240x320
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();             // Objeto display
TFT_eSprite sprite = TFT_eSprite(&tft); // Sprite para o puck

// Configurações do sprite do puck
const int puckSize = 16;
int puckX = 120;
int puckY = 160;

// Velocidade
int velX = 2;
int velY = 2;

// Botões (ajuste conforme seus botões)
const int btnPlusX = 32; // IO 32
const int btnMinusX = 33;
const int btnPlusY = 25;
const int btnMinusY = 26;

// Timers
unsigned long lastTime = 0;
const unsigned long frameTime = 1; // 1ms entre frames (ajuste para limitar FPS)

void setup() {
  Serial.begin(115200);

  // Inicializar display
  tft.init();
  tft.setRotation(0); // 0–3, ajuste para orientação correta
  tft.fillScreen(TFT_BLACK);

  // Inicializar sprite
  sprite.createSprite(puckSize, puckSize);
  sprite.fillSprite(TFT_BLACK);

  // Configurar botões como INPUT_PULLUP
  pinMode(btnPlusX, INPUT_PULLUP);
  pinMode(btnMinusX, INPUT_PULLUP);
  pinMode(btnPlusY, INPUT_PULLUP);
  pinMode(btnMinusY, INPUT_PULLUP);

  // Desenhar “quadro” inicial: borda branca em volta da tela
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_WHITE);
}

void loop() {
  // Controle simples de FPS
  unsigned long now = millis();
  if (now - lastTime < frameTime) return;
  lastTime = now;

  // Ler botões e ajustar velocidade
  if (digitalRead(btnPlusX) == LOW) velX++;
  if (digitalRead(btnMinusX) == LOW) velX--;
  if (digitalRead(btnPlusY) == LOW) velY++;
  if (digitalRead(btnMinusY) == LOW) velY--;

  // Limitar velocidade máxima/minima
  const int maxVel = 10;
  const int minVel = -10;
  if (velX > maxVel) velX = maxVel;
  if (velX < minVel) velX = minVel;
  if (velY > maxVel) velY = maxVel;
  if (velY < minVel) velY = minVel;

  // Mover puck
  puckX += velX;
  puckY += velY;

  // Colisão com bordas (reflete)
  if (puckX <= 0) {
    puckX = 0;
    velX = -velX;
  }
  if (puckX + puckSize >= tft.width()) {
    puckX = tft.width() - puckSize;
    velX = -velX;
  }
  if (puckY <= 0) {
    puckY = 0;
    velY = -velY;
  }
  if (puckY + puckSize >= tft.height()) {
    puckY = tft.height() - puckSize;
    velY = -velY;
  }

  // Desenhar puck no sprite e enviar para a tela
  sprite.fillSprite(TFT_BLACK);
  sprite.fillCircle(puckSize / 2, puckSize / 2, puckSize / 2, TFT_CYAN);
  sprite.pushSprite(puckX, puckY);
}