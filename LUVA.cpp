#include <Wire.h>
#include <math.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs para integração Web Bluetooth com o site LinkGlove
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEDevice::startAdvertising(); // Reativa a visibilidade BLE se desconectar
    }
};

const int MPU_ADDR = 0x68;
const int PIN_SDA  = 21; // GPIO 21 (D21)
const int PIN_SCL  = 22; // GPIO 22 (D22)

// Pinos Analógicos dos Flexores (ADC1 do ESP32)
const int PIN_MINIMO    = 32; // D32
const int PIN_MEDIO     = 33; // D33
const int PIN_INDICADOR = 34; // D34
const int PIN_POLEGAR   = 35; // D35

// Limiares de Calibração (Valor <= Limiar = ESTICADO | Valor > Limiar = DOBRADO)
const int LIMIAR_MINIMO    = 1200;
const int LIMIAR_MEDIO     = 1400;
const int LIMIAR_INDICADOR = 1000;
const int LIMIAR_POLEGAR   = 1000;
const int LIMIAR_ANELAR    = (LIMIAR_MEDIO + LIMIAR_MINIMO) / 2; // Média: 1300

char ultimaLetraEnviada = ' ';
unsigned long tempoUltimoEnvio = 0;

// Filtro de Média Móvel para estabilização analógica
int lerFiltrado(int pino) {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(pino);
    delayMicroseconds(300);
  }
  return soma / 10;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("\n--- LinkGlove: Sistema do Alfabeto LIBRAS + BLE ---");

  Wire.begin(PIN_SDA, PIN_SCL);

  // Inicializa o MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  pinMode(PIN_MINIMO, INPUT);
  pinMode(PIN_MEDIO, INPUT);
  pinMode(PIN_INDICADOR, INPUT);
  pinMode(PIN_POLEGAR, INPUT);

  // Configuração do Servidor BLE para Web Bluetooth
  BLEDevice::init("LinkGlove-LIBRAS");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("✓ Servidor BLE Ativo. Aguardando conexão do site LinkGlove...");
}

// Cálculo dos ângulos de inclinação da mão (Pitch / Roll)
void calcularAngulos(float ax, float ay, float az, float &pitch, float &roll) {
  pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;
  roll  = atan2(ay, az) * 180.0 / M_PI;
}

// Reconhecimento de Todas as Letras do Alfabeto em LIBRAS (A a Z)
char reconhecerLetra(bool ePol, bool eInd, bool eMed, bool eAne, bool eMin, float pitch, float roll) {
  bool maoParaCima  = (pitch > -35 && pitch < 35 && abs(roll) < 45);
  bool maoParaBaixo = (pitch < -40);
  bool maoDeLado    = (abs(roll) >= 45 || abs(pitch) >= 40);

  // A: Soco fechado com Polegar esticado ao lado
  if (!eInd && !eMed && !eAne && !eMin && ePol && maoParaCima) return 'A';

  // B: 4 dedos esticados para cima, Polegar dobrado na palma
  if (eInd && eMed && eAne && eMin && !ePol && maoParaCima) return 'B';

  // C: Mão curvada em formato de C (dedos em meia-dobra, maoDeLado/frente)
  if (!eInd && !eMed && !eAne && !eMin && ePol && maoDeLado) return 'C';

  // D: Apenas Indicador esticado para cima
  if (eInd && !eMed && !eAne && !eMin && !ePol && maoParaCima) return 'D';

  // E: Todos os dedos dobrados (soco totalmente fechado)
  if (!eInd && !eMed && !eAne && !eMin && !ePol && maoParaCima) return 'E';

  // F: Indicador dobrado tocando o polegar, outros 3 esticados
  if (!eInd && eMed && eAne && eMin && ePol && maoParaCima) return 'F';

  // G: Indicador e Polegar esticados em paralelo apontando para cima
  if (eInd && ePol && !eMed && !eAne && !eMin && maoParaCima) return 'G';

  // H: Indicador e Médio esticados na horizontal / de lado
  if (eInd && eMed && !eAne && !eMin && maoDeLado) return 'H';

  // I: Apenas Mínimo esticado para cima
  if (!eInd && !eMed && !eAne && eMin && !ePol && maoParaCima) return 'I';

  // J: Mínimo esticado com rotação do pulso (mão de lado)
  if (!eInd && !eMed && !eAne && eMin && maoDeLado) return 'J';

  // K: Indicador e Médio esticados para cima com o Polegar entre eles
  if (eInd && eMed && ePol && !eAne && !eMin && maoParaCima) return 'K';

  // L: Indicador e Polegar esticados formando formato L
  if (eInd && ePol && !eMed && !eAne && !eMin && maoParaCima) return 'L';

  // M: Indicador, Médio e Anelar apontados para baixo
  if (eInd && eMed && eAne && !eMin && maoParaBaixo) return 'M';

  // N: Indicador e Médio apontados para baixo
  if (eInd && eMed && !eAne && !eMin && maoParaBaixo) return 'N';

  // O: Mão em formato de O (fechando pontas dos dedos)
  if (!eInd && !eMed && !eAne && !eMin && ePol && !maoParaBaixo && !maoParaCima) return 'O';

  // P: Indicador e Médio esticados apontados para baixo
  if (eInd && eMed && !eAne && !eMin && !ePol && maoParaBaixo) return 'P';

  // Q: Indicador e Polegar apontados para baixo
  if (eInd && ePol && !eMed && !eAne && !eMin && maoParaBaixo) return 'Q';

  // R: Indicador e Médio cruzados esticados para cima
  if (eInd && eMed && !eAne && !eMin && !ePol && maoParaCima) return 'R';

  // S: Soco fechado com Polegar cobrindo os dedos
  if (!eInd && !eMed && !eAne && !eMin && !ePol && maoParaCima) return 'S';

  // T: Polegar por dentro do Indicador dobrado, outros 3 esticados
  if (!eInd && eMed && eAne && eMin && !ePol && maoParaCima) return 'T';

  // U / V: Indicador e Médio esticados para cima
  if (eInd && eMed && !eAne && !eMin && !ePol && maoParaCima) return 'U';

  // W: Indicador, Médio e Anelar esticados para cima
  if (eInd && eMed && eAne && !eMin && !ePol && maoParaCima) return 'W';

  // X: Indicador em formato de gancho (mão de lado)
  if (!eInd && !eMed && !eAne && !eMin && ePol && maoDeLado) return 'X';

  // Y: Polegar e Mínimo esticados para cima (Sinal do Shaka)
  if (ePol && eMin && !eInd && !eMed && !eAne && maoParaCima) return 'Y';

  // Z: Indicador esticado desenhando no ar (mão de lado)
  if (eInd && !eMed && !eAne && !eMin && maoDeLado) return 'Z';

  return '?';
}

