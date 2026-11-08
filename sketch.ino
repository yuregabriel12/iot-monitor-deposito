#include <WiFi.h>
#include "DHTesp.h"
#include "ThingSpeak.h"   // sempre depois de WiFi.h

// ===================== CONFIGURAÇÃO =====================
const char*   MATRICULA     = "SUA_MATRICULA";
unsigned long CANAL_ID      = SEU_ID;
const char*   CHAVE_ESCRITA = "SUA_CHAVE";  // TROQUE POR "SUA_CHAVE" ANTES DE SALVAR/COMPARTILHAR!
// ========================================================

const unsigned long INTERVALO_ENVIO_MS = 20000;

const int PINO_DHT    = 15;
const int PINO_LED    = 2;
const int PINO_BUZZER = 4;

DHTesp     dht;
WiFiClient cliente;
int  limite = 30;   // MUDANÇA 1: limite fixo, escolha entre 26 e 32 °C
bool alarme = false;
unsigned long ultimaLeitura = 0;
unsigned long ultimoEnvio   = 0;
float temperatura = 0, umidade = 0;

// (a função calcularLimite() foi removida: gerava 24 a 29 °C, fora da regra)

bool decidirAlarme(float t, bool alarmeAtual) {
  if (!alarmeAtual && t > limite)      return true;
  if (alarmeAtual  && t < limite - 1)  return false;
  return alarmeAtual;
}

void atualizarSaidas(bool ligado) {
  digitalWrite(PINO_LED, ligado ? HIGH : LOW);
  if (ligado) tone(PINO_BUZZER, 1000, 200);
}

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" conectado!");
}

void enviarParaNuvem() {
  ThingSpeak.setField(1, temperatura);
  ThingSpeak.setField(2, umidade);

  // MUDANÇA 2 (RF4): avisa o gestor no mesmo envio
  ThingSpeak.setField(3, alarme ? 1 : 0);
  if (alarme) ThingSpeak.setStatus("ALERTA");

  int codigo = ThingSpeak.writeFields(CANAL_ID, CHAVE_ESCRITA);   // um único envio
  Serial.printf("COMPROVANTE A2 | mat=%s | limite=%d | canal=%lu | intervalo=%lus | HTTP=%d\n",
                MATRICULA, limite, CANAL_ID, INTERVALO_ENVIO_MS / 1000, codigo);
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  dht.setup(PINO_DHT, DHTesp::DHT22);
  // (a linha limite = calcularLimite(MATRICULA); foi removida)
  Serial.printf("BASE 2 | matricula=%s | limite=%d C\n", MATRICULA, limite);
  conectarWiFi();
  ThingSpeak.begin(cliente);
}

void loop() {
  unsigned long agora = millis();

  if (agora - ultimaLeitura >= 2000) {
    ultimaLeitura = agora;
    TempAndHumidity leitura = dht.getTempAndHumidity();
    if (dht.getStatus() == DHTesp::ERROR_NONE) {
      temperatura = leitura.temperature;
      umidade     = leitura.humidity;
      alarme = decidirAlarme(temperatura, alarme);
      atualizarSaidas(alarme);
      Serial.printf("T=%.1f C | H=%.0f %% | alarme=%s\n",
                    temperatura, umidade, alarme ? "LIGADO" : "desligado");
    }
  }

  if (agora - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = agora;
    enviarParaNuvem();
  }
}