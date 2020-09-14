#include "WiFi.h";
#include "SPIFFS.h";
#include "ESPAsyncWebServer.h";
#include "MPU6050_tockn.h";
#include "Wire.h";

/*const char* ssid = "APS-n";
const char* password = "apsrospbs";*/

/*const char* ssid = "PBS S8";
const char* password = "apsrospbs";*/

const char* ssid = "ZON-EB32";
const char* password = "bom1dia2";

/*const char* ssid = "Festodromo";
const char* password = "gandafesta";*/

const char* PARAM_VELOCIDADE = "velocidadeAjuste";
const char* PARAM_DIRECAO = "direcaoAjuste";

//AsyncWebServer server (80);
AsyncWebServer server (8888);

String spiffs_velocidade = "0";
String spiffs_direcao = "0";

/*IPAddress ip(192,168,1,253);  //Node static IP
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);*/

/*IPAddress ip(192,168,43,253);  //Node static IP
IPAddress gateway(192,168,43,1);
IPAddress subnet(255,255,255,0);*/

IPAddress ip(192,168,1,240);  //Node static IP
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

MPU6050 mpu6050(Wire);

const char values_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <link rel="stylesheet" type="text/css" href="style.css">
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <div>
    <h1>Calibração</h1>
      <form action="/get">
        <label for="vA" class="sample2">Valor de ajuste de velocidade</label>
        <input type="number" id="vA" name="velocidadeAjuste" value="1"><br>
        <label for="dA" class="sample2">Valor de ajuste da direção</label>
        <input type="number" id="dA" name="direcaoAjuste" value="0"><br>
        <label for="dA" class="sample2">Reset automático</label>
        <input type="text" id="rA" name="reset" value="nao"><br>
        <input type="submit" value="Ajuste" class="botao">
      </form>
  </div>
  <footer>
    <p class="f">Pedro Branco Santos</p>
  </footer>
</body> 
</html>)rawliteral";

const char next_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <link rel="stylesheet" type="text/css" href="style.css">
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <div>
    <a href="/">Clique para voltar à pagina de configuração</a>
  </div>
  <footer>
    <p class="f">Pedro Branco Santos</p>
  </footer>
</body>  
</html>)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <link rel="stylesheet" type="text/css" href="style.css">
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
<h1>Monitorização da Direção e</h1>
<h1>da Velocidade do Vento</h1>
     <p class="sample"> Direção:  
      <span id="angle">%ANGLE%</span> &#176;</p>
     <p class="sample"> Velocidade: 
      <span id="speed">%SPEED%</span> km/h
     </p>
     <img id="wind" src="direcao_N" width="200", height="200">
  <footer>
    <p class="f">Pedro Branco Santos</p>
  </footer>
</body>
<script>

  setInterval(function(){
    updateAngle();
    updateSpeed();
  },1000);
  
  function updateAngle(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function(){
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("angle").innerHTML = this.responseText;
        document.getElementById("wind").src = windDirection(this.responseText);
      }
    };
    xhttp.open("GET", "/angle", true);
    xhttp.send();
  }

  function windDirection(value){
    image = "";
    if((value >= 0 && value < 22.5) || (value <= 360 && value > 337.5)){
      image = "direcao_N";
    }
    else if((value >= 22.5 && value < 67.5)){
      image = "direcao_NE";
    }
    else if((value >= 67.5 && value < 112.5)){
      image = "direcao_E";
    }
    else if((value >= 112.5 && value < 157.5)){
      image = "direcao_SE";
    }
    else if((value >= 157.5 && value < 202.5)){
      image = "direcao_S";
    }
    else if(value >= 202.5 && value < 247.5){
      image = "direcao_SO";
    }
    else if(value >= 247.5 && value < 292.5){
      image = "direcao_O";
    }
    else if(value >= 292.5 && value <= 337.5){
      image = "direcao_NO";
    }
    return image;
  }

  function updateSpeed(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function(){
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("speed").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/speed", true);
    xhttp.send();
  }
</script>
</html>)rawliteral";

String readAngle(){
  spiffs_direcao = readFile("/valor_direcao.txt");
  mpu6050.update();
  int angle = mpu6050.getAngleX();
  angle = angle + spiffs_direcao.toInt();
  if(angle < 0){
    angle = 360 + angle;
  }
  return String(angle);
}

String readSpeed(){
  float a = analogRead(32);
  spiffs_velocidade = readFile("/valor_velocidade.txt");
  float value = (a*3.3)/4095;
  if(!spiffs_velocidade.equals("0")){
    value = value * spiffs_velocidade.toFloat();
  }
  float m_s = (value*13.9)/1.0;
  m_s = m_s * 3.6;
  int spe = (m_s*100)/100;
  return String(spe);
}

String processor(const String& var){
  if (var == "ANGLE"){
    return readAngle();
  }
  else if(var == "SPEED"){
    return readSpeed();
  }
}

String readFile(String path){
  File file = SPIFFS.open(path);
  if(!file){
    Serial.println("ficheiro vazio ou não é possível abrir");
    return String();
  }
  String conteudo;
  while(file.available()){
    conteudo+=String((char)file.read());
  }
  return conteudo;
}

void writeFile(fs::FS &fs, const char* path, const char* mensagem){
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("Falha na abertura do ficheiro");
    return;
  }
  if(file.print(mensagem)){
    Serial.println("Mensagem escrita com sucesso");
  }
  else{
    Serial.println("Falha na escrita da mensagem");
  }
}

void setup(){
  
  Serial.begin(115200);
  Wire.begin();

  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);

  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(ssid);
    Serial.print(" ");
    Serial.println(password);
    Serial.println("Connecting to WiFi....");
  }

  if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
  }

  Serial.println(WiFi.localIP());

  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/values_html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", values_html, processor);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/direcao_E", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_E.jpg", "image/jpeg");
  });

  server.on("/direcao_SE", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_SE.jpg", "image/jpeg");
  });

  server.on("/direcao_NE", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_NE.jpg", "image/jpeg");
  });

  server.on("/direcao_SO", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_SO.jpg", "image/jpeg");
  });

  server.on("/direcao_NO", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_NO.jpg", "image/jpeg");
  });

  server.on("/direcao_N", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_N.jpg", "image/jpeg");
  });

  server.on("/direcao_S", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_S.jpg", "image/jpeg");
  });

  server.on("/direcao_O", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/direcao_O.jpg", "image/jpeg");
  });

  server.on("/angle", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readAngle().c_str());
  });

  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSpeed().c_str());
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String input;
    String reset_auto;
    reset_auto = request->getParam("reset")->value();
    Serial.println(reset_auto);
    if(reset_auto.equals("sim")){
      ESP.restart();
      request->send_P(200, "text/html", next_html, processor);
    }
    else{
      if(request->hasParam("velocidadeAjuste")){
        input = request->getParam("velocidadeAjuste")->value();
        writeFile(SPIFFS, "/valor_velocidade.txt", input.c_str());
        spiffs_velocidade = readFile("/valor_velocidade.txt");
      }
      if(request->hasParam("direcaoAjuste")){
        input = request->getParam("direcaoAjuste")->value();
        writeFile(SPIFFS, "/valor_direcao.txt", input.c_str());
        spiffs_direcao = readFile("/valor_direcao.txt");
      }
      request->send_P(200, "text/html", values_html, processor);
    }
  });

  server.begin();
}

void loop(){}
