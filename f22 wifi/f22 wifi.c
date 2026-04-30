#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ==================== REDE WI-FI ======================
const char* ssid = "F22ESP32";
const char* password = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// ==================== PINOS ======================
#define MOTOR_PWM_PIN     13
#define SERVO_WING_LEFT   12
#define SERVO_WING_RIGHT  14
#define LED_PIN           4

Servo wingLeftServo;
Servo wingRightServo;

// ==================== PWM e Rampa ======================
// No Core 3.0+, o gerenciamento de canais é automático.
int pwmFreq = 5000;
int pwmResolution = 8;

int currentMotorSpeed = 0;
bool isRamping = false;
unsigned long lastRampStepTime = 0;
const int RAMP_STEP = 20;
const unsigned long RAMP_INTERVAL = 200;

// ==================== WebServer ======================
WebServer server(80);

// ==================== HTML - MANTIDO ORIGINAL ======================
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='pt-br'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>
    <title>Controle do F22</title>
    <style>
        body {margin:0;padding:20px;font-family:Arial;text-align:center;background:#111;color:#0f0;}
        h2 {margin:10px 0;color:#0f0;}
        .container {display:grid;grid-template-columns:repeat(7,1fr);gap:12px;max-width:80%;margin:30px auto;}
        button {font-size:28px;height:80px;background:#00f;color:#000;border:none;border-radius:15px;box-shadow:0 6px 12px #000;transition:0.15s; line-height:1.1;}
        button:active {transform:scale(0.92);box-shadow:0 3px 6px #000;}
        .cima {background:#00f;}
        .baixo {background:#00f; color:white;}
        .stop {background:#f00;color:white;}
        .motor {background:#0f0;color:#000;}
        #status {font-size:22px;margin:15px;color:#ff0;}
    </style>
</head>
<body>
    <h2>Controle do F22</h2>
    <div class="container">
        <button style="visibility:hidden;"></button>
        <button id='cima' class='cima'>🔼</button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button id='esq'>◀️</button>
        <button style="visibility:hidden;"></button>
        <button id='dir'>▶️</button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button id='motor' class='motor'>📶</button>
        <button style="visibility:hidden;"></button>
        <button id='baixo' class='baixo'>🔽</button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
        <button id='parar' class='stop'>■</button>
        <button style="visibility:hidden;"></button>
        <button style="visibility:hidden;"></button>
    </div>
    <p id='status'>Conectado</p>
<script>
    let throttleRamp = 0;
    let motorStop = 0;
    let servoLeftAngle = 90;
    let servoRightAngle = 90;
    function enviar() {
        fetch('/api/comando', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
                throttle_ramp: throttleRamp,
                motor_stop: motorStop,
                servo_left: servoLeftAngle,
                servo_right: servoRightAngle
            })
        });
    }
    document.querySelectorAll('button').forEach(btn => {
        const id = btn.id;
        const start = () => {
            throttleRamp = 0; motorStop = 0;
            if (id === 'cima') { servoLeftAngle = 45; servoRightAngle = 45; }
            else if (id === 'baixo') { servoLeftAngle = 135; servoRightAngle = 135; }
            else if (id === 'esq') { servoLeftAngle = 45; servoRightAngle = 135; }
            else if (id === 'dir') { servoLeftAngle = 135; servoRightAngle = 45; }
            else if (id === 'parar') { servoLeftAngle = 90; servoRightAngle = 90; motorStop = 1; }
            else if (id === 'motor') { throttleRamp = 1; }
            enviar();
        };
        const stop = () => {
            throttleRamp = 0; motorStop = 0;
            if (id === 'cima' || id === 'baixo' || id === 'esq' || id === 'dir') {
                servoLeftAngle = 90; servoRightAngle = 90;
            }
            enviar();
        };
        btn.ontouchstart = btn.onmousedown = e => { e.preventDefault(); start(); };
        btn.ontouchend = btn.onmouseup = btn.onmouseleave = stop;
    });
</script>
</body>
</html>
)rawliteral";

// ==================== FUNÇÕES CORRIGIDAS ======================
void setupPWM() {
  // Substitui ledcSetup e ledcAttachPin
  ledcAttach(MOTOR_PWM_PIN, pwmFreq, pwmResolution);
  ledcWrite(MOTOR_PWM_PIN, 0);
}

void startMotorRamp() {
  if (!isRamping && currentMotorSpeed == 0) {
    currentMotorSpeed = 0;
    isRamping = true;
    lastRampStepTime = millis();
  }
}

void stopMotor() {
  currentMotorSpeed = 0;
  isRamping = false;
  ledcWrite(MOTOR_PWM_PIN, 0); // Usa o PINO em vez do canal
}

void controlarServos(int leftAngle, int rightAngle) {
  wingLeftServo.write(leftAngle);
  wingRightServo.write(rightAngle);
}

// ==================== API ======================
void handleAPI() {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) return;

  int throttleRampCmd = doc["throttle_ramp"] | 0;
  int motorStopCmd   = doc["motor_stop"]   | 0;
  int servoL         = doc["servo_left"]   | 90;
  int servoR         = doc["servo_right"]  | 90;

  if (motorStopCmd == 1) stopMotor();
  if (throttleRampCmd == 1) startMotorRamp();

  controlarServos(servoL, servoR);
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ==================== SETUP ======================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setupPWM();

  wingLeftServo.attach(SERVO_WING_LEFT, 500, 2500);
  wingRightServo.attach(SERVO_WING_RIGHT, 500, 2500);
  wingLeftServo.write(90);
  wingRightServo.write(90);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  server.on("/", HTTP_GET, [](){ server.send_P(200, "text/html", html); });
  server.on("/api/comando", HTTP_POST, handleAPI);

  server.begin();
  Serial.println("F22 WiFi Pronto!");
  Serial.print("IP -> http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();

  if (isRamping) {
    unsigned long now = millis();
    if (now - lastRampStepTime >= RAMP_INTERVAL) {
      currentMotorSpeed += RAMP_STEP;
      if (currentMotorSpeed > 255) currentMotorSpeed = 255;
      
      ledcWrite(MOTOR_PWM_PIN, currentMotorSpeed); // Atualizado para o pino
      lastRampStepTime = now;

      if (currentMotorSpeed >= 255) isRamping = false;
    }
  }
}
