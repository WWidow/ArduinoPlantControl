/********************************************************************************
 * Author:          Kevin Burger                                                *
 * File:            Smartgrow.ino                                               *
 * File-Version:    0.32                                                        *
 ********************************************************************************
 * Info:            Made for Arduino Mega 2560                                  *
 ********************************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <DHT22.h>
#include <DS3231.h>

/********************************************************************************
 * Konstanten/Variablen - Messwerte                                             *
 ********************************************************************************
 * MIN_HUMID: untere Grenze der LF in %                                         *
 * MAX_HUMID: obere Grenze der LF in %                                          *
 * MIN_TEMP: untere Grenze der Temp in °C                                       *
 * MAX_TEMP: obere Grenze der Temp in °C                                        *
 * TOLERANCE: Toleranz bei der Messung                                          *
 ********************************************************************************/
 
  float min_humid       = 60;                                                 
  float max_humid       = 80;
  float min_temp        = 18; 
  float max_temp        = 30;
  const float TOLERANCE = 0.05;

  float limit_min_temp  = (max_temp + min_temp) / 2;
  float limit_max_temp  = max_temp - (max_temp * TOLERANCE);
        
  float limit_min_humid = min_humid;
  float limit_max_humid = max_humid + (max_humid * TOLERANCE);
      
/********************************************************************************
 * Konstanten/Variablen - Ethernet                                              *
 ********************************************************************************
 * mac: MAC-Adresse dieses Arduinos                                             *
 * ip: statische IP-Adresse dieses Arduinos                                     *
 * SERVER: IP-Adresse des Servers                                               *
 * HOST: Domain zum Server                                                      *
 * URL: Serverpfad zur PHP-Datei                                                *
 ********************************************************************************/

  byte mac[]          = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };                    
  byte ip[]           = { 192, 168, 178, 201 };                                    
  const byte SERVER[] = { 192, 168, 178, 20 };                               
  const char HOST[]   = "http://192.168.178.20";                             
  const char URL[]    = "/ARDUINO/SaveTempToMySQL.php";                      

/********************************************************************************
 * Konstanten - Arduino Pins                                                    *
 ********************************************************************************
 * CAUTION: On Uno pin 4, 10, 11, 12 and 13 are used by the ethernet shield.    *
 * On Mega those pins are 4, 10, 50, 51, 52 and 53.                             *
 * CAUTION: On Mega pin 20 (SDA) and 21 (SCL) are used by RTC module.           *
 ********************************************************************************/
 
  const int DHT22_1   = 41;    // Temp/Humid 
	const int RELAIS_8  = 42;    // frei
  const int RELAIS_7  = 43;    // frei  
  const int RELAIS_6  = 44;    // Umwälzpumpe (nicht dran)
  const int RELAIS_5  = 45;    // Wasserpumpe
  const int RELAIS_4  = 46;    // frei 
  const int RELAIS_3  = 47;    // Venti / Fan (nicht dran)
  const int RELAIS_2  = 48;    // Lüftung Stufenumschaltung (Off = 1, On = 2) / Venti
  const int RELAIS_1  = 49;    // Lüftung An/Aus (Off = Aus, On = An)      
	
	const int RELAIS_VENT_ON = RELAIS_1;
	const int RELAIS_VENT_LVL = RELAIS_2; 
	const int RELAIS_FAN = RELAIS_3; 
	const int RELAIS_PUMP = RELAIS_5;
	const int RELAIS_WATERFLOW = RELAIS_6;     
    
