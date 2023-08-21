// Bibliotecas para pantalla LCD I2C
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// Bibliotecas para sensor de humedad y temperatura ambiental
#include <DHT.h>
#include <DHT_U.h>

// Biblioteca para modificar y leer la memoria EEPROM
#include <EEPROM.h>

// Definiciones de los pines de entradas y salidas
#define HUMIDITY_SENSOR_PIN A1
#define PUMP_PIN 13
#define DHT_PIN 2

// Valores de calibracion del sensor de humedad del suelo
int airValue = EEPROM.get(0, airValue);     // Valor que da el sensor de humedad del suelo en el aire (Su valor se guarda en 0)
int waterValue = EEPROM.get(4, waterValue); // Valor que da el sensor de humedad del suelo en el agua (Su valor se guarda en 4)

int soilMoistureRaw;
int soilMoisturePercent;

int temperature;
int humidity;

int minHumidity = 60;
int maxHumidity = 80;

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

DHT dht(DHT_PIN, DHT11);

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

/**
 * Se encarga de ejecutar una sola
 * vez todo el codigo encargado de
 * activar y preparar todo para que
 * el bucle se mantenga correctamente
 */
void setup()
{
  Serial.begin(9600);
  dht.begin();

  Serial.println(EEPROM.length());

  pinMode(PUMP_PIN, OUTPUT);

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

  preSoilMoisturePercent = condicionalClearLCDByNumberInLCD(soilMoisturePercent, preSoilMoisturePercent);
  preTemperature = condicionalClearLCDByNumberInLCD(temperature, preTemperature);
  preHumidity = condicionalClearLCDByNumberInLCD(humidity, preHumidity);

  showAmbientData();

  if (soilMoisturePercent < minHumidity)
  {
    wateringState = true;
  }

  wateringState = watering(wateringState);
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

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

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
 * se debe regar o no, en cuanto llega al maximo del
 * rango de humedad requerido detiene el riego y devuelve
 * false
 *
 * @param boolean state
 */
boolean watering(boolean state)
{
  if (state)
  {
    if (soilMoisturePercent < maxHumidity)
    {
      digitalWrite(PUMP_PIN, HIGH);
      wateringMsj();
      return true;
    }
    else if (soilMoisturePercent >= maxHumidity)
    {
      digitalWrite(PUMP_PIN, LOW);
      lcd.clear();
      return false;
    }
  }
  else
  {
    digitalWrite(PUMP_PIN, LOW);
    return false;
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
 * Se encarga de limpiar la pantalla solo cuando
 * se reduce los digitos del numero que se muestra
 * en la pantalla para eliminar los numeros sobrantes
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
