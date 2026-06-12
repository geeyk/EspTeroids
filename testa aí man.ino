// =========================================================
//  EspTeroids - Versão Polida
//  Com sprites customizados, controles ajustados
// =========================================================

#include <TFT_eSPI.h>
#include <SPI.h>
#include "sprites_data.h"  // Arquivo com sprites convertidos

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spriteNave = TFT_eSprite(&tft);
TFT_eSprite spriteAsteroide = TFT_eSprite(&tft);
TFT_eSprite spriteTiro = TFT_eSprite(&tft);

// ==================== CONSTANTES ====================
const int NAVE_SIZE = 20;
const int ASTEROIDE_SIZE = 20;
const int TIRO_SIZE = 4;
const int POT_PIN = 33;  // ⭐ Mudado para GPIO 33
const int MAX_ASTEROIDES = 8;
const int MAX_TIROS = 15;
const int PONTOS_EXTRA_VIDA = 2000;

// ==================== ESTRUTURAS ====================
struct Nave {
  int x, y;
  int angulo; // -120 até +120 (240 graus total)
  int velXTiro, velYTiro;
};

struct Asteroide {
  int x, y;
  int velX, velY;
  bool ativo;
  int tipo; // 0, 1, 2 (3 variações)
};

struct Tiro {
  int x, y;
  float velX, velY;
  bool ativo;
};

// ==================== VARIÁVEIS GLOBAIS ====================
Nave nave;
Asteroide asteroides[MAX_ASTEROIDES];
Tiro tiros[MAX_TIROS];

int score = 0;
int vidas = 3;
int ultimaVida = 0;
int dificuldade = 1;
unsigned long startTime = 0;

unsigned long lastAsteroideTime = 0;
unsigned long lastTiroTime = 0;
unsigned long lastFrameTime = 0;
const int FRAME_TIME = 20; // ~50 FPS

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  // Inicializar display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  // Inicializar sprites
  spriteNave.createSprite(NAVE_SIZE, NAVE_SIZE);
  spriteAsteroide.createSprite(ASTEROIDE_SIZE, ASTEROIDE_SIZE);
  spriteTiro.createSprite(TIRO_SIZE, TIRO_SIZE);
  
  // Inicializar nave
  nave.x = tft.width() / 2;
  nave.y = tft.height() / 2;
  nave.angulo = 0;
  
  // Inicializar asteroides e tiros
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    asteroides[i].ativo = false;
  }
  for (int i = 0; i < MAX_TIROS; i++) {
    tiros[i].ativo = false;
  }
  
  pinMode(POT_PIN, INPUT);
  startTime = millis();
  
  Serial.println("EspTeroids iniciado!");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  unsigned long now = millis();
  if (now - lastFrameTime < FRAME_TIME) return;
  lastFrameTime = now;
  
  // Verificar game over
  if (vidas <= 0) {
    gameOver();
    return;
  }
  
  // Ler entrada
  lerPotenciometro();
  
  // Atualizar
  atualizarNave();
  dispararTiro();
  atualizarTiros();
  atualizarAsteroides();
  gerarAsteroides();
  atualizarDificuldade();
  
  // Verificar colisões
  verificarColisoes();
  
  // Desenhar (começa limpo)
  tft.fillScreen(TFT_BLACK);
  desenharBorda();
  desenharAsteroides();
  desenharTiros();
  desenharNave();
  
  // HUD separado (não pisca)
  desenharHUD();
}

// ==================== ENTRADA ====================
void lerPotenciometro() {
  int valorPot = analogRead(POT_PIN); // 0-4095
  // Mapear para -120 até +120 graus (240° total)
  nave.angulo = map(valorPot, 0, 4095, -120, 120);
}

// ==================== ATUALIZAÇÃO ====================
void atualizarNave() {
  // Calcular direção do tiro baseado no ângulo
  // +90 porque 0° (centro) aponta para cima
  float rad = (nave.angulo + 90) * PI / 180.0;
  nave.velXTiro = cos(rad) * 3; // velocidade reduzida = 3
  nave.velYTiro = sin(rad) * 3;
}

