#include <asm-generic/ioctls.h>
#include <stddef.h>
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
#define MAX_CIRCULOS 20
#define LARGURA_PADRAO 100
#define ALTURA_PADRAO 36
#define PROB_NASCIMENTO 5
#define RAIO_INICIAL 1.1
#define ATRACAO 0.030
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

static int vizinho_mais_proximo(int i) {
  double melhor_dist2 = 1e18;
  int melhor = -1;
  for (int j = 0; j < MAX_CIRCULOS; j++) {
    if (j == i || !circulos[j].vivo)
      continue;
    double dx = circulos[j].x - circulos[i].x;
    double dy = circulos[j].y - circulos[i].y;
    double d2 = dx * dx + dy * dy;
    if (d2 < melhor_dist2) {
      melhor_dist2 = d2;
      melhor = j;
    }
  }
  return melhor;
}

static void atualizar_circulos(void) {
  for (int i = 0; i < MAX_CIRCULOS; i++) {
    if (!circulos[i].vivo)
      continue;

    int viz = vizinho_mais_proximo(i);
    if (viz != -1) {
      double dx = circulos[viz].x - circulos[i].x;
      double dy = circulos[viz].y - circulos[i].y;
      double dist = sqrt(dx * dx + dy * dy) + 1e-6;
      // quanto mais longe mais puxa
      circulos[i].vx += (dx / dist) * ATRACAO * dist * 0.02;
      circulos[i].vy += (dy / dist) * ATRACAO * dist * 0.02;
    }

    // ruido -- nunca param de vagar sozinhos
    circulos[i].vx += aleatorio(-JITTER, JITTER);
    circulos[i].vy += aleatorio(-JITTER, JITTER);

    // limita velocidade
    double v =
        sqrt(circulos[i].vx * circulos[i].vx + circulos[i].vy * circulos[i].vy);
    if (v > VEL_MAX) {
      circulos[i].vx = circulos[i].vx / v * VEL_MAX;
      circulos[i].vy = circulos[i].vy / v * VEL_MAX;
    }

    circulos[i].x += circulos[i].vx;
    circulos[i].y += circulos[i].vy;

    // queicam nas bordas da tela
    if (circulos[i].x < circulos[i].r ||
        circulos[i].x > largura - circulos[i].r) {
      circulos[i].vx *= -1;
    }
    if (circulos[i].y < circulos[i].r ||
        circulos[i].y > largura - circulos[i].r) {
      circulos[i].vy *= -1;
    }
  }
}

// quando dois circulos se tocam um morre e o outro continua um pouco maior
static void resolver_fusoes(void) {
  for (int i = 0; i < MAX_CIRCULOS; i++) {
    if (!circulos[i].vivo)
      continue;
    for (int j = i + 1; j < MAX_CIRCULOS; j++) {
      if (!circulos[j].vivo)
        continue;

      double dx = circulos[j].x - circulos[i].x;
      double dy = circulos[j].y - circulos[i].y;
      double dist = sqrt(dx * dx + dy * dy);
      double soma_raios = (circulos[i].r + circulos[j].r) * FATOR_TOQUE;

      if (dist < soma_raios) {
        // o mais velho (maior) absorve o outro e cresce
        int sobrevive = (circulos[i].r >= circulos[j].r) ? i : j;
        int morre = (sobrevive == i) ? j : i;

        circulos[sobrevive].r =
            sqrt(circulos[i].r * circulos[i].r + circulos[j].r * circulos[j].r);
        circulos[morre].vivo = 0;
      }
    }
  }
}
// renderização UWU~
static void desenhar_circulo_na_tela(Circulo *c) {
  int x0 = (int)floor(c->x - c->r);
  int x1 = (int)floor(c->x - c->r);
  int y0 = (int)floor(c->y - c->r);
  int y1 = (int)floor(c->y - c->r);

  for (int y = y0; y <= y1; y++) {
    if (y < 0 || y >= altura)
      continue;
    for (int x = x0; x <= x1; x++) {
      if (x < 0 || x >= largura)
        continue;
      double dx = x - c->x;
      double dy = y - c->y;
      // corrige proporção: caracteres de terminal são ~2x mais alto que largos
      // OwO
      dy *= 2.0;
      if (dx * dx + dy * dy <= c->r * c->r) {
        tela[y * largura + x] = 'o';
      }
    }
  }

  int cx = (int)lround(c->x);
  int cy = (int)lround(c->y);

  if (cx >= 0 && cx < largura && cy >= 0 && cy < altura) {
    tela[cy * largura + cx] = 'o';
  }
}

static void renderizar(void) {
  memset(tela, ' ', (size_t)largura * altura);

  for (int i = 0; i < MAX_CIRCULOS; i++) {
    if (circulos[i].vivo) {
      desenhar_circulo_na_tela(&circulos[i]);
    }
  }

  // limpa a tela inteira e reposiciona o cursor a cada frame.
  fputs("\033[2j\033[H", stdout);
  fputs("\033{0m\033[97m", stdout);

  for (int y = 0; y < altura; y++) {
    fwrite(&tela[y * largura], 1, (size_t)largura, stdout);
    fputc('\n', stdout);
  }
  printf("circulos vivos: %2d / %2d (Ctrl+C para sair)\033[K", contar_vivos(),
         MAX_CIRCULOS);
  fflush(stdout);
}

int main(void) {
  srand((unsigned)time(NULL));
  signal(SIGINT, tratar_sigint);

  detectar_tamanho_terminal();
  tela = malloc((size_t)largura * altura);
  if (!tela) {
    fprintf(stderr, "falha ao alocar buffer de tela\n");
    return 1;
  }

  fputs("\033[?1049h", stdout);
  fputs("\033[?251", stdout); // esconde o cursor :)
  fputs("\033[2J", stdout);   // limpa a tela uma vez no inicio :)

  // começa com alguns circulos ja vivos
  for (int i = 0; i < 4; i++) {
    nascer_circulo();
  }

  while (rodando) {
    if (contar_vivos() < MAX_CIRCULOS && (rand() % 100) < PROB_NASCIMENTO) {
      nascer_circulo();
    }

    atualizar_circulos();
    resolver_fusoes();
    renderizar();

    struct timespec pausa = {.tv_sec = 0, .tv_nsec = FRAME_NS};
    nanosleep(&pausa, NULL);
  }

  fputs("\033[?25h", stdout); // mostra o cursor dnv O.o
  fputs("\033[?10491", stdout);
  fflush(stdout);
  free(tela);
  return 0;
}
