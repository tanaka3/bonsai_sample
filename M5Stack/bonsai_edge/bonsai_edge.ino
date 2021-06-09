/**
 * Bonsaiの学習結果を元に、追いかけっこをtoioで実行するサンプルです。 
 * M5Stack Core2を使用していますが、ライブラリ等を修正いただければM5Stackでも動作すると思います。
 * 
 * 前提
 * Bonsaiのローカル環境と、M5Stackは同一ネットワークに接続されている必要があります。
 * toioが2台必要です。
 * toioの簡易プレイマットが必要です。
 * 
 * Toioとの接続は、以下のサイトを参考にさせていただきました。
 * https://note.com/vhideo/n/nff5e2845beb1
 */

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <M5Core2.h>
#include <M5GFX.h>
#include "BLEDevice.h"

#include "Model.h"
#include "Action.h"
#include "Config.h"
#include "State.h"

//-----------------------------------
// WiFi関連
//-----------------------------------
const char *ssid = "WiFiのSSID";                                   // WiFiのSSDI
const char *password = "WiFiのパスワード";                            // WiFiのパスワード
const char *api = "http://ラズパイのIPアドレス:5000/v1/prediction";   // Bonsaiのアクセス先

//-----------------------------------
// toio移動先などの
//-----------------------------------
Model model;            // 移動先の算出など
Config config;          // 環境情報
uint8_t edge_state = 0; // 状態

//-----------------------------------
// 描画関係
//-----------------------------------
M5GFX display;          // メイン画面
M5Canvas mapCanvas;     // バッファリング用


//-----------------------------------
// toio関係
//-----------------------------------
// https://toio.github.io/toio-spec/docs/hardware_position_id
static int PLAYMAT_LEFT= 98;        // 簡易プレイマットの右X座標
static int PLAYMAT_RIGHT= 401;      // 簡易プレイマットの左X座標
static int PLAYMAT_TOP= 142;        // 簡易プレイマットの上Y座標
static int PLAYMAT_BOTTOM= 358;     // 簡易プレイマットの下Y座標
static int PLAYMAT_WIDTH_NUM = 7;   // 簡易プレイマットの横のマス数
static int PLAYMAT_HEIGHT_NUM = 5;  // 簡易プレイマットの縦のマス数



static BLEUUID serviceUUID("10b20100-5b3b-4571-9508-cf3efcd7bbae");    // toio Core CUBE Service UUID
static BLEUUID charUUID_ID("10b20101-5b3b-4571-9508-cf3efcd7bbae");    // ID Information Characteristics UUID
static BLEUUID charUUID_MO("10B20102-5b3b-4571-9508-cf3efCD7BBAE");    // Motor Control Characteristics UUID

// キャラクタリスティックを格納する配列のINDEX -------------
#define ID_INFO     0     // ID読み取り
#define MOTOR_CONT  1     // モータコントロール

#define NUM_OF_MAX_CUBE 2 // CUBEの最大接続数

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLERemoteCharacteristic* pRemoteCharacteristic[ NUM_OF_MAX_CUBE ][ 2 ];
static BLEAdvertisedDevice* myDevice[ NUM_OF_MAX_CUBE ];
static uint16_t pRemotteHandler[2];

int cube_index = 0;

/**
 * toio1）Notifyを指定したキャラクタリスティックスを受信する部分
 */
static void notifyCallback_0(
 BLERemoteCharacteristic* pBLERemoteCharacteristic,
 uint8_t* pData,
 size_t length,
 bool isNotify){
  int i;
  uint16_t hnd = pBLERemoteCharacteristic->getHandle(); 
  notifyRecive( 0, hnd, pData, length );
}

/**
 * toio2）Notifyを指定したキャラクタリスティックスを受信する部分
 */
static void notifyCallback_1(
 BLERemoteCharacteristic* pBLERemoteCharacteristic,
 uint8_t* pData,
 size_t length,
 bool isNotify){
   int i;
   uint16_t hnd = pBLERemoteCharacteristic->getHandle(); 
   notifyRecive( 1, hnd, pData, length );
}

/**
 * toioの通知受信
 */
