
#include <Arduino.h>
#include <WiFiClient.h>

#include "dweet_device.h"

struct {
    float temperatura;
    float humidade;
    float sensacaotermica;
} dweet_data;

String arrayVariableNames[] = {"Temperatura","Humidade"};//THIS WAY WE REFER TO EACH VARIABLE AS AN INDEX IN THIS GLOBAL ARRAY.
int  arrayVariableValues[] = {0,0};

String GetDweetStringHttpBuilder() {
  //int i=0;
  //int numberVariables=sizeof(arrayVariableValues)/sizeof(arrayVariableValues[0]);//tells the number of arguments inside each array
  String dweetHttpGet="GET /dweet/for/";//initial empty get request
  
  dweetHttpGet = dweetHttpGet + String(THINGSNAME) + "?";//start concatenating the thing name (dweet.io)
  dweetHttpGet.concat("temperatura=");
  dweetHttpGet.concat(dweet_data.temperatura);
  dweetHttpGet.concat("&");
  dweetHttpGet.concat("humidade=");
  dweetHttpGet.concat(dweet_data.humidade);
  dweetHttpGet.concat("&");
  dweetHttpGet.concat("sensaceotermica=");
  dweetHttpGet.concat(dweet_data.sensacaotermica);
/**
  for(i=0;i<(numberVariables);i++){//concatenate all the variable names and values
   if(i==numberVariables-1){
    dweetHttpGet=dweetHttpGet + String(arrayVariableNames[i]) + "="+ String( arrayVariableValues[i]);//the lastone doesnt have a "&" at the end

   }
   else
    dweetHttpGet=dweetHttpGet + String(arrayVariableNames[i]) + "="+ String( arrayVariableValues[i]) + "&";  
  }
  **/
 
 dweetHttpGet= dweetHttpGet +   " HTTP/1.1\r\n"+
                                "Host: " + 
                                DWEETHOST + 
                                "\r\n" + 
                                "Connection: close\r\n\r\n";
                  
  return dweetHttpGet;//this is our freshly made http string request
}

void dweetdata(){//connects TCP,sends dweet,drops connection, prints the server response on the console 
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(DWEETHOST, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
    client.print(GetDweetStringHttpBuilder());

  delay(10);//slow doown ...stability stuff
  
  // Read all the lines of the reply from dweet server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  //just display ending conection on the serial port
  Serial.println();
  Serial.println("closing connection");
   
}

void dweet_loop(float *flttemperatura, float *flthumidade, float *fltsensacaotermica) {

    arrayVariableValues[0]=30;
    arrayVariableValues[1]=50;

    dweet_data.temperatura = *flttemperatura;
    dweet_data.humidade = *flthumidade;
    dweet_data.sensacaotermica = *fltsensacaotermica;
    
    dweetdata();

}