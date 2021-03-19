#include <WiFiManager.h>


#define TRIGGER_PIN 36                            // portal trigger pin 
#define PIN_RESET_BUTTON TRIGGER_PIN              // reset trigger pin

/* timeer variables */
typedef struct
{
  unsigned long currentTime;
  unsigned long previousTime;
} Time_t;

Time_t reconnect = {0, 0};
Time_t reset = {0, 0};
Time_t portal = {0, 0};

/* Status Flag */
bool isConfigRequest = true;
bool isWifiConnected = false;

WiFiManager wm;                                   // object to the wifimanager



/******************************* RTOS *********************************/

/* Task Handles */
TaskHandle_t task1_handle;
TaskHandle_t task2_handle;





/******************************* Setup ********************************/
void setup()
{
  /* IO pin config*/
  pinMode(TRIGGER_PIN, INPUT);

  /* Serial Config */
  Serial.begin(115200);

  /* Wifi Config */
  WiFi.mode(WIFI_STA);

  wm.setConfigPortalTimeout(60);                    // portal Timeout (60 seconds)
  wm.setDarkMode(1);                                // Dark mode

  WiFi.reconnect();                                 // wifi reconnect to connect the wifi if already configured


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


  /********************** RTOS config **********************/

  xTaskCreate(
    WifiWatch,                                /* Task function. */
    "wifi watch task",                        /* String with name of task. */
    4096,                                     /* Stack size in bytes. */
    NULL,                                     /* Parameter passed as input of the task */
    1,                                        /* Priority of the task. */
    &task1_handle);                           /* Task handle. */

  xTaskCreate(
    taskTwo,
    "TaskTwo",
    4096,
    NULL,
    1,
    &task2_handle);

}

void loop()
{
  // Idle task
}

/*
   Task One - Wifi Watch
   note: this task is used to monitor and change the wifi settings
*/

void WifiWatch( void * parameter )
{

  while (1)
  {
    /* wifi reconnect */

    if (WiFi.status() != WL_CONNECTED)
    {
      isConfigRequest = true;
      isWifiConnected = false;

      reconnect.currentTime = xTaskGetTickCount();
      if (reconnect.currentTime - reconnect.previousTime >= 3000)
      {

        Serial.print(".");
        WiFi.disconnect();
        WiFi.reconnect();


        reconnect.previousTime = reconnect.currentTime;
      }
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      //      Serial.println("connected");
      isConfigRequest = false;
      isWifiConnected = true;
    }


    /* Wifi reset sequence */

    if (isConfigRequest == false && isWifiConnected == true)
    {
      if (digitalRead(PIN_RESET_BUTTON) == HIGH)
      {
        reset.currentTime = xTaskGetTickCount();
        //Serial.println("Press detected");

        if (reset.currentTime - reset.previousTime >= 1000)
        {

          delay(3000);
          while ( digitalRead(PIN_RESET_BUTTON) == HIGH)
          {
            //Serial.println("Long press detected");
            Serial.println("Erase settings and restart ...");

            delay(1000);
            reset.previousTime = reset.currentTime;
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
        //        Serial.println("Press 1 detected");

        wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));      // Static ip for the webportal

        portal.currentTime = xTaskGetTickCount();

        if (portal.currentTime - portal.previousTime >= 1000)
        {
          delay(3000);
          while ( digitalRead(TRIGGER_PIN) == HIGH)
          {
            Serial.println("Starting Config portal...");
            delay(1000);

            startconfigportal = wm.startConfigPortal();                                 // Starting the config portal

            portal.previousTime = portal.currentTime;
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
