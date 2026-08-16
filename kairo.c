#include <asm-generic/ioctls.h>
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

// parametros ajustaveis
#define MAX_CIRCULOS 60
#define LARGURA_PADRAO 100
#define ALTURA_PADRAO 36
#define PROB_NASCIMENTO 6
#define RAIO_INICIAL 0.8
#define ATRACAO 0.010
#define JITTER 0.045
#define VEL_MAX 0.35
#define FATOR_TOQUE 0.9
#define FRAME_NS 45000000L

typedef struct {
  double x, y;
  double vx, vy;
  double r;
  int vivo;
} Circulo;

static Circulo circulos[MAX_CIRCULOS];
static int largura = LARGURA_PADRAO;
static int altura = ALTURA_PADRAO;
static char *tela = NULL;
static volatile sig_atomic_t rodando = 1;

// Utilidades

static double aleatorio01(void) { return (double)rand() / (double)RAND_MAX; }

static double aleatorio(double min, double max) {
  return min + aleatorio01() * (max - min);
}

static void tratar_sigint(int sinal) {
  (void)sinal;
  rodando = 0;
}

static void detectar_tamanho_terminal(void) {
  struct winsize w;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 10 &&
      w.ws_row > 10) {
    largura = w.ws_col;
    altura = w.ws_row - 1;
  }
}

// ciclo de vida dos circulos

static int contar_vivos(void) {
  int n = 0;
  for (int i = 0; i < MAX_CIRCULOS; i++) {
    if (circulos[i].vivo)
      n++;
  }
  return n;
}

static void nascer_circulo(void) {
  for (int i = 0; i < MAX_CIRCULOS; i++) {
    if (!circulos[i].vivo) {
      circulos[i].x = aleatorio(2.0, largura - 2.0);
      circulos[i].y = aleatorio(2.0, altura - 2.0);
      circulos[i].vx = aleatorio(-VEL_MAX, VEL_MAX);
      circulos[i].vy = aleatorio(-VEL_MAX, VEL_MAX);
      circulos[i].r = RAIO_INICIAL;
      circulos[i].vivo = 1;
      return;
    }
  }
}