/********************************************************************************
 * Variablen                                                                    *
 ********************************************************************************
 * ventiLevel:      aktuelle Stufe des Lüfters                                    *
 * fanLevel:    aktuelle Stufe des Ventilators                                *
 * waterLevel:    aktuelle Stufe der Wasserpumpe                                * 
 * flowLevel:     aktuelle Stufe der Umwälzpumpe                                *
 * switchVentiIn: Zeit zur nächsten Schaltung des Ventilators                   *
 * switchAirIn:   Zeit zur nächsten Schaltung der Lüftung                       *
 * switchWaterIn: Zeit zur nächsten Schaltung der Wasserpumpe                   *
 * switchFlowIn:  Zeit bis zur nächsten Schaltung der Umwälzpumpe               *
 * tempPlantTop:  Temp an der Pflanzenspitze                                    *
 * humidPlantTop: LF an der Pflanzenspitze                                      *
 * tempIn:        Temperatur der Zuluft (ungenutzt)                             *
 * humidIn:       LF der Zuluft (ungenutzt)                                     *  
 * loopNumber:    Number of the loop                                            *
 ********************************************************************************/

  int ventiLevel = 0;          //	Stufe der Lueftung			
  int fanLevel = 0;            // Stufe der Ventilation
  int waterLevel = 0;          // Stufe der Wasserpumpe
  int flowLevel = 0;           // Stufe der Umwaelzpumpe

  int switchVentiIn = 0;       
  int switchFanIn = 0;
  int switchWaterIn = 60;
  int switchFlowIn = 0;
  
  float tempPlantTop = 0.0;
  float humidPlantTop = 0.0;
  float tempPlantTopSave = 0.0;
  float humidPlantTopSave = 0.0;
  float tempIn = 0.0;
  float humidIn = 0.0;
    
  boolean isTempEqual = false;
  boolean isHumidEqual = false;

  int loopNumber = 0;  
   
	 
	boolean printSystem = false;
	boolean printWaterSys = false;
	boolean printAir = true;
    
	boolean printSensor = false;
	boolean printTemp = true;
	boolean printHumid = true;
	boolean printTop = true;
	boolean printIn = false;
	boolean printWaterSens = false;
        
	boolean printSend = false;
   
	 
	boolean isIncrease = false;
	boolean isDecrease = false;
	boolean isTempLow = false;
	boolean isHumidLow = false;
	boolean isTempHigh = false;
	boolean isHumidHigh = false;

    int mySec;
    int myMin;
    int myHour;

	
	EthernetClient client;
	DHT22 dht22One(DHT22_1);
	DS3231 rtc(SDA, SCL);
    Time ActTime;   

/********************************************************************************
 * Setup ************************************************************************
 ********************************************************************************/

void setup() {        
	setupSerial();              
	Serial.println("## Starting setup");
                
	setupRelais();      
	setupEthernet();        
	setupDHT22(); 
	setupRTC();  
        
	Serial.println("## Setup completed");
	Serial.println("## Starting loop...\n");
}
    
/********************************************************************************
 * Loop *************************************************************************
 ********************************************************************************
 * CAUTION:                                                                     *
 * - The DHT22 needs a 2s delay to be read again                                *
 * - DHCP lease should be checked < once a sec                                  *
 ********************************************************************************/

    void loop() {
            
            // repeat loop 10 times per second
            delay(100);         
                
            // check for incoming data
            //readData(); 
                
            // read sensors every 5 seconds -> every 50th loop
            if(loopNumber % 50 == 0) {
                readSensors();
            }
        
            // check DHCP every 60 seconds -> every 600th loop
            if(loopNumber % 600 == 0) {
                //checkDHCP();
            }
                
            // set devices every 5 seconds -> every 50th loop
            if(loopNumber % 50 == 0) {
                setDevices();
            }
                
            // send data to db every 20 seconds -> every 200th loop
            if(loopNumber % 100 == 0) { 
                //sendData();
            }     
            loopNumber += 1;
            loopNumber %= 600;
    }

/********************************************************************************
 * Methoden *********************************************************************
 ********************************************************************************/
 
/****************************************
 * readData()                           *
 ****************************************/
void readData() {
    Serial.print(".");
}

/****************************************
 * readSensors()                        *
 ****************************************/ 
void readSensors() {
	Serial.println("");
    
	// read DHT22
	dht22One.readData();
	tempPlantTop = dht22One.getTemperatureC();
	humidPlantTop = dht22One.getHumidity();
    
	Serial.print("# Sensors read - Time: ");
	printDateTime();
	Serial.print(" - Temperature: ");
	Serial.print(tempPlantTop);
	Serial.print("^C - Humidity: ");
	Serial.print(humidPlantTop);
	Serial.println("%");
} 
 
/****************************************
 * checkDHCP()                          *
 ****************************************/ 
void checkDHCP() {  
    Serial.print("# Checking DHCP...");
    if(client.available()) {
        Serial.print("client available...");
    }   
    if(client.connected()) {
        Serial.print("client connected...");
    }   
    switch(Ethernet.maintain()) {
        case 1:
            Serial.println("renew failed");
            break;          
        case 2:
            Serial.println("renew successful");
            break;          
        case 3:
            Serial.println("rebind failed");
            break;          
        case 4:
            Serial.println("rebind successful");
            break;          
        default:
            Serial.println("no action needed");
            break;         
    }
}
 