void atualizarTiros() {
  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) continue;
    
    tiros[i].x += tiros[i].velX;
    tiros[i].y += tiros[i].velY;
    
    // Remover se sair da tela
    if (tiros[i].x < 0 || tiros[i].x > tft.width() ||
        tiros[i].y < 0 || tiros[i].y > tft.height()) {
      tiros[i].ativo = false;
    }
  }
}

void atualizarAsteroides() {
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (!asteroides[i].ativo) continue;
    
    asteroides[i].x += asteroides[i].velX;
    asteroides[i].y += asteroides[i].velY;
    
    // Wrap-around
    if (asteroides[i].x < -ASTEROIDE_SIZE) asteroides[i].x = tft.width();
    if (asteroides[i].x > tft.width()) asteroides[i].x = -ASTEROIDE_SIZE;
    if (asteroides[i].y < -ASTEROIDE_SIZE) asteroides[i].y = tft.height();
    if (asteroides[i].y > tft.height()) asteroides[i].y = -ASTEROIDE_SIZE;
  }
}

void gerarAsteroides() {
  // Aumenta frequência com dificuldade
  int intervalo = 3000 - (dificuldade * 200); // vai diminuindo
  if (intervalo < 800) intervalo = 800; // mínimo
  
  if (millis() - lastAsteroideTime < intervalo) return;
  lastAsteroideTime = millis();
  
  // Encontrar slot livre
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (asteroides[i].ativo) continue;
    
    // Spawn nas bordas
    int lado = random(4);
    
    if (lado == 0) {
      asteroides[i].x = random(tft.width());
      asteroides[i].y = -ASTEROIDE_SIZE;
    } else if (lado == 1) {
      asteroides[i].x = tft.width();
      asteroides[i].y = random(tft.height());
    } else if (lado == 2) {
      asteroides[i].x = random(tft.width());
      asteroides[i].y = tft.height();
    } else {
      asteroides[i].x = -ASTEROIDE_SIZE;
      asteroides[i].y = random(tft.height());
    }
    
    // Velocidade variada mas consistente
    float baseVel = 1.5 + (dificuldade * 0.3); // aumenta com dificuldade
    asteroides[i].velX = (random(-10, 11) / 10.0) * baseVel;
    asteroides[i].velY = (random(-10, 11) / 10.0) * baseVel;
    
    // Garantir que se move
    if (asteroides[i].velX == 0 && asteroides[i].velY == 0) {
      asteroides[i].velX = 1;
    }
    
    // Escolher variação de sprite
    asteroides[i].tipo = random(0, 3);
    asteroides[i].ativo = true;
    
    break;
  }
}

void atualizarDificuldade() {
  unsigned long tempoDecorrido = (millis() - startTime) / 1000; // segundos
  dificuldade = 1 + (tempoDecorrido / 30); // aumenta a cada 30 segundos
  if (dificuldade > 10) dificuldade = 10; // máximo
}

// ==================== DISPARO ====================
void dispararTiro() {
  unsigned long now = millis();
  if (now - lastTiroTime < 150) return; // 150ms entre tiros (mais controlado)
  lastTiroTime = now;
  
  // Encontrar slot livre
  for (int i = 0; i < MAX_TIROS; i++) {
    if (tiros[i].ativo) continue;
    
    tiros[i].x = nave.x + NAVE_SIZE / 2;
    tiros[i].y = nave.y + NAVE_SIZE / 2;
    tiros[i].velX = nave.velXTiro;
    tiros[i].velY = nave.velYTiro;
    tiros[i].ativo = true;
    
    break;
  }
}

