// ==================================
// 全体共通のヘッダファイルのinclude
#include <Arduino.h>                         // Arduinoフレームワークを使用する場合は必ず必要
#include <Update.h>                          // 定義しないとエラーが出るため追加。
#include <Ticker.h>                          // 定義しないとエラーが出るため追加。
#include <M5Unified.h>                       // M5Unifiedライブラリ
// ================================== End

// ==================================
// for SystemConfig
#include <Stackchan_system_config.h>          // stack-chanの初期設定ファイルを扱うライブラリ
StackchanSystemConfig system_config;          // (Stackchan_system_config.h) プログラム内で使用するパラメータをYAMLから読み込むクラスを定義
// ================================== End

// ==================================
// #defines
// 各要素の使用／不使用を切り替え

#define USE_LED                      // LEDを使用する場合（Grove経由でLEDテープおよびNeoHEXを点灯させる想定）

#define USE_Servo                    // サーボモーターを使用する場合（背面のGPIOから360度サーボモーターを動かす想定）
#define USE_Servo_360_TowerPro       // 使用するサーボモーターのメーカーごとのパラメータ設定。3種から選択。
// #define USE_Servo_360_Feetech360
// #define USE_Servo_360_M5Stack

#define USE_ESPNow                      // ESPNowを使用する場合
// ================================== End

// ==================================
// for LED
#ifdef USE_LED
  #include <FastLED.h>
  #define ATOM_LED_PIN  27      // ATOM Lite本体のLED用
  #define NEOPIXEL_PIN  26      // NeoPixel LED用
  #define ATOM_NUM_LEDS 1       // Atom LED
  #define NEOHEX_NUM_LEDS 37    // HEX LED
  #define NEOTAPE_NUM_LEDS 29   // TAPE LED
  #define EXTRA_NUM_LEDS NEOHEX_NUM_LEDS + NEOTAPE_NUM_LEDS // HEX LEDとTAPE LEDを連結して使用する場合
  
  static CRGB atom_leds[ATOM_NUM_LEDS];
  static CRGB extra_leds[EXTRA_NUM_LEDS];
  
  uint8_t gHue = 0;             // Initial tone value.
  
  bool isLedON = false;  // サーボ動作のフラグ

  // Atom本体のLEDを光らせる用の関数
  void setLed(CRGB color)
  {
    // change RGB to GRB
    uint8_t t = color.r;
    color.r = color.g;
    color.g = t;
    atom_leds[0] = color;
    FastLED.show();
  }

  // LED点灯開始
  void startLED() {
    isLedON = true;
  }
  
  // LED点灯モード制御
  void flashLedMode() {
    fill_rainbow_circular(extra_leds, EXTRA_NUM_LEDS, gHue, 7);
    FastLED.show();  // Updated LED color.
    EVERY_N_MILLISECONDS(20)
    {
      gHue++;
    }  // The program is executed every 20 milliseconds.
  }
#endif
// ================================== End


// ==================================
// for Servo
#ifdef USE_LED
#include <ServoEasing.hpp>

/**
 * サーボのPWM値と回転方向の関係（最右列は180サーボの場合の角度）
 * 
 * Tower Pro
 * 時計回り   : 500 - 1500US : 0 - 90度
 * 停止       : 1500US : 90度
 * 反時計周り : 1500 - 2400US : 90 - 180度
 * 
 * Feetech
 * 時計回り   : 700 - 1500US : 0 - 90度
 * 停止       : 1500US : 90度
 * 反時計周り : 1500 - 2300US : 90 - 180度
 * 
 * M5Stack
 * 時計回り   : 500 - 1500US : 0 - 90度
 * 停止       : 1500US : 90度
 * 反時計周り : 1500 - 2500US : 90 - 180度
*/

#define SERVO_360_PIN 25      // サーボモーター用
ServoEasing servo360;  // 360度サーボ

