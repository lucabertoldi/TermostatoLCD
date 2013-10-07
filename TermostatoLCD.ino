/*
//  TERMOSTATO AMBIENTALE PER TERMOSIFONI CONTROLLATO VIA INTERNET
 //  Ho creato questo programma per sfizio e per utilità frutto di lunghe ricerche su internet e spunti presi qui e li.....
 //  Ho utilizzato un Arduino 2009,Ethernet Shield(senza SD),display 16x2 JHD162A(pin retroilluminazione collegato al pin D8),sensore temperatura LM35,
 //    2 pulsanti per la variazione manuale della temperatura(ovviamente pull-uppati),
 //    1 relè contatto pulito accensione pompa termosifoni(debitamente pilotato).
 
 Critiche e consigli sono bene accetti visti i difetti che avrà il mio programma...... :-(
 
 Fonti:
 Sono diverse,sicuramente http://www.giannifavilli.it/blog/ è stato uno dei punti di partenza per la parte web;
 le altre non le ricordo(non mene vogliano gli autori)!!!!!!!
 
 Versione IDE Arduino 1.0
 
 email: ioroberto@gmail.com
 */

//Librerie
#include <SPI.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>



//Dati login pagina web
String user="";          //Modificare per nome utente          <--------------------------------------------------------------
String pass="";             //Modificare per password          <--------------------------------------------------------------
long timeout=30000;               //Timeout sessione Web in millisecondi


#define loctempsetint  0          //ID locazione EEPROM numero temperatura impostata intero

#define loctempsetdec  1          //ID locazione EEPROM numero temperatura impostata decimale
#define delaymess 10000           //Tempo in ms che mantiene il messaggio dal Web sul displ
#define delaydis  5000            //Tempo in ms retroilluminazione display attiva
#define illon     500             //Tempo in ms*10 che se premuto uno dei tasti retroillumina soltanto 

#define ritardosel 500            //Tempo in ms che regola la lettura dei pulsanti variazione temperatura

#define PUSHED    1023            //Valore lettura analogica pulsante premuto su ingresso analogico 

#define ISTERESI   1              //isteresi(range temperatura sopra e sotto setpoint


#define RELE       51              //pin attuatore pompa termosifoni(da pilotare)



#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

long loginstart;                  //Variabile buffer millis per sessione Web

long backlightstart;              //Variabile buffer millis per retroillumizione display

int clrdis=false;                 //flag cancella display

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);   //configurazione collegamento display

double TMIN;                      //Variabile di appoggio temperatura impostata ricavata dal numero intero 
//e decimale su EEPROM(intero locazione loctempsetint,decimale locazione loctempsetdec)

double temp;                      //Variabile di appoggio lettura temperatura



// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 53

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;



void setup(){

  // EEPROM.write(loctempsetint,20);     
  //  EEPROM.write(loctempsetdec,0);    
  // sensors.begin();                // Start up the library Dallas
Serial.begin(9600);
  lcd.begin(16, 2);               // set up the LCD's number of rows and columns:

  //  pinMode(pinleddisp, OUTPUT);    //Dichiarazione tipo pin retroilluminazione

  pinMode(RELE, OUTPUT);          //Dichiarazione tipo pin pompa termisifoni


  // Cancella lcd
  lcd.clear();

  stringalogin=String("Nome="+user+"&Pwd="+pass);    //Formazione stringa di confronto login!

  //  digitalWrite(pinleddisp, 1);                       //Attiva la retrolluminazione

 
  //inizializzazione sensore
  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

  // Method 1:
  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them.  It might be a good idea to 
  // check the CRC to make sure you didn't get garbage.  The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 10);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
}




void loop(){

  /*inizio calcolo temperatura*/
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  double temp = sensors.getTempC(insideThermometer);
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(temp);  

  /*fine calcolo temperatura*/


  //Modifica temperatura manuale


    //Ciclo aumenta temperatura manuale a passi di mezzo grado
  if  (read_LCD_buttons()==btnUP){                    
      //lcd.clear();      
      TMIN=TMIN+0.5;
    // lcd.setCursor(0,1);
    //lcd.write("T.Imp: ");
    // lcd.print(TMIN);           
      delay(ritardosel);
       //Memorizza la temperatura selezionata nell'EEPROM nelle due locazioni(parte intera e decimale)
    EEPROM.write(loctempsetint,(int)TMIN);     
    EEPROM.write(loctempsetdec,(TMIN-(int)TMIN)*100);     
    
     clrdis=true;
  }

    //Ciclo diminuisce temperatura manuale a passi di mezzo grado
   if (read_LCD_buttons()==btnDOWN){
    //  lcd.clear();      
      TMIN=TMIN-0.5;
    // lcd.setCursor(0,1);
    //lcd.write("T.Imp: ");
    //  lcd.print(TMIN);           
      delay(ritardosel);
       //Memorizza la temperatura selezionata nell'EEPROM nelle due locazioni(parte intera e decimale)
    EEPROM.write(loctempsetint,(int)TMIN);     
    EEPROM.write(loctempsetdec,(TMIN-(int)TMIN)*100);     
    clrdis=true;
   }

   

 
  //  delay(ritardosel*3);
  //  backlightstart=millis();    
  

  if (clrdis==true){                                  //Controllo flag cancella display
    //clear the screen
    lcd.clear();
    clrdis==false; 
  }

  TMIN=EEPROM.read(loctempsetint)+(EEPROM.read(loctempsetdec)*0.01);    //costruzione setpoin temperatura dai dati letti dall'EEPROM

  //Visualizzazione temperatura letta e setpoint sul display
  lcd.setCursor(0,0);
  lcd.write("T.att:  ");
  lcd.print(temp);
  lcd.write("C");
  lcd.setCursor(0,1);  
  lcd.write("T.imp:  ");
  lcd.print(TMIN);
  lcd.write("C ");  

  // if((millis()-backlightstart) >= delaydis){
  //   digitalWrite(pinleddisp, 0);                       //Disattiva la retrolluminazione
  // }
if (temp >= 5 && temp <= 30) {
  if (temp < TMIN-ISTERESI ){                                           // freddo - attivare la caldaia
    digitalWrite(RELE, 1);
    TERMOSIFONI= true;
    lcd.blink();    
  }

  if (temp >= TMIN+ISTERESI){                                          // caldo - spegnere la caldaia
    digitalWrite(RELE, 0);
    TERMOSIFONI = false;
    lcd.noBlink();
  }
  } 
 else {
 digitalWrite(RELE, 0);
 lcd.noBlink();
 }


} // fine loop

int read_LCD_buttons()
{
  int adc_key_in = analogRead(0);      // read the value from the sensor

  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result

  if (adc_key_in < 50)   return btnRIGHT; 

  if (adc_key_in < 195)  return btnUP;

  if (adc_key_in < 380)  return btnDOWN;

  if (adc_key_in < 555)  return btnLEFT;

  if (adc_key_in < 790)  return btnSELECT;  

  return btnNONE;  // when all others fail, return this...

}
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


