// Bibliotecas para pantalla LCD I2C
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// Bibliotecas para sensor de humedad y temperatura ambiental
#include <DHT.h>
#include <DHT_U.h>

// Biblioteca para modificar y leer la memoria EEPROM
#include <EEPROM.h>

// Biblioteca para control sencillo del numpad 4x4
#include <Keypad.h>

// Definiciones de los pines de entradas y salidas
#define HUMIDITY_SENSOR_PIN A3
#define RELAY_PIN 3
#define DHT_PIN 2

// Valores de calibración del sensor de humedad del suelo
int airValue = EEPROM.get(0, airValue);     // Valor que da el sensor de humedad del suelo en el aire (Su valor se guarda en 0)
int waterValue = EEPROM.get(4, waterValue); // Valor que da el sensor de humedad del suelo en el agua (Su valor se guarda en 4)

int soilMoistureRaw;
int soilMoisturePercent;

int temperature;
int humidity;

int minHumidity = EEPROM.get(8, minHumidity);
// Valor de limite mínimo de humedad en suelo (Su valor se guarda en 8)

int maxHumidity = EEPROM.get(12, maxHumidity);
// Valor de limite máximo de humedad en suelo (Su valor se guarda en 12)

int preSoilMoisturePercent;
int preTemperature;
int preHumidity;

boolean wateringState = false;
boolean backlightLCD = true;

byte celsiusGrades[] = {
    B11100,
    B10100,
    B11100,
    B00000,
    B00111,
    B00100,
    B00100,
    B00111};

byte welcomeFace[] = {
    B00000,
    B01010,
    B01010,
    B01010,
    B00000,
    B10001,
    B01110,
    B00000};

byte blockBar[] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B00000};

// Número de filas y columnas del numpad
const byte ROWS = 4;
const byte COLUMNS = 4;