/****************************************
 * setDevices                           *
 ****************************************
 * executed every 5 sec                 *
 ****************************************/ 
void setDevices() {
    Serial.println("# Setting devices...");
    
    setVentilation();
    //setFan();
	//setFlow();
    setWater();
}
 
/****************************************
 * sendData                             *
 ****************************************/  
void sendData() {          
    Serial.print("# Trying to connect to server...");    
        
    if(client.connect(SERVER, 80)) {
        Serial.print("connecting succesful, sending data...");
        client.print("GET " + String(HOST) + String(URL));			
				
				// sensor data
        client.print("?tpt=");
        client.print(tempPlantTop);
        client.print("&hmt=");
        client.print(humidPlantTop);				
				
				// device levels
        client.print("&vl=");
        client.print(ventiLevel);				
        client.print("&fl=");
        client.print(fanLevel);	
        client.print("&wl=");
        client.print(waterLevel);	
        client.print("&fll=");
        client.print(flowLevel);
				
				// settings    
        client.print("&mxt=");  
        client.print(max_temp);
        client.print("&mnt=");
        client.print(min_temp);
        client.print("&mxh=");
        client.print(max_humid);
        client.print("&mnh=");
        client.print(min_humid);
        client.print("&to=");
        client.print(TOLERANCE);
				
        client.println(" HTTP/1.1");  
        client.print("Host: " + String(HOST));
        client.println();
        client.println("User-Agent: Arduino");
        client.println("Connection: close");
        client.println();
        client.stop();
        client.flush(); 

        Serial.println("data sent");
    } else {
        Serial.println("can't connect to server.");  
        checkDHCP();
    } 
} 
  
void setFlow() {
    Serial.println("Checking waterflow");   
  if(switchFlowIn == 0) {
        if(flowLevel == 0) {
            Serial.println("Starting waterflow");
            switchRelaisOn(RELAIS_WATERFLOW);
            flowLevel++;
            switchFlowIn = 18;
        } else {           
            Serial.println("Stopping waterflow");
            switchRelaisOff(RELAIS_WATERFLOW);
            flowLevel--;
            switchFlowIn = 1440;
        }
    } else {
        Serial.print("Water flow check has timeout for: ");
        Serial.println(switchFlowIn);  
        switchFlowIn--;        
    }
}

void setWater() {
    Serial.print("  Checking water pump...");
    if(switchWaterIn == 0) {
        if(waterLevel == 0) {   
            ActTime = rtc.getTime();
            myHour = ActTime.hour;    
            if((myHour < 23) &&(myHour > 4)) {    
                Serial.println("starting water pump");
                switchRelaisOn(RELAIS_PUMP);
                waterLevel++;
                switchWaterIn = 60;
            } else {
                Serial.print("timeout time: "); 
                Serial.println(myHour);    
            }
        } else {
            Serial.println("stopping water pump");
            switchRelaisOff(RELAIS_PUMP);
            waterLevel--;    
            switchWaterIn = 2160;
        }  
    } else {      
        Serial.print("water pump check has timeout for: ");
        Serial.println(switchWaterIn);  
        switchWaterIn--;
    }   
}

