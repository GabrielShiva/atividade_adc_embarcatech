#include <stdio.h> // inclui a biblioteca padrão para I/O 
#include <stdlib.h> // utilizar a função abs
#include "pico/stdlib.h" // inclui a biblioteca padrão do pico para gpios e temporizadores
#include "hardware/adc.h" // inclui a biblioteca para manipular o hardware adc
#include "hardware/pwm.h" // inclui a biblioteca para manipular o hardware pwm
#include "hardware/irq.h" // inclui a biblioteca para interrupções
#include "hardware/i2c.h" // inclui a biblioteca para utilizar oprotocolo i2c
#include "inc/ssd1306.h" // inclui a biblioteca com definição das funções para manipulação do display OLED
#include "inc/font.h" // inclui a biblioteca com as fontes dos caracteres para o display OLED

// definição dos pinos gpios para os periféricos utilizados
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define JOYSTICK_SW 22
#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define BTN_A 5

// definição de parametros para o protocolo i2c
#define I2C_ID i2c1
#define I2C_FREQ 400000
#define I2C_SDA 14
#define I2C_SCL 15
#define SSD_1306_ADDR 0x3C

// inicia a estrutura do display OLED
ssd1306_t ssd;

// definição de constantes para o display
volatile uint8_t border_counter = 0;

// definição de parêmetros para o pwm
const uint32_t sys_clk_freq = 125000000; // definição de constante para a frequência do sistema
const uint32_t target_freq = 1000; // definição de constante para a frequência desejada para o sinal pwm
const uint32_t wrap = 4095; // definição de constante para o valor máximo do contador pwm
float clk_divider = (float)sys_clk_freq / (target_freq * (wrap + 1)); // definição do divisor de clock (30.52)

// define o estado do pwm para os leds
volatile bool leds_pwm_state = true; 
volatile uint x_slice_num, y_slice_num;

const uint16_t central_x_pos = 2049;
const uint16_t central_y_pos = 1988;

// define o estado para o led verde
volatile bool led_g_state = false;

// define variáveis para debounce do botão
volatile bool btn_a_state = false;
volatile bool btn_sw_state = false;
volatile uint32_t last_time_btn_press = 0; 

// debounce delay 
const uint32_t debounce_delay_ms = 260;

// inicializa os botões
void btn_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

// configuração do protocolo i2c
void i2c_setup() {
    // inicia o modulo i2c (i2c1) do rp2040 com uma frequencia de 400kHz
    i2c_init(I2C_ID, I2C_FREQ);

    // define o pino 14 como o barramento de dados
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    // define o pino 15 como o barramento de clock
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // ativa os resistores internos de pull-up para os dois barramentos para evitar flutuações nos dois barramentos quando está no estado de espera (idle)
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void display_init(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD_1306_ADDR, I2C_ID); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
  
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// inicialização e configuração do hardware adc
void adc_setup() {
    // inicializa o hardware adc
    adc_init();
    
    // inicialização dos pinos analógicos
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

// inicializa o pwm para o pinos especificados
uint pwm_setup(uint gpio_pin, uint16_t gpio_level, bool pwm_enabled) {
    // define o pino especificado como uma saída pwm
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);

    // obtendo o slice referente ao pino especificado que foi definido como saída pwm
    uint slice_num = pwm_gpio_to_slice_num(gpio_pin);

    // configura os valores de wrap e divisor de clock para o slice do pino pwm escolhido (13)
    pwm_set_wrap(slice_num, wrap);
    pwm_set_clkdiv(slice_num, clk_divider);
    pwm_set_gpio_level(gpio_pin, gpio_level);
    pwm_set_enabled(slice_num, pwm_enabled);

    return slice_num;
}

// realiza a leitura do canal especificado e retorna o valor digital convertido
uint16_t adc_start_read(uint channel_selected) {
    uint16_t channel_value = 0;

    if (channel_selected == 0) {
        // seleciona o canal para o eixo x e realiza a leitura e conversão para o valor digital e armazena em uma variável de 2 bytes
        adc_select_input(0);
        // lê o valor do canal 0 para o eixo x [0-4095]
        channel_value = adc_read();
    } else if (channel_selected == 1) {
        // seleciona o canal para o eixo y e realiza a leitura e conversão para o valor digital e armazena em uma variável de 2 bytes
        adc_select_input(1);
        // lê o valor do canal 0 para o eixo y [0-4095]
        channel_value = adc_read();
    } 

    return channel_value;
}

uint16_t adc_convert_value(uint16_t central_pos, uint16_t raw_value) {
    uint16_t current_differential_pos = abs(central_pos - raw_value);

    float conversion_factor = 4095/2047.0;

    uint16_t value_converted = current_differential_pos * conversion_factor;

    return value_converted;
}

void set_display_border() {
    // limpa o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    if (!border_counter) {
        ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
    } else {
        for (uint x = 0; x < 128; x++) {
            if ((x % 2) == 0) {
                ssd1306_pixel(&ssd, x, 0, true);
            } else {
                ssd1306_pixel(&ssd, x, 0, false);
            }
        }

        for (uint x = 0; x < 128; x++) {
            if ((x % 2) == 0) {
                ssd1306_pixel(&ssd, x, 63, true);
            } else {
                ssd1306_pixel(&ssd, x, 63, false);
            }
        }

        for (uint y = 0; y < 64; y++) {
            if ((y % 2) == 0) {
                ssd1306_pixel(&ssd, 0, y, true);
            } else {
                ssd1306_pixel(&ssd, 0, y, false);
            }
        }

        for (uint y = 0; y < 64; y++) {
            if ((y % 2) == 0) {
                ssd1306_pixel(&ssd, 127, y, true);
            } else {
                ssd1306_pixel(&ssd, 127, y, false);
            }
        }
    } 
}

