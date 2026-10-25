#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoJson.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include <functional>
#include "georgia_logo.h"
#include "feed_ca.h"

// Single-USB ESP32-2432S028: ILI9341 display and XPT2046 touch.
constexpr uint16_t RED=0xB865, BG=0x0841, MUTED=0xBDF7, GOLD=0xF66C;
const char* PATHS[]={"football/college-football","basketball/mens-college-basketball","baseball/college-baseball"};
const char* TITLES[]={"FOOTBALL","MEN'S BASKETBALL","BASEBALL"};
const char* IDS[]={"61","61","78"};
TFT_eSPI tft;
TFT_eSprite screen(&tft);
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(33,36);
Preferences prefs;
SemaphoreHandle_t modelMutex;
volatile int selectedSport=0;
volatile bool manualSport=false, forceRefresh=true;
volatile uint32_t revision=0;
int calX1=200,calY1=240,calX2=3700,calY2=3800;
String serialLine;
bool stPanel=false;
bool scheduleView=false;
uint32_t scheduleUntil=0, viewSince=0;

// The family is sold with two controllers. Keep the choice changeable over USB.
void configurePanel(bool st){
  stPanel=st;tft.init();tft.setRotation(1);
  if(st){
    tft.writecommand(0x01);delay(150);tft.writecommand(0x11);delay(120);
    tft.writecommand(0x3A);tft.writedata(0x55);
    tft.writecommand(0xB2);for(uint8_t v:{0x0C,0x0C,0x00,0x33,0x33})tft.writedata(v);
    tft.writecommand(0xB7);tft.writedata(0x35);
    tft.writecommand(0xBB);tft.writedata(0x19);
    tft.writecommand(0xC0);tft.writedata(0x2C);
    tft.writecommand(0xC2);tft.writedata(0x01);
    tft.writecommand(0xC3);tft.writedata(0x12);
    tft.writecommand(0xC4);tft.writedata(0x20);
    tft.writecommand(0xC6);tft.writedata(0x0F);
    tft.writecommand(0xD0);tft.writedata(0xA4);tft.writedata(0xA1);
    tft.writecommand(0x36);tft.writedata(prefs.getBool("bgr",false)?0x68:0x60);
    tft.writecommand(prefs.getBool("invert",false)?0x21:0x20);tft.writecommand(0x13);tft.writecommand(0x29);delay(50);
  }
  tft.fillScreen(BG);
  Serial.printf("PANEL %s %dx%d ID04=%08lx IDD3=%08lx\n",st?"ST7789":"ILI9341",tft.width(),tft.height(),tft.readcommand32(0x04),tft.readcommand32(0xD3));
}

class CooperativeStream:public Stream{
  Stream& input;uint32_t bytes=0;
public:
  explicit CooperativeStream(Stream& s):input(s){setTimeout(20000);}
  int available()override{return input.available();}
  int peek()override{uint32_t start=millis();int v;while((v=input.peek())<0&&millis()-start<15000)delay(1);return v;}
  void flush()override{}
  size_t write(uint8_t)override{return 0;}
  int read()override{
    uint32_t start=millis();int v;
    while((v=input.read())<0&&millis()-start<15000)delay(1);
    if((++bytes&511)==0)delay(1);
    return v;
  }
};

struct Upcoming { String opponent,date,venue,city; bool home=false,neutral=false,tbd=false; };
struct Game {
  String id, date, opponent="OPPONENT", venue, city, state="", status="Loading...", detail;
  String lastPlay="Waiting for game data", score[2]={"-","-"}, bonus[2];
  String homeId, awayId;
  int timeouts[2]={-1,-1}, timeoutSlots=3, possession=-1, balls=-1,strikes=-1,outs=-1;
  bool first=false,second=false,third=false, live=false, loaded=false, error=false;
  uint32_t updated=0;
  Upcoming next[3]; int nextCount=0;
};
Game models[3];

