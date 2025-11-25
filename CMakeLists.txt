# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# versão mínima do CMake
cmake_minimum_required(VERSION 3.13)

# Incluir o arquivo de importação do SDK
include(pico_sdk_import.cmake)

# Dar um nome qualquer ao projeto
project(Led_pwm C CXX ASM)

# Inicializar o SDK do Pico
pico_sdk_init()

add_executable(${PROJECT_NAME}
    #DMA_PWM_LED.c
    DMA_PWM_LED_Debug.c
    #DMA_PWM_LED_Botao.c
    )

target_link_libraries(${PROJECT_NAME}
    pico_stdlib 
    hardware_dma 
    hardware_pwm
    )

# Em caso de Depuração via UART, desativar USB e ativar UART    
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})

