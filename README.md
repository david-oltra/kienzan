# Kienzan - Robot de Combate

![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow)
![Plataforma](https://img.shields.io/badge/plataforma-ESP32-blue)
![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)

Robot de combate basado en ESP32. Actualmente en **fase de desarrollo activo** — el código no está completo y puede contener cambios importantes sin previo aviso.

## Estado del proyecto

> ⚠️ **EN DESARROLLO** — Este repositorio se encuentra en construcción. La funcionalidad de control remoto vía **ESP-NOW** aún no está implementada. El robot actualmente solo ejecuta una rutina de prueba en bucle desde `app_main`.

## Descripción

Kienzan es un robot de combate controlado por un ESP32. El sistema actual contempla:

- **1 motor brushless** controlado por un ESC (Electronic Speed Controller) a través de PWM — destinado al arma del robot.
- **2 motores DC** controlados mediante un driver **TB6612FNG** — destinados a la tracción.

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

## Funcionalidades implementadas

- ✅ Inicialización y control del ESC con armado por pulso mínimo (1000 µs).
- ✅ Función `esc_set_speed_percent()` para establecer velocidad del motor brushless (0–100 %).
- ✅ Inicialización del driver TB6612 (pines de dirección, STBY y PWM).
- ✅ Función `motor_set()` para controlar velocidad y dirección de cada motor DC (-100 a 100 %).
- ✅ Función `motors_stop_all()` para detener ambos motores.

## Funcionalidades pendientes

- ❌ **Control por ESP-NOW** — El robot aún no recibe comandos desde el mando.
- ❌ Lógica de control en tiempo real (mixer diferencial, rampas de aceleración).
- ❌ Control del arma (motor brushless) por comandos del mando.
- ❌ Modos de operación (combate, test, calibración).
- ❌ Watchdog / failsafe ante pérdida de señal.
- ❌ Documentación de conexionado físico y diagrama de cableado.

## Uso actual

Al arrancar, el firmware:

1. Inicializa y arma el ESC (envía pulso mínimo durante 3 s).
2. Inicializa el driver TB6612.
3. Ejecuta una rutina de prueba en bucle:
   - Motor A adelante 60 % / Motor B atrás 40 % (2 s)
   - Ambos motores adelante 80 % (2 s)
   - Giro en el sitio: A adelante 70 % / B atrás 70 % (2 s)
   - Detención total (2 s)

El control del ESC está comentado en el bucle principal, listo para pruebas manuales.

## Próximos pasos

1. Implementar la comunicación **ESP-NOW** entre el mando y el robot.
2. Añadir tarea de recepción de comandos con `FreeRTOS`.
3. Implementar lógica de mezcla diferencial (mixer) para tracción.
4. Mapear los canales del mando a funciones (tracción, arma, auxiliares).
5. Añadir failsafe por pérdida de paquetes ESP-NOW.

## Configuración esperada de ESP-NOW (borrador)


## Licencia

Este proyecto está licenciado bajo la **GNU General Public License v3.0 (GPL-3.0)**.

Consulta el archivo [LICENSE](LICENSE) para el texto completo de la licencia.

Copyright (C) 2026 [David_Wiki]

## Autor

Proyecto personal — Robot de combate **Kienzan**.
