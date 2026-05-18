#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <RF24.h>

RF24 radio(4, 15);
const byte address[6] = "00001";

ESP8266WebServer server(80);

String lastMessage = "";

const char* ssid = "espChat1";
const char* password = "12345678";

String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>ESP Chat</title>
<style>
body{
font-family:Arial;
background:#111;
color:white;
padding:20px;
}
#chat{
height:300px;
overflow:auto;
border:1px solid #555;
padding:10px;
margin-bottom:10px;
}
input{
width:80%;
padding:10px;
}
button{
padding:10px;
}
</style>
</head>
<body>

<h2>ESP CHAT 1</h2>

<div id="chat"></div>

<input id="msg">
<button onclick="sendMsg()">Enviar</button>

<script>

async function sendMsg(){
let txt=document.getElementById("msg").value;

await fetch("/send?msg="+encodeURIComponent(txt));

document.getElementById("msg").value="";
}

async function update(){

let r = await fetch("/get");
let t = await r.text();

document.getElementById("chat").innerHTML=t;
}

setInterval(update,1000);

</script>

</body>
</html>
)rawliteral";

void handleRoot(){
  server.send(200,"text/html",html);
}

void handleSend(){

  String msg = server.arg("msg");

  radio.stopListening();

  char text[32];
  msg.toCharArray(text,32);

  radio.write(&text, sizeof(text));

  lastMessage += "<div><b>EU:</b> " + msg + "</div>";

  server.send(200,"text/plain","OK");
}

void handleGet(){
  server.send(200,"text/html",lastMessage);
}

void setup() {

  Serial.begin(115200);

  WiFi.softAP(ssid,password);

  server.on("/",handleRoot);
  server.on("/send",handleSend);
  server.on("/get",handleGet);

  server.begin();

  radio.begin();
  radio.openWritingPipe(address);
  radio.openReadingPipe(1,address);

  radio.setPALevel(RF24_PA_LOW);

  radio.startListening();
}

void loop() {

  server.handleClient();

  if(radio.available()){

    char text[32] = "";

    radio.read(&text,sizeof(text));

    lastMessage += "<div><b>OUTRO:</b> ";
    lastMessage += text;
    lastMessage += "</div>";
  }
}