#include <WiFiManager.h>

#define TRIGGER_PIN 36
#define PIN_RESET_BUTTON TRIGGER_PIN

unsigned long currentTime;
unsigned long previousTime = 0;
unsigned long currentTime1;
unsigned long previousTime1 = 0;

bool isConfigRequest = true;
bool isWifiConnected = false;

WiFiManager wm;

TaskHandle_t task1_handle;
TaskHandle_t task2_handle;

SemaphoreHandle_t wifiMutex;

void setup()
{

  pinMode(TRIGGER_PIN, INPUT);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  wm.setConfigPortalTimeout(60);
  wm.setDarkMode(1);

  WiFi.reconnect();
  Serial.println("Starting");

  delay(5000);
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi is not configured");
    delay(500);
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wifi is configured");
    delay(500);

    isConfigRequest = false;
    isWifiConnected = true;
  }

  wifiMutex = xSemaphoreCreateMutex();

  if (wifiMutex == pdFALSE)
  {
    Serial.println("Semaphore Creation Failed");
    while (1)
    {

    }
  }

  xTaskCreate(
    WifiWatch,          /* Task function. */
    "wifi watch task",        /* String with name of task. */
    4096,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    &task1_handle);            /* Task handle. */

  xTaskCreate(
    taskTwo,          /* Task function. */
    "TaskTwo",        /* String with name of task. */
    4096,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    &task2_handle);            /* Task handle. */

}

void loop()
{
  delay(1000);
}

void WifiWatch( void * parameter )
{


  while (1)
  {
    /*Wifi reset sequence */

    if (isConfigRequest == false && isWifiConnected == true)
    {
      if (digitalRead(PIN_RESET_BUTTON) == HIGH)
      {
        currentTime = xTaskGetTickCount();
        Serial.println("Press detected");

        if (currentTime - previousTime >= 1000)
        {
          delay(3000);

          while ( digitalRead(PIN_RESET_BUTTON) == HIGH)
          {
            Serial.println("Long press detected");
            Serial.println("Erase settings and restart ...");

            delay(1000);
            previousTime = currentTime;

            wm.resetSettings();

            Serial.println("Restarting ESP");
            delay(3000);

            ESP.restart();
            break;
          }
        }
      }
    }

    /* OnDemand Config portal*/

    if (isConfigRequest == true && isWifiConnected == false)
    {
      bool startconfigportal;
      
      if ( digitalRead(TRIGGER_PIN) == HIGH)
      {
        Serial.println("Press 1 detected");
        
        wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
        
        currentTime1 = xTaskGetTickCount();
       
        if (currentTime1 - previousTime1 >= 1000)
        {
          delay(3000);
          while ( digitalRead(TRIGGER_PIN) == HIGH)
          {
            Serial.println("Starting Config portal...");
            delay(1000);
            
            startconfigportal = wm.startConfigPortal();

            previousTime1 = currentTime1;
            break;
          }
        }

        if (!startconfigportal) 
        {
          Serial.println("failed to connect and hit timeout");
          delay(5000);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("connected...yeey :)");
          isConfigRequest = false;
          isWifiConnected = true;
        }

        else if (WiFi.status() != WL_CONNECTED)
        {
          Serial.println("connection...failed :(");
          isConfigRequest = true;
          isWifiConnected = false;
        }
      }

    }
    
    //wifi reconnect

    if (WiFi.status() != WL_CONNECTED)
    {
      isConfigRequest = true;
      isWifiConnected = false;

      currentTime2 = xTaskGetTickCount();
      if (currentTime2 - previousTime2 >= 3000)
      {
        Serial.print(".");
        WiFi.disconnect();
        WiFi.reconnect();

        previousTime2 = currentTime2;
      }
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("connected");
      isConfigRequest = false;
      isWifiConnected = true;
      delay(1000);
    }

    delay(500);
  }

  vTaskDelete( NULL );

}

void taskTwo( void * parameter)
{

  while (1)
  {
    Serial.println("task2");
    delay(1000);
  }

  vTaskDelete( NULL );

}
