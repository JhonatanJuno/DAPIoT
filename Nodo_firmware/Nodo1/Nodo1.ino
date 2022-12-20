#include <Adafruit_BMP280.h>
#define DEVEUI      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
#define APPEUI      {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01} 
#define APPKEY      {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
#define BAND        (RAK_REGION_AU915) //CH0-CH7
#define CLASS       (RAK_LORA_CLASS_A) //Modo de Operacion en Clase A
#define MODECONECT  (RAK_LORA_OTAA)//Modo de coneccion en OTTA
  
//variables globales
uint32_t PERIODO = 30000; //Tiempo Default para envio de datos, cada 30segundos
uint8_t data_uplink[64];  //Arreglo para recolectar datos
Adafruit_BMP280 bmp;      //Objeto de la clase BMP para el sensor

/******************************Funciones Callback*******************************************/
//recvCallback:  Esta funcion se activa cuando recibimos un downlink
void recvCallback(SERVICE_LORA_RECEIVE_T *data){ 
  if( data->BufferSize > 0){
    Serial.println("Se recibio datos en Downlink: ");
    for(int i = 0; i<data->BufferSize; i++){
      Serial.printf("%x",data->Buffer[i]);
    }
    Serial.println(" ");
  }
}

//joinCallback: Esta funcion se activa cuando logramos hacer el Join
void joinCallback(int32_t status){
  Serial.println("Coneccion Exitosa!!!");
  Serial.printf("Estado de la coneccion: %d\r\n", status);
}

//sendCallback : Esta funcion se activa cuando el Network Server confirma la recepcion del dato
void sendCallback(int32_t status){
  if(status == 0){
    Serial.println("Uplink enviado con exito");
  }else{
    Serial.println("Uplink fallo al enviarse");
  }
}

/***********************************Funciones*********************************************/
void  uplink_process(void){
  float temp_f = bmp.readTemperature(); //ºC
  float press_f = bmp.readPressure()/1000;  //KPa
  Serial.printf("T = %.2f | P = %.2f\r\n", temp_f, press_f);

  uint16_t t = (uint16_t)(temp_f*100);
  uint32_t pre =(uint16_t)(press_f*100);
  //construimos el payload a enviar
  uint8_t data_len = 0;
  data_uplink[data_len++] = 0x01; //Data Channel 1
  data_uplink[data_len++] = (uint8_t)(t>>8);
  data_uplink[data_len++] = (uint8_t)t;
  data_uplink[data_len++] = 0x02; //Data Channel 2
  data_uplink[data_len++] = (uint8_t)(pre>>8);
  data_uplink[data_len++] = (uint8_t)pre;

  Serial.println("Payload a enviar: ");
  for(int i = 0; i<data_len; i++){
    Serial.printf("0x%02X ", data_uplink[i]);
  }
  Serial.println(" ");

  //Enviamos el paquete payload
  if(api.lorawan.send(data_len, (uint8_t *) &data_uplink, 2, true, 1)){
    Serial.println("Se envio el payload ");
  }else{
    Serial.println("Fallo al enviar el payload");
  }
}

/*********************SETUP**************************************************************/
void setup(){
  Serial.begin(115200);
  Serial.println("Nodo RAK3172, modo prueba");
  /*******************************SENSOR BMP280*****************************/
  if (!bmp.begin(BMP280_ADDRESS_ALT,BMP280_CHIPID)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Modo de operación */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Presion oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtrado. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Tiempo Standby. */
  /********************************LORAWAN**********************************/
  //DEVEUI
  uint8_t node_device_deveui[8] = DEVEUI;
  //APPEUI
  uint8_t node_device_appeui[8] = APPEUI;
  //APPKEY
  uint8_t node_device_appkey[16] = APPKEY;

  //configuramos el dev eui
  api.lorawan.deui.set(node_device_deveui,8); 
  //configuramos el app eui
  api.lorawan.appeui.set(node_device_appeui,8);
  //configuramos el app key
  api.lorawan.appkey.set(node_device_appkey,16);
  //configuramos las bandas
  api.lorawan.band.set(BAND);
  //configuramos las Clase de operacion del nodo
  api.lorawan.deviceClass.set(CLASS);
  //Configuramos el modo de coneccion al network server
  api.lorawan.njm.set(MODECONECT);
  //Iniciamos la coneccion hacia el network server
  api.lorawan.join();

  //Esperar la coneccion( esperar que nos den la autorizacion a la Red por el N.S)
  while(api.lorawan.njs.get()== 0){
    Serial.print("Esperando conexion a la red LoRAWAN ...");
    //Iniciamos la coneccion hacia el network server
    api.lorawan.join();
    delay(10000); //Damos 10 segundos de espera
  }
  //Habilitamos el modo adaptativo del ADR y SF
  api.lorawan.adr.set(true);
  //Habilitamos el numero de intentos de coneccion
  api.lorawan.rety.set(1);
  //Habilitamos el modo de confirmacion
  api.lorawan.cfm.set(1);

  //Registramos las funciones CallBack
  api.lorawan.registerRecvCallback(recvCallback);
  api.lorawan.registerJoinCallback(joinCallback);
  api.lorawan.registerSendCallback(sendCallback);
}
/**********************LOOP***************************************************************/

void loop(){
    uplink_process();
  //Ponemos a dormir el nodo
  api.system.sleep.all(PERIODO);
  Serial.println("Durmiendo..\r\n");
}