/***** ***** ***** ***** ***** ***** VENTILATION FUNCTIONS ***** ***** ***** ***** ***** *****/

    void setVentilation() {
        
        Serial.println("  Adjusting ventilation...");
        if(switchVentiIn > 0) {
            Serial.println("  Ventilation recently adjusted, timed out for: " + String(switchVentiIn) + " loops.");
            switchVentiIn--;
            return;    
        }
        
        
        /* --- Humidity --- */    
        Serial.print("  - Checking humidity... ");       
        checkAir(true, false);
        checkAir(true, true);        
        if((!isHumidHigh) && (!isHumidLow)) {
            Serial.println("humidity is optimal at " + String(humidPlantTop) + "% (> " + String(limit_min_humid) + "% && < " + String(limit_max_humid) + "%)");
        }       
        
        /* --- Temperature --- */    
        Serial.print("  - Checking temperature... ");  
        checkAir(false, false);
        checkAir(false, true);        
        if((!isTempHigh) && (!isTempLow)) {
            Serial.println("temperature is optimal at " + String(tempPlantTop) + "^C : (> " + String(limit_min_temp) + "^C && < " + String(limit_max_temp) + "^C)");
        }

        /* --- Priorities --- */ 
        Serial.print("  - Setting priorities... ");

        // only increase
        if(isIncrease && !isDecrease) {
            Serial.println("priorities set: 'increase only'.");            
        // only decrease    
        } else if(!isIncrease && isDecrease) {
            if(isTempHigh || isHumidHigh) {
                Serial.println("priorities set: 'decrease only, aborted'.");
                isDecrease = false;
            } else {                
                Serial.println("priorities set: 'decrease only'.");
            }        
        // increase & decrease
        } else if (isIncrease && isDecrease) {  
            Serial.println("priorities set: 'increase and decrease, decrease aborted'.");
            isDecrease = false;
        } else {            
            Serial.println("priorities set: 'no change'.");
        }

        
        if(isIncrease) {
            increaseAir(); 
            printSystem = true;
        } else if(isDecrease) {
            decreaseAir();
            printSystem = true;
        } else {
            Serial.println("    ...no airflow adjustment needed.");  
        }

        tempPlantTopSave = tempPlantTop;
        humidPlantTopSave = humidPlantTop;   
        isIncrease = false;
        isDecrease = false;
        isTempLow = false;
        isHumidLow = false;
        isTempHigh = false;
        isHumidHigh = false;        
    } 
       
    void checkAir(boolean checkHumid, boolean checkLow) {
        float limit;
        float value;
        float save;
        float limit2;
        boolean isEqual;
        String word1;
        String sign;
        boolean isLimit;
        boolean continuing;
        String word2;
        String word3;
        String word4;
        char compare1;
        char compare2;
        
        if(checkHumid) {
                limit = limit_max_humid;
                value = humidPlantTop;
                save = humidPlantTopSave;
                limit2 = limit_min_humid;
                isEqual = isHumidEqual;
                word1 = "Humidity";
                sign = "%";
        } else { 
                limit = limit_max_temp;
                value = tempPlantTop;
                save = tempPlantTopSave;
                limit2 = limit_min_temp;
                isEqual = isTempEqual;
                word1 = "Temperature";
                sign = "^C";
        }
        
        if(checkLow) {
                limit = limit_min_temp;
                limit2 = limit_max_temp;
                if(checkHumid) {
                    limit = limit_min_humid;
                    limit2 = limit_max_humid;
                }           
                isLimit = (value < limit);
                continuing = (save > value);
                word2 = "low";
                word3 = "de";
                word4 = "in";
                compare1 = '<';
                compare2 = '>';
        } else {
                isLimit = (value >= limit);
                continuing = (save < value);
                word2 = "high";
                word3 = "in";
                word4 = "de";
                compare1 = '>';
                compare2 = '<';
        }
        
        // limit erreicht
        if(isLimit) {                       
            Serial.println(word1 + " is too " + word2 + "! (" + compare1 + limit + sign +")");
            
            // Und steigt/fällt weiter
            if(continuing) {
                Serial.println("  - " + word1 + " is at " + value + sign + " and " + word3 + "creasing! (Old: " + save + sign + ")");
                if(checkLow) {
                    isDecrease = true;
                    if(checkHumid) {
                        isHumidEqual = false;
                        isHumidLow = true;
                    } else {
                        isTempEqual = false;
                        isTempLow = true;
                    }
                } else {
                    isIncrease = true;
                    if(checkHumid) {
                        isHumidEqual = false;
                        isHumidHigh = true;
                    } else {
                        isTempEqual = false;
                        isTempHigh = true;
                    }                   
                }
                        

            // Und bleibt gleich
            } else if(save == value) { 
                // Bereit zum zweiten Mal
                if(isEqual) {
                    Serial.println("  - " + word1 + " stays constant on " + value + sign + ".");
                    if(checkLow) {
                        isDecrease = true;
                        if(checkHumid) {
                            isHumidEqual = false;
                            isHumidLow = true;
                        } else {
                            isTempEqual = false;
                            isTempLow = true;
                        }
                    } else {
                        isIncrease = true;
                        if(checkHumid) {
                            isHumidEqual = false;
                            isHumidHigh = true;
                        } else {
                            isTempEqual = false;
                            isTempHigh = true;
                        }
                    }
                // Erst zum ersten Mal
                } else {
                    Serial.println("  - " + word1 + " seems to be constant on " + value + sign + ", checking next time again.");
                    if(checkLow) {
                        if(checkHumid) {
                            isHumidEqual = true;
                            isHumidLow = true;
                        } else {
                            isTempEqual = true;
                            isTempLow = true;
                        }
                    } else {
                        if(checkHumid) {
                            isHumidEqual = false;
                            isHumidHigh = true;
                        } else {
                            isTempEqual = false;
                            isTempHigh = true;
                        }
                    }
                }
                
            // Aber steigt/fällt noch   
            } else {
                Serial.println("  - " + word1 + " is at " + value + sign + " but still " + word4 + "creasing. (Old: " + save + sign + ")");
                if(checkLow) {
                    if(checkHumid) {
                        isHumidEqual = false;
                        isHumidLow = true;
                    } else {
                        isTempEqual = false;
                        isTempLow = true;
                    }
                } else {
                    if(checkHumid) {
                        isHumidEqual = false;
                        isHumidHigh = true;
                    } else {
                        isTempEqual = false;
                        isTempHigh = true;
                    }
                }
            }
        }
    }

    void increaseAir() {
        Serial.print("    ...increasing ventilation...");
        switch(ventiLevel) {
            case 0:
                Serial.println("ventilation started (Timeout = 3)");
                switchRelaisOn(RELAIS_VENT_ON); // switch from off to on
                ventiLevel += 1;
                switchVentiIn = 5;
                printSystem = true;
                break;
            case 1:
                Serial.println("increased ventilation to level 2 (Timeout = 3)");
                switchRelaisOn(RELAIS_VENT_LVL); // switch from 1 to 2
                ventiLevel += 1;
                switchVentiIn = 4;
                printSystem = true;
                break;
            default:
                Serial.println("can't increase ventilation");
                break;
        }     
    }

    void decreaseAir() {
        Serial.print("    ...decreasing ventilation...");
        switch(ventiLevel) {
            case 1:
                switchRelaisOff(RELAIS_VENT_ON); // switch from on to off
                ventiLevel -= 1;
                Serial.println("stopped ventilation (Timeout = 2)");
                switchVentiIn = 3;
                printSystem = true;
                break;
            case 2:
                switchRelaisOff(RELAIS_VENT_LVL); // switch from 2 to 1
                ventiLevel -= 1;
                Serial.println("decreased ventilation to level 1 (Timeout = 2)");
                switchVentiIn = 3;
                printSystem = true;
                break;
            default:
                Serial.println("can't decrease ventilation");
                break;
        }    
    }

