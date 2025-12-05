#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/i2c.h" 
#include "lib/ssd1306.h"    // Para desenhar no diplay oled
#include "lib/font.h"  

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2812.pio.h"


// Para conexão i2c e configuração do display oled
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C // Endereço fisico display ssd1306
ssd1306_t ssd;


// Definição do número de LEDs e pinos.
#define LED_COUNT 25
#define MATRIZ_LED_PIN 7
#define BUTTON_A 5
#define BUTTON_B 6
#define DEBOUNCE_DELAY_MS 250  // Tempo de debounce em milissegundos

// Pinos do LED RGB
#define RGB_GREEN 11
#define RGB_BLUE 12
#define RGB_RED 13



// Função para exibir mensagens na tela 
void display_fan_status(ssd1306_t *ssd, bool fan_on, int speed)
{
    ssd1306_fill(ssd, false);
    ssd1306_rect(ssd, 0, 0, 127, 63, true, false);
    
    if (!fan_on)
    {
        ssd1306_draw_string(ssd, "VENTILADOR", 20, 10);
        ssd1306_draw_string(ssd, "DESLIGADO", 25, 30);
        ssd1306_draw_string(ssd, "Velocidade: 0", 20, 50);
    }
    else
    {
        ssd1306_draw_string(ssd, "VENTILADOR", 20, 10);
        ssd1306_draw_string(ssd, "LIGADO", 35, 30);
        
        char speed_str[20];
        snprintf(speed_str, sizeof(speed_str), "Velocidade: %d", speed);
        ssd1306_draw_string(ssd, speed_str, 20, 50);
    }
    
    ssd1306_send_data(ssd);
}

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 0
bool numero_0[LED_COUNT] = {
    0, 1, 1, 1, 1,
    1, 0, 0, 1, 0,
    0, 1, 0, 0, 1,
    1, 0, 0, 1, 0,
    0, 1, 1, 1, 1
};


// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 1
bool numero_1[LED_COUNT] = {
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 1,
    0, 1, 1, 0, 0,
    0, 0, 1, 0, 0
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 2
bool numero_2[LED_COUNT] = {
    0, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    0, 1, 1, 1, 1,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 1
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 3
bool numero_3[LED_COUNT] = {
    0, 1, 1, 1, 1,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 1,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 1
};



static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    // conserva o formato do exemplo original: retorna (r<<8)|(g<<16)|b
    return ((uint32_t)r << 8) | ((uint32_t)g << 16) | (uint32_t)b;
}

// Buffer de pixels que será transferido via DMA para o PIO (cada entry já no formato correto para o PIO)
static uint32_t pixel_buffer[LED_COUNT];

// Atualiza pixel_buffer com o mapa do número e cor desejada
void fill_number_buffer(bool numero_a_ser_desenhado[], uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t val = urgb_u32(r, g, b);
    for (int i = 0; i < LED_COUNT; i++)
    {
        if (numero_a_ser_desenhado[i])
            pixel_buffer[i] = val << 8; // o PIO example costuma esperar os dados deslocados em 8 bits
        else
            pixel_buffer[i] = 0;
    }
}

// Função para iniciar uma transferência DMA que envia o buffer para o PIO SM
int start_dma_transfer(PIO pio, int sm, int dma_chan)
{
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));

    void *pio_txf = (void *)&pio->txf[sm];
    dma_channel_configure(dma_chan, &c, pio_txf, pixel_buffer, LED_COUNT, true);
    return dma_chan;
}

// Função para controlar o LED RGB sincronizado com a matriz
void set_rgb_led(uint8_t r, uint8_t g, uint8_t b)
{
    gpio_put(RGB_GREEN, g > 0 ? 1 : 0);
    gpio_put(RGB_BLUE, b > 0 ? 1 : 0);
    gpio_put(RGB_RED, r > 0 ? 1 : 0);
}







