#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// ==================== CONFIGURAÇÕES DA CÂMERA ======================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ==================== REDE WI-FI ======================
const char* ssid = "ESP32-CAM-Car";
const char* password = "12345678";
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ==================== PINOS DOS MOTORES E LED ======================
#define motor_esquerdo_frente 12
#define motor_esquerdo_traz   13
#define motor_direito_frente  15
#define motor_direito_traz    14
#define LED_PIN               4   // LED Flash (GPIO 4)

WebServer server(80);

// ==================== NOVO HTML COM INTERRUPTOR GRANDE ======================
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang=\"pt-br\">
<head>
    <meta charset="UTF-8">
    <title>Controle do Carrinho</title>
    <style>
        * { user-select: none; }
        body { 
            font-family: Arial, sans-serif; 
            text-align: center;
            background-color: #f3f3f3; 
            padding: 20px;
        }
        .controle-container {
            display: inline-grid;
            grid-template-columns: 250px 250px 250px;
            gap: 10px; 
            margin-top: 20px;
        }
        #camera{
            border: solid black 2px;
            width: 90%; 
            max-width: 90%;  
            min-height: 480px;
            max-height: 70%;
            border-radius: 10px; 
            margin: 20px auto;
            display: block;
            transform: rotate(0deg);
        }
        .botao { 
            font-size: 84px; 
            width: 250px; 
            height: 250px; 
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 15px;
            cursor: pointer; 
            transition: 0.2s;
        }
        .botao-stop { background-color: #f44336; }
        .botao:active { background-color: #333; transform: scale(0.95); }

        /* INTERRUPTOR ESTILO LÂMPADA */
        .interruptor {
            grid-column: 1 / 4;
            margin: 30px auto;
        }
        .switch {
            position: relative;
            display: inline-block;
            width: 700px;
            height: 200px;
        }
        .switch input { 
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #4CAF50;
            transition: .4s;
            border-radius: 100px;
            box-shadow: inset 0 0 20px rgba(0,0,0,0.4);
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 170px;
            width: 170px;
            left: 15px;
            bottom: 15px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
            box-shadow: 0 5px 15px rgba(0,0,0,0.4);
            transform: translateX(500px);
        }
        input:not(:checked) + .slider {
            background-color: #ccc;
        }
        input:not(:checked) + .slider:before {
            transform: translateX(0);
        }
        .slider:after {
            content: 'LIGADO';
            color: white;
            font-weight: bold;
            font-size: 70px;
            position: absolute;
            top: 50%;
            left: 75%;
            transform: translateY(-50%);
            text-shadow: 2px 2px 5px rgba(0,0,0,0.6);
        }
        input:not(:checked) + .slider:after {
            content: 'DESLIGADO';
            left: 15%;
        }
    </style>
</head>
<body>
  <h2>Controle do Carrinho ESP32-CAM</h2>
  <img id=\"camera\" src=\"/cam\">
  
  <div class=\"controle-container\">
   <button class=\"botao\" id=\"esquerda_frente\">NW</button>
    <button class=\"botao\" id=\"frente\">NE</button>
    <button class=\"botao\" id=\"direita_frente\">NE</button>
    
    <button class=\"botao\" id=\"esquerda\">W</button>
    <button class=\"botao botao-stop\" id=\"parar\">STOP</button>
    <button class=\"botao\" id=\"direita\">E</button>
    
    <button class=\"botao\" id=\"esquerda_tras\">SW</button>
    <button class=\"botao\" id=\"tras\">S</button>
    <button class=\"botao\" id=\"direita_tras\">SE</button>
  </div>

  <h1 style=\"font-size:96px;margin:20px 0 0 0;\">FAROL</h1>
  <div class=\"interruptor\">
    <label class=\"switch\">
      <input type=\"checkbox\" id=\"luz_interruptor\" checked>
      <span class=\"slider\"></span>
    </label>
  </div>

<script>
    let luz_acesa_apagada = true;
    let estadoEsquerda = false;
    let estadoDireita = false;
    let estadoFrente  = false;
    let estadoTras    = false;

    function atualizarAcao() {
        let acao = "parar";
        if (estadoFrente && !estadoTras && !estadoEsquerda && !estadoDireita) acao = \"frente\";
        else if (!estadoFrente && estadoTras && !estadoEsquerda && !estadoDireita) acao = \"tras\";
        else if (estadoFrente && !estadoTras && estadoEsquerda && !estadoDireita) acao = \"esquerda_frente\";
        else if (estadoFrente && !estadoTras && !estadoEsquerda && estadoDireita) acao = \"direita_frente\";
        else if (!estadoFrente && estadoTras && estadoEsquerda && !estadoDireita) acao = \"esquerda_tras\";
        else if (!estadoFrente && estadoTras && !estadoEsquerda && estadoDireita) acao = \"direita_tras\";

        enviarComando(acao);
    }

    // === CONTROLES DOS BOTÕES ===
    const botoes = [
        'frente', 'tras', 'esquerda', 'direita',
        'esquerda_frente', 'esquerda_tras',
        'direita_frente', 'direita_tras'
    ];

    botoes.forEach(id => {
        const el = document.getElementById(id);
        el.ontouchstart = el.onmousedown = () => {
            if (id.includes('frente')) estadoFrente = true;
            if (id.includes('tras')) estadoTras = true;
            if (id.includes('esquerda')) estadoEsquerda = true;
            if (id.includes('direita')) estadoDireita = true;
            atualizarAcao();
        };
        el.ontouchend = el.onmouseup = el.onmouseleave = () => {
            if (id.includes('frente')) estadoFrente = false;
            if (id.includes('tras')) estadoTras = false;
            if (id.includes('esquerda')) estadoEsquerda = false;
            if (id.includes('direita')) estadoDireita = false;
            atualizarAcao();
        };
    });

    // BOTÃO PARAR
    document.getElementById('parar').ontouchstart = document.getElementById('parar').onmousedown = () => {
        estadoFrente = estadoTras = estadoEsquerda = estadoDireita = false;
        atualizarAcao();
    };

    // INTERRUPTOR DE LUZ
    document.getElementById('luz_interruptor').onclick = () => {
        luz_acesa_apagada = !luz_acesa_apagada;
        atualizarAcao(); // envia o estado atual da luz mesmo se parado
    };

    function enviarComando(acao) {
        fetch(\"/api/comando\", {
            method: \"POST\",
            headers: { \"Content-Type\": \"application/json\" },
            body: JSON.stringify({ 
                acao: acao, 
                luz: luz_acesa_apagada 
            })
        })
        .then(r => r.text())
        .then(d => console.log(\"Enviado:\", acao, \"Luz:\", luz_acesa_apagada))
        .catch(e => console.error(\"Erro:\", e));
    }

    // ATUALIZAÇÃO DA CÂMERA
    function atualizarCamera() {
        document.getElementById('camera').src = \"/cam?\" + new Date().getTime();
        setTimeout(atualizarCamera, 200);
    }
    atualizarCamera();
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
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Erro ao capturar imagem");
    return;
  }
  server.sendHeader("Content-Type", "image/jpeg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ==================== CONTROLE DOS MOTORES ======================
void parar() {
  digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, LOW);
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
  digitalWrite(motor_esquerdo_frente, HIGH);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, HIGH);
}

void direitoTras() {
  digitalWrite(motor_esquerdo_frente, HIGH);
  digitalWrite(motor_esquerdo_traz, LOW);
  digitalWrite(motor_direito_frente, LOW);
  digitalWrite(motor_direito_traz, HIGH);
}


void esquerdoTras() {
  digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, HIGH);
  digitalWrite(motor_direito_frente, HIGH);
  digitalWrite(motor_direito_traz, LOW);
}

void direitoFrente() {
  digitalWrite(motor_esquerdo_frente, LOW);
  digitalWrite(motor_esquerdo_traz, HIGH);
  digitalWrite(motor_direito_frente, HIGH);
  digitalWrite(motor_direito_traz, LOW);
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
  digitalWrite(LED_PIN, HIGH); // Inicia ligado

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
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", html); });
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
