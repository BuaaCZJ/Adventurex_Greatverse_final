#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID定义
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DATA_CHAR_UUID_TO_UNITY "85f8e296-cd32-4a02-bf43-5e0927295970"
#define CMD_CHAR_UUID_FROM_UNITY "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_NeoPixel  rgb_display_9(60);

// 全局变量
BLEServer* pServer = NULL;
BLECharacteristic* pDataCharacteristic = NULL;
BLECharacteristic* pCmdCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String serialInput = "";
bool serialComplete = false;

// 灯光效果状态机
enum EffectState { IDLE, RUNNING, COMPLETE, COMPLETE_TO_BREATH }; // 添加过渡状态
EffectState effectState = IDLE;
int currentEffect = 0;
int currentPixel = 0;
int currentPixel_1 = 0;
unsigned long previousMillis = 0;
const long effectInterval = 150; // 每个LED的时间间隔
int cnt=0;

// ================== 新增效果变量 ==================
// 呼吸灯效果变量
int breathBrightness = 0;    // 当前亮度值(0-255)
int breathDirection = 1;     // 亮度变化方向(1增加, -1减少)
const int breathStep = 5;    // 每次变化的步长
const int breathMin = 30;    // 最小亮度
const int breathMax = 200;   // 最大亮度

// 闪烁灯效果变量
bool flashOn = false;        // 当前闪烁状态
unsigned long flashLastChange = 0;
const long flashInterval = 500; // 闪烁间隔(毫秒)

// 效果颜色存储
uint32_t effectColor = 0;    // 存储当前效果的颜色
// ================================================

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
        
        // 只设置效果类型，不执行实际操作
        if(rxValue == "1" || rxValue == "2" || rxValue == "3" || rxValue == "4" || 
           rxValue == "5" || rxValue == "6") {
          if(effectState == IDLE || currentEffect == 5 || currentEffect == 6) {
            currentEffect = rxValue.toInt();
            effectState = RUNNING;
            currentPixel = 0;
            previousMillis = millis();
            
            // ===== 新增效果初始化 =====
            switch(currentEffect) {
              case 1: effectColor = rgb_display_9.Color(148, 0, 211); break; // 紫色
              case 2: effectColor = rgb_display_9.Color(0, 120, 0);   break; // 绿色
              case 3: effectColor = rgb_display_9.Color(0, 0, 120);   break; // 蓝色
              case 4: effectColor = rgb_display_9.Color(218, 165, 32);break; // 金色
              case 5: // 呼吸灯初始化

                breathBrightness = breathMin;
                breathDirection = 1;
                break;
              case 6: // 闪烁灯初始化
                effectColor = rgb_display_9.Color(255, 0, 0); // 红色闪烁
                flashOn = false;
                flashLastChange = millis();
                break;
            }
          }
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 BLE服务器启动中...");

  rgb_display_9.begin();
  rgb_display_9.setPin(9);
  rgb_display_9.clear();
  rgb_display_9.show();

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
  BLEDevice::startAdvertising();
  
  Serial.println("BLE服务器已启动，等待客户端连接...");
  Serial.println("服务UUID: " + String(SERVICE_UUID));
  Serial.println("数据特征UUID (ESP32->Unity): " + String(DATA_CHAR_UUID_TO_UNITY));
  Serial.println("命令特征UUID (Unity->ESP32): " + String(CMD_CHAR_UUID_FROM_UNITY));
  Serial.println("====================================");
  Serial.println("请在串口中输入要发送给Unity的消息:");
  Serial.println("可用灯光效果: 1=紫色, 2=绿色, 3=蓝色, 4=金色, 5=呼吸灯, 6=闪烁灯");
}




// 全局变量修改
int breathCycleCount = 0;          // 呼吸周期计数器
const int breathTotalCycles = 3;   // 呼吸灯运行周期数




// 在handleEffects()函数中修改流水灯完成后的处理
void handleEffects() {
  if(effectState != RUNNING && effectState != COMPLETE_TO_BREATH) return;
  
  unsigned long currentMillis = millis();
  
  // 处理效果1-4: 顺序点亮效果
  if(currentEffect >= 1 && currentEffect <= 4) {
    if(currentMillis - previousMillis >= effectInterval) {
      previousMillis = currentMillis;
      
      if(currentPixel < 60) {
        rgb_display_9.setPixelColor(currentPixel, effectColor);
        rgb_display_9.show();
        currentPixel++;
      } else {
        // 流水灯完成，进入过渡状态
        effectState = COMPLETE_TO_BREATH;
        currentEffect = 5;  // 设置为呼吸灯效果
        
        // 初始化呼吸灯变量
        breathBrightness = breathMin;
        breathDirection = 1;
        breathCycleCount = 0;
        
        Serial.println("流水灯完成，即将开始呼吸灯效果");
      }
    }
  }
  
  // 处理呼吸灯效果
  else if(currentEffect == 5) {
    if(currentMillis - previousMillis >= 30) {
      previousMillis = currentMillis;
      
      // 更新亮度
      breathBrightness += breathDirection * breathStep;
      
      // 检查边界
      if(breathBrightness >= breathMax) {
        breathBrightness = breathMax;
        breathDirection = -1;
      } else if(breathBrightness <= breathMin) {
        breathBrightness = breathMin;
        breathDirection = 1;
        breathCycleCount++; // 完成一个呼吸周期
      }
      
      // 应用呼吸效果到所有LED
      for(int i = 0; i < 60; i++) {
        rgb_display_9.setPixelColor(i, effectColor);
      }
      rgb_display_9.setBrightness(breathBrightness);
      rgb_display_9.show();
      
      // 检查是否完成指定周期数
      if(breathCycleCount >= breathTotalCycles) {
        // 呼吸灯完成，返回空闲状态
        rgb_display_9.clear();
        rgb_display_9.show();
        effectState = IDLE;
        currentEffect = 0;
        Serial.println("呼吸灯效果完成");
      }
    }
  }
  // ================== 新增效果6: 闪烁灯 ==================
  else if(currentEffect == 6) {
    if(currentMillis - flashLastChange >= flashInterval) {
      flashLastChange = currentMillis;
      flashOn = !flashOn; // 切换状态
      
      if(flashOn) {
        // 点亮所有LED
        for(int i = 0; i < 60; i++) {
          rgb_display_9.setPixelColor(i, effectColor);
        }
      } else {
        // 熄灭所有LED
        rgb_display_9.clear();
      }
      rgb_display_9.show();
    }
  }
}









void loop() {
  // 处理灯光效果
  handleEffects();
  
  // 处理串口输入
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      serialComplete = true;
    } else {
      serialInput += inChar;
    }
  }

  if (serialComplete) {
      if (serialInput.length() > 0) {
          if (deviceConnected) {
              serialInput.replace("\r", "");
              serialInput.replace("\n", "");

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
    delay(500);
    pServer->startAdvertising();
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