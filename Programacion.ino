//----------Librerías----------//
#include <ThingerESP8266.h>  //Librería para la plataforma IoT.
#include <ESP8266WiFi.h>  //Librería para las funciones de WiFi.
#include <OneWire.h>  //Librería para la comunicación One-Wire.
#include <DallasTemperature.h>  //Librería para el sensor DS18B20.

//-----Conexión entre la placa NodeMCU y Thinger.io-----//
#define USERNAME "TemperaturaIstea" //Nombre del usuario.
#define DEVICE_ID "SistemaIoT"  //ID del dispositivo.
#define DEVICE_CREDENTIAL "Temperatura32"  //Credenciales del Dispositivo.

//----------Conexión a una red WiFi----------//
#define APSSID "Istea" //Nombre de la red WiFi.
#define APPSK "123456789"  //Contraseña de la red WiFi.

//----------Definición de los GPIO----------//
//Cable de datos del sensor (amarillo) conectado al GPIO4 (D2).
#define pinDatosDQ 4
#define ventilador 12 //En el GPIO12 (D6) se conecta el ventilador.
#define led 14 //En el GPIO14 (D5) se conecta el diodo led.
//El GPIO5 (D1) es la salida a relé para el aire acondicionado.
#define aire_acondicionado 5
//El GPIO16 (D0) es la salida a relé para el calefactor.
#define calefactor 16

//----------Definición de Variables----------//
//Variable para el modo de funcionamiento (manual o automático).
bool modo = true;
//Límites de la temperatura de la zona de confort.
double  temp_max = 32.00, temp_min = 28.00;
//Límites de la temperatura para las alertas al correo electrónico.
double temp_max_alerta = 36.00, temp_min_alerta = 22.00;
double temperatura; //Variable para almacenar la temperatura.

//Crea un objeto en la Librería One-Wire para el sensor.
OneWire DS18B20(pinDatosDQ);
//Enlaza el objeto con la librería del sensor.
DallasTemperature sensorDS18B20(&DS18B20);

//Enlaza la placa NodeMCU con la plataforma IoT.
ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

//-----------Función Principal-----------//
void setup(){
  Serial.begin(115200); //Inicializa la comunicación serial.
  
  //Declara los GPIO como salidas.
  pinMode(ventilador,OUTPUT);
  pinMode(led,OUTPUT);
  pinMode(aire_acondicionado,OUTPUT);
  pinMode(calefactor,OUTPUT);
  
  //Apaga los GPIO.
  digitalWrite(ventilador,LOW);
  digitalWrite(led,LOW);
  digitalWrite(aire_acondicionado,LOW);
  digitalWrite(calefactor,LOW);
  
  //Verifica que la placa NodeMCU se ha conectado a la red WiFi.
  Serial.print("Conectando a ");
  Serial.println(APSSID);
  WiFi.begin(APSSID, APPSK);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado al WiFi");
  
  sensorDS18B20.begin();  //Inicializa el sensor.
  sensorDS18B20.requestTemperatures();  //Adquiere la temperatura.
  
  thing.add_wifi(APSSID, APPSK);
}

//-------------Función Cíclica-------------//
void loop(){
  //Evalúa si el sistema está en modo manual.
  if(modo == true){
    //Adquiere la información de la IoT.
    thing["calefactor"] << digitalPin(calefactor);
    thing["ventilador"] << digitalPin(ventilador);
    thing["modo_manual"] << inputValue(modo);
  }
 else{//Caso contrario.
    //Evalúa si la temperatura adquirida supera la temperatura máxima de la zona de confort.
    if (temperatura >= temp_max){
    //Se enciende el ventilador y el aire acondicionado.
    digitalWrite(ventilador,HIGH);  
    digitalWrite(aire_acondicionado,HIGH);
    //Se apaga el led y el calefactor.
    digitalWrite(led,LOW);
    digitalWrite(calefactor,LOW);
    //Imprime un mensaje por el puerto serial.
    Serial.println("Ventilador Activado");  
    }
    //Evalúa si la temperatura adquirida es menor la temperatura mínima de la zona de confort.
    else if (temperatura <= temp_min){
    //Apaga el ventilador y el aire acondicionado.
    digitalWrite(ventilador,LOW);
    digitalWrite(aire_acondicionado,LOW);
    //Se enciende el led y el calefactor.
    digitalWrite(led,HIGH);
    digitalWrite(calefactor,HIGH);
    //Imprime un mensaje por el puerto serial.
    Serial.println("Calefactor Activado");
    }
    //Evalúa si la temperatura sobrepasa los límites permitidos.
    else if (temperatura >= temp_max_alerta || temperatura <= temp_min_alerta){
      //Imprime un mensaje por el puerto serial.
      Serial.println("Temperatura fuera del rango permitido");
      //Envía la alerta por correo electrónico.
      thing.call_endpoint("Temperatura en ",thing["temperatura"]);
    }
    else{ //Evalúa si está en la zona de confort.
      //Se mantienen apagados todos los actuadores.
      digitalWrite(ventilador,LOW);
      digitalWrite(aire_acondicionado,HIGH);
      digitalWrite(led,LOW);
      digitalWrite(calefactor,HIGH);
      //Imprime un mensaje por el puerto serial
      Serial.println("Zona de Confort");
    }
  }
  //Obtiene la temperatura del sensor de la posición cero.
  temperatura = sensorDS18B20.getTempCByIndex(0);
  sensorDS18B20.requestTemperatures();

  //Envía la temperatura del sensor a la IoT.
  thing["temperatura"] >> [] (pson & out) {
    out["temperatura"] = temperatura;
  };
  //Imprime serialmente la temperatura.
  Serial.print("La temperatura es: ");    
  Serial.print(temperatura);              
  Serial.println("ºC");                   
  //Toma la temperatura cada segundo.
  delay(1000);
  thing.handle();
}
