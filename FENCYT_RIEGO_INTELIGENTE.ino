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

byte celsiusGrades[] = {
    B11100,
    B10100,
    B11100,
    B00000,
    B00111,
    B00100,
    B00100,
    B00111};

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
  Serial.begin(9600);
  dht.begin();

  Serial.println(EEPROM.length());
  Serial.println(airValue);
  Serial.println(waterValue);
  Serial.println(minHumidity);
  Serial.println(maxHumidity);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  lcd.clear();

  lcd.createChar(0, celsiusGrades);
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

  showAmbientData();
  optionSelector();

  watering();
  debuggingSoilMoisture();
  delay(500);
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
 * Usa el booleano de estado de riego para ver si
 * se debe regar o no, en cuanto llega al máximo del
 * rango de humedad requerido detiene el riego y establece
 * el estado de riego en false
 *
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
    showHumidityRange();
    break;

  default:
    break;
  }
}

/**
 * Muestra el rango de humedad actual
 * al que esta configurado el sistema
 */
void showHumidityRange()
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
  delay(5000);
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
