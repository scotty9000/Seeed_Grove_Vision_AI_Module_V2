APP_NAME := grove_uart_demo

APP_SRCS := grove_uart_demo.c
APP_TYPE := grove_uart_demo

# Add dependencies if needed
LIBS +=

# Ensure power management headers are available (used by trustzone headers)
LIB_SEL = pwrmgmt

# Build settings
APPL_DEFINES += -DGROVE_UART_DEMO
