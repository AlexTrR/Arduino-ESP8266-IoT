#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <time.h>

ESP8266WiFiMulti wifiMulti;
#define RELAY_PIN D1
WiFiServer server(80);

int i = 0, j = 0, k = 0, l = 0, m = 0;
int value = LOW;
tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;

void setup() {

  const char* NTP_SERVER = "pool.ntp.org";
  //Utilizar el link https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  //para definir el Timezone con DLS.
  const char* TZ_INFO    = "CST6CDT,M4.1.0,M10.5.0";
  Serial.begin(115200);
  delay(1000);
  pinMode(RELAY_PIN, OUTPUT);
  //Agrega las redes Wi-Fi a las que te quieres poder conectar.
  wifiMulti.addAP("RedWifi1", "ContraseñaRed1");
  wifiMulti.addAP("RedWifi2", "ContraseñaRed2");
  wifiMulti.addAP("RedWifi3", "ContraseñaRed3");
  wifiMulti.addAP("RedWifiN", "ContraseñaRedN");

  Serial.print("Conectando");
 //Espera la conección Wi-Fi: 
 //Escanea redes cercanas y se conecta a la más fuerte de las redes antes listadas.
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.print("\nConectado a: ");
  Serial.println(WiFi.SSID());
  //Inicia el Servidor ESP8266 en la red en la que está conectado.
  server.begin();
  Serial.println("Servidor iniciado");
  //Muestra el número de URL del Servidor ESP8266.
  Serial.print("Usa este URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);
  
  //Espera 10seg para sincronisación de lo contrario reinicia el controlador.
  if (getNTPtime(10)) {
  } else {
    Serial.println("NTP no actualizado");
    ESP.restart();
  }
  showTime(timeinfo);
  lastNTPtime = time(&now);
  lastEntryTime = millis();
}

void loop() {

  //Cada hora se conecta al resvidor para actualizar la hora correcta.
  getTimeReducedTraffic(3600);
  //Solo sirve para comunicar en serial los datos.
  showTime(timeinfo);
  //Revisa la conexión del cliente.
  WiFiClient client = server.available();
  //Lee la primera línea del requerimiento.
  String request = client.readStringUntil('\r');
  client.flush();
  //Gestiona la solicitud desde el servidor.
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(RELAY_PIN, HIGH);
    value = HIGH;
    m = 1;
  }
  if (request.indexOf("/LED=OFF") != -1) {
    digitalWrite(RELAY_PIN, LOW);
    value = LOW;
    m = 0;
  }
  // Avisa en el servidor el status de la luz mediante la creaci[on de la página web.
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: #77878A;}</style></head>");
  //Header del Servidor.
  if (value == HIGH) client.println("<body><h1>La luz esta actualmente: Prendida</h1>");
  if (value == LOW) client.println("<body><h1>La luz esta actualmente: Apagada</h1>");
  //Elige el boton con base en el valor "value".
  if (value == LOW) {
    client.println("<p><a href=\"/LED=ON\"><button class=\"button\">ON</button></a></p>");
  } else {
    client.println("<p><a href=\"/LED=OFF\"><button class=\"button button2\">OFF</button></a></p>");
  }
  client.println("<br>\n V.C.:");
  client.println(i);
  client.println(j);
  client.println(k);
  client.println(l);
  client.println("<br>");
  client.println("</body></html>");
  //La respuesta HTTP termina con una línea en blanco.
  client.println();
  //Código funcional:
  //Reinicia todo a las 02:01 horas
  if ( (timeinfo.tm_hour == 2) && (timeinfo.tm_min == 1) ) {
    i = 0;
    m = 0;
    digitalWrite(RELAY_PIN, LOW);
    value = LOW;
  }
  //Prende la luz 30 min a las 06:00 horas solo de Lunes a Viernes o durante las V.C. o si el cliente lo solicita.
  if ( (timeinfo.tm_hour == 6) && (timeinfo.tm_min >= 0) && (timeinfo.tm_min <= 30) && (timeinfo.tm_wday > 0) && (timeinfo.tm_wday < 6) || (timeinfo.tm_hour == j) && (timeinfo.tm_min >= k) && (timeinfo.tm_min <= k + l) || m == 1) {
    digitalWrite(RELAY_PIN, HIGH);
    value = HIGH;
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
    value = LOW;
  }
  //Después de las 18 horas de cada día hace un calculo aleatorio de tiempos para encender al menos 5 minutos la luz.
  if ( (timeinfo.tm_hour >= 19) && (i == 0) ) {
    j = random(20, 24);//Hora de inicio.
    k = random(0, 30);//Minuto de inicio.
    l = random(5, 30);//Tiempo de duración.
    i = 1;//V.C. Una vez al día.
  }
}

bool getNTPtime(int sec) {
  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      Serial.print(".");
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // Llamada de NTP no lograda
    Serial.print("now ");  Serial.println(now);
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    Serial.println(time_output);
    Serial.println();
  }
  return true;
}

void getTimeReducedTraffic(int sec) {
  tm *ptm;
  if ((millis() - lastEntryTime) < (1000 * sec)) {
    now = lastNTPtime + (int)(millis() - lastEntryTime) / 1000;
  }
  else {
    lastEntryTime = millis();
    lastNTPtime = time(&now);
    now = lastNTPtime;
    Serial.println("NTP actualizado");
  }
  ptm = localtime(&now);
  timeinfo = *ptm;
}

void showTime(tm localTime) {

  Serial.print("\n");
  Serial.print(localTime.tm_mday);
  Serial.print("/");
  Serial.print(localTime.tm_mon + 1);
  Serial.print("/");
  Serial.print(localTime.tm_year - 100);
  Serial.print(" - ");
  Serial.print(localTime.tm_hour);
  Serial.print(":");
  Serial.print(localTime.tm_min);
  Serial.print(":");
  Serial.print(localTime.tm_sec);
  Serial.print(" ");
  if (localTime.tm_wday == 0 ) Serial.print("Domingo");
  if (localTime.tm_wday == 1 ) Serial.print("Lunes");
  if (localTime.tm_wday == 2 ) Serial.print("Martes");
  if (localTime.tm_wday == 3 ) Serial.print("Miércoles");
  if (localTime.tm_wday == 4 ) Serial.print("Jueves");
  if (localTime.tm_wday == 5 ) Serial.print("Viernes");
  if (localTime.tm_wday == 6 ) Serial.print("Sábado");
  Serial.print(" ");
  Serial.print(" Luz:");
  Serial.print(value);
  Serial.print(" V.C.:");//Variables de Control.
  Serial.print(i);
  Serial.print(" ");
  Serial.print(j);
  Serial.print(k);
  Serial.print(l);
  Serial.print(" ");
  Serial.print(m);
}
