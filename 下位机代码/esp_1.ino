

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#include <Wire.h>
#include <Adafruit_NeoPixel.h>

// UUID定义
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DATA_CHAR_UUID_TO_UNITY "85f8e296-cd32-4a02-bf43-5e0927295970"
#define CMD_CHAR_UUID_FROM_UNITY "beb5483e-36e1-4688-b7f5-ea07361b26a8"

//定义PWM引脚为数字I/O口9，LED灯数量为60.
Adafruit_NeoPixel  rgb_display_9(60);

// 全局变量
BLEServer* pServer = NULL;
BLECharacteristic* pDataCharacteristic = NULL;  // ESP32发送给Unity的数据
BLECharacteristic* pCmdCharacteristic = NULL;   // Unity发送给ESP32的命令



bool deviceConnected = false;
bool oldDeviceConnected = false;
String serialInput = "";
bool serialComplete = false;

// 服务器回调类
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("设备已连接");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("设备已断开连接");
    }
};

// 特征回调类 - 处理从Unity接收的数据
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue().c_str();
    
      if (rxValue.length() > 0) {

        Serial.println("收到来自Unity的消息:");
        Serial.print("内容: ");
        Serial.println(rxValue);
        Serial.println("--------------------");

        //数字识别和处理
        if (rxValue == "love") {
          //求签love
          Serial.println("执行数字1的操作");
          for (int i = 1; i <= 60; i = i + (1)) {
              rgb_display_9.setPixelColor(i-1, 120,0,0); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(10);
              
          }
         
          //关闭所有灯珠
          rgb_display_9.clear();
          for (int i = 60; i <= 60; i = i - (1)) {
              rgb_display_9.setPixelColor(i-1, 120,0,0); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(10);
              
          }
          rgb_display_9.clear();
        }
        else if (rxValue == "health") {
          //求签health
          for (int i = 1; i <= 60; i = i + (1)) {
              rgb_display_9.setPixelColor(i-1, 0,120,0); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(100);
          }
          //关闭所有灯珠
          rgb_display_9.clear();
          for (int i = 60; i <= 60; i = i - (1)) {
              rgb_display_9.setPixelColor(i-1, 0,120,0); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(100);
          }
          rgb_display_9.clear();
        }
        else if (rxValue == "career") {  
          for (int i = 1; i <= 60; i = i + (1)) {
              rgb_display_9.setPixelColor(i-1, 0,0,120); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(100);
          }
          //关闭所有灯珠
          rgb_display_9.clear();
          for (int i = 60; i <= 60; i = i - (1)) {
              rgb_display_9.setPixelColor(i-1, 0,0,120); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(100);
          }
          rgb_display_9.clear();
        }
        else if (rxValue == "money") {  
          for (int i = 1; i <= 60; i = i + (1)) {
              rgb_display_9.setPixelColor(i-1, 200,200,0); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(100);
          }
          //关闭所有灯珠
          rgb_display_9.clear();
          for (int i = 60; i <= 60; i = i - (1)) {
              rgb_display_9.setPixelColor(i-1, 200,200,0); //实现蓝色流水灯效果
              rgb_display_9.show();
              delay(100);
          }
          rgb_display_9.clear();
        }
        else if (rxValue == "lot") {
          for(int k=0 ; k<10; k+=1){  
            for (int i = 0; i < 60; i += 2) {
              rgb_display_9.setPixelColor(i, 230, 230, 250);
            }
            rgb_display_9.show();
            delay(1000); // 保持1秒
            
            // 关闭所有灯珠
            rgb_display_9.clear();

            // 先点亮所有基数灯珠(1,3,5,...)
            for (int i = 1; i < 60; i += 2) {
              rgb_display_9.setPixelColor(i, 230, 230, 250);
            }
            rgb_display_9.show();
            }
        }
        else if (rxValue == "bowl") {  
          for(int j=1; j<100 ; j+=1){
              for (int i = 1; i < 60; i += 1) {
                rgb_display_9.setPixelColor(i, 0, 0, j);
            }
              rgb_display_9.show();
              delay(25);
            // 关闭所有灯珠
            rgb_display_9.clear();
            }
        }

      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 BLE服务器启动中...");

  rgb_display_9.begin(); //引脚初始化
  rgb_display_9.setPin(9);


  // 创建BLE设备
  BLEDevice::init("ESP32-Unity-BLE");
  
  // 创建BLE服务器
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 创建BLE服务
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 创建数据特征 (ESP32 -> Unity)
  pDataCharacteristic = pService->createCharacteristic(
                      DATA_CHAR_UUID_TO_UNITY,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pDataCharacteristic->addDescriptor(new BLE2902());

  // 创建命令特征 (Unity -> ESP32)
  pCmdCharacteristic = pService->createCharacteristic(
                      CMD_CHAR_UUID_FROM_UNITY,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCmdCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // 启动服务
  pService->start();

  // 开始广播
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  //pAdvertising->setMinPreferred(0x0);  // 设置为0x00以不通告txpower
  BLEDevice::startAdvertising();
  
  Serial.println("BLE服务器已启动，等待客户端连接...");
  Serial.println("服务UUID: " + String(SERVICE_UUID));
  Serial.println("数据特征UUID (ESP32->Unity): " + String(DATA_CHAR_UUID_TO_UNITY));
  Serial.println("命令特征UUID (Unity->ESP32): " + String(CMD_CHAR_UUID_FROM_UNITY));
  Serial.println("====================================");
  Serial.println("请在串口中输入要发送给Unity的消息:");
}

void loop() {
  // 处理串口输入
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      serialComplete = true;
    } else {
      serialInput += inChar;
    }
  }

  // 修改后的发送代码
  if (serialComplete) {
      if (serialInput.length() > 0) {
          if (deviceConnected) {
              // 移除换行符和回车符
              serialInput.replace("\r", "");
              serialInput.replace("\n", "");

              // 正确的方式：直接设置字符串值
              pDataCharacteristic->setValue(serialInput);
              pDataCharacteristic->notify();

              Serial.println("发送给Unity的消息: " + serialInput);
              Serial.println("--------------------");
          } else {
              Serial.println("没有设备连接，无法发送消息: " + serialInput);
          }
      }

      serialInput = "";
      serialComplete = false;
  }


  // 处理连接状态变化
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 给蓝牙栈时间准备    pServer->startAdvertising(); // 重新开始广播
    Serial.println("重新开始广播，等待新的连接...");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    Serial.println("Unity客户端已连接，可以开始通信");
    Serial.println("请在串口中输入要发送的消息:");
  }

  delay(10);
}
