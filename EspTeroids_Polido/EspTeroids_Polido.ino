/*
 * Jogo: Asteroids com Double Buffer (16 bpp)
 * Hardware: ESP32 + TFT (TFT_eSPI) + potenciômetro pino 32
 * 
 * Controle: potenciômetro define ângulo de tiro (auto-tiro a cada 300ms)
 * Sem pisca-pisca, sem carregamento progressivo.
 * SPI reduzida para 20 MHz (configure em User_Setup.h).
 */

#include <TFT_eSPI.h>
#include <SPI.h>

// ========== CONSTANTES ==========
const int SCREEN_W = 240;
const int SCREEN_H = 320;

const int FRAME_TIME_MS = 20;      // 50 FPS
const int TIRO_COOLDOWN_MS = 300;
const int MAX_ASTEROIDES = 8;
const int MAX_TIROS = 15;

const int NAVE_SIZE = 16;
const int ASTEROIDE_SIZE = 14;
const int TIRO_SIZE = 4;

const int PIN_POT = 32;

const int BASE_SPAWN_INTERVAL_MS = 3000;
const int MIN_SPAWN_INTERVAL_MS = 800;

// ========== ESTRUTURAS ==========
struct Nave {
  int x, y;
  int angulo;
  float velTiroX, velTiroY;
};

struct Asteroide {
  int x, y;
  float velX, velY;
  bool ativo;
  int tipo;   // não usado, mantido para compatibilidade
};

struct Tiro {
  float x, y;
  float velX, velY;
  bool ativo;
};

// ========== OBJETOS GLOBAIS ==========
TFT_eSPI tft = TFT_eSPI();

// Sprite de tela inteira (16 bpp)
TFT_eSprite telaBuffer = TFT_eSprite(&tft);

// Sprites individuais (usados como "carimbos")
TFT_eSprite spriteNave = TFT_eSprite(&tft);
TFT_eSprite spriteAsteroide = TFT_eSprite(&tft);
TFT_eSprite spriteTiro = TFT_eSprite(&tft);

// Dados do jogo
Nave nave;
Asteroide asteroides[MAX_ASTEROIDES];
Tiro tiros[MAX_TIROS];

int score = 0;
int vidas = 3;
int dificuldade = 1;
unsigned long startTimeJogo = 0;
unsigned long ultimoSpawnAsteroide = 0;
unsigned long ultimoTiro = 0;
unsigned long ultimoFrame = 0;

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(500);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Cria sprites individuais
  spriteNave.createSprite(NAVE_SIZE, NAVE_SIZE);
  spriteAsteroide.createSprite(ASTEROIDE_SIZE, ASTEROIDE_SIZE);
  spriteTiro.createSprite(TIRO_SIZE, TIRO_SIZE);

  // Cria sprite de tela inteira (16 bpp) – sem paleta
  telaBuffer.createSprite(SCREEN_W, SCREEN_H);

  pinMode(PIN_POT, INPUT);

  // Posição inicial da nave (centro)
  nave.x = SCREEN_W / 2 - NAVE_SIZE / 2;
  nave.y = SCREEN_H / 2 - NAVE_SIZE / 2;
  nave.angulo = 0;

  // Desativa todos os asteroides e tiros
  for (int i = 0; i < MAX_ASTEROIDES; i++) asteroides[i].ativo = false;
  for (int i = 0; i < MAX_TIROS; i++) tiros[i].ativo = false;

  startTimeJogo = millis();
  Serial.println("Jogo iniciado!");
}

// ========== LOOP PRINCIPAL ==========
void loop() {
  unsigned long agora = millis();
  if (agora - ultimoFrame < FRAME_TIME_MS) return;
  ultimoFrame = agora;

  if (vidas <= 0) {
    gameOver();
    return;
  }

  lerPotenciometro();
  calcularDirecaoTiro();
  dispararTiro();
  moverTiros();
  moverAsteroides();
  gerarAsteroides();
  atualizarDificuldade();
  verificarColisoes();

  desenharTudo();
}

// ========== ENTRADA ==========
void lerPotenciometro() {
  int leitura = analogRead(PIN_POT);
  nave.angulo = map(leitura, 0, 4095, -120, 120);
}

// ========== ATUALIZAÇÃO ==========
void calcularDirecaoTiro() {
  float rad = (nave.angulo + 90) * PI / 180.0;
  float vel = 5.0;
  nave.velTiroX = cos(rad) * vel;
  nave.velTiroY = sin(rad) * vel;
}

