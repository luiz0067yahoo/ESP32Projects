#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Servo.h>

// ==================== REDE WI-FI ======================

const char* ssid = "F22ESP32";
const char* password = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// ==================== PINOS ======================

#define MOTOR_PWM_PIN     13   // D7
#define SERVO_WING_LEFT   12   // D6 fio verde
#define SERVO_WING_RIGHT  14   // D5 fio amarelo
#define LED_PIN           2    // D2

#define SERVO_MIN           0    
#define SERVO_MEDIAN        90    
#define SERVO_MAX           180    

// ==================== SERVOS ======================

Servo wingLeftServo;
Servo wingRightServo;
Servo escMotor;

// ==================== MOTOR ESC ======================

const int MOTOR_MIN = 700;
const int MOTOR_MAX = 3000;

// ==================== SERVIDOR ======================

ESP8266WebServer server(80);

// ==================== HTML ======================

const char html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang='pt-br'>

<head>

<meta charset='UTF-8'>

<meta name='viewport'
content='width=device-width, initial-scale=1.0'>

<title>Controle F22</title>

<style>

body{
    margin:0;
    padding:20px;
    background:#111;
    color:#0f0;
    font-family:Arial;
    text-align:center;
}

h2{
    color:#0f0;
}

.container{
    display:grid;
    grid-template-columns:repeat(7,1fr);
    gap:5px;
    max-width:350px;
    margin:auto;
}

button{
    height:80px;
    font-size:28px;
    border:none;
    border-radius:15px;
    background:#00f;
    color:white;
}

button:active{
    transform:scale(0.9);
}

.stop{
    background:red;
}

.motor{
    background:lime;
    color:black;
}

.empty{
    visibility:hidden;
}

</style>

</head>

<body>

<h2>Controle do F22</h2>

<div class="container">

<button class="empty"></button>
<button id="cima">🔼</button>
<button class="empty"></button>

<button class="empty"></button>

<button class="empty"></button>
<button class="empty"></button>
<button class="empty"></button>

<button id="esq">◀️</button>
<button id="parar" class="stop">■</button>
<button id="dir">▶️</button>

<button class="empty"></button>

<button class="empty"></button>
<button id="motor" class="motor">
📶
</button>

<button class="empty"></button>

<button class="empty"></button>
<button id="baixo">🔽</button>
<button class="empty"></button>

<button class="empty"></button>

<button class="empty"></button>
<button class="empty"></button>
<button class="empty"></button>

</div>

<br>

<script>

let motorSpeed = 1000;

let motorPressed = false;

let servoLeftAngle = 90;
let servoRightAngle = 90;

const MOTOR_MIN = 700;
const MOTOR_MAX = 3000;
const SERVO_MIN    =          0;    
const SERVO_MEDIAN =         90;    
const SERVO_MAX    =        180;    

const UPDATE_INTERVAL = 200;

const TOTAL_TIME = 3000;

const TOTAL_STEPS =
    TOTAL_TIME / UPDATE_INTERVAL;

const STEP =
    (MOTOR_MAX - MOTOR_MIN) /
    TOTAL_STEPS;

function enviar(){

    fetch('/api/comando',{

        method:'POST',

        headers:{
            'Content-Type':'application/json'
        },

        body:JSON.stringify({

            motor_speed: Math.round(motorSpeed),

            servo_left: servoLeftAngle,
            servo_right: servoRightAngle

        })

    });

}

setInterval(() => {

    if(motorPressed){

        motorSpeed += STEP;

        if(motorSpeed > MOTOR_MAX){

            motorSpeed = MOTOR_MAX;
        }
    }

    else{

        motorSpeed -= STEP;

        if(motorSpeed < MOTOR_MIN){

            motorSpeed = MOTOR_MIN;
        }
    }

    enviar();

}, UPDATE_INTERVAL);

document.querySelectorAll('button').forEach(btn => {

    const id = btn.id;

    const start = () => {

        if(id === 'cima'){
            servoLeftAngle = SERVO_MAX;
            servoRightAngle = SERVO_MIN;
        }

        else if(id === 'baixo'){
            servoLeftAngle = SERVO_MIN;
            servoRightAngle = SERVO_MAX;            
        }

        else if(id === 'esq'){

            servoLeftAngle = SERVO_MIN;
            servoRightAngle = SERVO_MIN;
        }

        else if(id === 'dir'){

            servoLeftAngle = SERVO_MAX;
            servoRightAngle = SERVO_MAX;
        }

        else if(id === 'parar'){

            servoLeftAngle = SERVO_MEDIAN;
            servoRightAngle = SERVO_MEDIAN;

            motorSpeed = MOTOR_MIN;

            motorPressed = false;
        }

        else if(id === 'motor'){

            motorPressed = true;
        }

        enviar();
    };

    const stop = () => {

        if(
            id === 'cima' ||
            id === 'baixo' ||
            id === 'esq' ||
            id === 'dir'
        ){

            servoLeftAngle = SERVO_MEDIAN;
            servoRightAngle = SERVO_MEDIAN;
        }

        if(id === 'motor'){

            motorPressed = false;
        }

        enviar();
    };

    btn.ontouchstart =
    btn.onmousedown = e => {

        e.preventDefault();

        start();
    };

    btn.ontouchend =
    btn.onmouseup =
    btn.onmouseleave =
    btn.ontouchcancel = stop;

});

</script>

</body>
</html>

)rawliteral";

// ==================== SERVOS ======================

void controlarServos(int leftAngle, int rightAngle){

    wingLeftServo.write(leftAngle);
    wingRightServo.write(rightAngle);
}

// ==================== API ======================

void handleAPI(){

    StaticJsonDocument<200> doc;

    DeserializationError error =
        deserializeJson(doc, server.arg("plain"));

    if(error){

        server.send(
            400,
            "text/plain",
            "JSON invalido"
        );

        return;
    }

    int motorSpeed =
        doc["motor_speed"] | 1000;

    int servoL =
        doc["servo_left"] | 90;

    int servoR =
        doc["servo_right"] | 90;

    motorSpeed =
        constrain(
            motorSpeed,
            1000,
            2000
        );

    escMotor.writeMicroseconds(
        motorSpeed
    );

    controlarServos(
        servoL,
        servoR
    );

    server.send(
        200,
        "application/json",
        "{\"status\":\"ok\"}"
    );
}

// ==================== SETUP ======================

void setup(){

    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);

    // ===== SERVOS =====

    wingLeftServo.attach(SERVO_WING_LEFT);
    wingRightServo.attach(SERVO_WING_RIGHT);

    wingLeftServo.write(90);
    wingRightServo.write(90);

    // ===== ESC =====

    escMotor.attach(MOTOR_PWM_PIN);

    escMotor.writeMicroseconds(MOTOR_MIN);

    delay(3000);

    // ===== WIFI =====

    WiFi.softAP(ssid, password);

    WiFi.softAPConfig(
        local_IP,
        gateway,
        subnet
    );

    // ===== ROTAS =====

    server.on("/", HTTP_GET, [](){

        server.send_P(
            200,
            "text/html",
            html
        );
    });

    server.on(
        "/api/comando",
        HTTP_POST,
        handleAPI
    );

    server.begin();

    Serial.println();
    Serial.println("F22 ESP8266 PRONTO");
    Serial.print("IP -> ");

    Serial.println(WiFi.softAPIP());
}

// ==================== LOOP ======================

void loop(){

    server.handleClient();
}