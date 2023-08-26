PROYECTO RIEGO INTELIGENTE CON ARDUINO
======================================

FENCYT "EUREKA" XXXIII 2023
---------------------------

### Resumen

El proyecto *"Riego inteligente"* es un proyecto que fue hecho para la **Feria Nacional de Ciencia y Tecnología (FENCYT) "EUREKA"**.

Este proyecto esta basado en el microcontrolador ARDUINO UNO, y todo el código esta basado en C++ ya que es el lenguaje que usa el compilador de ARDUINO para el respectivo microcontrolador.

### Detalles de Hardware

Este proyecto usa un ARDUINO UNO basado en el microprocesador ATmega328p.  
A este microcontrolador es que se conecta el resto de periféricos que se encargaran de recopilar y mostrar los datos e información necesaria.  

Los componentes conectado al ARDUINO y sus respectivos pines a continuación:

- SENSOR DE HUMEDAD DE SUELO **YL-69**: **PIN A1**
- SENSOR DE HUMEDAD Y TEMPERATURA DEL AIRE **DHT-22**: **PIN 2**
- RELÉ MECÁNICO DE 5V PARA CONTROL: **PIN 3**
- PANTALLA LCD I2C 16X2:
    - **SCL: PIN A5**
    - **SDA: PIN A4**


### Contenido del repositorio

En este repositorio se encuentra todo el código del proyecto hecho por los estudiantes del 4to Grado B  
del colegio **Martin de la Riva y Herrera** de la ciudad de *Lamas, Lamas, San Martin*.

### Documentación

#### 1. Importación de librerías

Desde la linea 1 a la linea 11 se importan las librerías necesarias para varias de las funciones del programa en ARDUINO:

> Permite el correcto funcionamiento de los pines I2C para la pantalla LCD

    // Bibliotecas para pantalla LCD I2C
    #include <Wire.h>
    #include <LCD.h>
    #include <LiquidCrystal_I2C.h>

> Permite la fácil recolección de datos de cualquier sensor DHT-XX

    // Bibliotecas para sensor de humedad y temperatura ambiental
    #include <DHT.h>
    #include <DHT_U.h>

> Permite el eficiente manejo de la EEPROM de ARDUINO

    // Biblioteca para modificar y leer la memoria EEPROM
    #include <EEPROM.h>

Sin estas librerías el programa no se podría ejecutar.

#### 2. Variables globales

En esta sección del código se inicializan u obtienen las variables globales necesarias para el programa y a las cuales se accederá y modificara en todo momento:
