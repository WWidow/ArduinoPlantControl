/****************************************************
 * Author:          Kevin Burger                    *
 * File:            Smartgrow.ino                   *
 * File-Version:    0.31                            *
 ****************************************************
 * Info:            Made for Arduino Mega 2560      *
 ****************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <DHT22.h>

/*************************************************
 *       Konstanten/Variablen - Messwerte        *
 *************************************************
 * MIN_HUMID: untere Grenze der LF in %          *
 * MAX_HUMID: obere Grenze der LF in %           *
 * MIN_TEMP: untere Grenze der Temp in °C        *
 * MAX_TEMP: obere Grenze der Temp in °C         *
 * TOLERANCE: Toleranz bei der Messung           *
 *************************************************/
 
    float min_humid       = 45;                                                 
    float max_humid       = 75;
    float min_temp        = 18; 
    float max_temp        = 30;
    const float TOLERANCE = 0.05;

    float limit_max_temp  = max_temp - (max_temp * TOLERANCE);
    float limit_max_humid = max_humid + (max_humid * TOLERANCE);
      
    float limit_min_temp  = (max_temp + min_temp) / 2;
    float limit_min_humid = min_humid;
    //float limit_min_humid = ((max_humid + min_humid) / 2) - (max_humid * TOLERANCE);

/*************************************************
 *       Konstanten/Variablen - Ethernet         *
 *************************************************
 * mac: MAC-Adresse dieses Arduinos              *
 * ip: statische IP-Adresse dieses Arduinos      *
 * SERVER: IP-Adresse des Servers                *
 * HOST: Domain zum Server                       *
 * URL: Serverpfad zur PHP-Datei                 *
 *************************************************/

    byte mac[]          = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };                    
    byte ip[]           = { 192, 168, 178, 201 };                                    
    const byte SERVER[] = { 192, 168, 178, 20 };                               
    const char HOST[]   = "http://192.168.178.20";                             
    const char URL[]    = "/ARDUINO/SaveTempToMySQL.php";                      

/*************************************************
 *          Konstanten - Arduino Pins            *
 *************************************************
 * CAUTION: On Uno pin 4, 10, 11, 12 and 13 are  *
 * used by the ethernet shield. On Mega those    *
 * pins are 4, 10, 50, 51, 52 and 53.            *
 *************************************************/
 
    const int DHT22_1   = 41;    // Temp/Humid TopPlant   

    const int RELAIS_1  = 42;    // defekt
    const int RELAIS_2  = 43;    // Lüftung An/Aus (Off = Aus, On = An)
    const int RELAIS_3  = 44;    // Lüftung Stufenumschaltung (Off = 1, On = 2)  
    const int RELAIS_4  = 45;    // Wasserpumpe

    const int RELAIS_5  = 46;    // Umwälzpumpe   
    const int RELAIS_6  = 47;    // Venti 
    const int RELAIS_7  = 48;    // frei
    const int RELAIS_8  = 49;    // frei

    const int GND_HUM_1 = A15;   // Humid Erde
    const int GND_HUM_2 = A14;
    const int GND_HUM_3 = A13;
    const int GND_HUM_4 = A12;

