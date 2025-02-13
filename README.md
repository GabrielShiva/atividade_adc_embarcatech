# Controle de Joystick com ADC e Display OLED - Raspberry Pi Pico

Este projeto faz parte do **Capítulo 8 da Unidade 4** do curso **Embarcatech**, focando no uso de **ADC (Conversor Analógico-Digital)** para leitura de um joystick, controle de LEDs via PWM e exibição de dados em um display OLED. O código foi desenvolvido para a placa **Raspberry Pi Pico** utilizando a linguagem C e a biblioteca `pico-sdk`.

---

## Descrição do Projeto

O objetivo deste projeto é demonstrar a integração de múltiplos componentes:
- **Leitura analógica** dos eixos X/Y de um joystick via ADC.
- **Controle de intensidade de LEDs** (vermelho e azul) via PWM, baseado na posição do joystick.
- **Exibição em tempo real** das coordenadas do joystick e de um quadrado móvel em um display OLED.
- **Interrupções** para tratar botões (A e SW do joystick) com debounce.
- **Comunicação I2C** para controlar o display OLED.

---

## Configuração do Hardware

### Componentes Utilizados
- **Raspberry Pi Pico**: Microcontrolador principal.
- **Joystick Analógico**: 
  - Eixo X conectado ao pino GPIO 27 (ADC1).
  - Eixo Y conectado ao pino GPIO 26 (ADC0).
  - Botão SW conectado ao pino GPIO 22.
- **LEDs RGB**:
  - LED Vermelho (R) no pino GPIO 13.
  - LED Verde (G) no pino GPIO 11.
  - LED Azul (B) no pino GPIO 12.
- **Display OLED SSD1306**: Conectado via I2C (pinos GPIO14-SDA e GPIO15-SCL).
- **Botão A**: Conectado ao pino GPIO 5.

---

## Funcionalidades Principais

### 1. Leitura do Joystick via ADC
- Os valores analógicos dos eixos X e Y são convertidos para valores digitais (0-4095).
- A posição central do joystick é calibrada para `(2049, 1988)`.
- Os valores são normalizados para controlar a intensidade dos LEDs e a posição do quadrado no display.

### 2. Controle de LEDs com PWM
- **LED Vermelho (X)**: Intensidade proporcional ao deslocamento horizontal do joystick.
- **LED Azul (Y)**: Intensidade proporcional ao deslocamento vertical do joystick.
- **LED Verde (G)**: Ativado/desativado pelo botão SW do joystick.

### 3. Display OLED
- Exibe um quadrado móvel que se desloca conforme a posição do joystick.
- Bordas dinâmicas alternam entre sólidas e pontilhadas via botão SW.
- Atualização em tempo real das coordenadas do joystick.

### 4. Interrupções e Debounce
- **Botão A**: Habilita/desabilita o PWM dos LEDs.
- **Botão SW**: Controla o LED verde e alterna o estilo da borda do display.
- Debounce de **260 ms** para evitar leituras falsas.

---

## Configuração do Código

### Bibliotecas Utilizadas
- `hardware/adc.h`: Para leitura analógica.
- `hardware/pwm.h`: Para controle dos LEDs.
- `ssd1306.h`: Para comunicação com o display OLED.
- `hardware/irq.h`: Para tratamento de interrupções.

### Funções-Chave
- **`adc_setup()`**: Inicializa o ADC e os pinos do joystick.
- **`pwm_setup()`**: Configura os parâmetros de PWM para os LEDs.
- **`gpio_irq_handler()`**: Gerencia interrupções dos botões.
- **`move_square()`**: Calcula a posição do quadrado no display com base no joystick.

---

## Como Utilizar

1. **Conecte os componentes** conforme a pinagem definida no código.
2. **Compile e carregue** o código na Raspberry Pi Pico (ex: usando VS Code com ambiente configurado para Pico).
3. **Monitore a saída serial** para ver os valores dos LEDs em tempo real.
4. **Interaja com o joystick**:
   - Mova os eixos para controlar a intensidade dos LEDs e o quadrado no display.
   - Pressione o botão SW para alternar o LED verde e o estilo da borda.
   - Pressione o botão A para habilitar/desabilitar os LEDs.

---

## Requisitos
- **Raspberry Pi Pico**
- **Display OLED SSD1306** (128x64 pixels, I2C)
- **Joystick Analógico**
- **LEDs RGB**
- **Bibliotecas**: `pico-sdk`, `ssd1306`, `hardware/adc`, `hardware/pwm`

---

### Compilação e Upload
1. Clone o repositório utilizando o comando abaixo.
```
git clone https://github.com/GabrielShiva/atividade_adc_embarcatech.git
```
2. No VsCode, abra a pasta do projeto. Não é necessário importar o projeto utilizando a extensão *Raspberry Pi Pico Project*. O projeto já contém a pasta *build*.

## Vídeo de Apresentação

Assita ao vídeo com a apresentação do projeto (clicando aqui)[https://youtu.be/vZ3_kzzOPuw].