String str(JsonVariantConst v, const char* fallback="") {
  if(v.is<const char*>()) return String(v.as<const char*>());
  if(v.is<int>()) return String(v.as<int>());
  if(v.is<float>()) return String(v.as<float>(),0);
  return String(fallback);
}
String ascii(String s) {
  s.replace("’","'"); s.replace("–","-"); s.replace("—","-"); s.replace("•","|");
  String out; out.reserve(s.length());
  for(size_t i=0;i<s.length();i++) if((uint8_t)s[i]>=32&&(uint8_t)s[i]<127) out+=s[i];
  return out;
}
time_t epoch(String iso) {
  int y,m,d,h=0,min=0;
  if(sscanf(iso.c_str(),"%d-%d-%dT%d:%d",&y,&m,&d,&h,&min)<3) return 0;
  y-=m<=2;int era=(y>=0?y:y-399)/400;unsigned yoe=y-era*400;
  unsigned doy=(153*(m+(m>2?-3:9))+2)/5+d-1;
  unsigned doe=yoe*365+yoe/4-yoe/100+doy;
  return (int64_t(era)*146097+doe-719468)*86400+h*3600+min*60;
}
String localDate(String iso,bool tbd=false) {
  time_t ts=epoch(iso); if(!ts) return "Date TBD";
  struct tm t;localtime_r(&ts,&t);char buf[48];
  strftime(buf,sizeof(buf),tbd?"%a %b %d | TBD":"%a %b %d | %I:%M %p ET",&t);
  return String(buf);
}

bool getJson(const String& url, JsonDocument& output, JsonDocument& filter) {
  WiFiClientSecure tls;tls.setCACert(FEED_CA);tls.setTimeout(15);
  HTTPClient http; http.useHTTP10(true);http.setConnectTimeout(12000);http.setTimeout(15000);
  if(!http.begin(tls,url)) return false;
  http.addHeader("Accept","application/json");http.addHeader("User-Agent","GeorgiaDeskDashboard/1.0");
  http.addHeader("Accept-Encoding","identity");
  int code=http.GET();
  if(code!=200){Serial.printf("FEED_HTTP %d\n",code);http.end();return false;}
  CooperativeStream response(http.getStream());
  auto err=deserializeJson(output,response,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(40));
  http.end();
  if(err){Serial.printf("FEED_JSON %s\n",err.c_str());return false;}
  return true;
}

// Parse a large feed array incrementally; never retain a whole day's games.
bool walkArray(const String& url,const char* key,JsonDocument& filter,std::function<bool(JsonVariantConst)> visit){
  WiFiClientSecure tls;tls.setCACert(FEED_CA);tls.setTimeout(20);
  HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(12000);http.setTimeout(20000);
  if(!http.begin(tls,url))return false;
  http.addHeader("Accept-Encoding","identity");http.addHeader("User-Agent","GeorgiaDeskDashboard/1.1");
  int code=http.GET();if(code!=200){Serial.printf("ARRAY_HTTP %d\n",code);http.end();return false;}
  CooperativeStream stream(http.getStream());String token=String("\"")+key+"\"";
  if(!stream.find(token.c_str())||!stream.find("[")){http.end();return false;}
  bool ok=true;JsonDocument item;
  for(;;){
    int ch=stream.peek();while(ch==' '||ch=='\r'||ch=='\n'||ch=='\t'||ch==','){stream.read();ch=stream.peek();}
    if(ch==']')break;
    if(ch!='{'){ok=false;break;}
    auto err=deserializeJson(item,stream,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(40));
    if(err){Serial.printf("ARRAY_JSON %s\n",err.c_str());ok=false;break;}
    if(!visit(item.as<JsonVariantConst>()))break;
    item.clear();delay(1);
  }
  http.end();return ok;
}

