int matched_id;
bool matchFace;

#include <DFRobotDFPlayerMini.h>
//#include <Softwareserial.h>
HardwareSerial myHardwareSerial(1); 
#define RX 16
#define TX 4
DFRobotDFPlayerMini myDFPlayer;

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2);

#include <time.h> 
const char *ntpServer = "pool.ntp.org";   //ntp伺服器網址
const long gmtOffset_sec = 28800;         //GMT+8 為28800秒
const int daylightOffset_sec = 0;         //台灣不使用日光節約時間

#include <WiFi.h> 
#include <HTTPClient.h>
#include <WiFiClientSecure.h>  
const char *ssid = "esp32Test";          //請改成當前網路名稱
const char *password = "12345678";  //請改成當前網路密碼
String Linetoken = "JXEYENLBv8TQ6q1otjTBj5nWMsC0WVNmHZaAfn15R5o";
String IFTTTUrl="http://maker.ifttt.com/trigger/RecordSave/with/key/OFAE4Jv-tYz8PLA4QMW8k";

WiFiClientSecure clientLine;//網路連線物件
HTTPClient http;
char host[] = "notify-api.line.me";//LINE Notify API網址

#define waterSens A5       //define water sensor
//#define led 13

int waterVal;              //define the water sensor value
char playSitu;
int VisitorNumber = 1;

//MLX90614函式庫匯入
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#define I2C_SDA1 14
#define I2C_SCL1 15

//OV2640 使用參數
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

static ETSTimer lcdTimer;
struct tm timeinfo; 

TaskHandle_t Task1;
char SendFlag;
 
void startCameraServer();

/*
static void ICACHE_FLASH_ATTR lcdTimerCB(void *arg)
{
    //lcd.clear(); 
    //lcd.setCursor(3,0);
    //lcd.print(&timeinfo, "%F");
    lcd.setCursor(4,1);
    lcd.print(&timeinfo, "%T");

    ets_timer_disarm(&lcdTimer);
    ets_timer_setfn(&lcdTimer, lcdTimerCB, NULL);
    ets_timer_arm(&lcdTimer, 1000, 1);
}*/

