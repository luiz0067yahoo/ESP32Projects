#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// ==================== CONFIGURAÇÕES DA CÂMERA ======================
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// ==================== REDE WI-FI ======================
const char* ssid = "ESP32-CAM-Car";
const char* password = "12345678";
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ==================== PINOS DOS MOTORES E LED ======================
#define motor_esquerdo_frente 02
#define motor_esquerdo_traz 13
#define motor_direito_frente 15
#define motor_direito_traz 14
#define LED_PIN 4  // LED Flash (GPIO 4)

WebServer server(80);

// ==================== NOVO HTML COM INTERRUPTOR GRANDE ======================
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='pt-br'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>
    <title>Controle do Carrinho</title>
</head>
<body style='margin:0;padding:10px;font-family:Arial,sans-serif;text-align:center;background:linear-gradient(#1e1e1e,#333);color:white;min-height:100vh;'>
    <h2 style='margin:10px 0 5px;font-size:24px;'>Controle do Carrinho ESP32-CAM</h2>
    <img id='camera' src='/cam' alt='Câmera' style='width:100%;max-width:640px;min-height:200px;border:4px solid #00ff00;border-radius:15px;margin:15px auto;display:block;box-shadow:0 0 20px rgba(0,255,0,0.6);'>
    
    <div style='display:grid;grid-template-columns:repeat(3,1fr);gap:15px;max-width:780px;margin:20px auto;padding:10px;'>
        <button id='esquerda_frente' style='font-size:36px;height:50px;background:#00ff00;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>↖</button>
        <button id='frente' style='font-size:36px;height:50px;background:#00ff00;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>▲</button>
        <button id='direita_frente' style='font-size:36px;height:50px;background:#00ff00;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>↗</button>
        
        <button  style='font-size:36px;height:50px;background:transparent;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>&nbsp;</button>
        <button id='parar' style='font-size:36px;height:50px;background:#ff0000;color:white;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>⬛</button>
        <button  style='font-size:36px;height:50px;background:transparent;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>&nbsp;</button>
        
        <button id='esquerda_tras' style='font-size:36px;height:50px;background:#00ff00;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>↙</button>
        <button id='tras' style='font-size:36px;height:50px;background:#00ff00;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>▼</button>
        <button id='direita_tras' style='font-size:36px;height:50px;background:#00ff00;color:#000;border:none;border-radius:20px;box-shadow:0 8px 15px rgba(0,0,0,0.4);transition:all 0.2s;'>↘</button>
    </div>

    <h1 style='font-size:30px;margin:10px 0 10px;text-shadow:0 0 10px #00ff00;'>FAROL</h1>
    <div style='margin:10px auto;'>
        <label style='position:relative;display:inline-block;width:200px;height:50px;cursor:pointer;'>
            <input type='checkbox' id='luz_interruptor' checked style='opacity:0;width:0;height:0;'>
            <span id='slider' style='position:absolute;top:0;left:0;right:0;bottom:0;background:#00ff00;transition:.4s;border-radius:90px;box-shadow:inset 0 0 20px rgba(0,0,0,0.5);'></span>
            <span id='bola' style='position:absolute;height:30px;width:30px;left:15px;bottom:10px;background:white;transition:.4s;border-radius:50%;box-shadow:0 6px 20px rgba(0,0,0,0.4);transform:translateX(140px);'></span>
        </label>
    </div>