/*************************************************
 *                   Variablen                   *
 *************************************************
 * airLevel: aktuelle Stufe des Lüfters          *
 * ventiLevel: aktuelle Stufe des Ventilators    *
 * waterLevel: aktuelle Stufe der Wasserpumpe    * 
 * flowLevel: aktuelle Stufe der Umwälzpumpe     *
 *                                               *
 * switchVentiIn: Zeit zur nächsten Schaltung    *                                          
 *                des Ventilators                *
 * switchAirIn: Zeit zur nächsten Schaltung der  *
 *              Lüftung                          *
 * switchWaterIn: Zeit zur nächsten Schaltung    *
 *                der Wasserpumpe                *
 * switchFlowIn: Zeit bis zur nächsten Schaltung *
 *             der Umwälzpumpe                   *
 *                                               *
 * tempPlantTop: Temp an der Pflanzenspitze      *
 * humidPlantTop: LF an der Pflanzenspitze       *
 * tempIn: Temperatur der Zuluft                 *
 * humidIn: LF der Zuluft                        *     
 *                                               *
 * loopNumber: Number of the loop                *
 *************************************************/

    int airLevel = 0;
    int ventiLevel = 0;
    int waterLevel = 0;
    int flowLevel = 0;

    int switchVentiIn = 0;
    int switchAirIn = 0;
    int switchWaterIn = 0;
    int switchFlowIn = 0;
  
    float tempPlantTop = 0.0;
    float humidPlantTop = 0.0;
    float tempPlantTopSave = 0.0;
    float humidPlantTopSave = 0.0;
    float tempIn = 0.0;
    float humidIn = 0.0;
    boolean isTempEqual = false;
    boolean isHumidEqual = false;
  
    int gndHumid_1 = 0;
    int gndHumid_2 = 0;
    int gndHumid_3 = 0;
    int gndHumid_4 = 0;

    int loopNumber = 0;  
    int sensorLoop = 0;
    
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

    EthernetClient client;
    DHT22 dht22One(DHT22_1);

/*************************************************
 *                      Setup                    *
 *************************************************/

    void setup() {
    
        // init console
        Serial.begin(9600); 
        while(!Serial) {}

        Serial.println("##### Serial port initialized #####\n");
        Serial.println("### Starting setup...");
        Serial.print("# Initialising relais...");
    
        // init relaispins
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
    
        Serial.println(" ...relais initialized.\n");
  
        // set up ethernet
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
        Serial.println("# Setting up ethernet finished.\n");

        // delay for dht-22
        Serial.print("# Waiting for DHT-22 to be ready... ");
        delay(1500);
        Serial.println("DTH-22 is ready.\n");
     
        Serial.println("### Setup completed.");
        Serial.println("### Starting loop...\n");
    }

/*************************************************
 *                      Loop                     *
 *************************************************
 * CAUTION:                                      *
 * - The DHT22 needs a 2s delay to be read again *
 * - DHCP lease should be checked < once a sec   *
 *************************************************/

    void loop() {
        // repeat loop every 0.5 seconds
        delay(500);

        // read sensors every 5 seconds
        if(loopNumber % 10 == 0) {
            readSensors();
        }
        
        // check DHCP every 60 seconds
        if(loopNumber % 100 == 0) {
            checkDHCP();
        }

        // set ventilation every 50 seconds
        if(loopNumber % 100 == 0) {
            setVentilation();  
        }
 
        // set air every 50 seconds
        if(loopNumber % 100 == 0) { 
            setAir();
        }   

        // send data to db every 50 seconds
        if(loopNumber % 100 == 0) { 
            sendData();
        }  
				
        loopNumber += 1;
        loopNumber %= 600;
    }

/*************************************************
 *                    Methoden                   *
 *************************************************/

/***** ***** ***** ***** ***** ***** SENSOR FUNCTIONS ***** ***** ***** ***** ***** *****/

    void readSensors() {
        Serial.print(".");
    
        // read DHT22
        dht22One.readData();
        tempPlantTop = dht22One.getTemperatureC();
        humidPlantTop = dht22One.getHumidity();
  
        // read humidity
        gndHumid_1 = analogRead(GND_HUM_1);
        gndHumid_2 = analogRead(GND_HUM_2);
        gndHumid_3 = analogRead(GND_HUM_3);
        gndHumid_4 = analogRead(GND_HUM_4);
    }
    

