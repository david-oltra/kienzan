# Kienzan - Robot de Combate

![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow)
![Plataforma](https://img.shields.io/badge/plataforma-ESP32-blue)
![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)

Robot de combate basado en ESP32-C3. Actualmente en **fase de desarrollo activo** — el código funciona pero sigue evolucionando.

## Estado del proyecto

> ⚠️ **EN DESARROLLO** — El robot ya recibe comandos por **ESP-NOW** desde un mando externo y controla los dos motores DC de tracción y el motor brushless del arma. Falta pulir la lógica de control, añadir modos de operación y documentar el cableado.

## Descripción

Kienzan es un robot de combate controlado por un ESP32-C3. El sistema actual contempla:

- **1 motor brushless** controlado por un ESC (Electronic Speed Controller) a través de PWM — destinado al arma del robot.
- **2 motores DC** controlados mediante un driver **TB6612FNG** — destinados a la tracción.
- **Comunicación inalámbrica ESP-NOW** con un mando externo basado en ESP32.

## Hardware

| Componente            | GPIO / Pin | Notas                                      |
|-----------------------|------------|--------------------------------------------|
| ESC (motor brushless) | GPIO 4     | PWM 50 Hz, resolución 13 bits              |
| PWMA (Motor A)        | GPIO 5     | PWM 20 kHz, resolución 10 bits             |
| AIN2                  | GPIO 6     | Dirección Motor A                          |
| AIN1                  | GPIO 7     | Dirección Motor A                          |
| STBY (TB6612)         | GPIO 8     | Habilitación del driver                    |
| BIN1                  | GPIO 9     | Dirección Motor B                          |
| BIN2                  | GPIO 10    | Dirección Motor B                          |
| PWMB (Motor B)        | GPIO 20    | PWM 20 kHz, resolución 10 bits             |
| GND                   | GPIO 21    | ⚠️ GND "virtual" — idealmente usar GND real |

### Detalles de PWM

- **ESC:** 50 Hz, resolución 13 bits (0–8191). Ancho de pulso entre 1000 µs y 2000 µs.
- **Motores DC:** 20 kHz (fuera del rango audible), resolución 10 bits (0–1023).

## Comunicación ESP-NOW

El robot actúa como **receptor** y solo acepta paquetes procedentes de la MAC del mando:

- **Canal WiFi:** 1 (fijo, debe coincidir con el emisor)
- **MAC del mando:** `C8:2E:18:F1:83:40`
- **Filtro de origen:** los paquetes de otras MACs se descartan en el callback.
- **Failsafe:** si no se recibe ningún paquete en **500 ms**, se detienen los motores DC y se pone el ESC al mínimo.

### Estructura del paquete recibido

```c
typedef struct __attribute__((packed)) {
    int8_t  joystick_right_Y;   /* -100..+100 */
    int8_t  joystick_right_X;   /* -100..+100 */
    uint8_t joystick_left_Y;    /*    0..100  */
    int8_t  joystick_left_X;    /* -100..+100 */
} remote_data_t;
```

### Mapeo de controles

| Control             | Rango       | Función                                      |
|---------------------|-------------|----------------------------------------------|
| Joystick derecho Y  | -100..+100  | Velocidad de tracción (positivo = adelante)  |
| Joystick derecho X  | -100..+100  | Giro (diferencial)                           |
| Joystick izquierdo Y| 0..100      | Velocidad del arma (motor brushless)         |
| Joystick izquierdo X| -100..+100  | Reservado                                    |

### Mezcla de tracción (tipo RC / tank mixing)

```c
left_speed  = y + x;
right_speed = y - x;
/* saturar a [-100, 100] */
```

- Centro (X=0) con Y al máximo → **ambos motores al 100%**.
- X negativo/positivo → reparte la velocidad entre los motores para girar.
- Y negativo → invierte el sentido (marcha atrás).

## Funcionalidades implementadas

- ✅ Inicialización y control del ESC con armado por pulso mínimo (1000 µs).
- ✅ Función `esc_set_speed_percent()` con **umbral mínimo de arranque del 30%** — por debajo, el motor queda parado.
- ✅ Inicialización del driver TB6612 (pines de dirección, STBY y PWM).
- ✅ Función `motor_set()` para controlar velocidad y dirección de cada motor DC (-100 a 100 %).
- ✅ Flags `MOTOR_A_INVERT` / `MOTOR_B_INVERT` para invertir el sentido de giro por software.
- ✅ Función `motors_stop_all()` para detener ambos motores.
- ✅ Recepción de comandos por **ESP-NOW** con filtrado por MAC.
- ✅ **Failsafe por timeout** (500 ms) ante pérdida de señal del mando.
- ✅ Mezcla diferencial tipo RC para tracción.
- ✅ Control del arma (motor brushless) desde el joystick izquierdo.
- ✅ Cola FreeRTOS de profundidad 1 con `xQueueOverwrite` para mantener siempre el último comando.

## Funcionalidades pendientes

- ❌ Modos de operación (combate, test, calibración).
- ❌ Rampas de aceleración suaves en la tracción.
- ❌ Calibración del ESC al arranque (opcional).
- ❌ Documentación de conexionado físico y diagrama de cableado.
- ❌ Código del mando emisor (repositorio aparte o carpeta `remote/`).
- ❌ Telemetría de vuelta (estado de batería, RSSI, etc.).

## Uso actual

Al arrancar, el firmware:

1. Inicializa el driver TB6612 y deja los motores parados.
2. Inicializa y **arma el ESC** (envía pulso mínimo de 1000 µs durante 3 s).
3. Inicializa WiFi en modo STA en el canal 1.
4. Inicializa ESP-NOW, registra el peer del mando y el callback de recepción.
5. Entra en el bucle principal:
   - Espera paquetes del mando por la cola (timeout 50 ms).
   - Al recibir uno, aplica la mezcla y actualiza motores DC y ESC.
   - Si pasan más de 500 ms sin datos, para todo (failsafe).

## Estructura del proyecto

```
Kienzan/
├── platformio.ini
├── README.md
├── LICENSE
└── src/
    └── main.c
```

## Compilación y flasheo

Requiere [PlatformIO](https://platformio.org/).

```bash
# Compilar
pio run

# Flashear
pio run --target upload

# Monitor serie
pio device monitor
```

### Nota sobre `intelhex`

En algunas versiones recientes de PlatformIO puede aparecer un error del tipo:

```
ModuleNotFoundError: No module named 'intelhex'
```

Se soluciona instalándolo en el entorno Python de PlatformIO:

```bash
~/.platformio/penv/bin/pip install intelhex
```

## Próximos pasos

1. Añadir rampas de aceleración en la tracción.
2. Implementar modos de operación (combate / test / calibración).
3. Añadir telemetría de retorno al mando.
4. Documentar el cableado físico con diagrama.
5. Publicar el código del mando emisor.

## Licencia

Este proyecto está licenciado bajo la **GNU General Public License v3.0 (GPL-3.0)**.

Consulta el archivo [LICENSE](LICENSE) para el texto completo de la licencia.

Copyright (C) 2026 [David_Wiki]

## Autor

Proyecto personal — Robot de combate **Kienzan**.
