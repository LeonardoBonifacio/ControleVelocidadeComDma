# Controle de LED PWM via DMA no RP2040 utilizando a BitDogLab (Método Chaining).

Este projeto demonstra uma implementação avançada de **PWM (Pulse Width Modulation)** utilizando **DMA (Direct Memory Access)** no microcontrolador RP2040 (Raspberry Pi Pico), focado na placa de desenvolvimento **BitDogLab**.

O código cria um efeito de *fading*  suave em um LED. A principal vantagem desta abordagem é que, após a configuração inicial, todo o processo é gerenciado pelo controlador de DMA, deixando a CPU (núcleos ARM) 100% livre para outras tarefas ou para entrar em modo de baixo consumo.


## Funcionalidades

* **Zero CPU Overhead:** O loop `main` permanece vazio (`tight_loop_contents()`), provando que a CPU não está atuando na atualização do brilho do LED.
* **DMA Chaining (Encadeamento):** Utiliza dois canais DMA:
    1.  **Canal de Dados:** Transfere os níveis de brilho do buffer para o hardware PWM.
    2.  **Canal de Controle:** Reconfigura o endereço de leitura do canal de dados quando este termina, criando um loop infinito via hardware.
* **Fading Suave:** Implementa uma curva quadrática para o cálculo do brilho, resultando em uma transição visual mais natural para o olho humano do que uma transição linear.


## Detalhes Técnicos

Esta implementação difere do método por interrupção pois cria um **loop de hardware puro**, onde um canal DMA reprograma o outro. O fluxo de funcionamento é:

1.  **Buffer de Fade:**
    * Um array `fade[]` de 64 posições (`NUM_VALORES`) é preenchido com valores de 0 a 800 (`PWM_TOP`) seguindo uma lógica quadrática para suavidade visual.

2.  **Sincronização (Pacing via DREQ):**
    * O **Canal de Dados** é configurado para mover 16 bits (um valor de brilho) para o registrador `CC` do PWM.
    * Ele não transfere livremente; ele obedece ao sinal de requisição (`DREQ`) do PWM. Ou seja, um novo valor de brilho só é carregado quando o ciclo PWM atual termina.

3.  **Mecanismo de Ping-Pong (Chaining):**
    * **Canal de Dados (O Executor):** Ao terminar de transferir todo o buffer (64 valores), ele aciona o Canal de Controle através do recurso `chain_to`.
    * **Canal de Controle (O Gerente):** Este canal possui uma configuração especial.
        * *Origem:* Um ponteiro estático contendo o endereço inicial do array `fade`.
        * *Destino:* O registrador de **endereço de leitura** do Canal de Dados (`&dma_hw->ch[data_chan].al1_read_addr`).
        * *Ação:* Ele escreve o endereço inicial de volta no Canal de Dados e, ao terminar essa única transferência, aciona o Canal de Dados novamente (`chain_to`).
    * **Resultado:** O sistema entra em um loop infinito de transferências e reconfigurações sem que a CPU precise executar uma única linha de código após o `main`.