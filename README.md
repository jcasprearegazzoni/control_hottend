# control_hottend

Proyecto de control de temperatura de un hotend con Arduino Mega 2560 y PlatformIO.

## Contenido

- `src/main.cpp`: lectura del termistor NTC y control de la salida PWM.
- `platformio.ini`: configuracion de PlatformIO para Arduino Mega 2560.
- `tools/plot_serial.py`: grafico de temperatura y PWM recibidos por puerto serie.
- `espidf_control_hotend/`: proyecto inicial independiente de ESP-IDF que imprime un mensaje de prueba.

## Configuracion del controlador

- Termistor: entrada A13, NTC de 100 kohm, Beta 3950 y resistencia serie de 4,7 kohm.
- Salida de control: pin 10 hacia el driver/MOSFET del calentador.
- Consigna: 80 grados Celsius.
- Periodo de control: 100 ms.

## Compilar y cargar

Con PlatformIO instalado, ejecutar desde la raiz del proyecto:

```sh
pio run
pio run --target upload
pio device monitor --baud 115200
```

## Graficar datos

```sh
python -m pip install pyserial matplotlib
python tools/plot_serial.py COM3
```

Reemplazar `COM3` por el puerto de la placa y cerrar otros monitores serie antes de abrir el grafico.