/***** ***** ***** ***** ***** ***** FAN FUNCTIONS ***** ***** ***** ***** ***** *****/

void setFan() {
    if(switchFanIn <= 0) {            
        switch(fanLevel) {
            
            case 0:  
                Serial.println("  Turning fan on.");
                switchRelaisOn(RELAIS_FAN);
                fanLevel += 1;
                switchFanIn = 180;
                break;
                
            case 1:
                Serial.println("  Turning fan off.");
                switchRelaisOff(RELAIS_FAN);
                fanLevel -= 1;
                switchFanIn = 60;
                break;
                
            default:
                break;
        }           
        printSystem = true;
    } else {
        switchVentiIn--;
    }  
}
   
/***** ***** ***** ***** ***** ***** PRINT FUNCTIONS ***** ***** ***** ***** ***** *****/

	void printSensorData() {  
        
        float tmp = 0.00;
        
        Serial.println("");
        Serial.println("# SENSOR-VALUES:");
        if(printTemp && printTop) {
            Serial.print("  Temperature  (top): ");
            Serial.print(tempPlantTop);      
            Serial.println(" C");
        }
        if(printHumid && printTop) {
            Serial.print("  Humidity     (top): ");
            Serial.print(humidPlantTop);  
            Serial.println(" %");
        }
        
        if(printTemp && printIn) {
            Serial.print("  Temperature   (in): "); 
            Serial.print(tempIn);       
            Serial.println(" C");
        }
        if(printHumid && printIn) {
            Serial.print("  Humidity      (in): "); 
            Serial.print(humidIn);  
            Serial.println(" %");
        }
    }
    
	void printSystemData() {  
        
        Serial.println("");
        Serial.println("# SYSTEM-VALUES:");    
        if(printAir) {
            Serial.print("  Fan  (lvl): "); 
            Serial.println(fanLevel);  
            Serial.print("  Ventilation      (lvl): "); 
            Serial.println(ventiLevel);  
        }
        if(printWaterSys) {
            Serial.print("  Water:       (lvl): "); 
            Serial.println(waterLevel); 
            Serial.print("  Waterflow    (lvl): "); 
            Serial.println(flowLevel); 
        }
    }  
    
	void printSendData() {
        
        Serial.println("GET " + String(HOST) + String(URL));
        Serial.print("?tpt=");
        Serial.println(tempPlantTop);
        Serial.print("&hpt=");
        Serial.println(humidPlantTop);
        Serial.print("&vl=");
        Serial.println(ventiLevel);
        Serial.print("&tpb=");
        Serial.println("0");
        Serial.print("&tpm=");
        Serial.println("0");
        Serial.print("&ti=");
        Serial.println(tempIn);
        Serial.print("&mxt=");
        Serial.println(max_temp);
        Serial.print("&mnt=");
        Serial.println(min_temp);
        Serial.print("&mxh=");
        Serial.println(max_humid);
        Serial.print("&mnh=");
        Serial.println(min_humid);
        Serial.print("&tol=");
        Serial.println(TOLERANCE);
        Serial.println("");
    }
    
	void printDateTime() {	
        ActTime = rtc.getTime();
        mySec = ActTime.sec;
        myMin = ActTime.min;
        myHour = ActTime.hour;
        Serial.print("Hour: ");
        Serial.print(myHour);
        Serial.print(" Minute: ");
        Serial.print(myMin);
        Serial.print(" --- ");
		Serial.print(rtc.getDOWStr());
		Serial.print(", ");
		Serial.print(rtc.getDateStr());
		Serial.print(" - ");
		Serial.println(rtc.getTimeStr());
	}   
	