/***** ***** ***** ***** ***** ***** SEND DATA FUNCTIONS ***** ***** ***** ***** ***** *****/

    void sendData() {  
        
        Serial.print("# Trying to connect to server... ");    
        
        if(client.connect(SERVER, 80)) {
            Serial.print("connecting succesful, sending data... ");

            client.print("GET " + String(HOST) + String(URL));
            client.print("?tpt=");
            client.print(tempPlantTop);
            client.print("&hpt=");
            client.print(humidPlantTop);
            client.print("&vl=");
            client.print(airLevel);
            client.print("&tpb=");
            client.print("0");
            client.print("&tpm=");
            client.print("0");
            client.print("&ti=");
            client.print(tempIn);
            client.print("&gh1=");
            client.print(gndHumid_1);
            client.print("&gh2=");
            client.print(gndHumid_2);
            client.print("&gh3=");
            client.print(gndHumid_3);       
            client.print("&mxt=");  
            client.print(max_temp);
            client.print("&mnt=");
            client.print(min_temp);
            client.print("&mxh=");
            client.print(max_humid);
            client.print("&mnh=");
            client.print(min_humid);
            client.print("&tol=");
            client.print(TOLERANCE);
            client.println(" HTTP/1.1");  
            client.print("Host: " + String(HOST));
            client.println();
            client.println("User-Agent: Arduino");
            client.println("Connection: close");
            client.println();
            client.stop();
            client.flush(); 

            Serial.println("data send.");

        } else {
            Serial.println("can't connect to server.");  
            Ethernet.begin(mac);
        } 
    }



/***** ***** ***** ***** ***** ***** AIRFLOW FUNCTIONS ***** ***** ***** ***** ***** *****/



    void setAir() {
        
        Serial.println("# Adjusting airflow...");
        if(switchAirIn > 0) {
            Serial.println("  Ventilation recently adjusted, timed out for: " + String(switchAirIn) + " loops.");
            switchAirIn--;
            return;    
        }
        
        
        /* --- Humidity --- */    
        Serial.print("  Checking humidity... ");     
   
        checkAir(true, false);
        checkAir(true, true);
        
        if((!isHumidHigh) && (!isHumidLow)) {
            Serial.println("humidity is optimal at " + String(humidPlantTop) + "% : (> " + String(limit_min_humid) + "% && < " + String(limit_max_humid) + "%)");
        }       
        
        /* --- Temperature --- */    
        Serial.print("  Checking temperature... ");  

        checkAir(false, false);
        checkAir(false, true);
        
        if((!isTempHigh) && (!isTempLow)) {
            Serial.println("temperature is optimal at " + String(tempPlantTop) + "^C : (> " + String(limit_min_temp) + "^C && < " + String(limit_max_temp) + "^C)");
        }

        /* --- Priorities --- */ 
        Serial.print("  Setting priorities... ");

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
            Serial.println("# No airflow adjustment needed.");  
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
                Serial.println("  " + word1 + " is at " + value + sign + " and " + word3 + "creasing! (Old: " + save + sign + ")");
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
                    Serial.println("  " + word1 + " stays constant on " + value + sign + ".");
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
                    Serial.println("  " + word1 + "seems to be constant on " + value + sign + ", checking next time again.");
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
                Serial.println("  " + word1 + " is at " + value + sign + " but still " + word4 + "creasing. (Old: " + save + sign + ")");
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
        Serial.print("  Increasing ventilation...");
        switch(airLevel) {
            case 0:
                Serial.println("ventilation started. (Timeout = 3)");
                switchRelaisOn(RELAIS_2); // switch from off to on
                airLevel += 1;
                switchAirIn = 5;
                printSystem = true;
                break;
            case 1:
                Serial.println("increased ventilation to level 2. (Timeout = 3)");
                switchRelaisOn(RELAIS_3); // switch from 1 to 2
                airLevel += 1;
                switchAirIn = 4;
                printSystem = true;
                break;
            default:
                Serial.println("Can't increase ventilation!");
                break;
        }     
    }



    void decreaseAir() {
        Serial.print("  Decreasing ventilation...");
        switch(airLevel) {
            case 1:
                switchRelaisOff(RELAIS_2); // switch from on to off
                airLevel -= 1;
                Serial.println("stopped ventilation. (Timeout = 2)");
                switchAirIn = 3;
                printSystem = true;
                break;
            case 2:
                switchRelaisOff(RELAIS_3); // switch from 2 to 1
                airLevel -= 1;
                Serial.println("decreased ventilation to level 1. (Timeout = 2)");
                switchAirIn = 3;
                printSystem = true;
                break;
            default:
                Serial.println("can't decrease ventilation!");
                break;
        }    
    }



