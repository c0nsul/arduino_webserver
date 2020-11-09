#include <SPI.h>
#include <Ethernet.h>

#define NUM_READINGS 200
// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 180); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80


int rotatorLeft = 8;
int rotatorRight = 9;
int switchOnOff = 7;
int CurrentAzimuth = A0;  // select the input pin for the antenna potentiometer


int newAzimuth = NULL;

String setDirection = "stop";
bool powerStatusBool = false;
String HTTP_req;            // stores the HTTP request





void setup()
{
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    Serial.begin(9600);       // for diagnostics   

    pinMode (rotatorLeft, OUTPUT); //CCW 
    pinMode (rotatorRight, OUTPUT);//CW
    pinMode (switchOnOff, OUTPUT); //ON-OFF 
    Serial.println(Ethernet.localIP());  
}

void loop()
{

  checkAngle();

  //sequring  
  if (CurrentAzimuth >= 360 || CurrentAzimuth <= 0) {
     setDirection = "stop";
     digitalWrite (rotatorLeft, LOW);
     digitalWrite (rotatorRight, LOW);
  }
  

  //presession stop
  if (newAzimuth != NULL) {
    if ((setDirection.compareTo("left") == 0 && CurrentAzimuth <= newAzimuth+3) || (setDirection.compareTo("right") == 0  && CurrentAzimuth >= newAzimuth-3)) {
      Serial.println("//STOP after GO");    
      setDirection = "stop";     
      digitalWrite (rotatorLeft, LOW);
      digitalWrite (rotatorRight, LOW);    
    }
  }
  
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) { 
                char c = client.read();
                                
                if (HTTP_req.length() < 50) {          
                   HTTP_req += c;
                }
                
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println(F("HTTP/1.1 200 OK"));
                    client.println(F("Content-Type: text/html"));
                    client.println(F("Connection: keep-alive"));
                    client.println();
                    // AJAX request for switch state

                    if (HTTP_req.indexOf("start") > -1){                                  
                        client.println("POWER ON");
                        //relay ON
                         digitalWrite (switchOnOff, HIGH);
                         powerStatusBool = true;                                          
                         //STOP (just in case)
                        digitalWrite (rotatorLeft, LOW);
                        digitalWrite (rotatorRight, LOW);
                    
                    }else if (HTTP_req.indexOf("finish") > -1){                                       
                        powerStatusBool = false;                            
                        digitalWrite (rotatorLeft, LOW);
                        digitalWrite (rotatorRight, LOW);
                        digitalWrite (switchOnOff, LOW);                        
                        setDirection = "stop";
                        client.println("POWER OFF");
                    }else if (HTTP_req.indexOf("left") > -1){                       
                      client.println("ROTATION: LEFT");
                       setDirection = "left";
                       digitalWrite (rotatorRight, LOW);
                       digitalWrite (rotatorLeft, HIGH);
                       
                    }else if (HTTP_req.indexOf("right") > -1){                       
                      client.println("ROTATION: RIGHT");
                       setDirection = "right";
                       digitalWrite (rotatorLeft, LOW);
                       digitalWrite (rotatorRight, HIGH);
                       
                    }else if (HTTP_req.indexOf("stop") > -1) {
                       client.println("ROTATION: STOP!");
                       setDirection = "stop";
                       newAzimuth = NULL;
                       digitalWrite (rotatorLeft, LOW);
                       digitalWrite (rotatorRight, LOW);
                       
                    } else if (HTTP_req.indexOf("azimuth") >= 0)   {
                      String azimuth;
                      azimuth = HTTP_req.substring(HTTP_req.indexOf("azimuth")+8, HTTP_req.indexOf(" H"));                      
                      newAzimuth = azimuth.toInt();
                      if (!newAzimuth || newAzimuth >= 360 || newAzimuth <= 0){                        
                        newAzimuth = NULL;
                        setDirection = "stop";                        
                        digitalWrite (rotatorLeft, LOW);
                        digitalWrite (rotatorRight, LOW);    
                      } else {
                        if (newAzimuth > CurrentAzimuth)  {
                          setDirection = "right";
                          digitalWrite (rotatorLeft, LOW);
                          digitalWrite (rotatorRight, HIGH);
                        } else {
                          setDirection = "left";
                          digitalWrite (rotatorRight, LOW);
                          digitalWrite (rotatorLeft, HIGH);
                        }
                      }           
                      client.print("NEW AZIMUTH:");
                      client.print(newAzimuth);
                       
                    } else {  
                       // HTTP request for web page
                        // send web page - contains JavaScript with AJAX calls
                        client.println(F("<!DOCTYPE html>"));
                        client.println(F("<html lang='en'>"));
                        client.println(F("<head>"));
                        client.println(F("<title>Arduino Web Page</title>"));
                        client.println(F("<style>"));
                        client.println(F(".label {font-size: 22px; text-align:center;}"));
                        client.println(F(".btn {width: 140px; height: 50px; font-size: 22px; -webkit-appearance: none; background-color:#dfe3ee; }"));
                        client.println(F(".btn_small { -webkit-appearance: none; background-color:#dfe3ee; }"));
                        client.println(F(".active {width: 140px; height: 50px; font-size: 22px; -webkit-appearance: none; background-color:red; color:white }"));
                        client.println(F("</style>"));
                        client.println(F("<script>"));                        
                        client.println(F("var nocache = \"&nocache=\" + Math.random() * 1000000;"));
                        client.println(F("var request = new XMLHttpRequest();"));
                        client.println(F("function doCmd(param) {"));    
                        client.println(F("if (param == 'azimuth') {value = document.getElementById(\"azimuth_value\").value;} else {value = 1;}"));           
                        client.println(F("request.onreadystatechange = function() {"));
                        client.println(F("if (this.readyState == 4) {"));
                        client.println(F("if (this.status == 200) {"));
                        client.println(F("if (this.responseText != null) {"));
                        client.println(F("document.getElementById(\"status_txt\").innerHTML = this.responseText;"));
                        client.println(F("}}}}"));
                        client.println(F("request.open(\"GET\", param +\"=\"+value + nocache, true);"));
                        client.println(F("request.send(null);"));
                        //client.println(F("if (param == 'start' || param == 'finish' || param == 'azimuth'){ location.reload();}"));
                        client.println(F("location.reload();"));
                        client.println(F("}"));
                        client.println(F("</script>"));
                        client.println(F("</head>"));
                        client.println(F("<body>"));
                        client.println(F("<h1>Ethernet Rotator Controller</h1>"));
                        client.println(F("<p id=\"status_txt\">Awaiting...</p>"));                       
                        if (!powerStatusBool) {
                          client.println(F("<div id=\"startButton\"><button class=\"btn\" onclick=\"doCmd('start')\">START</button></div>"));
                        } else {
                          client.println(F("<div id=\"controlCenter\">"));

                          if (setDirection.compareTo("left") == 0) {
                             client.println(F("<button class=\"active\" id=\"btn_1\" onclick=\"doCmd('left')\">LEFT</button>"));
                          } else {
                             client.println(F("<button class=\"btn\" id=\"btn_1\" onclick=\"doCmd('left')\">LEFT</button>"));
                          }

                          if (setDirection.compareTo("stop") == 0) {
                             client.println(F("<button class=\"active\" id=\"btn_1\" onclick=\"doCmd('stop')\">STOP</button>"));
                          } else {
                             client.println(F("<button class=\"btn\" id=\"btn_1\" onclick=\"doCmd('stop')\">STOP</button>"));
                          }

                          if (setDirection.compareTo("right") == 0) {
                             client.println(F("<button class=\"active\" id=\"btn_1\" onclick=\"doCmd('right')\">RIGHT</button>"));
                          } else {
                             client.println(F("<button class=\"btn\" id=\"btn_1\" onclick=\"doCmd('right')\">RIGHT</button>"));
                          }
                                                    
                          client.println(F("<button class=\"btn\" onclick=\"doCmd('finish')\">SHUTDOWN</button></div>"));

                          client.println(F("<br/><br/>"));
                          client.println(F("New Azimuth: <input type=\"text\" value=180 id=\"azimuth_value\" /><button class=\"btn_small\" onclick=\"doCmd('azimuth')\" id=\"goBtn\">Go!</button>"));

                          client.print(F("<br/><br/><table border=\"1\" width=\"20%\"><tr><td colspan=\"2\" width=\"100%\" align=\"center\">Azimuth</td></tr>"));
                          client.print(F("<tr><td width=\"50%\" align=\"center\">Current</td><td align=\"center\">New</td></tr>"));
                          client.print(F("<tr><td width=\"50%\"><span style=\"font-size:50px\" align=\"center\">"));
                          client.print(CurrentAzimuth);
                          client.print(F("</span></td><td style=\"font-size:50px\" align=\"center\">"));
                          if (newAzimuth > 0) {
                            client.print(newAzimuth);  
                          } else {
                            client.print(F("-"));
                          }
                          client.print(F("</td></tr></table></div>"));
                          

                        } 
                        client.println(F("</body>"));
                        client.println(F("</html>"));
                    }
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    HTTP_req = "";            // finished with request, empty string
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(200);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

void checkAngle() {
  //INPUT FILTER
  long sum = 0;    
  int sensorValue = 0;   
  int angle;
  
  for (int i = 0; i < NUM_READINGS; i++) {      // согласно количеству усреднений
    sum += analogRead(CurrentAzimuth);              // суммируем значения с датчика в переменную sum
  }
  sensorValue = sum / NUM_READINGS;  //get avarage DATA
  Serial.println("Row sensor:");
  Serial.println(sensorValue);
  delay(1000);
  
  
  angle = ceil((sensorValue/1.9));  
  // diapazon: 0- 685
  //angle = sensorValue / 2;  

  CurrentAzimuth = angle;
}

void checkRelay(){
  
}
