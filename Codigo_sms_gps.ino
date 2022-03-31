#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <ESP8266WiFi.h>
//#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Defino pino que mantem alimentação da placa
#define MOSFET_PIN D1

//Define Portas Serial
SoftwareSerial serialGSM(D5, D6); // RX, TX
SoftwareSerial serialGPS(D3, D4);

//Define GPS
TinyGPS gps;

// Boleano que indica se chegou ou não SMS (Default = Falso)
bool temSMS = false;

// String onde guarda numero de telemovel que envia sms
String telefoneSMS;

// String onde guarda data/hora de chegada da sms
String dataHoraSMS;

// String onde guarda texto da sms
String mensagemSMS;

//Identificar
String comandoGSM = "";
String ultimoGSM = "";


// leitura porta analógica para calculo Nivel Bateria
float voltage;

//Converte para uma casa decimal
String percentagem;
String voltagem;

//Define função de verificação de chegada de SMS
void validaGSM();

//Define função de verificação de chegada de GPS
void validaGPS();

//Define função de envio de SMS com as coordenadas GPS
void enviaSMS(String telefone, String mensagem);

//Define função de arranque do modulo GSM
void configuraGSM();

//Define função mede nivel bateria
void bateria();

//Comandos SMS Admitidos
#define smsgps "Localizar"
#define smsbateria "Bateria"
#define smsoff "Desligar"


//Numeros de telemovel adminitos
#define telelemovel1 "+351919164363"

void setup() {

// Inicia as comunicações por Serial
  Serial.begin(9600);                                                                                                                                                                                                               
  serialGPS.begin(9600);   
  serialGSM.begin(9600); 

//Define Pinos
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(MOSFET_PIN, OUTPUT); 
  digitalWrite(MOSFET_PIN, LOW);

// Informa que botão foi acionado ( falta envio de sms)  
Serial.println("!!! Botão SOS Acionado !!!");
digitalWrite(LED_BUILTIN, HIGH);  
//Inicio de comunicação com modulo GSM
  configuraGSM();
// Inicio de comunicação com modulo GPS
  validaGPS();
//enviaSMS(telelemovel1, "!!! SOS Acionado !!!");  
}

//Inicio de ciclos de verificação de sinal GPS e recepção de SMS
void loop() {

// Faz leitura de GPS de 5 em 5 seg. 
static unsigned long delayvalidaGPS = millis();
  if ( (millis() - delayvalidaGPS) > 5000 ) {
     validaGPS();     
     delayvalidaGPS = millis(); 
  }
  
// No intervalo dos 5 Seg lê o GSM 
  validaGSM();

  if (comandoGSM != "") {
      Serial.print("comandoGSM: ");
      Serial.println(comandoGSM);
      ultimoGSM = comandoGSM;
      comandoGSM = "";
  }
//Caso receba SMS
  if (temSMS) {

// Informa chegadas de SMS
     Serial.println("Chegou Mensagem!!");
     Serial.println();
// Informa remetente da SMS
     Serial.print("Remetente: ");  
     Serial.println(telefoneSMS);
     Serial.println();
// Informa Data/Hora de chegada
     Serial.print("Data/Hora: ");  
     Serial.println(dataHoraSMS);
     Serial.println();
// Informa corpo da SMS
     Serial.println("Mensagem:");  
     Serial.println(mensagemSMS);
     Serial.println();
// termina a leitura da SMS
     mensagemSMS.trim();   

// Valida numero se numero de telemovel é autorizado    
if (telefoneSMS == telelemovel1 /*|| telefoneSMS == telelemovel2*/){ 

// Valida se é pedido de localização      
     if ( mensagemSMS == smsgps ) {
// Verifica sinal gps 
       Serial.println("A Processar Localização...");  
       validaGPS();

//Define variaveis para guardar Coordenadas
        float flat, flon;
        unsigned long age;

//Obtem coordenadas
        gps.f_get_position(&flat, &flon, &age);

//Caso não tenha sinal informa por SMS
        if ( (flat == TinyGPS::GPS_INVALID_F_ANGLE) || (flon == TinyGPS::GPS_INVALID_F_ANGLE) ) {
           enviaSMS(telefoneSMS, "GPS Sem Sinal !!!");

//Caso tenha Sinal, cria link google maps
        } else {
           String urlMapa = "Local Identificado: \n https://maps.google.com/maps/?&z=10&q=";
           urlMapa += String(flat,6);
           urlMapa += ",";
           urlMapa += String(flon,6);
           urlMapa += "\n";
           urlMapa += "Latitude:";
           urlMapa += String(flat,6);
           urlMapa += "Longitude:";
           urlMapa += String(flon,6);

// Envia para o numero que solicitou
           enviaSMS(telefoneSMS, urlMapa);
        }
// Valida se é pedido nivel da bateria
     }else if ( mensagemSMS == smsbateria ){
      bateria();
      String nivelbateria = " Nivel Bateria: ";
      nivelbateria +=  String (voltagem);
      nivelbateria += "V - ";
      nivelbateria +=  String (percentagem);
      enviaSMS(telefoneSMS, nivelbateria);

// Valida se é comando para desligar
     }else if ( mensagemSMS == smsoff ){
      enviaSMS(telefoneSMS, "A Desligar !!!");
      desliga();

//Informa que o texto do SMS não é um comando válido      
     }else{
       Serial.println("Codigo Desconhecido!");
       enviaSMS(telefoneSMS, "Codigo Desconhecido!"); 
       }

//Informa que o numero de télemovel não é premitido       
}else{
       Serial.println("Numero não autorizado!");
       enviaSMS(telefoneSMS, "Numero não autorizado!"); 
}
     temSMS = false;
  }  
}