/***** ***** ***** ***** ***** ***** VENTILATION FUNCTIONS ***** ***** ***** ***** ***** *****/



    void setVentilation() {
        if(switchVentiIn <= 0) {
            
            switch(ventiLevel) {
                case 0:  
                    Serial.println("  Turning fan on.");
                    switchRelaisOn(RELAIS_6);
                    ventiLevel += 1;
                    switchVentiIn = 15;
                    break;
                case 1:
                    Serial.println("  Turning fan off.");
                    switchRelaisOff(RELAIS_6);
                    ventiLevel -= 1;
                    switchVentiIn = 10;
                    break;
                default:
                    break;
            }     
            printSystem = true;
        } else {
            switchVentiIn--;
        }  
    }



/***** ***** ***** ***** ***** ***** NETWORK FUNCTIONS ***** ***** ***** ***** ***** *****/



    void checkDHCP() {
        Serial.println("");
        if(client.available()) {
            Serial.println("# DHCP: Client available.");
        } 
        if(client.connected()) {
            Serial.println("# DHCP: Client connected.");
        }
        switch(Ethernet.maintain()) {
            case 1:
                Serial.println("# DHCP: Renew failed!");
                break;
            case 2:
                Serial.println("# DHCP: Renew successful.");
                break;
            case 3:
                Serial.println("# DHCP: Rebind failed!");
                break;
            case 4:
                Serial.println("# DHCP: Rebind successful.");
                break;
            default:
                Serial.println("# DHCP: No action needed.");
                break;         
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
  
        if(printWaterSens) {
            Serial.print("  Water   (Plant #1): "); 
            tmp = (gndHumid_1 / 1023) * 100;
            Serial.print(gndHumid_1 / 10.23);
            Serial.println(" %");  
            Serial.print("  Water   (Plant #2): "); 
            tmp = (gndHumid_2 / 1023);
            Serial.print(gndHumid_2 / 10.23);
            Serial.println(" %"); 
            Serial.print("  Water   (Plant #3): "); 
            tmp = (gndHumid_3 / 1023);
            Serial.print(gndHumid_3 / 10.23);
            Serial.println(" %");   
            Serial.print("  Water   (Plant #4): "); 
            tmp = (gndHumid_4 / 1023);
            Serial.print(gndHumid_4 / 10.23); 
            Serial.println(" %");  
        }
    }
    
    void printSystemData() {  
        
        Serial.println("");
        Serial.println("# SYSTEM-VALUES:");    
        if(printAir) {
            Serial.print("  Ventilation  (lvl): "); 
            Serial.println(ventiLevel);  
            Serial.print("  Airflow      (lvl): "); 
            Serial.println(airLevel);  
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
        Serial.println(airLevel);
        Serial.print("&tpb=");
        Serial.println("0");
        Serial.print("&tpm=");
        Serial.println("0");
        Serial.print("&ti=");
        Serial.println(tempIn);
        Serial.print("&gh1=");
        Serial.println(gndHumid_1);
        Serial.print("&gh2=");
        Serial.println(gndHumid_2);
        Serial.print("&gh3=");    
        Serial.println(gndHumid_3);  
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

    
    
/***** ***** ***** ***** ***** ***** RELAIS FUNCTIONS ***** ***** ***** ***** ***** *****/



    void switchRelaisOn(int relais) {
        digitalWrite(relais, LOW);  
    }

    void switchRelaisOff(int relais) {
        digitalWrite(relais, HIGH);  
    }











  