// função para tratar as interrupções das gpios
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // retorna o tempo total em ms desde o boot do rp2040

    // verifica se a diff entre o tempo atual e a ultima vez que o botão foi pressionado é maior que o tempo de debounce
    if (current_time - last_time_btn_press > debounce_delay_ms) {
        last_time_btn_press = current_time;

        // verifica se o botão A foi pressionado
        if (gpio == BTN_A) {
            // inverte o estado atual para o pwm dos leds
            leds_pwm_state = !leds_pwm_state;

            printf("Estado dos leds pwm: %d\n", leds_pwm_state);

            // ativa/desativa o estado do pwm para os leds
            pwm_set_enabled(x_slice_num, leds_pwm_state);
            pwm_set_enabled(y_slice_num, leds_pwm_state);
        } else if (gpio == JOYSTICK_SW) { // verifica se o botão SW foi pressionado
            // inverte o estado do LED verde
            led_g_state = !led_g_state; 

            // troca o estilo da borda
            border_counter = !border_counter;

            printf("Estado dos leds verde: %d\n", led_g_state);

            // aplica o estado invertido para o LED verde
            gpio_put(LED_G, led_g_state); 

            // adicionar aqui o codigo para mundaça de borda do display
        }
    } 
}

int main() {
    // chama função para comunicação serial via usb para debug
    stdio_init_all(); 

    // inicializa o LED verde
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, led_g_state);

    // chama a função para configuração do protocolo i2c
    i2c_setup();

    // inicializa o display OLED
    display_init();
    
    // chama a função que inicializa o adc
    adc_setup();

    // configuração do pwm e define o nível do pwm para 0
    x_slice_num = pwm_setup(LED_R, 0, leds_pwm_state);
    y_slice_num = pwm_setup(LED_B, 0, leds_pwm_state);

    // configuração dos botões SW e A
    btn_init(BTN_A);
    btn_init(JOYSTICK_SW);

    // configuração da interrupção para o botão A e SW
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(JOYSTICK_SW, GPIO_IRQ_EDGE_FALL, true);

    // define o estilo de borda inicial
    border_counter = 0;

    while (true) {
        // define o tipo de borda
        set_display_border();

        // realiza leitura para o eixo x
        uint16_t x_value = adc_start_read(1);
        // converte o valor para controle da intensidade do led vermelho tomado como menor intensidade a posição central
        uint16_t x_value_converted = adc_convert_value(central_x_pos, x_value);

        // realiza leitura para o eixo y
        uint16_t y_value = adc_start_read(0);
        // converte o valor para controle da intensidade do led azul tomado como menor intensidade a posição central
        uint16_t y_value_converted = adc_convert_value(central_y_pos, y_value);

        // calcula a distância do centro
        int16_t x_diff = x_value - central_x_pos;
        int16_t y_diff = central_y_pos - y_value;

        // normaliza a distância do centro para a faixa [-1, 1]
        float max_displacement_x = 2049.0f;
        float max_displacement_y = 2107.0f;
        float normalized_x = (float)x_diff / max_displacement_x;
        float normalized_y = (float)y_diff / max_displacement_y;

        // calcula a nova posição do quadrado que está centrado inicialmente em (60, 28)
        int new_x = 60 + (int)(normalized_x * 60.0f);
        int new_y = 28 + (int)(normalized_y * 28.0f);

        // limita as posições para o tamnho da tela
        new_x = (new_x < 0) ? 0 : (new_x > 120) ? 120 : new_x;
        new_y = (new_y < 0) ? 0 : (new_y > 56) ? 56 : new_y;

        // cria o quadrado 8X8
        ssd1306_rect(&ssd, new_y, new_x, 8, 8, true, true);

        // atualiza o display OLED
        ssd1306_send_data(&ssd);

        // define a intensidade do led vermelho
        pwm_set_gpio_level(LED_R, x_value_converted);
        // ativa/desativa pwm com base no botão A
        pwm_set_enabled(x_slice_num, leds_pwm_state);

        // define a intensidade do led azul
        pwm_set_gpio_level(LED_B, y_value_converted);
        // ativa/desativa pwm com base no botão A
        pwm_set_enabled(y_slice_num, leds_pwm_state);

        printf("\n LED VERMELHO (X): %.2f%%, LED AZUL (Y): %.2f%%\n", x_value_converted/4095.0*100, y_value_converted/4095.0*100);
        sleep_ms(50);
    }
}