int main()
{
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A,GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B,GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Inicializa pinos do LED RGB
    gpio_init(RGB_GREEN);
    gpio_set_dir(RGB_GREEN, GPIO_OUT);
    gpio_put(RGB_GREEN, 0);

    gpio_init(RGB_BLUE);
    gpio_set_dir(RGB_BLUE, GPIO_OUT);
    gpio_put(RGB_BLUE, 0);

    gpio_init(RGB_RED);
    gpio_set_dir(RGB_RED, GPIO_OUT);
    gpio_put(RGB_RED, 0);

    stdio_init_all();
    sleep_ms(2000); // Aguarde a inicialização da serial

    // Inicializa I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    sleep_ms(100);

    // Inicializa display OLED SSD1306
    ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT);
    sleep_ms(50);
    ssd1306_config(&ssd);  // IMPORTANTE: configurar o display após inicializar
    sleep_ms(50);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    sleep_ms(100);

    // Váriaveis e configurações PIO
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, MATRIZ_LED_PIN, 800000, false);

    // Reivindica um canal DMA para transferir os pixels
    int dma_chan = dma_claim_unused_channel(true);

    // Estado do ventilador
    bool fan_on = false;
    int speed = 1; // 1..3

    // Inicializa display com 0 (desligado)
    fill_number_buffer(numero_0, 0, 0, 40); // dim azul
    set_rgb_led(0, 0, 40);
    display_fan_status(&ssd, false, 0);
    start_dma_transfer(pio, sm, dma_chan);
    dma_channel_wait_for_finish_blocking(dma_chan);
    sleep_ms(1);

    // Debounce
    absolute_time_t last_a = get_absolute_time();
    absolute_time_t last_b = get_absolute_time();

    while (true)
    {
        bool a_pressed = !gpio_get(BUTTON_A); // pull-up ativo
        bool b_pressed = !gpio_get(BUTTON_B);
        absolute_time_t now = get_absolute_time();

        if (a_pressed && absolute_time_diff_us(last_a, now) / 1000 > DEBOUNCE_DELAY_MS)
        {
            // Alterna ligado/desligado
            fan_on = !fan_on;
            last_a = now;
            if (!fan_on)
            {
                fill_number_buffer(numero_0, 0, 0, 40);
                set_rgb_led(0, 0, 40);
                display_fan_status(&ssd, false, 0);
            }
            else
            {
                // Ao ligar, mostra velocidade atual
                if (speed == 1)
                {
                    fill_number_buffer(numero_1, 0, 255, 0);
                    set_rgb_led(0, 255, 0);
                    display_fan_status(&ssd, true, 1);
                }
                else if (speed == 2)
                {
                    fill_number_buffer(numero_2, 255, 160, 0);
                    set_rgb_led(255, 160, 0);
                    display_fan_status(&ssd, true, 2);
                }
                else
                {
                    fill_number_buffer(numero_3, 255, 0, 0);
                    set_rgb_led(255, 0, 0);
                    display_fan_status(&ssd, true, 3);
                }
            }
            start_dma_transfer(pio, sm, dma_chan);
            dma_channel_wait_for_finish_blocking(dma_chan);
            sleep_ms(1);
        }

        if (b_pressed && absolute_time_diff_us(last_b, now) / 1000 > DEBOUNCE_DELAY_MS)
        {
            last_b = now;
            // altera velocidade apenas quando ligado
            if (fan_on)
            {
                speed++;
                if (speed > 3)
                    speed = 1;

                if (speed == 1)
                {
                    fill_number_buffer(numero_1, 0, 255, 0);
                    set_rgb_led(0, 255, 0);
                    display_fan_status(&ssd, true, 1);
                }
                else if (speed == 2)
                {
                    fill_number_buffer(numero_2, 255, 160, 0);
                    set_rgb_led(255, 160, 0);
                    display_fan_status(&ssd, true, 2);
                }
                else
                {
                    fill_number_buffer(numero_3, 255, 0, 0);
                    set_rgb_led(255, 0, 0);
                    display_fan_status(&ssd, true, 3);
                }

                start_dma_transfer(pio, sm, dma_chan);
                dma_channel_wait_for_finish_blocking(dma_chan);
                sleep_ms(1);
            }
            else
            {
                // se desligado, apenas atualiza variável speed (não mostra na matriz)
                speed++;
                if (speed > 3)
                    speed = 1;
            }
        }

        tight_loop_contents();
    }
}