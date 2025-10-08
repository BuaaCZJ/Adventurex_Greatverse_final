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
enum EffectState { IDLE, RUNNING, BREATH_EFFECT, OFF_DELAY }; // 修改状态定义
EffectState effectState = IDLE;
int currentEffect = 0;
int currentPixel = 0;
int currentPixel_1 = 0;
unsigned long previousMillis = 0;
const long effectInterval = 150; // 每个LED的时间间隔
int cnt = 0;

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

// 熄灯延迟变量
unsigned long offDelayStart = 0;
const long offDelayDuration = 2000; // 熄灯延迟时间(毫秒)

// 渐变效果变量
unsigned long fadeLastUpdate = 0;
const long fadeInterval = 20;    // 渐变更新间隔(毫秒)，值越小渐变越平滑
int fadeStep = 1;               // 渐变步长
uint32_t currentColor = 0;      // 当前颜色
uint32_t targetColor = 0;       // 目标颜色
int currentR = 0, currentG = 0, currentB = 0; // 当前RGB分量
int targetR = 0, targetG = 0, targetB = 0;    // 目标RGB分量
int colorIndex = 0;             // 当前颜色索引

// 预定义颜色数组 - 使用RGB分量而不是颜色值
const uint8_t colorPalette[][3] = {
  {255, 0, 0},     // 红色
  {255, 165, 0},   // 橙色
  {255, 255, 0},   // 黄色
  {0, 255, 0},     // 绿色
  {0, 0, 255},     // 蓝色
  {75, 0, 130},    // 靛蓝色
  {148, 0, 211},   // 紫色
  {255, 105, 180}  // 粉红色
};
const int colorCount = sizeof(colorPalette) / sizeof(colorPalette[0]);

// 效果颜色存储
uint32_t effectColor = 0;    // 存储当前效果的颜色

// 呼吸周期计数器
int breathCycleCount = 0;
const int breathTotalCycles = 3;   // 呼吸灯运行周期数

// ================== 新增亮度切换变量 ==================
int idleBrightnessLevel = 0;        // 当前空闲亮度级别 (0,1,2)
const int idleBrightnessValues[] = {50, 128, 200};  // 空闲状态亮度值
const int idleBrightnessCount = 3;  // 亮度级别数量

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
              case 1: effectColor = rgb_display_9.Color(255, 105, 180); break; // 紫色
              case 2: effectColor = rgb_display_9.Color(0, 120, 0);   break; // 绿色
              case 3: effectColor = rgb_display_9.Color(0, 0, 120);   break; // 蓝色
              case 4: effectColor = rgb_display_9.Color(218, 165, 32);break; // 金色
              case 5: // 呼吸灯初始化
                breathBrightness = breathMin;
                breathDirection = 1;
                breathCycleCount = 0;
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

// 辅助函数：从32位颜色值中提取RGB分量
void extractRGB(uint32_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
}

// 辅助函数：将RGB分量合并为32位颜色值
uint32_t combineRGB(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// ================== 新增函数：切换到下一个空闲亮度 ==================
void switchToNextIdleBrightness() {
  idleBrightnessLevel = (idleBrightnessLevel + 1) % idleBrightnessCount;
  int newBrightness = idleBrightnessValues[idleBrightnessLevel];
  
  // 应用新的亮度
  rgb_display_9.setBrightness(newBrightness);
  rgb_display_9.show();
  
  Serial.print("切换到空闲状态，亮度级别: ");
  Serial.print(idleBrightnessLevel);
  Serial.print(" (亮度值: ");
  Serial.print(newBrightness);
  Serial.println(")");
}

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
  
  // 初始化渐变效果
  currentR = colorPalette[0][0];
  currentG = colorPalette[0][1];
  currentB = colorPalette[0][2];
  currentColor = combineRGB(currentR, currentG, currentB);
  
  // 设置下一个目标颜色
  colorIndex = 1;
  targetR = colorPalette[colorIndex][0];
  targetG = colorPalette[colorIndex][1];
  targetB = colorPalette[colorIndex][2];
  targetColor = combineRGB(targetR, targetG, targetB);
  
  // 初始化空闲状态亮度
  idleBrightnessLevel = 0;
  rgb_display_9.setBrightness(idleBrightnessValues[idleBrightnessLevel]);
  Serial.print("初始空闲亮度: ");
  Serial.println(idleBrightnessValues[idleBrightnessLevel]);
}

// 自定义min和max函数，处理int和uint8_t混合的情况
int myMin(int a, int b) {
  return (a < b) ? a : b;
}

int myMax(int a, int b) {
  return (a > b) ? a : b;
}