void dispararTiro() {
  unsigned long agora = millis();
  if (agora - ultimoTiro < TIRO_COOLDOWN_MS) return;
  ultimoTiro = agora;

  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) {
      tiros[i].x = nave.x + NAVE_SIZE / 2.0;
      tiros[i].y = nave.y + NAVE_SIZE / 2.0;
      tiros[i].velX = nave.velTiroX;
      tiros[i].velY = nave.velTiroY;
      tiros[i].ativo = true;
      break;
    }
  }
}

void moverTiros() {
  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) continue;
    tiros[i].x += tiros[i].velX;
    tiros[i].y += tiros[i].velY;
    if (tiros[i].x < 0 || tiros[i].x > SCREEN_W ||
        tiros[i].y < 0 || tiros[i].y > SCREEN_H)
      tiros[i].ativo = false;
  }
}

void moverAsteroides() {
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (!asteroides[i].ativo) continue;
    asteroides[i].x += asteroides[i].velX;
    asteroides[i].y += asteroides[i].velY;
    // Wrap-around
    if (asteroides[i].x < -ASTEROIDE_SIZE) asteroides[i].x = SCREEN_W;
    if (asteroides[i].x > SCREEN_W) asteroides[i].x = -ASTEROIDE_SIZE;
    if (asteroides[i].y < -ASTEROIDE_SIZE) asteroides[i].y = SCREEN_H;
    if (asteroides[i].y > SCREEN_H) asteroides[i].y = -ASTEROIDE_SIZE;
  }
}

void gerarAsteroides() {
  int intervalo = BASE_SPAWN_INTERVAL_MS - (dificuldade * 150);
  if (intervalo < MIN_SPAWN_INTERVAL_MS) intervalo = MIN_SPAWN_INTERVAL_MS;
  if (millis() - ultimoSpawnAsteroide < intervalo) return;
  ultimoSpawnAsteroide = millis();

  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (asteroides[i].ativo) continue;
    int lado = random(4);
    switch (lado) {
      case 0:
        asteroides[i].x = random(SCREEN_W);
        asteroides[i].y = -ASTEROIDE_SIZE;
        break;
      case 1:
        asteroides[i].x = SCREEN_W;
        asteroides[i].y = random(SCREEN_H);
        break;
      case 2:
        asteroides[i].x = random(SCREEN_W);
        asteroides[i].y = SCREEN_H;
        break;
      case 3:
        asteroides[i].x = -ASTEROIDE_SIZE;
        asteroides[i].y = random(SCREEN_H);
        break;
    }
    float velBase = 1.5 + (dificuldade * 0.2);
    asteroides[i].velX = (random(-100, 101) / 50.0) * velBase;
    asteroides[i].velY = (random(-100, 101) / 50.0) * velBase;
    if (asteroides[i].velX == 0 && asteroides[i].velY == 0) asteroides[i].velX = 1;
    asteroides[i].tipo = random(3);
    asteroides[i].ativo = true;
    break;
  }
}

void atualizarDificuldade() {
  unsigned long segundos = (millis() - startTimeJogo) / 1000;
  dificuldade = 1 + (segundos / 30);
  if (dificuldade > 10) dificuldade = 10;
}

// ========== COLISÕES ==========
float distancia(float x1, float y1, float x2, float y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return sqrt(dx*dx + dy*dy);
}

void verificarColisoes() {
  // Tiros vs Asteroides
  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) continue;
    for (int j = 0; j < MAX_ASTEROIDES; j++) {
      if (!asteroides[j].ativo) continue;
      float cxT = tiros[i].x;
      float cyT = tiros[i].y;
      float cxA = asteroides[j].x + ASTEROIDE_SIZE/2.0;
      float cyA = asteroides[j].y + ASTEROIDE_SIZE/2.0;
      if (distancia(cxT, cyT, cxA, cyA) < 12) {
        tiros[i].ativo = false;
        asteroides[j].ativo = false;
        score += 100 * dificuldade;
      }
    }
  }

  // Nave vs Asteroides
  float cxN = nave.x + NAVE_SIZE/2.0;
  float cyN = nave.y + NAVE_SIZE/2.0;
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (!asteroides[i].ativo) continue;
    float cxA = asteroides[i].x + ASTEROIDE_SIZE/2.0;
    float cyA = asteroides[i].y + ASTEROIDE_SIZE/2.0;
    if (distancia(cxN, cyN, cxA, cyA) < 15) {
      asteroides[i].ativo = false;
      vidas--;
    }
  }
}