void eventFilter(JsonObject v) {
  v["id"]=true;v["date"]=true;v["status"]=true;
  JsonObject c=v["competitions"][0].to<JsonObject>();c["id"]=true;c["date"]=true;c["dateValid"]=true;
  c["status"]=true;c["venue"]=true;c["neutralSite"]=true;c["situation"]=true;
  JsonObject t=c["competitors"][0].to<JsonObject>();
  t["id"]=true;t["homeAway"]=true;t["score"]=true;t["possession"]=true;
  t["timeoutsRemaining"]=true;t["bonus"]=true;t["doubleBonus"]=true;
  t["team"]["id"]=true;t["team"]["shortDisplayName"]=true;
  t["team"]["displayName"]=true;t["team"]["abbreviation"]=true;
}
void location(JsonVariantConst venue, String& name,String& city) {
  name=ascii(str(venue["fullName"]));
  city=ascii(str(venue["address"]["city"]));
  String region=str(venue["address"]["state"]);
  if(region.length())city+=(city.length()?", ":"")+region;
}
int teamIndex(String id,int sport) {return id==IDS[sport]?0:1;}
void readEvent(JsonVariantConst e,Game& g,int sport) {
  auto c=e["competitions"][0];
  g.id=str(e["id"]);g.date=str(e["date"]);
  auto status=e["status"].isNull()?c["status"]:e["status"];
  g.state=str(status["type"]["state"]);g.live=g.state=="in";
  g.status=ascii(str(status["type"]["shortDetail"],"Scheduled"));
  if(g.state=="pre")g.status=localDate(g.date,c["dateValid"]==false);
  location(c["venue"],g.venue,g.city);
  g.possession=-1;g.timeouts[0]=g.timeouts[1]=-1;g.bonus[0]=g.bonus[1]="";
  g.balls=g.strikes=g.outs=-1;g.first=g.second=g.third=false;
  g.timeoutSlots=sport==0?3:4;
  for(auto t:c["competitors"].as<JsonArrayConst>()) {
    String id=str(t["id"]);if(!id.length())id=str(t["team"]["id"]);
    int n=teamIndex(id,sport);
    if(n==1)g.opponent=ascii(str(t["team"]["shortDisplayName"],str(t["team"]["displayName"],"Opponent").c_str()));
    if(str(t["homeAway"])=="home")g.homeId=id;else g.awayId=id;
    g.score[n]=str(t["score"]["displayValue"]);
    if(!g.score[n].length())g.score[n]=str(t["score"],"-");
    if(g.state=="pre")g.score[n]="-";
    if(g.live&&t["possession"]==true)g.possession=n;
    if(g.live&&t["timeoutsRemaining"].is<int>())g.timeouts[n]=t["timeoutsRemaining"];
    if(g.live&&t["doubleBonus"]==true)g.bonus[n]="DOUBLE BONUS";
    else if(g.live&&t["bonus"]==true)g.bonus[n]="BONUS";
  }
  auto sit=c["situation"];
  g.detail=ascii(str(sit["downDistanceText"]));
  if(g.live&&!sit.isNull()) {
    String poss=str(sit["possession"]);
    if(poss.length())g.possession=teamIndex(poss,sport);
    if(sport==2){
      if(sit["balls"].is<int>())g.balls=sit["balls"];
      if(sit["strikes"].is<int>())g.strikes=sit["strikes"];
      if(sit["outs"].is<int>())g.outs=sit["outs"];
      g.first=sit["onFirst"]|false;g.second=sit["onSecond"]|false;g.third=sit["onThird"]|false;
      String prefix=str(status["periodPrefix"]);String low=g.status;low.toLowerCase();
      if(g.possession<0&&(prefix=="Top"||low.indexOf("top")>=0))g.possession=teamIndex(g.awayId,sport);
      if(g.possession<0&&(prefix=="Bottom"||low.indexOf("bot")>=0))g.possession=teamIndex(g.homeId,sport);
      g.detail=g.first||g.second||g.third?String(g.first?"1B ":"")+String(g.second?"2B ":"")+String(g.third?"3B ":"")+"occupied":"Bases empty";
    }
  }
  g.loaded=true;
}