void handleFadeEffect() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - fadeLastUpdate >= fadeInterval) {
    fadeLastUpdate = currentMillis;
    
    // 计算RGB分量的渐变
    bool allReached = true;
    
    // 红色分量渐变
    if (currentR != targetR) {
      if (currentR < targetR) { 
        currentR = myMin(currentR + fadeStep, targetR);
      } else {
        currentR = myMax(currentR - fadeStep, targetR);
      }
      allReached = false;
    }
    
    // 绿色分量渐变
    if (currentG != targetG) {
      if (currentG < targetG) {
        currentG = myMin(currentG + fadeStep, targetG);
      } else {
        currentG = myMax(currentG - fadeStep, targetG);
      }
      allReached = false;
    }
    
    // 蓝色分量渐变
    if (currentB != targetB) {
      if (currentB < targetB) {
        currentB = myMin(currentB + fadeStep, targetB);
      } else {
        currentB = myMax(currentB - fadeStep, targetB);
      }
      allReached = false;
    }
    
    // 更新当前颜色
    currentColor = combineRGB(currentR, currentG, currentB);
    
    // 设置所有灯珠为当前颜色
    for (int i = 0; i < 60; i++) {
      rgb_display_9.setPixelColor(i, currentColor);
    }
    rgb_display_9.show();
    
    // 如果所有分量都达到目标值，设置下一个目标颜色
    if (allReached) {
      colorIndex = (colorIndex + 1) % colorCount;
      targetR = colorPalette[colorIndex][0];
      targetG = colorPalette[colorIndex][1];
      targetB = colorPalette[colorIndex][2];
      targetColor = combineRGB(targetR, targetG, targetB);
      
      Serial.print("切换到下一个颜色: R=");
      Serial.print(targetR);
      Serial.print(" G=");
      Serial.print(targetG);
      Serial.print(" B=");
      Serial.println(targetB);
    }
  }
}

void handleEffects() {
  if(effectState != RUNNING && effectState != BREATH_EFFECT && effectState != OFF_DELAY) {
    // 在空闲状态下运行渐变效果
    handleFadeEffect();
    return;
  }
  
  unsigned long currentMillis = millis();
  
  // 处理熄灯延迟状态
  if(effectState == OFF_DELAY) {
    if(currentMillis - offDelayStart >= offDelayDuration) {
      // 熄灯延迟结束，切换到下一个空闲亮度并进入空闲状态
      switchToNextIdleBrightness();
      effectState = IDLE;
      currentEffect = 0;
      Serial.println("熄灯延迟结束，进入空闲状态");
    }
    return; // 在熄灯延迟期间不处理其他效果
  }
  
  // 处理效果1-4: 顺序点亮效果
  if(currentEffect >= 1 && currentEffect <= 4 && effectState == RUNNING) {
    if(currentMillis - previousMillis >= effectInterval) {
      previousMillis = currentMillis;
      
      if(currentPixel < 60) {
        rgb_display_9.setPixelColor(currentPixel, effectColor);
        rgb_display_9.show();
        currentPixel++;
      } else {
        // 流水灯完成，进入呼吸灯效果
        effectState = BREATH_EFFECT;
        
        // 初始化呼吸灯变量
        breathBrightness = breathMin;
        breathDirection = 1;
        breathCycleCount = 0;
        
        Serial.println("流水灯完成，开始呼吸灯效果");
      }
    }
  }
  
  // 处理呼吸灯效果
  else if(currentEffect >= 1 && currentEffect <= 4 && effectState == BREATH_EFFECT) {
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
        // 呼吸灯完成，进入熄灯延迟状态
        effectState = OFF_DELAY;
        offDelayStart = currentMillis; // 记录熄灯开始时间
        
        // 熄灭所有灯
        rgb_display_9.clear();
        rgb_display_9.setBrightness(idleBrightnessValues[idleBrightnessLevel]); // 恢复空闲亮度
        rgb_display_9.show();
        
        Serial.println("呼吸灯效果完成，进入2秒熄灯延迟");
      }
    }
  }
  
  // 处理独立呼吸灯效果（效果5）
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
        // 呼吸灯完成，进入熄灯延迟状态
        effectState = OFF_DELAY;
        offDelayStart = currentMillis; // 记录熄灯开始时间
        
        // 熄灭所有灯
        rgb_display_9.clear();
        rgb_display_9.setBrightness(idleBrightnessValues[idleBrightnessLevel]); // 恢复空闲亮度
        rgb_display_9.show();
        
        Serial.println("呼吸灯效果完成，进入2秒熄灯延迟");
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