<script>
    let luz = true;
    let frente = false, tras = false, esquerda = false, direita = false;

    const comandos = {
        frente: () => { frente = true; tras = esquerda = direita = false; },
        tras: () => { tras = true; frente = esquerda = direita = false; },
        esquerda_frente: () => { frente = esquerda = true; tras = direita = false; },
        direita_frente: () => { frente = direita = true; tras = esquerda = false; },
        esquerda_tras: () => { tras = esquerda = true; frente = direita = false; },
        direita_tras: () => { tras = direita = true; frente = esquerda = false; },
        parar: () => { frente = tras = esquerda = direita = false; }
    };

    function enviar() {
        let acao = 'parar';
        if (frente && esquerda) acao = 'esquerda_frente';
        else if (frente && direita) acao = 'direita_frente';
        else if (tras && esquerda) acao = 'esquerda_tras';
        else if (tras && direita) acao = 'direita_tras';
        else if (frente) acao = 'frente';
        else if (tras) acao = 'tras';

        fetch('/api/comando', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ acao, luz })
        });
    }

    document.querySelectorAll('button').forEach(btn => {
        const id = btn.id;
        const press = () => { btn.style.transform = 'scale(0.92)'; btn.style.boxShadow = '0 4px 8px rgba(0,0,0,0.6)'; };
        const release = () => { btn.style.transform = ''; btn.style.boxShadow = '0 8px 15px rgba(0,0,0,0.4)'; };

        btn.ontouchstart = btn.onmousedown = e => {
            e.preventDefault();
            press();
            if (comandos[id]) comandos[id]();
            else if (id === 'parar') comandos.parar();
            enviar();
        };
        btn.ontouchend = btn.onmouseup = btn.onmouseleave = () => {
            release();
            comandos.parar();
            enviar();
        };
    });

    // Interruptor
    const input = document.getElementById('luz_interruptor');
    const slider = document.getElementById('slider');
    const bola = document.getElementById('bola');
    const texto = document.getElementById('texto');

    input.onchange = function() {
        luz = this.checked;
        if (this.checked) {
            slider.style.background = '#00ff00';
            bola.style.transform = 'translateX(140px)';
        } else {
            slider.style.background = '#ccc';
            bola.style.transform = 'translateX(0)';
        }
        enviar();
    };

    // Câmera 30 FPS
    setInterval(() => {
        document.getElementById('camera').src = '/cam?t=' + Date.now();
    }, 300);
</script>
</body>
</html>
)rawliteral";

// ==================== FUNÇÕES DA CÂMERA E MOTORES ======================
void startCamera() {
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  esp_camera_init(&config);
}

void handleCam() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Erro ao capturar imagem");
    return;
  }
  server.sendHeader("Content-Type", "image/jpeg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ==================== CONTROLE DOS MOTORES ======================
void parar() {
  digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, LOW);
  digitalWrite(motor_esquerdo_frente, LOW);
}

void frente() {
  digitalWrite(motor_esquerdo_frente, HIGH);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, HIGH);
  digitalWrite(motor_direito_traz, LOW);
}

void tras() {
  digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, HIGH);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, HIGH); 
}

void esquerdoFrente() {
  digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, HIGH);
  digitalWrite(motor_direito_frente, HIGH);
  digitalWrite(motor_direito_traz, LOW);
}


void direitoTras() {
   digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, HIGH);
  digitalWrite(motor_direito_frente, HIGH);
  digitalWrite(motor_direito_traz, LOW);
}

void esquerdoTras() {
   digitalWrite(motor_esquerdo_frente, HIGH);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, HIGH);
}


void direitoFrente() {
   digitalWrite(motor_esquerdo_frente, HIGH);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, HIGH);
}

// ==================== API COM LUZ ======================
void handleAPI() {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"JSON inválido\"}");
    return;
  }

  String comando = doc["acao"] | "parar";
  bool luz = doc["luz"] | true;  // padrão ligado

  // === CONTROLA O LED ===
  digitalWrite(LED_PIN, luz ? HIGH : LOW);  // HIGH = ligado no ESP32-CAM

  // === CONTROLA MOTORES ===
  if (comando == "frente") frente();
  else if (comando == "tras") tras();
  else if (comando == "esquerda_frente") esquerdoFrente();
  else if (comando == "esquerda_tras") esquerdoTras();
  else if (comando == "direita_frente") direitoFrente();
  else if (comando == "direita_tras") direitoTras();
  else parar();

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ==================== SETUP ======================
void setup() {
  Serial.begin(115200);

  // Configura LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Inicia ligado

  // Configura motores
  pinMode(motor_esquerdo_frente, OUTPUT);
  pinMode(motor_esquerdo_traz, OUTPUT);
  pinMode(motor_direito_frente, OUTPUT);
  pinMode(motor_direito_traz, OUTPUT);


  parar();

  // WiFi AP
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Câmera
  startCamera();

  // Rotas
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", html);
  });
  server.on("/cam", HTTP_GET, handleCam);
  server.on("/api/comando", HTTP_POST, handleAPI);
  server.begin();

  Serial.println("Servidor iniciado!");
  Serial.print("Acesse: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
