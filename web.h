#ifndef WEB
#define WEB

#include <WiFi.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#ifndef WEB_ACCESS_POINT
const char* ssid = "enpassant";
const char* passwd = "DzHb-Zq5n-YJrV-Y3K8";
#endif

#ifdef WEB_ACCESS_POINT
const char* ssid = "Coolchess7000";
const char* passwd = "";
#endif

AsyncWebServer server(80);

String header;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 5000;
extern char board[8][8];
extern int flashOccupants;
extern struct color colors[];
int counts[64*(sizeof(colors) / sizeof(colors[0]))];
int countscounter = 0;
extern uint8_t * jpg_buf;
extern size_t jpg_len;


String htdoc() {
  String htdoc = "";

  htdoc += "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"><title>Coolchess7000</title></head>";
  htdoc += "<body style=\"background-color: #404040;\"><center><h1 style=\"font-size: 48px; color: white; font-family: Arial, Helvetica, sans-serif;\">Coolchess7000</h1><canvas id=\"cv\" width=\"576\" height=\"576\" style=\"border: 8px solid #323232;\"></canvas></center>";
  htdoc += "<script>var c = document.getElementById(\"cv\"); var ctx = c.getContext(\"2d\"); const subc = document.createElement('canvas');	const subctx = subc.getContext('2d'); ctx.moveTo(0, 0); let k = 0; for (let i = 0; i < 8; i++){ for (let j = 0; j < 8; j++){ if(k % 2 == 0){ ctx.fillStyle = \"rgb(198, 198, 172)\"; } else {	ctx.fillStyle = \"rgb(125, 150, 90)\"; } ctx.fillRect(i * 72, j * 72, (i + 1) * 72, (j + 1) * 72); k++; }	k++; }";
  htdoc += "let empty = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];";
  htdoc += "let rook = [ 1, 1, -1, 0, 1, 1, -1, 0, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 ];";
  htdoc += "let knight = [ 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 0, 1, -1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, -1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 ];";
  htdoc += "let bishop = [ 0, 0, 0, 0, 1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, -1, 0, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, -1, 0, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, -1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, -1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, -1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 ];";                              
  htdoc += "let king = [ 0, 0, 0, 0, 1, 1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, -1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 ];";
  htdoc += "let queen = [ 0, 0, 0, 1, 1, -1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, -1, 1, 1, 1, 1, 1, -1, 1, 1, -1, 1, 1, -1, 1, 1, 1, 1, 1, -1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 ];";
  htdoc += "let pawn = [ 0, 0, 0, 0, 1, 1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 ];";
  htdoc += "function getbmp(piece) { subc.width = 72; subc.height = 72; subctx.clearRect(0, 0, 72, 72); let pieceData; switch(piece.toLowerCase()){";
  htdoc += "case 'r':";
  htdoc += "	pieceData = rook;";
  htdoc += "	break;";
  htdoc += "case 'n':";
  htdoc += "	pieceData = knight;";
  htdoc += "	break;";
  htdoc += "case 'b':";
  htdoc += "	pieceData = bishop;";
  htdoc += "	break;";
  htdoc += "case 'k':";
  htdoc += "	pieceData = king;";
  htdoc += "	break;";
  htdoc += "case 'q':";
  htdoc += "	pieceData = queen;";
  htdoc += "	break;";
  htdoc += "case 'p':";
  htdoc += "	pieceData = pawn;";
  htdoc += "	break;";
  htdoc += "default:";
  htdoc += "	pieceData = empty;";
  htdoc += "	break;";
  htdoc += "} for (let y = 0; y < 14; y++){ for (let x = 0; x < 12; x++){ if (pieceData[y * 12 + x] === 1) { subctx.fillStyle = piece.toLowerCase() === piece ? \"rgb(245, 245, 245)\" : \"black\"; subctx.fillRect(x*4 + 12, y*4 + 8, 4, 4); } if (pieceData[y * 12 + x] === -1){ subctx.fillStyle = \"gray\"; subctx.fillRect(x*4 + 12, y*4 + 8, 4, 4); } } } const img = new Image(); img.src = subc.toDataURL(); return img; } let game = [";
  char* r1; char* r2; char* r3; char* r4; char* r5; char* r6; char* r7; char* r8;
  asprintf(&r1, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[0][0], board[0][1], board[0][2], board[0][3], board[0][4], board[0][5], board[0][6], board[0][7]);
  asprintf(&r2, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[1][0], board[1][1], board[1][2], board[1][3], board[1][4], board[1][5], board[1][6], board[1][7]);
  asprintf(&r3, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[2][0], board[2][1], board[2][2], board[2][3], board[2][4], board[2][5], board[2][6], board[2][7]);
  asprintf(&r4, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[3][0], board[3][1], board[3][2], board[3][3], board[3][4], board[3][5], board[3][6], board[3][7]);
  asprintf(&r5, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[4][0], board[4][1], board[4][2], board[4][3], board[4][4], board[4][5], board[4][6], board[4][7]);
  asprintf(&r6, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[5][0], board[5][1], board[5][2], board[5][3], board[5][4], board[5][5], board[5][6], board[5][7]);
  asprintf(&r7, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ], ", board[6][0], board[6][1], board[6][2], board[6][3], board[6][4], board[6][5], board[6][6], board[6][7]);
  asprintf(&r8, "[ '%c', '%c', '%c', '%c', '%c', '%c', '%c', '%c' ] " , board[7][0], board[7][1], board[7][2], board[7][3], board[7][4], board[7][5], board[7][6], board[7][7]);
  htdoc += String(r1);
  htdoc += String(r2);
  htdoc += String(r3);
  htdoc += String(r4);
  htdoc += String(r5);
  htdoc += String(r6);
  htdoc += String(r7);
  htdoc += String(r8);
  htdoc += "]; ctx.fillStyle = \"rgb(75, 75, 75)\"; ctx.font = \"50px sans-serif\"; for (let i = 0; i < 8; i++){ for (let j = 0; j < 8; j++){ ctx.drawImage(getbmp(game[j][i]), i * 72, j * 72); } }";
  #ifdef CV_DEBUG_COUNTS
  htdoc += "const web_counts = [];";
  for(int i = 0; i < sizeof(counts)/4; i++){
    char *count; asprintf(&count, "%d", counts[i]);
    htdoc += "web_counts.push("; htdoc += String(count); htdoc += ");";
  }
  htdoc += "let counts3D = [ [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ], [ 0, 0, 0, 0, 0, 0, 0, 0 ] ];";
  htdoc += "for (var i = 0; i < 8; i++){ for (var j = 0; j < 8; j++){ counts3D[i][j] = [ web_counts[8*i+j], web_counts[8*i+j+1], web_counts[8*i+j+2], web_counts[8*i+j+3], web_counts[8*i+j+4] ] } }";
  #endif
  htdoc += "</script></body></html>"; 
  
  //char ncolorchar*; sprintf(ncolorchar, "%d", ncolors); htdoc += String(ncolorchar); htdoc += "; i++){ console.log(i); } </script></body></html>";
  //free(r1); free(r2); free(r3); free(r4); free(r5); free(r6); free(r7); free(r8); 
  return htdoc;
}

void initWiFi(){
  #ifndef WEB_ACCESS_POINT
  WiFi.begin(ssid, passwd);
  Serial.println();
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected, IP: ");
  Serial.println(WiFi.localIP());
  #endif

  #ifdef WEB_ACCESS_POINT
  Serial.println(WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0)) ? "Access point configured" : "Access point configuration failed");
  Serial.println(WiFi.softAP(ssid, passwd) ? "Access point ready" : "Access point creation failed" );
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  #endif

  if(!MDNS.begin("coolchess7000")){
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("URI: http://coolchess7000.local/");
  }   
  MDNS.addService("http", "tcp", 80);



  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htdoc());
  });

  server.on("/cam", HTTP_GET, [](AsyncWebServerRequest *request) {
    //flashOccupants++;
    //analogWrite(GPIO_NUM_4, 255);
    //delay(75);
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", jpg_buf, jpg_len);
    response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
    request->send(response);

    //flashOccupants--;
    //if(flashOccupants == 0){
    //  analogWrite(GPIO_NUM_4, 0);
    //  delay(75);
    //}
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/html", "Not found");
  });

  server.begin();
}

#endif