static void notifyRecive( int cid, uint16_t hnd, uint8_t* pData, size_t length ){
  int i;

  // ID認識時
  if( hnd == pRemotteHandler[ID_INFO] ){
    // Position ID 
    if( pData[0]==0x01 ){

      // 情報を取得
      int id_pos_x = (int)pData[1] + (int)pData[2]*256;   // toioのX座標
      int id_pos_y = (int)pData[3] + (int)pData[4]*256;   // toioのY座標
      int id_pos_a = (int)pData[5] + (int)pData[6]*256;   // toioの角度
          
      Serial.printf("Cno=%2d:X=%5d, Y=%5d, A=%5d\n", cid, id_pos_x, id_pos_y, id_pos_a); 

      //ゲーム開始待ちの場合は、駒の移動を有効する
      if(edge_state == 7){
        switch(cid){
          case 0:
            //逃げる駒の座標をプレイマットの座標に変換
            //プレイマットは中心が0座標なので、マットに合うように調整            
            config.run_position_x = (id_pos_x - PLAYMAT_LEFT) /((PLAYMAT_RIGHT - PLAYMAT_LEFT) / PLAYMAT_WIDTH_NUM) - 3;
            config.run_position_y = 2 - (id_pos_y - PLAYMAT_TOP) / ((PLAYMAT_BOTTOM - PLAYMAT_TOP) / PLAYMAT_HEIGHT_NUM) ;
            break;

           //追う駒の座標をプレイマットの座標に変換
           //プレイマットは中心が0座標なので、マットに合うように調整
           case 1:
            config.catch_position_x = (id_pos_x - PLAYMAT_LEFT) /((PLAYMAT_RIGHT - PLAYMAT_LEFT) / PLAYMAT_WIDTH_NUM) - 3;
            config.catch_position_y = 2 - (id_pos_y - PLAYMAT_TOP) / ((PLAYMAT_BOTTOM - PLAYMAT_TOP) / PLAYMAT_HEIGHT_NUM) ;            
            break;
        }
      }       
    }    
  }
} 

/**
 * toio接続のCallback
 */
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("**** onConnect ****");    
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

/**
 * toioに接続する
 */
bool connectToServer( int n ) {
   
  BLEClient*  pClient  = BLEDevice::createClient();   

  pClient->setClientCallbacks(new MyClientCallback());

  // toioに接続
  pClient->connect(myDevice[n]);  

  // サービスを取得する
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.printf(" -Connect CUBE-%d\n", n );    

   // ID取得のCharacteristicを取得する   
   pRemoteCharacteristic[n][ID_INFO] = pRemoteService->getCharacteristic(charUUID_ID);
   if (pRemoteCharacteristic[n][ID_INFO] == nullptr) { pClient->disconnect(); return false; }
    
   // モーター制御のCharacteristicを取得する 
   pRemoteCharacteristic[n][MOTOR_CONT] = pRemoteService->getCharacteristic(charUUID_MO);
   if (pRemoteCharacteristic[n][MOTOR_CONT] == nullptr) { pClient->disconnect(); return false; }

   // 通知用のHandlerを取得する
   pRemotteHandler[ID_INFO] = pRemoteCharacteristic[n][ID_INFO]->getHandle();

   // 通知を開始する（toioごとにCallbackを分ける）
   if(pRemoteCharacteristic[n][ID_INFO]->canNotify()){
     switch(n){
       case 0:pRemoteCharacteristic[n][ID_INFO]->registerForNotify(notifyCallback_0);break;
       case 1:pRemoteCharacteristic[n][ID_INFO]->registerForNotify(notifyCallback_1);break;     
     }
   }
   connected = true;
}
 
/**
 * アドバタイズのCallback
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  /**
  * 
  * BLEのアドバタイズ受信時の処理
  */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
   
    int rssi = advertisedDevice.getRSSI();  //RSSIの取得
    if( rssi <= -60 ){                      //RSSIが小さいものは無視する
      return;
    }
    Serial.printf(">>>: RSSI=%d : %s\n",rssi, advertisedDevice.toString().c_str() );

    // 指定のServiceUUIDでない場合は、無視する
    if (!(advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))) {
      return;
    }
          
    if( cube_index < NUM_OF_MAX_CUBE ){
      Serial.printf("Find toio Cube!! %d\n", cube_index);

      //すでにtoioを検出済みで、同じ情報の場合は、無視する
      if(cube_index != 0){                   
        for(int i=0; i<cube_index; i++){
          if(advertisedDevice.getAddress().equals( myDevice[ i ]->getAddress())){
            return;
          }                
        }
      }

      myDevice[ cube_index ] = new BLEAdvertisedDevice(advertisedDevice);
      cube_index++;
    }

    //指定台数検出した場合は、検出を止め接続処理に移る
    if( cube_index >= NUM_OF_MAX_CUBE ){
      BLEDevice::getScan()->stop();
      doConnect = true;
      doScan = true;
    }        
  }
};


