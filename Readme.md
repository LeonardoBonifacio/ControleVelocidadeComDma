# Controlador de Ventilador com Matriz LED WS2812 e Display OLED

Este projeto implementa um **controlador de ventilador de 3 velocidades** utilizando o microcontrolador RP2040 (Raspberry Pi Pico), com interface visual completa via:
- **Matriz 5x5 de LEDs RGB (WS2812)** — Exibe o número da velocidade (0-3)
- **LED RGB independente** — Sincronizado com as cores da matriz
- **Display OLED SSD1306** — Mostra estado e velocidade do ventilador
- **Comunicação via PIO + DMA** — Eficiente transferência de dados para os LEDs

O sistema utiliza **DMA para otimizar a transferência** de pixels para o PIO, deixando a CPU livre para processar os botões e lógica de controle.


## Funcionalidades

* **Controle por Botões:**
  * **Botão A (GPIO 5):** Alterna o ventilador entre ligado/desligado
  * **Botão B (GPIO 6):** Altera a velocidade (1 → 2 → 3 → 1) apenas quando ligado
  
* **Matriz 5x5 RGB (WS2812 no GPIO 7):**
  * Exibe o número **0** quando ventilador está desligado (azul fraco)
  * Exibe números **1, 2, 3** conforme a velocidade (verde, laranja, vermelho)
  * Transferência via **PIO + DMA** para otimizar latência

* **LED RGB Independente (GPIOs 11, 12, 13):**
  * **GPIO 11 (Verde)** — Velocidade 1
  * **GPIO 12 (Azul)** — Desligado
  * **GPIO 13 (Vermelho)** — Velocidades 2 e 3
  * Sincronizado em tempo real com a matriz

* **Display OLED SSD1306 (I2C1 - GPIOs 14/15):**
  * Exibe "VENTILADOR DESLIGADO" ou "VENTILADOR LIGADO"
  * Mostra a velocidade atual (0, 1, 2 ou 3)
  * Atualiza imediatamente ao pressionar os botões

* **Debounce via Software:**
  * 250ms de debounce para evitar múltiplas detecções


## Pinagem

| Componente | GPIO | Função |
|-----------|------|--------|
| Botão A | 5 | Entrada (pull-up) |
| Botão B | 6 | Entrada (pull-up) |
| Matriz LED WS2812 | 7 | PIO Output |
| LED RGB - Verde | 11 | GPIO Output |
| LED RGB - Azul | 12 | GPIO Output |
| LED RGB - Vermelho | 13 | GPIO Output |
| OLED SDA | 14 | I2C1 |
| OLED SCL | 15 | I2C1 |


## Detalhes Técnicos

### Arquitetura de Comunicação

1. **Matriz WS2812 via PIO + DMA:**
   - O programa PIO recebe pixels no formato GRB (Green-Red-Blue)
   - Um canal DMA transfere o buffer `pixel_buffer[25]` para o FIFO do PIO
   - Frequência de clock: 800 kHz
   - Operação não-bloqueante com `dma_channel_wait_for_finish_blocking()`

2. **Display OLED via I2C + Inicialização:**
   - I2C1 em 400 kHz
   - Configuração completa do SSD1306: contraste, direção, clock, charge pump
   - Função `display_fan_status()` atualiza tela em tempo real
   - Buffer de pixels gerenciado pela biblioteca `ssd1306.c`

3. **Lógica de Controle:**
   - **Estado global:** `fan_on` (bool), `speed` (1..3)
   - **Detecção de botão:** leitura de GPIO com validação de tempo (debounce)
   - **Sincronização:** ao mudar estado, atualiza matriz, LED RGB e display OLED simultaneamente


## Estrutura de Buffers

### Mapas de Dígitos (5x5)

Cada número é representado como um array booleano de 25 elementos (5 linhas × 5 colunas):
```c
bool numero_0[25] = { /* 0 em padrão de LED */ };
bool numero_1[25] = { /* 1 em padrão de LED */ };
bool numero_2[25] = { /* 2 em padrão de LED */ };
bool numero_3[25] = { /* 3 em padrão de LED */ };
```

### Buffer de Pixels

```c
uint32_t pixel_buffer[25];  // Preenchido por fill_number_buffer()
```

Cada entrada contém a cor em formato GRB: `(G<<24 | R<<8 | B)`


## Fluxo de Execução

1. **Inicialização:**
   - GPIO dos botões, LED RGB
   - I2C e display OLED com configuração completa
   - PIO para WS2812
   - DMA para transferência de pixels

2. **Loop Principal:**
   - Leitura contínua dos botões com debounce
   - Se botão pressionado: atualiza estado
   - Chama `fill_number_buffer()` com novo número/cor
   - Chama `set_rgb_led()` para sincronizar LED RGB
   - Chama `display_fan_status()` para atualizar OLED
   - Inicia transferência DMA do pixel_buffer para o PIO
   - Aguarda conclusão com `dma_channel_wait_for_finish_blocking()`


## Compilação e Upload

```bash
# Compilar
cd build
cmake ..
make

# Ou usar task do VS Code:
Ctrl+Shift+B → "Compile Project"

# Upload
make -j4
# Ou task: "Run Project" / "Flash"
```


## Dependências

### Hardware
- Raspberry Pi Pico (RP2040)
- Matriz 5x5 WS2812 (endereçável)
- LED RGB comum com resistores (ou mosfets para maior corrente)
- Display OLED SSD1306 (128x64) com I2C
- 2× Botões tácteis
- Fios de conexão e fonte de alimentação

### Software
- Pico SDK 2.1.1 (ou superior)
- CMake 3.13+
- GCC Arm para RP2040
- Python (para ferramentas de build)