bool fetchSchedule(int sport,Game& result) {
  JsonDocument filter;eventFilter(filter.to<JsonObject>());
  String url=String("https://site.api.espn.com/apis/site/v2/sports/")+PATHS[sport]+"/teams/"+IDS[sport]+"/schedule";
  time_t now=time(nullptr),lastTime=0,soon=LONG_MAX;Game live,last,upcoming;bool haveLive=false,haveLast=false,haveNext=false;int count=0;
  std::vector<Upcoming> next;
  auto visitor=[&](JsonVariantConst e){
    count++;
    auto c=e["competitions"][0];auto st=e["status"].isNull()?c["status"]:e["status"];
    String state=str(st["type"]["state"]);time_t ts=epoch(str(e["date"]));
    if(state=="in"){readEvent(e,live,sport);haveLive=true;}
    if(state=="post"&&ts>lastTime){lastTime=ts;readEvent(e,last,sport);haveLast=true;}
    if(state=="pre"&&ts>now-86400){
      Game temp;readEvent(e,temp,sport);if(ts<soon){soon=ts;upcoming=temp;haveNext=true;}
      Upcoming u;u.date=temp.date;u.opponent=temp.opponent;u.venue=temp.venue;u.city=temp.city;
      u.tbd=c["dateValid"]==false;u.neutral=c["neutralSite"]|false;
      u.home=temp.homeId==IDS[sport];next.push_back(u);
      std::sort(next.begin(),next.end(),[](const Upcoming& a,const Upcoming& b){return a.date<b.date;});
      if(next.size()>3)next.pop_back();
    }
    return true;
  };
  if(!walkArray(url,"events",filter,visitor))return false;
  if(!count){struct tm t;localtime_r(&now,&t);String season=String(t.tm_year+1900+(sport==1&&t.tm_mon>=6?1:0));if(!walkArray(url+"?season="+season,"events",filter,visitor))return false;}
  String previousId=result.id,previousPlay=result.lastPlay;
  if(haveLive)result=live;else if(haveLast&&now-lastTime<18*3600)result=last;else if(haveNext)result=upcoming;else if(haveLast)result=last;
  else {result=Game();result.loaded=true;result.status="Schedule unavailable";result.lastPlay="No published games found";}
  if(result.id==previousId&&previousId.length())result.lastPlay=previousPlay;
  result.nextCount=next.size();for(int i=0;i<result.nextCount;i++)result.next[i]=next[i];
  return true;
}
String latestPlay(JsonVariantConst plays) {
  String found;
  for(auto p:plays.as<JsonArrayConst>()){
    String txt=ascii(str(p["text"]));String low=txt;low.toLowerCase();
    if(txt.length()&&low.indexOf("end of")==-1&&low!="end game")found=txt;
  }
  return found;
}
void summaryFilter(JsonDocument& f) {
  f["gameInfo"]["venue"]=true;
  JsonObject p=f["plays"][0].to<JsonObject>();p["text"]=true;p["type"]=true;p["team"]["id"]=true;
  p["participants"][0]["athlete"]["displayName"]=true;
  f["drives"]["current"]["plays"][0]["text"]=true;
  f["drives"]["previous"][0]["plays"][0]["text"]=true;
  f["atBats"][0]["text"]=true;
}
String timeoutLabel(String text,String teamId,Game& g,int sport){
  String low=text;low.toLowerCase();
  if(low.indexOf("timeout")<0&&low.indexOf("time out")<0)return text;
  if(low.indexOf("media")>=0||low.indexOf("official")>=0||low.indexOf("tv timeout")>=0)return "TIMEOUT - OFFICIAL";
  if(teamId.length())return "TIMEOUT - "+(teamIndex(teamId,sport)==0?String("GEORGIA"):g.opponent);
  String opp=g.opponent;opp.toLowerCase();
  if(low.indexOf("georgia")>=0||low.indexOf("uga")>=0)return "TIMEOUT - GEORGIA";
  if(opp.length()&&low.indexOf(opp)>=0)return "TIMEOUT - "+g.opponent;
  return "TIMEOUT";
}
bool fetchSummary(int sport,Game& g) {
  if(!g.id.length()||g.state=="pre"){g.lastPlay=g.state=="pre"?"Game has not started":"Play-by-play unavailable";return true;}
  JsonDocument f,doc;summaryFilter(f);
  String url=String("https://site.api.espn.com/apis/site/v2/sports/")+PATHS[sport]+"/summary?event="+g.id;
  if(sport==1){
    JsonDocument pf;pf["text"]=true;pf["team"]["id"]=true;
    String latest,team;
    bool ok=walkArray(url,"plays",pf,[&](JsonVariantConst p){String s=ascii(str(p["text"]));String low=s;low.toLowerCase();if(s.length()&&low.indexOf("end of")==-1){latest=s;team=str(p["team"]["id"]);}return true;});
    if(ok)g.lastPlay=latest.length()?timeoutLabel(latest,team,g,sport):"Play-by-play unavailable";
    return ok;
  }
  if(!getJson(url,doc,f))return false;
  if(!doc["gameInfo"]["venue"].isNull())location(doc["gameInfo"]["venue"],g.venue,g.city);
  String text=latestPlay(doc["plays"]),team;
  JsonVariantConst lastRelevant;
  for(auto p:doc["plays"].as<JsonArrayConst>())if(ascii(str(p["text"]))==text)lastRelevant=p;
  if(!lastRelevant.isNull())team=str(lastRelevant["team"]["id"]);
  if(!text.length())for(auto drive:doc["drives"]["previous"].as<JsonArrayConst>()){String s=latestPlay(drive["plays"]);if(s.length())text=s;}
  String current=latestPlay(doc["drives"]["current"]["plays"]);if(current.length())text=current;
  if(!text.length())text=latestPlay(doc["atBats"]);
  g.lastPlay=text.length()?timeoutLabel(text,team,g,sport):"Play-by-play unavailable";
  return true;
}
bool scoreboard(int sport,Game& g,bool& foundLive) {
  JsonDocument f;eventFilter(f.to<JsonObject>());
  time_t now=time(nullptr);struct tm t;localtime_r(&now,&t);char date[10];strftime(date,sizeof(date),"%Y%m%d",&t);
  String url=String("https://site.api.espn.com/apis/site/v2/sports/")+PATHS[sport]+"/scoreboard?limit=1000&dates="+date+(sport==0?"&groups=80":sport==1?"&groups=50":"");
  foundLive=false;
  return walkArray(url,"events",f,[&](JsonVariantConst e){
    bool uga=false;for(auto c:e["competitions"][0]["competitors"].as<JsonArrayConst>())if(str(c["id"])==IDS[sport])uga=true;
    if(!uga)return true;
    String state=str(e["status"]["type"]["state"]);
    if(state=="in"||str(e["id"])==g.id||state=="post"){
      readEvent(e,g,sport);foundLive=g.live;
      if(!e["competitions"][0]["situation"]["lastPlay"]["text"].isNull())
        g.lastPlay=timeoutLabel(ascii(str(e["competitions"][0]["situation"]["lastPlay"]["text"])),str(e["competitions"][0]["situation"]["lastPlay"]["team"]["id"]),g,sport);
      if(foundLive)return false;
    }
    return true;
  });
}
Game snapshot(int sport){xSemaphoreTake(modelMutex,portMAX_DELAY);Game g=models[sport];xSemaphoreGive(modelMutex);return g;}
void publish(int sport,const Game& g){xSemaphoreTake(modelMutex,portMAX_DELAY);models[sport]=g;revision++;xSemaphoreGive(modelMutex);}
void feedTask(void*){
  uint32_t scheduleAt[3]={},boardAt[3]={};int lastSelected=-1;
  for(;;){
    if(WiFi.status()!=WL_CONNECTED){vTaskDelay(pdMS_TO_TICKS(1000));continue;}
    if(time(nullptr)<1700000000){vTaskDelay(pdMS_TO_TICKS(1000));continue;}
    int active=selectedSport;bool force=forceRefresh||lastSelected!=active;forceRefresh=false;lastSelected=active;
    int livePriority=-1;
    for(int s=0;s<3;s++){
      Game g=snapshot(s);uint32_t now=millis();bool ok=true;
      if(!g.loaded||now-scheduleAt[s]>900000){ok=fetchSchedule(s,g);scheduleAt[s]=now;}
      if(now-boardAt[s]>(s==active?30000:90000)||boardAt[s]==0||(force&&s==active)){
        bool isLive=false;bool b=scoreboard(s,g,isLive);ok=ok&&b;boardAt[s]=millis();
        if(s==active){String existing=g.lastPlay;if(!fetchSummary(s,g)){ok=false;g.lastPlay=existing;}}
        if(ok){g.updated=millis();g.error=false;}else g.error=true;
        publish(s,g);
        Serial.printf("SPORT %d state=%s score=%s-%s feed=%s heap=%u\n",s,g.state.c_str(),g.score[0].c_str(),g.score[1].c_str(),ok?"OK":"ERROR",ESP.getFreeHeap());
      }else if(!ok){g.error=true;publish(s,g);}
      if(g.live&&livePriority<0)livePriority=s;
      vTaskDelay(pdMS_TO_TICKS(150));
    }
    if(!manualSport&&livePriority>=0&&selectedSport!=livePriority){selectedSport=livePriority;forceRefresh=true;}
    vTaskDelay(pdMS_TO_TICKS(800));
  }
}