// Teclas del numpad
char keys[ROWS][COLUMNS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Pines del numpad
byte rowPins[ROWS] = {11, 10, 9, 8};
byte columnPins[COLUMNS] = {7, 6, 5, 4};

Keypad numpad = Keypad(makeKeymap(keys), rowPins, columnPins, ROWS, COLUMNS);

DHT dht(DHT_PIN, DHT22);

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

/**
 * Se encarga de ejecutar una sola
 * vez todo el código encargado de
 * activar y preparar todo para que
 * el bucle se mantenga correctamente
 */
void setup()
{
  dht.begin();
  // Serial.begin(9600);
  // debuggingEEPROMData();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  lcd.clear();

  lcd.createChar(0, celsiusGrades);
  lcd.createChar(1, welcomeFace);
  lcd.createChar(2, blockBar);

  startScreen();
}

/**
 * Es el bucle que se repite a cada ciclo
 * del procesador
 */
void loop()
{
  soilMoistureRaw = analogRead(HUMIDITY_SENSOR_PIN);
  soilMoisturePercent = fixPercent(map(soilMoistureRaw, airValue, waterValue, 0, 100));

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  preSoilMoisturePercent = condicionalClearLCDByNumberInLCD(soilMoisturePercent, preSoilMoisturePercent);
  preTemperature = condicionalClearLCDByNumberInLCD(temperature, preTemperature);
  preHumidity = condicionalClearLCDByNumberInLCD(humidity, preHumidity);

  if (!wateringState)
  {
    showAmbientData();
  }
  optionSelector();

  watering();
  // debuggingSoilMoisture();
  delay(250);
}

/**
 * Muestra en la pantalla LCD la temperatura
 * ambiental, la humedad del aire y la humedad
 * del suelo
 */
void showAmbientData()
{
  lcd.setCursor(0, 0);

  lcd.print("Tmp:");
  lcd.print((int)temperature);
  lcd.write((uint8_t)0);
  lcd.print(" Hmd:");
  lcd.print((int)humidity);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Humedad:");
  lcd.print((int)soilMoisturePercent);
  lcd.print("%");
}

/**
 * Usa el booleano de estado de riego para ver si
 * se debe regar o no, en cuanto llega al máximo del
 * rango de humedad requerido detiene el riego y establece
 * el estado de riego en false
 */
void watering()
{
  if (soilMoisturePercent < minHumidity)
  {
    wateringState = true;
  }

  if (wateringState)
  {
    if (soilMoisturePercent < maxHumidity)
    {
      digitalWrite(RELAY_PIN, LOW);
      wateringMsj();
      wateringState = true;
    }
    else if (soilMoisturePercent >= maxHumidity)
    {
      digitalWrite(RELAY_PIN, HIGH);
      lcd.setCursor(0, 0);
      lcd.print("                ");
      wateringState = false;
    }
  }
  else
  {
    digitalWrite(RELAY_PIN, HIGH);
    wateringState = false;
  }
}

/**
 * Muestra en la pantalla LCD que se esta
 * regando en ese momento por la bomba
 */
void wateringMsj()
{
  lcd.setCursor(0, 0);
  lcd.print("*****REGANDO****");
  lcd.setCursor(0, 1);
  lcd.print("Humedad:");
  lcd.print((int)soilMoisturePercent);
  lcd.print("%");
}

/**
 * Se encarga de verificar si hay o no una
 * tecla siendo presionada y si esta equivale
 * a alguna opción existente
 */
void optionSelector()
{
  char key = numpad.getKey();
  switch (key)
  {
  case 'A':
    showHumidityRange(5000);
    break;

  case 'B':
    setHumidityRange();
    break;

  case 'C':
    calibrateSoilSensor();
    break;

  case 'D':
    toggleBacklightLCD();
    break;

  default:
    break;
  }
}

/**
 * Muestra el rango de humedad actual
 * al que esta configurado el sistema
 */
void showHumidityRange(int msDelay)
{
  digitalWrite(RELAY_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MINIMO:  MAXIMO:");
  lcd.setCursor(2, 1);
  lcd.print((int)minHumidity);
  lcd.print("%");
  lcd.setCursor(11, 1);
  lcd.print((int)maxHumidity);
  lcd.print("%");
  delay(msDelay);
  lcd.clear();
}

/**
 * Se encarga de colocar los limites
 * mínimo y máximo del rango de humedad
 * del suelo, verifica cada valor ingresado
 * para evitar errores y los guarda en la
 * memoria EEPROM
 */
void setHumidityRange()
{
  digitalWrite(RELAY_PIN, HIGH);

  boolean successfulNewLimit = false;
  int index = 0;

  int minHumidityTemp = -1;
  String textMinHumidity = "";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humedad minima:");
  lcd.blink();

  while (!successfulNewLimit)
  {
    char key = numpad.getKey();
    lcd.setCursor(index, 1);
    if (key != NULL)
    {
      if (key == 'B' || key == 'D' || key == '#' || key == '*')
      {
        // Nothing
      }
      else if (key == 'A')
      {
        minHumidityTemp = textMinHumidity.toInt();
        if (!percentChecker(minHumidityTemp))
        {
          invalidValueHumidityScreen();
          lcd.print("Humedad minima:");
          textMinHumidity = "";
          minHumidityTemp = -1;
          index = 0;
        }
        else
        {
          successfulNewLimit = true;
        }
      }
      else if (key == 'C')
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CANCELADO");
        delay(500);
        lcd.noBlink();
        showHumidityRange(1500);
        return;
      }
      else
      {
        lcd.print((char)key);
        textMinHumidity += key;
        index++;
      }
    }
  }

  successfulNewLimit = false;
  index = 0;

  int maxHumidityTemp = -1;
  String textMaxHumidity = "";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humedad maxima:");

  while (!successfulNewLimit)
  {
    char key = numpad.getKey();
    lcd.setCursor(index, 1);
    if (key != NULL)
    {
      if (key == 'B' || key == 'D' || key == '#' || key == '*')
      {
        // Nothing
      }
      else if (key == 'A')
      {
        maxHumidityTemp = textMaxHumidity.toInt();
        if (!percentChecker(maxHumidityTemp))
        {
          invalidValueHumidityScreen();
          lcd.print("Humedad maxima:");
          textMaxHumidity = "";
          maxHumidityTemp = -1;
          index = 0;
        }
        else if (maxHumidityTemp <= minHumidityTemp)
        {
          invalidValueHumidityScreen();
          lcd.print("MINIMO:");
          lcd.print(minHumidityTemp);
          delay(1000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Humedad maxima:");
          textMaxHumidity = "";
          maxHumidityTemp = -1;
          index = 0;
        }
        else
        {
          successfulNewLimit = true;
        }
      }
      else if (key == 'C')
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CANCELADO");
        delay(500);
        break;
      }
      else
      {
        lcd.print((char)key);
        textMaxHumidity += key;
        index++;
      }
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MIN:");
  lcd.print(minHumidityTemp);
  lcd.print(" MAX:");
  lcd.print(maxHumidityTemp);

  lcd.setCursor(0, 1);
  lcd.print("[A]/[C]");

  while (successfulNewLimit)
  {
    char key = numpad.getKey();
    if (key == 'A')
    {
      minHumidity = minHumidityTemp;
      maxHumidity = maxHumidityTemp;

      EEPROM.put(12, maxHumidity);
      EEPROM.put(8, minHumidity);

      wateringState = false;

      break;
    }
    else if (key == 'C')
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANCELADO");
      delay(500);
      break;
    }
  }

  lcd.noBlink();

  showHumidityRange(1500);

  lcd.clear();
}

/**
 * Muestra en la pantalla LCD el
 * mensaje de que el valor es invalido
 */
void invalidValueHumidityScreen()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INGRESE UN VALOR");
  lcd.setCursor(0, 1);
  lcd.print("VALIDO");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
}

/**
 * Inicia el proceso de calibración del
 * sensor de humedad del suelo, obtiene
 * los valores del sensor en ambos casos
 * luego de confirmación del usuario y
 * los guarda en la EEPROM
 */
void calibrateSoilSensor()
{
  digitalWrite(RELAY_PIN, HIGH);

  boolean successfulCalibrate = false;

  int airValueTemp;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SENSOR DE HMD EN");

  while (!successfulCalibrate)
  {
    airValueTemp = analogRead(HUMIDITY_SENSOR_PIN);
    lcd.setCursor(0, 1);
    lcd.print("AIRE:");
    lcd.print(airValueTemp);
    lcd.print("    ");
    lcd.setCursor(10, 1);
    lcd.print("[A/C]");

    char key = numpad.getKey();
    switch (key)
    {
    case 'A':
      airValueTemp = airValueTemp;
      successfulCalibrate = true;
      break;

    case 'C':
      successfulCalibrate = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANCELADO");
      delay(500);
      lcd.clear();
      return;

    default:
      break;
    }
    delay(250);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SENSOR DE HMD EN");

  int waterValueTemp;
  successfulCalibrate = false;

  while (!successfulCalibrate)
  {
    waterValueTemp = analogRead(HUMIDITY_SENSOR_PIN);
    lcd.setCursor(0, 1);
    lcd.print("AGUA:");
    lcd.print(waterValueTemp);
    lcd.print("    ");
    lcd.setCursor(10, 1);
    lcd.print("[A/C]");

    char key = numpad.getKey();
    switch (key)
    {
    case 'A':
      waterValueTemp = waterValueTemp;
      successfulCalibrate = true;
      break;

    case 'C':
      successfulCalibrate = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANCELADO");
      delay(500);
      lcd.clear();
      return;

    default:
      break;
    }
    delay(250);
  }

  airValue = airValueTemp;
  waterValue = waterValueTemp;

  EEPROM.put(0, airValue);
  EEPROM.put(4, waterValue);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CALIBRACION");
  lcd.setCursor(0, 1);
  lcd.print("COMPLETA");
  delay(1000);
  lcd.clear();
}

/**
 * Cambia el estado del LED del
 * backlight de la pantalla LCD
 */
void toggleBacklightLCD()
{
  if (backlightLCD)
  {
    backlightLCD = false;
    lcd.noBacklight();
  }
  else
  {
    backlightLCD = true;
    lcd.backlight();
  }
}

/**
 * Imprime en pantalla el mensaje de inicio del programa
 */
void startScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("INICIALIZANDO ");
  lcd.write((int8_t)1);

  lcd.setCursor(0, 1);
  lcd.print("[");

  lcd.setCursor(15, 1);
  lcd.print("]");
  for (int i = 1; i <= 14; i++)
  {
    lcd.setCursor(i, 1);
    lcd.write((uint8_t)2);
    delay(75);
  }
  delay(100);
  lcd.clear();
}

/**
 * Se encarga de limpiar la pantalla solo cuando
 * se reduce los dígitos del numero que se muestra
 * en la pantalla para eliminar los números sobrantes
 * que se quedan al no borrar toda la pantalla
 *
 * @param int number numero base a revisar
 * @param int preNumber numero anterior al revisado actualmente
 */
int condicionalClearLCDByNumberInLCD(int number, int preNumber)
{
  if (preHumidity == 0 && preTemperature == 0 && preSoilMoisturePercent == 0)
  {
    return;
  }

  if (preNumber < 10 && number < 10)
  {
    return;
  }

  if ((10 <= preNumber < 100) && number < 10)
  {
    lcd.clear();
  }
  if (preNumber == 100 && number < 100)
  {
    lcd.clear();
  }

  return number;
}

/**
 * Arregla el porcentaje que se le ingresa
 * para que no sea superior a 100 ni menor
 * a 0
 *
 * @param int toFix
 * @return int
 *
 */
int fixPercent(int toFix)
{
  int fixed;

  if (toFix < 0)
  {
    fixed = 0;
  }
  else if (toFix > 100)
  {
    fixed = 100;
  }
  else
  {
    fixed = toFix;
  }

  return fixed;
}

/**
 * Regresa si es que el valor ingresado es
 * un porcentaje valido o no
 */
boolean percentChecker(int percentToCheck)
{
  return (0 <= percentToCheck && percentToCheck <= 100);
}

/**
 * Imprime en monitor serial el porcentaje y el valor
 * en raw del sensor de humedad de suelo
 */
void debuggingSoilMoisture()
{
  Serial.print("Soil Moisture Percent: ");
  Serial.print(soilMoisturePercent);
  Serial.print("% | Soil Moisture Raw: ");
  Serial.println(soilMoistureRaw);
}

/**
 * Inicia e imprime en puerto Serial los datos
 * que se guardaron en la EEPROM
 */
void debuggingEEPROMData()
{
  Serial.println(EEPROM.length());
  Serial.println(airValue);
  Serial.println(waterValue);
  Serial.println(minHumidity);
  Serial.println(maxHumidity);
}
