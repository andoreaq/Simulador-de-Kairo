# Kairo Pulse

Simulação em C, no terminal, inspirada na cena do file **Pulse / Kairo** (Kiyoshi Kurosawa, 2001), em que a personagem Harue mostra um programa com pontos brancos numa tela escura: eles são atraídos uns pelos outros quando estão longe demais, mas se chegarem perto demais, um deles morre.

> "As pessoas não se conectam de verdade, sabe? Como esses pontos simulando seres humanos. Todos vivemos totalmente separados."

## Como funciona

- Círculos nascem espontaneamente em posições aleatórias da tela.
- Cada círculo é levemente atraído pelo círculo vivo mais próximo. (quanto mais longe, mais forte a atração)
- Quando dois círculos se tocam, um deles morre e o outro absorve seu raio, ficando um pouco maior e continuando a existir.

Tudo é desenhado no terminal usando apenas ANSI escape codes, sem nenhuma biblioteca gráfica externa.

## Requisitos

- Qualquer compilador compatível com C11 (`gcc` ou `clang`)
- Biblioteca matemática padrão do sistema (`libm`)

## Compilar

```bash  
clang -Wall -Wextra -g kairo.c -o harue -lm 
```



## Executar


```
./harue
```

Para sair, use `Ctrl+c`