void text(String s,int x,int y,int font=2,uint16_t color=TFT_WHITE){screen.setTextColor(color,BG);screen.drawString(ascii(s),x,y,font);}
void centered(String s,int y,int font=2,uint16_t color=TFT_WHITE){screen.setTextColor(color,BG);screen.drawCentreString(ascii(s),160,y,font);}
void moving(String s,int x,int y,int width,int font,uint16_t color,uint32_t since){
  s=ascii(s);screen.setTextColor(color,BG);int tw=screen.textWidth(s,font);
  screen.setViewport(x,y,width,screen.fontHeight(font));
  uint32_t age=millis()-since;int offset=tw>width&&age>1800?((age-1800)/45)%(tw+width):0;
  screen.drawString(s,-offset,0,font);if(offset>0)screen.drawString(s,tw+width-offset,0,font);
  screen.resetViewport();
}
void ballIcon(int sport,int x,int y){
  if(sport==0){screen.fillEllipse(x,y,10,6,0xA285);screen.drawLine(x-5,y,x+5,y,TFT_WHITE);for(int a=-3;a<=3;a+=3)screen.drawLine(x+a,y-2,x+a,y+2,TFT_WHITE);}
  else if(sport==1){screen.fillCircle(x,y,8,0xFBE0);screen.drawCircle(x,y,8,BG);screen.drawLine(x-8,y,x+8,y,BG);screen.drawLine(x,y-8,x,y+8,BG);screen.drawEllipse(x,y,4,8,BG);}
  else{screen.fillCircle(x,y,8,TFT_WHITE);screen.drawLine(x-5,y-5,x-3,y+5,RED);screen.drawLine(x+5,y-5,x+3,y+5,RED);}
}
void drawTeam(const Game& g,int team,int y,int sport,bool fresh){
  String name=team==0?"GEORGIA":g.opponent;name.toUpperCase();
  if(name.length()>16)name=name.substring(0,16);
  text(name,36,y,2,team==0?TFT_WHITE:MUTED);
  if(fresh&&g.live&&g.possession==team)ballIcon(sport,18,y+10);
  if(fresh&&g.timeouts[team]>=0&&sport<2){int tx=std::min(234,42+screen.textWidth(name,2));
    for(int i=0;i<g.timeoutSlots;i++)screen.fillCircle(tx+i*8,y+8,2,i<g.timeouts[team]?TFT_WHITE:0x4208);}
  if(sport==1&&fresh&&g.bonus[team].length())text(g.bonus[team],36,y+20,1,GOLD);
  screen.setTextColor(TFT_WHITE,BG);screen.drawRightString(g.score[team],307,y-4,4);
}
void countRow(const Game& g,int y,bool fresh){
  String b=fresh&&g.balls>=0?String(g.balls):"-";
  String s=fresh&&g.strikes>=0?String(g.strikes):"-";
  String o=fresh&&g.outs>=0?String(g.outs):"-";
  text("Balls "+b,36,y,1,MUTED);text("Strikes "+s,112,y,1,MUTED);text("Outs "+o,203,y,1,MUTED);
}
void draw(){
  int sport=selectedSport;Game g=snapshot(sport);screen.fillSprite(BG);
  screen.fillRect(0,0,320,44,RED);screen.setSwapBytes(true);screen.pushImage(7,6,46,32,GEORGIA_LOGO);
  screen.setTextColor(TFT_WHITE,RED);screen.drawString("GEORGIA BULLDOGS",61,5,2);screen.drawString(String(TITLES[sport])+" >",61,27,1);
  bool fresh=g.updated&&millis()-g.updated<120000&&WiFi.status()==WL_CONNECTED&&!g.error;
  screen.drawRightString(scheduleView?"NEXT":g.live&&fresh?"LIVE":g.state=="post"?"FINAL":"",312,15,1);
  if(scheduleView){
    text("NEXT 3 GAMES",10,49,2);text(String(std::max(1,(int)((scheduleUntil-millis()+999)/1000)))+"s",291,50,2,MUTED);
    for(int i=0;i<g.nextCount;i++){
      auto u=g.next[i];int y=74+i*48;
      moving(String(u.neutral?"vs ":u.home?"vs ":"at ")+u.opponent,10,y,300,2,TFT_WHITE,viewSince);
      text(localDate(u.date,u.tbd),10,y+17,1,MUTED);
      moving(u.venue+(u.city.length()?" | "+u.city:""),10,y+29,300,1,MUTED,viewSince);
    }
    if(!g.nextCount)centered(g.loaded?"No published upcoming games":"Loading schedule...",105,2);
  }else{
    moving(g.status,10,49,181,2,MUTED,viewSince);
    moving(g.detail,194,50,116,1,MUTED,viewSince);
    int y0=76,y1=114;
    if(sport==2&&g.possession==0)y1=127;
    drawTeam(g,0,y0,sport,fresh);
    if(sport==2&&g.possession==0)countRow(g,y0+24,fresh);
    drawTeam(g,1,y1,sport,fresh);
    if(sport==2&&g.possession==1)countRow(g,y1+24,fresh);
    if(sport==2&&g.possession<0&&g.live)countRow(g,148,false);
    String venue=g.venue;if(g.city.length())venue+=(venue.length()?" | ":"")+g.city;
    moving(venue.length()?venue:"Location not published",10,165,300,1,MUTED,viewSince);
    screen.drawFastHLine(10,179,300,0x4208);text("LAST PLAY",10,184,1,0xF34C);
    moving(g.lastPlay,10,198,300,2,TFT_WHITE,viewSince);
  }
  screen.fillRect(0,225,320,15,0x2104);screen.setTextColor(MUTED,0x2104);
  String footer;
  if(WiFi.status()!=WL_CONNECTED)footer="Wi-Fi reconnecting...";
  else if(!g.loaded)footer="Wi-Fi connected | Loading scores...";
  else if(!g.updated||g.error)footer="Feed unavailable | Retrying...";
  else if(!fresh)footer="STALE | Last update "+String((millis()-g.updated)/1000)+"s ago";
  else footer="Wi-Fi | Updated "+String((millis()-g.updated)/1000)+"s ago";
  screen.drawString(footer,7,229,1);screen.drawRightString("ET",312,229,1);
  screen.pushSprite(0,0);
}