// サーボの種類毎のPWM幅や初期角度、回転速度のレンジ設定など
#ifdef USE_Servo_360_TowerPro
  const int MIN_PWM_360 = 500;
  const int MAX_PWM_360 = 2400;
  // const int START_DEGREE_VALUE_SERVO_360 = 90;     // 360度サーボの停止位置：仕様では90で停止
  const int START_DEGREE_VALUE_SERVO_360 = 95;        // サーボ個体差で、90度指定で停止しなかった場合値を変えてみる（試作に使用したsg90-hvの場合95付近で停止だった)
  const int SERVO_DEG_RANGE_MAX = 12;
  const int SERVO_DEG_RANGE_MIN = -1 * SERVO_DEG_RANGE_MAX;
#endif
#ifdef USE_Servo_360_Feetech360
  const int MIN_PWM = 700;
  const int MAX_PWM = 2300;
  // const int START_DEGREE_VALUE_SERVO_360 = 90;     // 360度サーボの停止位置：仕様では90で停止
  const int START_DEGREE_VALUE_SERVO_360 = 93;        // サーボ個体差で、90度指定で停止しなかった場合値を変えてみる（試作に使用したsg90-hvの場合95付近で停止だった)
  const int SERVO_DEG_RANGE_MAX = 6;
  const int SERVO_DEG_RANGE_MIN = -1 * SERVO_DEG_RANGE_MAX;
#endif
#ifdef USE_Servo_360_M5Stack
  const int MIN_PWM = 500;
  const int MAX_PWM = 2500;
  const int START_DEGREE_VALUE_SERVO_360 = 90;         // 360度サーボの停止位置：仕様では90で停止（M5Stack公式は停止のレンジが85～95あたりと広めにとられている様子。手元では個体差なし）
  const int SERVO_DEG_RANGE_MAX = 12;
  const int SERVO_DEG_RANGE_MIN = -1 * SERVO_DEG_RANGE_MAX;
#endif

bool isRandomRunning = false;  // サーボ動作のフラグ
unsigned long prevTime360 = 0;
unsigned long interval360 = 0;

int servo360_speed = 0; // 360サーボの速度用変数

// ================================== End

// ==================================
// for Servo Running Mode

// サーボのタイマーを初期化する関数
void initializeServoTimers() {
  unsigned long now = millis();
  prevTime360 = now;
  interval360 = random(0, 101);
}

// ランダムモード開始時にタイマーをリセット
void startRandomMode() {
  isRandomRunning = true;
  initializeServoTimers();  // タイマー初期化
}

// ランダムモード
void servoRandomRunningMode(unsigned long currentMillis) {

  // === 360°サーボの動作 (7秒〜30秒間隔) ===
  if (currentMillis - prevTime360 >= interval360) {
    prevTime360 = currentMillis;
    interval360 = random(7000, 30001); // 7秒〜30秒のランダム間隔

    // 66 ～ 114 度が示す速度（初期位置は90、2度刻み）
    int rand_speed_offset_360 = random(SERVO_DEG_RANGE_MIN, SERVO_DEG_RANGE_MAX)* 2;
    servo360.startEaseTo(START_DEGREE_VALUE_SERVO_360 + rand_speed_offset_360);
  }
}

// ★テストモード
int count_360 = 0;
void servoTestRunningMode(unsigned long currentMillis) {

  // === 360°サーボの動作 (5秒間隔) ===
  if (currentMillis - prevTime360 >= interval360) {
    prevTime360 = currentMillis;
    interval360 = 5000; // 5秒間隔固定

    // 60 ～ 120 度が示す速度（初期位置は90）
    servo360.startEaseTo(START_DEGREE_VALUE_SERVO_360 + (count_360 % 7) * 10 - 30);
    count_360 = (count_360 + 1) % 99;
  }
}
#endif
// ================================== End

// ==================================
// for ESPNow
#ifdef USE_ESPNow
#include <WiFi.h>  // ESPNOWを使う場合はWiFiも必要
#include <esp_now.h> // ESPNOW本体

// ESP-NOW受信時に呼ばれる関数
void OnDataReceived(const uint8_t *mac_addr, const uint8_t *data, int data_len) {

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);

  if (data[0] == 1) {
    if (!isRandomRunning || !isLedON) {
      #ifdef USE_Servo
        startRandomMode();
      #endif
      #ifdef USE_LED
        setLed(CRGB::Blue);
        startLED();
      #endif
    } else {
      #ifdef USE_Servo
        servo360.startEaseTo(START_DEGREE_VALUE_SERVO_360);  // 360°サーボを停止
        isRandomRunning = false;
      #endif
      #ifdef USE_LED
        FastLED.clear();  // Set All LED Black color.
        FastLED.show();   // Updated LED color.
        isLedON = false;
      #endif
    }
  }
}
#endif
// ================================== End