//Função de validação de chegadas de SMS
void validaGSM()
{
  static String textoRec = "";
  static unsigned long delay1 = 0;
  static int count=0;  
  static unsigned char buffer[64];

  serialGSM.listen(); 
  if (serialGSM.available()) {            
 
     while(serialGSM.available()) {         
   
        buffer[count++] = serialGSM.read();     
        if(count == 64)
        break;
     }

     textoRec += (char*)buffer;
     delay1   = millis();
     
     for (int i=0; i<count; i++) {
         buffer[i]=NULL;
     } 
     count = 0;                       
  }


  if ( ((millis() - delay1) > 100) && textoRec != "" ) {

     if ( textoRec.substring(2,7) == "+CMT:" ) {
        temSMS = true;
     }

     if (temSMS) {
            
        telefoneSMS = "";
        dataHoraSMS = "";
        mensagemSMS = "";

        byte linha = 0;  
        byte aspas = 0;
        for (int nL=1; nL < textoRec.length(); nL++) {

            if (textoRec.charAt(nL) == '"') {
               aspas++;
               continue;
            }                        
          
            if ( (linha == 1) && (aspas == 1) ) {
               telefoneSMS += textoRec.charAt(nL);
            }

            if ( (linha == 1) && (aspas == 5) ) {
               dataHoraSMS += textoRec.charAt(nL);
            }

            if ( linha == 2 ) {
               mensagemSMS += textoRec.charAt(nL);
            }

            if (textoRec.substring(nL - 1, nL + 1) == "\r\n") {
               linha++;
            }
        }
     } else {
       comandoGSM = textoRec;
     }
     
     textoRec = "";  
  }     
}

//Função de leitura de GPS
void validaGPS() {
unsigned long delayGPS = millis();
  serialGPS.listen();
   bool lido = false;
//Lê sinal GPS durante 500 milisegundos, 
   while ( (millis() - delayGPS) < 500 ) { 
      while (serialGPS.available()) {
          char cIn = serialGPS.read(); 
          lido = gps.encode(cIn); 
      }

      if (lido) { 
         
         float flat, flon;
         unsigned long age;
    
         gps.f_get_position(&flat, &flon, &age);
    
         String urlMapa = "Local Identificado: https://maps.google.com/maps/?&z=10&q=";
           urlMapa += String(flat,6);
           urlMapa += ",";
           urlMapa += String(flon,6);
           urlMapa += "\n";
           urlMapa += "Latitude:";
           urlMapa += String(flat,6);
           urlMapa += "Longitude:";
           urlMapa += String(flon,6);
         Serial.println(urlMapa);
         Serial.println("GPS:OK");  
         break; 
      }
   }   
}

//Função que processa o envio da SMS
void enviaSMS(String telefone, String mensagem) {
  serialGSM.print("AT+CMGS=\"" + telefone + "\"\n");
  serialGSM.print(mensagem + "\n");
  serialGSM.print((char)26); 
}

//Função mostra parametros arranque do GSM
void configuraGSM() {
   serialGSM.print("AT+CMGF=1\n;AT+CNMI=2,2,0,0,0\n;ATX4\n;AT+COLP=1\n"); 
}

//Função leitura nivel de bateria
void bateria() {
voltage = analogRead(A0) * (4.5/1023);

  if (voltage >= 4.40) 
  {
    percentagem = "100%";
    Serial.print("Bateria:");
    Serial.println(percentagem);
    Serial.println(voltage);
  } 
   else if (voltage <= 4.30 && voltage >= 4.20)  
  {
    percentagem = "75%";
    Serial.print("Bateria:");
    Serial.println(percentagem);
    Serial.println(voltage);
    }     
    else if (voltage <= 4.10 && voltage >= 4.00)
    {
        percentagem = "50%";
       Serial.print("Bateria:");
       Serial.println(percentagem);
       Serial.println(voltage); 
      } 
      else if (voltage <= 3.90 && voltage >= 3.70)
      {
        percentagem = "25% - Recomendado colocar a carregar ";
        Serial.print("Bateria:");
        Serial.println(percentagem);
        Serial.println(voltage);    
        }
        else
        {
        percentagem = "0% - Colocar a carregar";
        Serial.print("Bateria:");
        Serial.println(percentagem);
        Serial.println(voltage);  
          }
voltagem = String(voltage,1);  
}

//Função que desliga o sistema, cortando alimentação do pino "mosfet_pin"
void desliga() {
  //Desliga
  Serial.print("ADEUS! /n");
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(MOSFET_PIN, HIGH);
  ESP.deepSleep(0);
}