// ==================== COLISÕES ====================
void verificarColisoes() {
  // Tiros vs Asteroides
  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) continue;
    
    for (int j = 0; j < MAX_ASTEROIDES; j++) {
      if (!asteroides[j].ativo) continue;
      
      if (distancia(tiros[i].x, tiros[i].y, 
                    asteroides[j].x + ASTEROIDE_SIZE/2, 
                    asteroides[j].y + ASTEROIDE_SIZE/2) < 15) {
        tiros[i].ativo = false;
        asteroides[j].ativo = false;
        score += (100 * dificuldade);
        
        // Recuperar vida
        if (score - ultimaVida >= PONTOS_EXTRA_VIDA) {
          vidas++;
          ultimaVida = score;
        }
      }
    }
  }
  
  // Nave vs Asteroides
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (!asteroides[i].ativo) continue;
    
    if (distancia(nave.x + NAVE_SIZE/2, nave.y + NAVE_SIZE/2,
                  asteroides[i].x + ASTEROIDE_SIZE/2, 
                  asteroides[i].y + ASTEROIDE_SIZE/2) < 18) {
      asteroides[i].ativo = false;
      vidas--;
    }
  }
}

int distancia(int x1, int y1, int x2, int y2) {
  int dx = x2 - x1;
  int dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}

// ==================== DESENHO ====================
void desenharBorda() {
  tft.drawRect(0, 0, tft.width()-1, tft.height()-1, TFT_WHITE);
}

void desenharNave() {
  spriteNave.fillSprite(TFT_BLACK);
  spriteNave.pushImage(0, 0, NAVE_SIZE, NAVE_SIZE, nave_sprite);
  spriteNave.pushSprite(nave.x, nave.y);
}

void desenharAsteroides() {
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    if (!asteroides[i].ativo) continue;
    
    spriteAsteroide.fillSprite(TFT_BLACK);
    
    // Escolher sprite baseado no tipo
    if (asteroides[i].tipo == 0) {
      spriteAsteroide.pushImage(0, 0, ASTEROIDE_SIZE, ASTEROIDE_SIZE, asteroide1_sprite);
    } else if (asteroides[i].tipo == 1) {
      spriteAsteroide.pushImage(0, 0, ASTEROIDE_SIZE, ASTEROIDE_SIZE, asteroide2_sprite);
    } else {
      spriteAsteroide.pushImage(0, 0, ASTEROIDE_SIZE, ASTEROIDE_SIZE, asteroide3_sprite);
    }
    
    spriteAsteroide.pushSprite(asteroides[i].x, asteroides[i].y);
  }
}

void desenharTiros() {
  spriteTiro.fillSprite(TFT_BLACK);
  spriteTiro.fillCircle(TIRO_SIZE / 2, TIRO_SIZE / 2, 2, TFT_CYAN);
  
  for (int i = 0; i < MAX_TIROS; i++) {
    if (!tiros[i].ativo) continue;
    spriteTiro.pushSprite(tiros[i].x - TIRO_SIZE / 2, tiros[i].y - TIRO_SIZE / 2);
  }
}

void desenharHUD() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  
  // Score (canto superior esquerdo)
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.print(score);
  
  // Vidas (canto superior direito)
  tft.setCursor(tft.width() - 70, 5);
  tft.print("Vidas: ");
  tft.print(vidas);
  
  // Dificuldade (centro superior)
  tft.setCursor(tft.width() / 2 - 30, 5);
  tft.print("Onda: ");
  tft.print(dificuldade);
}

// ==================== GAME OVER ====================
void gameOver() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(tft.width() / 2 - 90, tft.height() / 2 - 60);
  tft.print("GAME OVER");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(tft.width() / 2 - 60, tft.height() / 2 - 10);
  tft.print("Score Final:");
  tft.print(score);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(tft.width() / 2 - 40, tft.height() / 2 + 20);
  tft.print("Onda atingida: ");
  tft.print(dificuldade);
  
  delay(5000);
  
  // Reiniciar
  vidas = 3;
  score = 0;
  dificuldade = 1;
  ultimaVida = 0;
  startTime = millis();
  lastAsteroideTime = 0;
  
  for (int i = 0; i < MAX_ASTEROIDES; i++) {
    asteroides[i].ativo = false;
  }
  for (int i = 0; i < MAX_TIROS; i++) {
    tiros[i].ativo = false;
  }
}
