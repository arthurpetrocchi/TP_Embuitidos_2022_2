#include <WiFi.h>


//------------- Definição das variáveis globais do WIFI --------------------
const char* ssid = "iPhone de Leonardo";
const char* password = "00000000";
WiFiServer server(80);
#define LED_WIFI_STATUS 15
//--------------------------------------------------------------------------

//-------------- Definição das GPIO's de processo globais ------------------
#define LED_BOMBA  23 // Pino do Led indicador de funcionamento da Bomba
#define BOMBA 18      // Pino de saída para acionamento da Bomba 
#define pingPin  14   // Pino do sensor HC-SR04 de comunicação.
//--------------------------------------------------------------------------

//------------ Definição das variáveis de processo globais -----------------
volatile int modo_de_operacao;
volatile long distancia_cm;
volatile int status_Led_Bomba;
int distanciaMax = 18;
int distanciaMin = 4;
//--------------------------------------------------------------------------

void setup() {
  // initialize serial communication:
  Serial.begin(115200);
  pinMode(LED_BOMBA, OUTPUT);
  pinMode(BOMBA, OUTPUT);

  // Inicializando o WIFI-----------------
  pinMode(LED_WIFI_STATUS, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando-se a rede");
    Serial.flush();
  } 
  Serial.println("WiFi conectada.");
  Serial.print("Endereço de IP: ");
  Serial.println(WiFi.localIP());
 
  server.begin();
  //--------------------------------------
}

void loop() {

  Ciclo_Processo();
  Cliente_WIFI();  
}

void Ciclo_Processo(){
  Distancia();
  if(distancia_cm >= distanciaMax){
    Ligar_Bomba();
    Tela_serial();
  }
  if (distancia_cm <= distanciaMin){
    Desligar_Bomba();
    Tela_serial();
  }
  if ((distancia_cm > distanciaMin) && (distancia_cm < distanciaMax)){
    Tela_serial();
  }
}

long Distancia(){  
  long duration;
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  pinMode(pingPin, INPUT);
  duration = pulseIn(pingPin, HIGH);
  // convert the time into a distance
  distancia_cm = microsecondsToCentimeters(duration);
}

long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the object we
  // take half of the distance travelled.
  return microseconds / 29 / 2;
}

void Ligar_Bomba(){
  if(distancia_cm > distanciaMin){
    digitalWrite(LED_BOMBA, HIGH);
    digitalWrite(BOMBA, LOW);
    status_Led_Bomba = 1;
  }   
}

void Desligar_Bomba(){
  digitalWrite(LED_BOMBA, LOW);
  digitalWrite(BOMBA, HIGH);
  status_Led_Bomba = 0;  
}


void Tela_serial(){
  Serial.println("--------------------------------");
  Serial.print("Endereço de IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Distância: ");
  Serial.print(distancia_cm);
  Serial.print("cm");
  Serial.println();
  Serial.print("Volume do Reservatório: "); 
  Serial.print(CalculaVolume());
  Serial.println("cm³");
  Serial.print("Nivel do Reservatório: ");
  Serial.print(CalculaNivel());
  Serial.println("%");
  if (status_Led_Bomba == 1){
    Serial.print("Bomba: LIGADA");
    Serial.println();
  }
  if (status_Led_Bomba == 0){
    Serial.print("Bomba: DESLIGADA");
    Serial.println();
  }
  delay(250);
  Serial.flush();
}

long CalculaVolume(){
  long raio = 6;
  long  distanciaUtil, volume;
  distanciaUtil = distanciaMax - distancia_cm;
  volume = 3.1415 * raio * distanciaUtil * distanciaUtil;
  if (distancia_cm > distanciaMax){
    volume = 0;
  }
  if (distancia_cm < distanciaMin){
    distanciaUtil = distanciaMax - distanciaMin; 
    volume = 3.1415 * raio * distanciaUtil * distanciaUtil;
  }
  return volume;
}

int CalculaNivel(){
  int nivel ;
  nivel = 100* (distanciaMax - distancia_cm)/(distanciaMax - distanciaMin);
  if (distancia_cm > distanciaMax){
    nivel = 0;
  }
  if (distancia_cm < distanciaMin){
    nivel = 100;
  }
  return nivel;
}

 void Cliente_WIFI() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      digitalWrite(LED_WIFI_STATUS, HIGH);
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            Ciclo_Processo();  

            client.print("Sistema de controle e monitorameto do reservatorio");
            client.println();
            
            client.print("Volume do Reservatorio: "); 
            client.print(CalculaVolume());            
            client.print("cm³");
            client.println();
            
            client.print("Nivel do Reservatorio: ");
            client.print(CalculaNivel());
            client.print("%");
            client.println();

            if (status_Led_Bomba == 1){
              client.print("Bomba: LIGADA");
              client.println();
              client.print("Clique <a href=\"/L\">aqui</a> para desligar a bomba.<br>");
              client.println();
            }
            if (status_Led_Bomba == 0){
              client.println("Bomba: DESLIGADA");
              client.println();              
              client.println("Clique <a href=\"/H\">aqui</a> para Ligar a Bomba.<br>");
              client.println();
            }
            
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
        if (currentLine.endsWith("GET /H")) {
          Ligar_Bomba();
        }
        if (currentLine.endsWith("GET /L")) {
          Desligar_Bomba();
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
    digitalWrite(LED_WIFI_STATUS, LOW);
  }
}
