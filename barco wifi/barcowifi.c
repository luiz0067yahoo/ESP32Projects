#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ==================== REDE WI-FI ======================
const char* ssid = "BarcoESP32";
const char* password = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// ==================== PINOS ======================
#define MOTOR_PIN1    12     // Para Frente: LOW  /  Para Trás: HIGH
#define MOTOR_PIN2    13     // Para Frente: HIGH /  Para Trás: LOW
#define SERVO_LEME    14     // Servo (Direita / Esquerda)

Servo lemeServo;

#define LED_PIN       4      // LED Flash (opcional)

// ==================== WebServer ======================
WebServer server(80);

// ==================== HTML ======================
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='pt-br'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>
    <title>Controle do Barco</title>
    <style>
        body {margin:0;padding:20px;font-family:Arial;text-align:center;background:#111;color:#0f0;}
        h2 {margin:10px 0;color:#0f0;}
        .container {display:grid;grid-template-columns:repeat(3,1fr);gap:12px;max-width:420px;margin:30px auto;}
        button {font-size:42px;height:80px;background:#0f0;color:#000;border:none;border-radius:15px;box-shadow:0 6px 12px #000;transition:0.15s;}
        button:active {transform:scale(0.92);box-shadow:0 3px 6px #000;}
        .frente {background:#0f0;}
        .tras {background:#00f; color:white;}
        .stop {background:#f00;color:white;}
        #status {font-size:22px;margin:15px;color:#ff0;}
    </style>
</head>
<body>
    <h2>Controle do Barco</h2>

    <div class="container">
        <button id='esq'>←</button>
        <button id='frente' class='frente'>▲</button>
        <button id='dir'>→</button>
        
        <button style="visibility:hidden;"></button>
        <button id='tras' class='tras'>▼</button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button id='parar' class='stop'>■</button>
        <button style="visibility:hidden;"></button>
    </div>

    <p id='status'>Conectado</p>

<script>
    let direcaoMotor = 0;   // 1 = frente, -1 = trás, 0 = parado
    let direcaoLeme = 0;    // -1 = esq, 1 = dir, 0 = centro

    function enviar() {
        fetch('/api/comando', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
                motor: direcaoMotor,
                leme: direcaoLeme
            })
        });
    }

    // Botões
    document.querySelectorAll('button').forEach(btn => {
        const id = btn.id;

        const start = () => {
            if (id === 'frente') direcaoMotor = 1;
            else if (id === 'tras') direcaoMotor = -1;
            else if (id === 'esq') direcaoLeme = -1;
            else if (id === 'dir') direcaoLeme = 1;
            else if (id === 'parar') {
                direcaoMotor = 0;
                direcaoLeme = 0;
            }
            enviar();
        };

        const stop = () => {
            if (id === 'esq' || id === 'dir') direcaoLeme = 0;
            if (id === 'frente' || id === 'tras') direcaoMotor = 0;
            enviar();
        };

        btn.ontouchstart = btn.onmousedown = e => { e.preventDefault(); start(); };
        btn.ontouchend = btn.onmouseup = btn.onmouseleave = stop;
    });
</script>
</body>
</html>
)rawliteral";

// ==================== FUNÇÕES ======================
void controlarMotor(int motorDir) {
  if (motorDir == 1) {        // Frente
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, HIGH);
  } 
  else if (motorDir == -1) {  // Trás
    digitalWrite(MOTOR_PIN1, HIGH);
    digitalWrite(MOTOR_PIN2, LOW);
  } 
  else {                      // Parado
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);
  }
}

void controlarLeme(int lemeDir) {
  int angulo = 90 + (lemeDir * 90);   // Ajuste o 45 se quiser mais giro
  lemeServo.write(angulo);
}

// ==================== API ======================
void handleAPI() {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) return;

  int motor = doc["motor"] | 0;
  int leme  = doc["leme"]  | 0;

  controlarMotor(motor);
  controlarLeme(leme);

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ==================== SETUP ======================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Motor
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);

  // Servo
  lemeServo.attach(SERVO_LEME, 500, 2500);
  lemeServo.write(90);

  // WiFi AP
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Rotas
  server.on("/", HTTP_GET, [](){ server.send_P(200, "text/html", html); });
  server.on("/api/comando", HTTP_POST, handleAPI);

  server.begin();
  Serial.println("Barco pronto!");
  Serial.print("IP → http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