void setup() {
  // 設定用の情報を抽出
  auto cfg = M5.config();
  // M5Stackをcfgの設定で初期化
  M5.begin(cfg);

  // ---------------------------------------------------------------
  // LEDの初期化
  // ---------------------------------------------------------------
  #ifdef USE_LED
    FastLED.addLeds<WS2811, ATOM_LED_PIN, RGB>(atom_leds, ATOM_NUM_LEDS);
    FastLED.addLeds<WS2811, NEOPIXEL_PIN, RGB>(extra_leds, EXTRA_NUM_LEDS);
    FastLED.setBrightness(5);
    for (int i = 0; i < 3; i++)
    {
      setLed(CRGB::Red);
      delay(500);
      setLed(CRGB::Black);
      delay(500);
    }
  #endif

  // ---------------------------------------------------------------
  // servoの初期化
  // ---------------------------------------------------------------
  #ifdef USE_Servo
    M5_LOGI("attach servo");
    ESP32PWM::allocateTimer(0);                                  // ESP32Servoはタイマーを割り当てる必要がある

    servo360.setPeriodHertz(50);                                 // サーボ用のPWMを50Hzに設定
    servo360.attach(SERVO_360_PIN, MIN_PWM_360, MAX_PWM_360);
    servo360.setEasingType(EASE_LINEAR);                         // 一定の速度で動かす場合は EASE_LINEAR に変更
    setSpeedForAllServos(60);
    servo360.startEaseTo(START_DEGREE_VALUE_SERVO_360);          // 360°サーボを停止位置にセット

    M5.Power.setExtOutput(!system_config.getUseTakaoBase());     // 設定ファイルのTakaoBaseがtrueの場合は、Groveポートの5V出力をONにする。
    M5_LOGI("ServoType: %d\n", system_config.getServoType());    // サーボのタイプをログに出力

    // ランダム動作用の変数初期化、個体差を出すためMACアドレスを使用する
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    uint32_t seed = mac[0] | (mac[1] << 8) | (mac[2] << 16) | (mac[3] << 24);
    randomSeed(seed);
    interval360 = random(7000, 30001); // 7秒〜30秒のランダム間隔
  #endif

  // ---------------------------------------------------------------
  // ESPNowの初期化
  // ---------------------------------------------------------------
  #ifdef USE_LED
    // WiFi初期化
    WiFi.mode(WIFI_STA);

    // ESP-NOWの初期化(出来なければリセットして繰り返し)
    if (esp_now_init() != ESP_OK) {
      return;
    }

    // ESP-NOW受信時に呼ばれる関数の登録
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataReceived));
  #endif
}

void loop() {
  M5.update();

  // === ボタンAが押されたらテスト動作モードの開始/停止を切り替え ===
  if (M5.BtnA.wasPressed()) {
    if (!isRandomRunning || !isLedON) {
#ifdef USE_Servo
      startRandomMode();
#endif
#ifdef USE_LED
      startLED();
#endif
    } else {
#ifdef USE_Servo
      servo360.startEaseTo(START_DEGREE_VALUE_SERVO_360);  // 360°サーボを停止
      isRandomRunning = false;
#endif
#ifdef USE_LED
      FastLED.clear();  // Set All LED Black color.
      FastLED.show();   // Updated LED color.
      isLedON = false;
#endif
    }
  }
  
#ifdef USE_Servo
  if (!isRandomRunning) return;  // 停止中なら何もしない
  unsigned long currentMillis = millis();
  // ランダム動作モード（デフォルト）
  if (isRandomRunning) servoRandomRunningMode(currentMillis);
  // ★テストモード（段階的に回転速度を変えるデモ）を動かしたい場合はこちらを使用
  // if (isRandomRunning) servoTestRunningMode(currentMillis);
#endif

#ifdef USE_LED
  if (isLedON) flashLedMode();
#endif
}