void splash(String title,String line){screen.fillSprite(BG);centered(title,57,2);centered(line,92,2,MUTED);screen.pushSprite(0,0);}
bool processSerial(){
  while(Serial.available()){
    char c=Serial.read();if(c=='\r')continue;
    if(c=='\n'){
      if(serialLine.startsWith("WIFI ")){
        JsonDocument d;auto err=deserializeJson(d,serialLine.substring(5));
        if(!err&&d["ssid"].is<const char*>()&&d["password"].is<const char*>()){
          prefs.putString("ssid",d["ssid"].as<String>());prefs.putString("pass",d["password"].as<String>());
          serialLine="";Serial.println("WIFI_SAVED");return true;
        }
      }else if(serialLine=="STATUS")Serial.printf("STATUS wifi=%d ip=%s heap=%u sport=%d panel=%s\n",WiFi.status(),WiFi.localIP().toString().c_str(),ESP.getFreeHeap(),selectedSport,stPanel?"ST7789":"ILI9341");
      else if(serialLine=="PANEL ST7789"||serialLine=="PANEL ILI9341"){bool st=serialLine=="PANEL ST7789";prefs.putBool("st-panel",st);configurePanel(st);}
      else if(serialLine.startsWith("INVERT ")){prefs.putBool("invert",serialLine.endsWith("1"));configurePanel(stPanel);}
      else if(serialLine.startsWith("BGR ")){prefs.putBool("bgr",serialLine.endsWith("1"));configurePanel(stPanel);}
      else if(serialLine=="CALIBRATE")prefs.putBool("cal-next",true);
      serialLine="";
    }else if(serialLine.length()<256)serialLine+=c;else serialLine="";
  }return false;
}
void calibrate(){
  int values[4];int xs[]={20,300},ys[]={20,220};
  for(int i=0;i<2;i++){
    screen.fillSprite(BG);centered("TOUCH SETUP",86,2);centered("Press the cross firmly",112,2,MUTED);
    screen.drawFastHLine(xs[i]-10,ys[i],21,TFT_WHITE);screen.drawFastVLine(xs[i],ys[i]-10,21,TFT_WHITE);screen.pushSprite(0,0);
    while(touch.touched())delay(10);delay(150);
    int64_t sx=0,sy=0;int n=0;while(n<12){processSerial();if(touch.touched()){auto p=touch.getPoint();if(p.z>200){sx+=p.x;sy+=p.y;n++;}}delay(15);}
    values[2*i]=sx/n;values[2*i+1]=sy/n;
  }
  if(abs(values[2]-values[0])>500&&abs(values[3]-values[1])>500){
    calX1=values[0]-(values[2]-values[0])*20/280;calX2=values[2]+(values[2]-values[0])*19/280;
    calY1=values[1]-(values[3]-values[1])*20/200;calY2=values[3]+(values[3]-values[1])*19/200;
    prefs.putInt("x1",calX1);prefs.putInt("x2",calX2);prefs.putInt("y1",calY1);prefs.putInt("y2",calY2);
  }
  prefs.putBool("cal-next",false);Serial.println("TOUCH_CALIBRATED");
}
void setup(){
  Serial.begin(115200);pinMode(0,INPUT_PULLUP);pinMode(4,OUTPUT);pinMode(16,OUTPUT);pinMode(17,OUTPUT);
  digitalWrite(4,HIGH);digitalWrite(16,HIGH);digitalWrite(17,HIGH);
  prefs.begin("dawgs",false);
  configurePanel(prefs.getBool("st-panel",true));pinMode(21,OUTPUT);digitalWrite(21,HIGH);
  screen.setColorDepth(8);if(!screen.createSprite(320,240)){Serial.println("DISPLAY_MEMORY_ERROR");for(;;)delay(1000);}
  touchSPI.begin(25,39,32,33);touch.begin(touchSPI);touch.setRotation(1);
  modelMutex=xSemaphoreCreateMutex();
  calX1=prefs.getInt("x1",200);calX2=prefs.getInt("x2",3700);calY1=prefs.getInt("y1",240);calY2=prefs.getInt("y2",3800);
  splash("GEORGIA BULLDOGS","Configuring Wi-Fi...");Serial.println("READY_CONFIG");
  uint32_t start=millis();uint32_t wait=prefs.getString("ssid").length()?2500:45000;
  while(millis()-start<wait){if(processSerial())break;delay(10);}
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.setAutoReconnect(true);
  String ssid=prefs.getString("ssid"),pass=prefs.getString("pass");
  if(ssid.length())WiFi.begin(ssid.c_str(),pass.c_str());else WiFi.begin();
  splash("GEORGIA BULLDOGS","Connecting to Wi-Fi...");start=millis();
  while(WiFi.status()!=WL_CONNECTED&&millis()-start<25000){if(processSerial()){ssid=prefs.getString("ssid");pass=prefs.getString("pass");WiFi.begin(ssid.c_str(),pass.c_str());}delay(50);}
  if(WiFi.status()!=WL_CONNECTED){
    splash("Join Wi-Fi: Dawgs-Setup","Password: GoDawgs!");
    Serial.println("WIFI_PORTAL Dawgs-Setup http://192.168.4.1");
    WiFiManager wm;wm.setDebugOutput(false);wm.setConfigPortalTimeout(180);wm.setBreakAfterConfig(true);
    wm.startConfigPortal("Dawgs-Setup","GoDawgs!");
  }
  Serial.printf("WIFI_CONNECTED %d IP %s\n",WiFi.status()==WL_CONNECTED,WiFi.localIP().toString().c_str());
  configTzTime("EST5EDT,M3.2.0/2,M11.1.0/2","pool.ntp.org","time.nist.gov");
  if(prefs.getBool("cal-next",false)||digitalRead(0)==LOW)calibrate();
  viewSince=millis();draw();xTaskCreatePinnedToCore(feedTask,"scores",24576,nullptr,1,nullptr,0);
  Serial.println("DASHBOARD_READY");
}
void loop(){
  static bool pressed=false;static uint32_t lastDraw=0,bootStart=0;static String previousPlay;static int previousSport=-1;
  if(processSerial()){WiFi.disconnect();String s=prefs.getString("ssid"),p=prefs.getString("pass");WiFi.begin(s.c_str(),p.c_str());}
  if(digitalRead(0)==LOW){if(!bootStart)bootStart=millis();if(millis()-bootStart>2500){calibrate();bootStart=0;pressed=true;}}else bootStart=0;
  bool nowPressed=touch.touched();
  if(nowPressed&&!pressed){auto p=touch.getPoint();if(p.z>200){int x=map(p.x,calX1,calX2,0,319),y=map(p.y,calY1,calY2,0,239);
    if(y>=0&&y<46){
      if(x>=0&&x<59){scheduleView=!scheduleView;scheduleUntil=millis()+7000;viewSince=millis();}
      else if(x>=59&&x<275){selectedSport=(selectedSport+1)%3;manualSport=true;forceRefresh=true;scheduleView=false;viewSince=millis();Serial.printf("SELECT_SPORT %d\n",selectedSport);}
    }
    pressed=true;
  }}
  if(!nowPressed)pressed=false;
  if(scheduleView&&(int32_t)(millis()-scheduleUntil)>=0){scheduleView=false;viewSince=millis();}
  if(millis()-lastDraw>180){Game g=snapshot(selectedSport);if(previousSport!=selectedSport||previousPlay!=g.lastPlay){viewSince=millis();previousSport=selectedSport;previousPlay=g.lastPlay;}draw();lastDraw=millis();}
  delay(10);
}