void loop() {
  // 1. Leituras analógicas filtradas dos flexores
  int valMinimo    = lerFiltrado(PIN_MINIMO);
  int valMedio     = lerFiltrado(PIN_MEDIO);
  int valIndicador = lerFiltrado(PIN_INDICADOR);
  int valPolegar   = lerFiltrado(PIN_POLEGAR);
  int valAnelar    = (valMedio + valMinimo) / 2;

  // 2. Avaliação lógica do estado dos dedos (true = ESTICADO, false = DOBRADO)
  bool esticadoMinimo    = (valMinimo <= LIMIAR_MINIMO);
  bool esticadoMedio     = (valMedio <= LIMIAR_MEDIO);
  bool esticadoIndicador = (valIndicador <= LIMIAR_INDICADOR);
  bool esticadoPolegar   = (valPolegar <= LIMIAR_POLEGAR);
  bool esticadoAnelar    = (valAnelar <= LIMIAR_ANELAR);

  // 3. Leitura dos dados de rotação do MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6);

  float ax = 0, ay = 0, az = 0;
  if (Wire.available() >= 6) {
    int16_t rawAX = Wire.read() << 8 | Wire.read();
    int16_t rawAY = Wire.read() << 8 | Wire.read();
    int16_t rawAZ = Wire.read() << 8 | Wire.read();

    ax = rawAX / 16384.0;
    ay = rawAY / 16384.0;
    az = rawAZ / 16384.0;
  }

  float pitch = 0, roll = 0;
  calcularAngulos(ax, ay, az, pitch, roll);

  // 4. Mapeamento do caractere segundo a tabela LIBRAS
  char letra = reconhecerLetra(esticadoPolegar, esticadoIndicador, esticadoMedio, esticadoAnelar, esticadoMinimo, pitch, roll);

  // 5. Envio Bluetooth (BLE) imediato para o site quando houver nova letra válida
  if (letra != '?' && (letra != ultimaLetraEnviada || millis() - tempoUltimoEnvio > 1200)) {
    ultimaLetraEnviada = letra;
    tempoUltimoEnvio = millis();

    if (deviceConnected) {
      String strLetra = String(letra);
      pCharacteristic->setValue(strLetra.c_str());
      pCharacteristic->notify();
    }
  }

  // 6. Monitor Serial para depuração em tempo real
  Serial.print("Pol:"); Serial.print(esticadoPolegar ? "EST" : "DOB");
  Serial.print(" | Ind:"); Serial.print(esticadoIndicador ? "EST" : "DOB");
  Serial.print(" | Med:"); Serial.print(esticadoMedio ? "EST" : "DOB");
  Serial.print(" | Min:"); Serial.print(esticadoMinimo ? "EST" : "DOB");
  Serial.print(" | Pitch:"); Serial.print((int)pitch);
  Serial.print("° | Roll:"); Serial.print((int)roll);
  Serial.print("° => Letra: ");
  Serial.println(letra);

  delay(200);
}