/***** ***** ***** ***** ***** ***** RELAIS FUNCTIONS ***** ***** ***** ***** ***** *****/

  void switchRelaisOn(int relais) {
        digitalWrite(relais, LOW);  
    }

    void switchRelaisOff(int relais) {
        digitalWrite(relais, HIGH);  
    }
 
/***** ***** ***** ***** ***** ***** SETUP FUNCTIONS ***** ***** ***** ***** ***** *****/
 
    void setupSerial() {
        Serial.begin(9600); 
    while(!Serial) {}

    Serial.println("##### Serial port initialized #####\n");
    }
    
    void setupRelais() {
        Serial.println("# Initialising relais...");  
    pinMode(RELAIS_1, OUTPUT);
    switchRelaisOff(RELAIS_1);
    pinMode(RELAIS_2, OUTPUT);
    switchRelaisOff(RELAIS_2);
    pinMode(RELAIS_3, OUTPUT);
    switchRelaisOff(RELAIS_3);
    pinMode(RELAIS_4, OUTPUT);
    switchRelaisOff(RELAIS_4);
    pinMode(RELAIS_5, OUTPUT);
    switchRelaisOff(RELAIS_5);
    pinMode(RELAIS_6, OUTPUT);
    switchRelaisOff(RELAIS_6);
    pinMode(RELAIS_7, OUTPUT);
    switchRelaisOff(RELAIS_7);
    pinMode(RELAIS_8, OUTPUT);
    switchRelaisOff(RELAIS_8);    
    Serial.println("  relais initialized\n");
    }
    
    void setupEthernet() {
        Serial.println("# Setting up ethernet...");
    Serial.print("  Trying to receive IP address from DHCP server... ");
    if(Ethernet.begin(mac) == 0) {
      Serial.println("receiving IP address failed!");
      Serial.print("  Setting up static IP address... ");
      Ethernet.begin(mac, ip); 
      delay(500);
      Serial.println("static IP address configurated."); 
    } else {
      Serial.println("receiving IP address successful.");
    }               
    Serial.print("  IP address: ");
    Serial.println(Ethernet.localIP());  
    Serial.println("  Setting up ethernet finished.\n");
    }
    
    void setupDHT22() {
        Serial.print("# Waiting for DHT-22 to be ready... ");
    delay(1500);
    Serial.println("DTH-22 is ready\n");
    }
		
	void setupRTC() {
		Serial.print("# Starting real time clock");
		rtc.begin();
		printDateTime();
		Serial.println("");
	}