// ========== DESENHO (Double Buffer 16 bpp) ==========
void desenharTudo() {
  // Limpa o buffer com preto
  telaBuffer.fillSprite(TFT_BLACK);

  // Borda branca
  telaBuffer.drawRect(0, 0, SCREEN_W-1, SCREEN_H-1, TFT_WHITE);

  // Desenha asteroides (cada sprite é copiado por completo, sem transparência)
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (!asteroides[i].ativo) continue;
    // Prepara o sprite do asteroide
    spriteAsteroide.fillSprite(TFT_BLACK);
    spriteAsteroide.fillCircle(ASTEROIDE_SIZE/2, ASTEROIDE_SIZE/2, ASTEROIDE_SIZE/2 - 2, TFT_DARKGREY);
    spriteAsteroide.drawCircle(ASTEROIDE_SIZE/2, ASTEROIDE_SIZE/2, ASTEROIDE_SIZE/2 - 2, TFT_WHITE);
    spriteAsteroide.fillCircle(ASTEROIDE_SIZE/3, ASTEROIDE_SIZE/3, 2, TFT_BLACK);
    spriteAsteroide.fillCircle(2*ASTEROIDE_SIZE/3, 2*ASTEROIDE_SIZE/3, 2, TFT_BLACK);
    // Copia para o buffer (sem parâmetro de transparência)
    spriteAsteroide.pushToSprite(&telaBuffer, asteroides[i].x, asteroides[i].y);
  }

  // Desenha tiros
  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) continue;
    spriteTiro.fillSprite(TFT_BLACK);
    spriteTiro.fillCircle(TIRO_SIZE/2, TIRO_SIZE/2, TIRO_SIZE/2 - 1, TFT_YELLOW);
    spriteTiro.pushToSprite(&telaBuffer, (int)tiros[i].x - TIRO_SIZE/2, (int)tiros[i].y - TIRO_SIZE/2);
  }

  // Desenha nave
  spriteNave.fillSprite(TFT_BLACK);
  int cx = NAVE_SIZE/2;
  spriteNave.fillTriangle(cx, 2, 2, NAVE_SIZE-3, NAVE_SIZE-3, cx, TFT_GREEN);
  spriteNave.fillTriangle(cx, 2, NAVE_SIZE-3, NAVE_SIZE-3, cx, NAVE_SIZE-3, TFT_GREEN);
  spriteNave.fillCircle(cx, cx-2, 2, TFT_WHITE);
  spriteNave.pushToSprite(&telaBuffer, nave.x, nave.y);

  // Desenha HUD diretamente no buffer (fundo transparente)
  telaBuffer.setTextColor(TFT_WHITE, TFT_BLACK);
  telaBuffer.setTextSize(1);
  telaBuffer.setCursor(5, 5);
  telaBuffer.print("SCORE: ");
  telaBuffer.print(score);
  telaBuffer.setCursor(SCREEN_W - 70, 5);
  telaBuffer.print("VIDAS: ");
  telaBuffer.print(vidas);
  telaBuffer.setCursor(SCREEN_W/2 - 30, 5);
  telaBuffer.print("DIFIC: ");
  telaBuffer.print(dificuldade);

  // Transfere o buffer completo para a tela (em uma única rajada SPI)
  tft.startWrite();
  telaBuffer.pushSprite(0, 0);
  tft.endWrite();
}

// ========== GAME OVER ==========
void gameOver() {
  telaBuffer.fillSprite(TFT_BLACK);
  telaBuffer.setTextColor(TFT_RED, TFT_BLACK);
  telaBuffer.setTextSize(3);
  telaBuffer.setCursor(SCREEN_W/2 - 70, SCREEN_H/2 - 30);
  telaBuffer.print("GAME OVER");
  telaBuffer.setTextSize(1);
  telaBuffer.setTextColor(TFT_WHITE, TFT_BLACK);
  telaBuffer.setCursor(SCREEN_W/2 - 50, SCREEN_H/2 + 10);
  telaBuffer.print("FINAL SCORE: ");
  telaBuffer.print(score);
  telaBuffer.pushSprite(0, 0);

  delay(5000);

  // Reinicia todas as variáveis
  vidas = 3;
  score = 0;
  dificuldade = 1;
  startTimeJogo = millis();
  ultimoSpawnAsteroide = 0;
  ultimoTiro = 0;
  for (int i = 0; i < MAX_ASTEROIDES; i++) asteroides[i].ativo = false;
  for (int i = 0; i < MAX_TIROS; i++) tiros[i].ativo = false;
  nave.x = SCREEN_W/2 - NAVE_SIZE/2;
  nave.y = SCREEN_H/2 - NAVE_SIZE/2;
}