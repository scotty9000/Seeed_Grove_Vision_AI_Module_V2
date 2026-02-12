Grove UART demo (UART2) 📡

Overview
- This demo shows how to use UART2 on PB9 (RX) / PB10 (TX) to connect to a Grove/Wio device in parallel with the console (UART0 → USB).

Wiring
- Connect Grove UART (4-pin) as follows:
  - Grove TX -> Wio RX
  - Grove RX -> Wio TX
  - GND -> GND
  - DO NOT connect VCC unless you want to power the Wio from the module and have verified voltage compatibility.

Build
- Build and enable the PB9/PB10 mapping with:
    make BOARD=epii_evb APP_TYPE=grove_uart_demo APPL_DEFINES+=-DENABLE_GROVE_UART2

Run
- Open your PC serial terminal to the USB console (UART0) at 115200 to see the "Grove UART2 demo started" message.
- Connect a serial terminal or the Wio device to the Grove UART port at 115200 and send characters; the demo echoes back any received bytes.

Notes
- This keeps the existing UART0 console output on the USB port; UART2 is an additional port for ad-hoc messaging with the Wio.
- If you want this permanently enabled, add `BOARD_SUB_DEFINES += -DENABLE_GROVE_UART2` to `board/epii_evb/epii_evb.mk`.
