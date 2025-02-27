#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdio.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define BUTTON_SHELF_PIN 5      // Botão A (alternar prateleiras)
#define BUTTON_CONFIRM_PIN 6    // Botão B (adicionar caixa)
#define LED_R_PIN 11            // Pino vermelho do LED RGB
#define LED_G_PIN 12            // Pino verde do LED RGB 
#define LED_B_PIN 13            // Pino azul do LED RGB

// Variável para armazenar o número atual (0 a 9)
volatile int current_number = 0;

// Buffer para armazenar quais LEDs estão ligados na matriz 5x5
volatile bool led_buffer[NUM_PIXELS] = {0};

// Flags para debounce
volatile bool debounce_shelf = false;
volatile bool debounce_confirm = false;

// Tabela de frames para os números de 0 a 9 (matrizes 5x5)
const bool frames[10][NUM_PIXELS] = {
    // Número 0
    {
        1,1,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    // Número 1
    {
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0
    },
    // Número 2
    {
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,1
    },
    // Número 3
    {
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1
    },
    // Número 4
    {
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,0,1
    },
    // Número 5
    {
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1
    },
    // Número 6
    {
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    // Número 7
    {
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0
    },
    // Número 8
    {
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    // Número 9
    {
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,0,1
    }
};

// Variáveis para controle de estoque
volatile int current_shelf = 0;  // Prateleira atual (0 a 2)
volatile int shelf_stock[3] = {0, 0, 0};  // Quantidade de caixas em cada prateleira

// Cores para cada prateleira - cores iguais para LED RGB e matriz WS2812
const uint8_t shelf_colors[3][3] = {
    {255, 0, 0},   // Vermelho para a prateleira 0
    {0, 255, 0},   // Verde para a prateleira 1
    {0, 0, 255}    // Azul para a prateleira 2
};

// Função para atualizar o buffer de LEDs com base no número atual
void update_led_buffer(int number) {
    // Mapeamento correto dos LEDs considerando o padrão zigue-zague
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int index;
            if (row % 2 == 0) {
                // Linhas pares: esquerda para direita
                index = row * 5 + col;
            } else {
                // Linhas ímpares: direita para esquerda
                index = row * 5 + (4 - col);
            }
            led_buffer[index] = frames[number][row * 5 + col];
        }
    }
}

// Função para enviar um pixel para a matriz de LEDs
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função para converter valores de cor RGB em um formato compatível com WS2812
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// Função para atualizar o LED RGB externo
void update_rgb_led() {
    if (shelf_stock[current_shelf] == 9) {
        // Prateleira cheia - acende vermelho e azul (roxo)
        gpio_put(LED_R_PIN, 1);
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_B_PIN, 1);
        printf("Prateleira cheia - LED roxo\n");
    } else {
        // CORREÇÃO: Certifica-se de que as cores correspondam corretamente
        // Para prateleira 0 (Vermelho): LED_R_PIN = 1, outros = 0
        // Para prateleira 1 (Verde): LED_G_PIN = 1, outros = 0
        // Para prateleira 2 (Azul): LED_B_PIN = 1, outros = 0
        if (current_shelf == 0) {         // Prateleira vermelha
            gpio_put(LED_R_PIN, 1);
            gpio_put(LED_G_PIN, 0);
            gpio_put(LED_B_PIN, 0);
        } else if (current_shelf == 1) {  // Prateleira verde
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_G_PIN, 1);
            gpio_put(LED_B_PIN, 0);
        } else if (current_shelf == 2) {  // Prateleira azul
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_G_PIN, 0);
            gpio_put(LED_B_PIN, 1);
        }
        printf("LED da cor da prateleira %d\n", current_shelf);
    }
}

// Função para definir a cor dos LEDs na matriz
void set_matrix_leds() {
    uint32_t color;
    uint32_t black = urgb_u32(0, 0, 0);
    
    // Verifica se a prateleira está cheia
    if (shelf_stock[current_shelf] == 9) {
        // Usa a cor roxa para indicar prateleira cheia
        color = urgb_u32(255, 0, 255); // Roxo (R+B)
    } else {
        // CORREÇÃO: Obtém as cores da prateleira atual
        uint8_t r = shelf_colors[current_shelf][0];
        uint8_t g = shelf_colors[current_shelf][1];
        uint8_t b = shelf_colors[current_shelf][2];
        color = urgb_u32(r, g, b);
    }

    for (int i = 0; i < NUM_PIXELS; i++) {
        if (led_buffer[i]) {
            put_pixel(color); // Liga o LED
        } else {
            put_pixel(black); // Desliga o LED
        }
    }
}

// Funções de debounce usando alarmes (temporizadores)

// Callback para o botão de alternar prateleiras
int64_t shelf_alarm_callback(alarm_id_t id, void *user_data) {
    debounce_shelf = false; // Permite nova leitura do botão
    return 0;
}

// Callback para o botão de adicionar caixa
int64_t confirm_alarm_callback(alarm_id_t id, void *user_data) {
    debounce_confirm = false; // Permite nova leitura do botão
    return 0;
}

// Callback das interrupções de GPIO
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_SHELF_PIN && !debounce_shelf) {
        printf("Botão de prateleira pressionado\n");
        debounce_shelf = true; // Inicia o debounce
        // Inicia temporizador de debounce (200ms)
        add_alarm_in_ms(200, shelf_alarm_callback, NULL, false);
        // Alterna para a próxima prateleira
        current_shelf = (current_shelf + 1) % 3;
        // Atualiza o número exibido para a quantidade na prateleira atual
        current_number = shelf_stock[current_shelf];
        update_led_buffer(current_number);
        // Atualiza o LED RGB
        update_rgb_led();
    } else if (gpio == BUTTON_CONFIRM_PIN && !debounce_confirm) {
        printf("Botão de confirmação pressionado\n");
        debounce_confirm = true; // Inicia o debounce
        // Inicia temporizador de debounce (200ms)
        add_alarm_in_ms(200, confirm_alarm_callback, NULL, false);
        // Adiciona uma caixa na prateleira atual
        if (shelf_stock[current_shelf] < 9) {
            shelf_stock[current_shelf]++;
            current_number = shelf_stock[current_shelf];
            update_led_buffer(current_number);
            // Atualiza o LED RGB - pode ter atingido o máximo
            update_rgb_led();
        }
    }
}

int main() {
    // Inicializa stdio para debug
    stdio_init_all();
    printf("Iniciando o programa de controle de estoque\n");
    
    // Inicializa o PIO e o programa WS2812
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Inicializa o buffer de LEDs com o número 0
    update_led_buffer(current_number);

    // Configura os pinos dos botões como entrada
    gpio_init(BUTTON_SHELF_PIN);
    gpio_set_dir(BUTTON_SHELF_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_SHELF_PIN);

    gpio_init(BUTTON_CONFIRM_PIN);
    gpio_set_dir(BUTTON_CONFIRM_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_CONFIRM_PIN);

    // Configura os pinos do LED RGB como saída
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    
    // Configura interrupções nos pinos dos botões - ambos com o mesmo callback
    gpio_set_irq_enabled_with_callback(BUTTON_SHELF_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_CONFIRM_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    printf("Botões e LED RGB configurados\n");
    
    // Inicializa o LED RGB com a cor da primeira prateleira
    update_rgb_led();

    while (1) {
        // Atualiza a matriz de LEDs com o número atual
        set_matrix_leds();
        sleep_ms(10); // Pequeno atraso para estabilidade
    }

    return 0;
}