void setup()
{ 
  //xTaskCreatePinnedToCore(
  //Task1code, /* Task function. */
  //"Task1",   /* name of task. */
  //10000,     /* Stack size of task */
  //NULL,      /* parameter of the task */
  //0,         /* priority of the task */
  //&Task1,    /* Task handle to keep track of created task */
  //0);        /* pin task to core 0 */	

  Wire.begin(I2C_SDA1,I2C_SCL1);

  lcd.begin();
	lcd.backlight();
  lcd.clear();

  lcd.setCursor(4,0);
  lcd.print("system");
  lcd.setCursor(1,1);
  lcd.print("initialization");
  delay(500);

  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Initializing");
  lcd.setCursor(14,0);
  lcd.blink();
  delay(50);
  
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif


  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("WIFI");
  lcd.setCursor(1,1);
  lcd.print("initialization");
  delay(500);
  

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WIFI CONNECTING");
    lcd.setCursor(15,0);
    lcd.blink();
    delay(50);
    }
    Serial.println("");
    Serial.println("WiFi connected");
   
    lcd.clear();
    lcd.noBlink();
    lcd.setCursor(0,0);
    lcd.print("WIFI CONNECTED");
    lcd.setCursor(0,1);
    lcd.print(ssid);
    delay(2000);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Website URL");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());
    delay(2000);

  startCameraServer();
  
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("initialization");
  lcd.setCursor(1,1);
  lcd.print("camera");
  delay(500);

  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Initializing");
  lcd.setCursor(14,0);
  lcd.blink();
  delay(50);
  
  

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  mlx.begin();
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
  //設定NTP參數
  

  myHardwareSerial.begin(9600, SERIAL_8N1, RX, TX);
  myDFPlayer.begin(myHardwareSerial);//將DFPlayer播放器宣告在HardwareSerial控制
  delay(500);
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(25);
  

  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("System OK");
  delay(5000);

  myDFPlayer.playMp3Folder(6);  
  delay(5000);
 /*
    ets_timer_disarm(&lcdTimer);
    ets_timer_setfn(&lcdTimer, lcdTimerCB, NULL);
    ets_timer_arm(&lcdTimer, 1000, 1);
*/

  //pinMode(led,OUTPUT);
}
/*
void BlinkLED(int pin,int *led_state)
{
  digitalWrite(pin,*led_state);
  for (size_t i = 0; i < 6; i++)
  {
    //digitalWrite(pin,*led_state);
    delay(1000);
    *led_state=!(*led_state);//改變LED狀態
  }
}
*/
/*
void Task1code( void * pvParameters ){
  for(;;){
    if(SendFlag == 'a'){
      playSitu = '0';   
      lcd.clear(); 
      lcd.setCursor(3,0);
      lcd.print(&timeinfo, "%F");
      lcd.setCursor(4,1);
      lcd.print(&timeinfo, "%T");
    }
     else {
      delay(1);
    }
  }
}
*/
void loop()
{
  lcd.noBlink();
  //static int state=HIGH;  //LED 的狀態，只初始化一次，所以聲明為靜態變量

	//struct tm timeinfo;                   //建立一時間結構為timeinfo
  if (!getLocalTime(&timeinfo)){        //取回NTP時間
    Serial.println("網路時間獲取失敗");
    return;
  }

  float tempC = mlx.readObjectTempC() + 6;
  waterVal = analogRead(waterSens); //read the water sensor
  String messageTempC = "偵測到異常 請盡速處理  使用者" + String(matched_id) +" : 體溫過高禁止進入";
  String messageVisitorError = "偵測到異常 請盡速處理  訪客" + String(VisitorNumber) +" : 體溫過高禁止進入";
  String messageWater = "偵測到異常 請盡速處理  酒精不足";
  String messagePass = "\n使用者" + String(matched_id) +" : 溫度=" + String(tempC) + " *C";
  String messageVisitor = "\n訪客" + String(VisitorNumber) +" : 溫度=" + String(tempC) + " *C";
  char wrongInfo;
    wrongInfo = '0';
  String messageSitu;

  Serial.println(waterVal);  
  Serial.println(&timeinfo, "%F %T"); 
  Serial.println(mlx.readObjectTempC());
  Serial.println(matched_id); 
  Serial.println(xPortGetCoreID());//xPortGetCoreID()顯示執行核心編號
  delay(50);
  
  if (waterVal > 500){      //當液位過低
    if (matchFace == true){       
      if (tempC >38){
        wrongInfo = 'a';
        playSitu = '4';
        lcd.clear();
        //LCD輸出"temperature too high"
        lcd.setCursor(0,0);
        lcd.print("temperature");
        lcd.setCursor(1,8);
        lcd.print("too high");
        delay(5000);
        //BlinkLED(led,&state);//調用函數
        matchFace = false;
        }
      else {   //若額溫小於攝氏38度
        wrongInfo = 'c';
        playSitu = '3';
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("your temperature");
        lcd.setCursor(0,1);
        lcd.print(tempC);
        delay(2000);
        //playSitu = '0';
        matchFace = false;
      }
    }
    else {
      if (tempC >33){
        delay(2000);
        tempC = mlx.readObjectTempC() + 6;
        if (tempC >33){
          delay(2000);
          tempC = mlx.readObjectTempC() + 6;
          if (tempC >33){
            delay(2000);
            tempC = mlx.readObjectTempC() + 6;
            if (tempC >33 and tempC <38){
              wrongInfo = 'd';
              playSitu = '9';
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("your temperature");
              lcd.setCursor(0,1);
              lcd.print(tempC);
              delay(2000);
              VisitorNumber++;
              matchFace = false;
            }
            else if (tempC >=38) {
              wrongInfo = 'e';
              playSitu = '4';
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("your temperature");
              lcd.setCursor(0,1);
              lcd.print("too high");
              delay(5000); 
              VisitorNumber++;
            }
          }
        }
      }
      else{
        SendFlag = 'a';    
        playSitu = '0';   
        lcd.clear(); 
        lcd.setCursor(3,0);
        lcd.print(&timeinfo, "%F");
        delay(1);
        lcd.setCursor(4,1);
        lcd.print(&timeinfo, "%T");
        delay(1);
        }
      }
    }
  else {
    wrongInfo = 'b';
    playSitu = '1';
    lcd.clear(); 
    lcd.setCursor(1,0);           //設定LCD顯示位置
    lcd.print("Alcohol  Empty");   //顯示"Alcohol Empty" 酒精不足
    delay(100);
    }

  switch (playSitu){
    //偵測到酒精量不足，請補充
    case '1':
      myDFPlayer.playMp3Folder(1);  
      delay(3500);
      break;
    //臉部辨識成功
    case '3':
      myDFPlayer.playMp3Folder(3);  
      delay(2500);
      break;
    //體溫過高禁止進入
    case '4':
      myDFPlayer.playMp3Folder(4);  //Play the first mp3
      delay(2750);
      break;
    //陳義文給我錢
    case '7':
      myDFPlayer.playMp3Folder(7);  
      delay(2500);
      break;
    //體溫正常
    case '9':
      myDFPlayer.playMp3Folder(9);  
      delay(2050);
      break;
    case '0':
      myDFPlayer.stop();
      break;
    }
    
  switch (wrongInfo){
    case 'a':
      messageSitu = messageTempC;
      break;
    case 'b':
      messageSitu = messageWater;
      break;
    case 'c':
      messageSitu = messagePass;
      break;
    case 'd':
      messageSitu = messageVisitor;
      break;
    case 'e':
      messageSitu = messageVisitorError;
      break;
    }

  //設定觸發LINE訊息條件
  if (wrongInfo != '0'){
    //組成Line訊息內容
    String message = "";
    message +=  messageSitu;
    String url;
    if (wrongInfo == 'd'){
      url=IFTTTUrl+"?value1="+ "Visitor" + String(VisitorNumber) +"&value2="+String(tempC);
    }
    else{
      url=IFTTTUrl+"?value1="+ "User" + String(matched_id) +"&value2="+String(tempC);
    }
    http.begin(url);
    int httpCode = http.GET(); 
    if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
    http.end();                                                           
 
  delay(500);
    
    if (clientLine.connect(host, 443)) {
      int LEN = message.length();
      //傳遞POST表頭
      String urlLine = "/api/notify";
      clientLine.println("POST " + urlLine + " HTTP/1.1");
      clientLine.print("Host: "); clientLine.println(host);
      //權杖
      clientLine.print("Authorization: Bearer "); clientLine.println(Linetoken);
      clientLine.println("Content-Type: application/x-www-form-urlencoded");
      clientLine.print("Content-Length: "); clientLine.println( String((LEN + 8)) );
      clientLine.println();      
      clientLine.print("message="); clientLine.println(message);
      clientLine.println();
      //等候回應
      delay(2000);
      String response = clientLine.readString();
      //顯示傳遞結果
      Serial.println(response);
      clientLine.stop(); //斷線，否則只能傳5次
    }
    else {
      //傳送失敗
      Serial.println("connected fail");
      lcd.clear(); 
      lcd.setCursor(0,0);           //設定LCD顯示位置
      lcd.print("Line Nessage");
      lcd.setCursor(0,1);           //設定LCD顯示位置
      lcd.print("Failed"); 
      delay(500);
    }
    delay(500);
  }
}