/**
 * モーターを制御する
 * 仕様
 * https://toio.github.io/toio-spec/docs/ble_motor#%E7%9B%AE%E6%A8%99%E6%8C%87%E5%AE%9A%E4%BB%98%E3%81%8D%E3%83%A2%E3%83%BC%E3%82%BF%E3%83%BC%E5%88%B6%E5%BE%A1
 */
void MotorSetValue(int n, uint8_t type, int x, int y, uint16_t degree){
  byte comm[] = {0x03, 0x00, 0x05, 0x03, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 

  int width = (PLAYMAT_RIGHT - PLAYMAT_LEFT) / PLAYMAT_WIDTH_NUM;
  int height = (PLAYMAT_BOTTOM - PLAYMAT_TOP) / PLAYMAT_HEIGHT_NUM;     
  y = abs(y-2) * width + PLAYMAT_TOP + width/2;
  x = abs(x+3) * height + PLAYMAT_LEFT  + height/2;

 Serial.print("Toio X:");
 Serial.print(x);
 Serial.print(" Y:");
 Serial.println(y); 

 //移動の仕方
 comm[3] = (byte)type;
 
 //X座標移動先（リトルエンディアン注意）
 comm[7] =  x & 0xFF; //(byte)(x % 255);
 comm[8] =  (x >> 8) &0xFF; //(byte)(x / 255);
 
 //Y座標移動先（リトルエンディアン注意）
 comm[9] =   y & 0xFF;
 comm[10] =  (y >> 8) &0xFF;

 //角度の指定（移動後は角度を変えない設定とする 0xA0)
 comm[11] =   degree & 0xFF;
 comm[12] =   (degree >> 8) &0xFF | 0xA0;

 //移動指示
 pRemoteCharacteristic[n][MOTOR_CONT]->writeValue(comm, sizeof(comm));
}

/**
 * 初期処理
 */
void setup() {
  
  Serial.begin(115200);
  
  M5.begin(true, true, false, true);  //M5の初期化

  // Displayの初期化
  display.begin();
  display.setEpdMode(epd_mode_t::epd_fastest);
  display.setPivot(display.width() /2 -0.5, display.height() /2 - 0.5);  
  display.setColorDepth(16);
  display.fillScreen(0xFFFFFFu);

  // バッファ画面の初期化
  mapCanvas.createSprite(211, 151);
  mapCanvas.setColorDepth(16);  
  mapCanvas.fillScreen(0xFFFFFFu);

  // ヘッダを表示
  drawTitleBar(0,0);

  // WiFiに接続する
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("");

  // 状態の変更（WiFi接続完了）
  edge_state = 1;
  //ヘッダを更新
  drawTitleBar(1,0);

  // toioに接続する
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(60, false);

  //画面を表示する
  drawMap(config.run_position_x, config.run_position_y,
                  config.catch_position_x, config.catch_position_y);
}

/**
 * ループ処理
 */
void loop() {
  M5.update();
  
  checkButton();

  // 準備完了時
  if(edge_state ==  7){
    drawTurn(0); 
    drawMap(config.run_position_x, config.run_position_y,
                  config.catch_position_x, config.catch_position_y);
  }

  //エラー時
  if(edge_state < 7){
    drawTurn(-1);
  }

  // 開始時
  if(edge_state == 15){
    bonsai();  
    edge_state = 31;  
  }

  int m;
  //検出が終わり、接続可能状態時
  if (doConnect == true) {

    // 台数分接続を試みる
    for( m=0 ; m<NUM_OF_MAX_CUBE ; m++ ){
      Serial.printf("Try to Connect CUBE %d\n", m);
      if (connectToServer( m )) {
        Serial.printf("CUBE %d Connection OK\n",m);
      } else {
        Serial.printf("CUBE %d can't connect\n",m);
      } 
    }
    // 注意）サンプルであるため、接続に失敗したことを考慮されていません。 
    doConnect = false;
    
    // toio接続とする
    drawTitleBar(1,1);

    // 準備完了状態にする
    edge_state = 7;

    // 初期位置に移動させる
    MotorSetValue(0, 2, config.run_position_x, config.run_position_y,  0);
    MotorSetValue(1, 2, config.catch_position_x, config.catch_position_y, 180);
  }

  if (connected) {
    //String newValue = "Time since boot: " + String(millis()/1000);
    //Serial.println("Setting new characteristic value to \"" + newValue + "\"");
 
    // Set the characteristic's value to be the array of bytes that is actually a string.
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    //byte comm[] = {01,01,01,0x0b,0x02,0x02,0x0b};    
    //pRemoteCharacteristic[MOTOR_CONT]->writeValue(comm, 7);
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  } 
  
}

/**
 * ボタン入力のチェック
 */
void checkButton(){
  
  if(M5.BtnA.wasPressed()) {
    Serial.println("M5.BtnA.wasPressed");      

    // 準備完了の場合は、開始状態に移行する
    if(edge_state == 7){
      edge_state = 15;

      // 現在の状態で環境（位置）を指定する
      model.start(config);
    }
  }
  else if(M5.BtnB.wasPressed()) {   
    Serial.println("M5.BtnB.wasPressed");

    //  終了状態から、準備状態に移行する
    if(edge_state == 31){
      edge_state = 7;
    }
  }
  else if(M5.BtnC.wasPressed()) {
    Serial.println("M5.BtnC.wasPressed");

    // 準備完了状態の場合に、逃げる駒の移動ロジックを変更する
    if(edge_state == 7){
      model.move_type = model.move_type ? 0 :1;
    }      
  }
}

/**
 * 追いかけっこ
 */
void bonsai(){

  //角度を算出するため、前回場所を保存する
  int before_run_position_x = config.run_position_x;
  int before_run_position_y = config.run_position_y;
  int before_catch_position_x = config.catch_position_x;
  int before_catch_position_y = config.catch_position_x;
  
  model.start(config);

  int count = 1;  //実行回数
  do{
    Serial.print("COUNT:");
    Serial.println(count);

    //距離が1になったら終了させる
    State model_state = model.state();
    if(model_state.distance == 1){
      drawTurn(3);      
      break;
    }

    //bonsaiに環境情報を送信し、移動先情報を取得する
    JSONVar state = model_state.toJSON();     
    String json = JSON.stringify(state);
    Serial.print("REQUEST:");
    Serial.println(json);

    HTTPClient http;
    http.begin(api);
    http.setTimeout(5000);
    http.addHeader("Content-Type", "application/json");
    int status_code = http.POST(json);

    Serial.print("HTTP:");
    Serial.println(status_code);

    // 失敗した場合は、1秒って再送
    if( status_code != 200 ){
      delay(1000);
      continue;
    }

    String response_str = http.getString();
    Serial.print("RESPONSE:");
    Serial.println(response_str);
          
    JSONVar response = JSON.parse(response_str);
    if (JSON.typeof(response) != "undefined") {
      continue;
    }

    //Bonsaiから移動情報を取得する
    Action action;
    if (response.hasOwnProperty("catch_position_x")) {
      action.catch_position_x = response["catch_position_x"];
    }
    if (response.hasOwnProperty("catch_position_y")) {
      action.catch_position_y = response["catch_position_y"];
    }
    
    // 移動情報もとに結果を計算する
    model.step(action);

    // 追うターン表示
    drawTurn(1);

    // 追うtoioを移動
    MotorSetValue(1, 2, 
                    model.catch_position_x, model.catch_position_y, 
                    toioDegree(model.catch_position_x , model.catch_position_y, 
                            before_catch_position_y, before_catch_position_x));

    //m5の画面も更新
    drawMap(model_state.run_position_x, model_state.run_position_y,
                  model.catch_position_x, model.catch_position_y);
                        
    before_run_position_x = model.run_position_x;
    before_run_position_y = model.run_position_y;

    // 移動を待つ（時間は適当です）
    delay(2500);                    

    //逃げるターン表示
    drawTurn(2);

    // 逃げるtoioを移動      
    MotorSetValue(0, 2, 
                    model.run_position_x, model.run_position_y, 
                    toioDegree(model.run_position_x , model.run_position_x, 
                                before_run_position_y, before_run_position_x));
    //m5の画面も更新
    drawMap(model.run_position_x, model.run_position_y,
                    model.catch_position_x, model.catch_position_y);

    before_catch_position_x = model.catch_position_x;
    before_catch_position_y = model.catch_position_y;                    
    delay(2500); 

    count++;     

  }while(1);
}

/**
 * toioの角度を算出（移動先に向くように指定するため）
 */
int toioDegree(int x1, int y1, int x2, int y2){
  double radian = atan2(y1 - y2, x1 - y2);
  int degree = radian * 180 /PI;

  int offset = 270;
  switch(degree){
    case 0:
      if(y2 - y1 < 0){
        offset = 90;
      }
      break;
    case 45:
      if(y2 - y1 < 0){
        offset = 90;
      }
      break;
    case -45:
      if(y2 - y1 > 0){
        offset = 90;
      }    
      break;      
  }
  return degree + offset;
}

/**
 * 画面にメッセージを表示する
 * 
 * param message    表示メッセージ
 * param font_color 文字の色
 * param back_color 背景の色
 */
void drawTitleBar(int wifi, int toio){

  display.fillRect(0, 0, display.width(), 30, 0x0000000);
  display.setTextSize(1.5);
  display.setTextColor(0xFFFFFFU, 0x0000000);    
  display.setTextDatum( middle_center );

  String wifi_state = "";
  switch(wifi){
    case 0:
      wifi_state = "WiFi Connecting.";
      break;
    case 1:
      wifi_state = "WiFi Connected.";
      break;      
  }

  String toio_state = "";
  switch(toio){
    case 0:
      toio_state = "Toio Connecting.";
      break;      
    case 1:
      toio_state = "Toio Connected.";
      break;      
  }
  
  display.drawString(wifi_state, 80, 15);
  display.drawString(toio_state, 240, 15);
  
}

/**
 * マップの表示
 */
void drawMap(int run_x, int run_y, int catch_x, int catch_y){

  //マップの枠作成
  mapCanvas.fillScreen(0xFFFFFFu);
  for(int i=0; i<8; i++){
    mapCanvas.drawFastVLine( i*30, 0,150, 0x696969U);
  }
  
  for(int i=0; i<6; i++){
    mapCanvas.drawFastHLine (0 , i*30, 210, 0x696969U);    
  }
  //駒の情報を描画
  mapCanvas.fillRect(1 + (run_x+3) * 30, 1 + abs((run_y-2)) * 30, 29, 29, 0xDDA0DDU);
  mapCanvas.fillRect(1 + (catch_x+3) * 30, 1 + abs((catch_y-2)) * 30, 29, 29, 0xFFA500U);

  //メイン画面に反映
  mapCanvas.pushSprite(&display, 60, 80);
}

/**
 * 状態メッセージの表示
 */
void drawTurn(int turn){

  static int before_turn = -2;
  //ちらつき防止（同じメッセージの場合は何もしない）
  if(turn == before_turn){
    return;
  }
  
  uint32_t back_color = 0x0000000;
  uint32_t font_color = 0xFFFFFFU;

  String msg = "";
  switch(turn){
    case -1:
      msg = "* check wifi & toio *";  
      back_color = 0x000000U;
          
    case 0:
      msg = "-- push button --";  
      back_color = 0x9ACD32U;
      break;
    case 1:
      msg = "Catch Toio Turn.";
      back_color = 0xFFA500U;
      break;
    case 2:
      msg = "Run Toio Turn.";
      back_color = 0xDDA0DDU;
      break;
    case 3:
      msg = "** Catch **";
      back_color = 0xff4500U;
      break;    
  }
  
  display.fillRect(0, 35, display.width(), 40, back_color);
  display.setTextSize(2);
  display.setTextColor(font_color, back_color);    
  display.setTextDatum( middle_center );
  display.drawString(msg, display.width()/2, 55);

  before_turn = turn;

}
