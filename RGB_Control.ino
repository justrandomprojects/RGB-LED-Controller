
/*  
 * RGB LED Driver Project (aka Bedroom Mood Lighting
 * -------------------------------------------------------------------
 * Original Code from JustRandomProjects.wordpress.com
 * Please credit if this code is used elsewhere, thanks!
 * 
 * This code includes five functions of operation:
 * 1. Custom Colour Mix
 * 2. Custom Colour Flash A
 * 3. Custom Colour Flash B 
 * 4. Custom Colour Twinkle
 * 5. Random Colour Mixing
 *
 * Using interrupts and digital debounce to monitor the 
 * button so that we don't get bounce or miss any presses.
 */

//define the pin numbers to be constant
const int v = 13;      //testing purposes only
const int r = 3;       //pin connected to RED anode
const int g = 6;       //pin connected to GREEN anode
const int b = 5;       //pin connected to BLUE anode
const int w = 4;       //pin connected to Acknowledgement LED
const int x = 7;       //pin connected to Cathode X
const int y = 8;       //pin connected to Cathode Y
const int z = 2;       //pin connected to Mode Button
const int rPot = A0;   //pin connected to Red Potentiometer (10kR)
const int gPot = A2;   //pin connected to Green Potentiometer (10kR)
const int bPot = A4;   //pin connected to Blue potentiometer (10kR)

//define variables 
int cm = 750;          //ms for flash duration
int cn = cm - 1;       //starting pointer to avoid misfire on startup
byte rLevel;           //mapped value for red
byte gLevel;           //mapped value for green
byte bLevel;           //mapped value for blue
byte currentMode = 1;  //value for mode holder

boolean countPress = false;    //flag for counting presses
long durCountPress = 1500;     //1500ms max between presses
long durDeBounce = 150;         //50ms deBounce
long lastPress = 0;            //calculator for last press time
byte presses = 0;              //how many presses?

byte pRed = 0;         
byte pGrn = 0;
byte pBlu = 0;
byte rTar = 0;
byte gTar = 0;
byte bTar = 0;
byte rDif = 0;
byte gDif = 0;
byte bDif = 0;
byte rVal = 0;
byte gVal = 0;
byte bVal = 0;

void setup () {
  //Set the pin modes as OUTPUT
  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(v, OUTPUT);
  pinMode(w, OUTPUT);
  pinMode(x, OUTPUT);
  pinMode(y, OUTPUT);

  //Set the button pin mode as INPUT_PULLUP
  //this means INVERTED LOGIC, but saves me a 10k resistor!
  //i.e. button state un-pressed returns TRUE
  //     button state   pressed returns FALSE
  pinMode(z, INPUT_PULLUP);

  //set the random seed for Mode 5
  randomSeed(goReadPot(gPot));

  //show a test pattern on start-up to highlight any issues
  testFunction();
  
  //attach the button routine to the interrupt pin
  attachInterrupt (0, buttonPressed, LOW);
}

void loop () {
  cn++;
  switch (currentMode)  {
  case 1:
    customMix();
    break;
  case 2:
    customFlashA();
    break;
  case 3:
    customFlashB();
    break;
  case 4:
    customTwinkle();
    break;
  case 5:
    randomColourMix();
    break;
  default:
    customMix();
    break;
  }
  modeChanger();
}

void testFunction() {
  //this function will cycle test all the LEDs
  //taking approximately 5 seconds
  //using arrays to handle the variables means less lines!
  int anodes[4] = {
    b, g, r, w  
  };
  int cathodes[2] = {
    x, y
  };

  //set everything LOW
  digitalWrite (x, LOW);
  digitalWrite (y, LOW);
  digitalWrite (r, LOW);
  digitalWrite (g, LOW);
  digitalWrite (b, LOW);
  for (int inner = 0; inner <= 3 ; inner++) {
    for (int outer = 0; outer <= 1; outer++) {
      //set HIGH the testing LEDs
      digitalWrite (cathodes[outer], HIGH);
      digitalWrite (anodes[inner], HIGH);
      delay(750);
      //set LOW the testing LEDs
      digitalWrite (cathodes[outer], LOW);
      digitalWrite (anodes[inner], LOW);
    }
  }
  flashAck(currentMode);  //acknowledge the current mode
}

void flashAck(byte nFlash) {
  //function to use the LED to acknowledge the user's choice in mode
  //by flashing the LED a number of times
  //set everything LOW
  digitalWrite (x, LOW);
  digitalWrite (y, LOW);
  digitalWrite (r, LOW);
  digitalWrite (g, LOW);
  digitalWrite (b, LOW);
  digitalWrite (w, LOW);      
  digitalWrite (v, LOW);      
  delay (250);

  for (int n = 1; n <= nFlash; n++){
    //HIGH
    digitalWrite (w, HIGH);
    digitalWrite (v, HIGH);
    delay(250);
    //LOW
    digitalWrite (w, LOW);
    digitalWrite (v, LOW);
    delay (250);
  }
}

void customMix () {
  //Mode 1
  //This function displays plain the chosen colour
  rLevel = goReadPot(rPot);
  gLevel = goReadPot(gPot);
  bLevel = goReadPot(bPot);
  goWriteLEDs (rLevel, gLevel, bLevel);
  digitalWrite (x, HIGH);
  digitalWrite (y, HIGH);
  if (cn > 5000) cn = 0;
}

void customFlashA () {
  //Mode 2
  //This function flashes the custom colour on all LEDs
  rLevel = goReadPot(rPot);
  gLevel = goReadPot(gPot);
  bLevel = goReadPot(bPot);
  goWriteLEDs (rLevel, gLevel, bLevel);
  if (cn == cm) {
    digitalWrite (x, LOW);
    digitalWrite (y, LOW);
  } 
  else if (cn > (cm * 2)) {
    digitalWrite (x, HIGH);
    digitalWrite (y, HIGH);
    cn=0;
  } 
}

void customFlashB () {
  //Mode 3
  //This function flashes the custom colour on 
  //cathode X, then Y, alternately
  rLevel = goReadPot(rPot);
  gLevel = goReadPot(gPot);
  bLevel = goReadPot(bPot);
  goWriteLEDs (rLevel, gLevel, bLevel);
  if (cn == cm) {
    digitalWrite (x, HIGH);
    digitalWrite (y, LOW);
  } 
  else if (cn > (cm * 2)) {
    digitalWrite (x, LOW);
    digitalWrite (y, HIGH);
    cn=0;
  } 
}

void customTwinkle() {
  //Mode 4
  //This function twinkles the custom colour on all LEDs
  digitalWrite (x, HIGH);
  digitalWrite (y, HIGH);  
  rLevel = goReadPot(rPot);
  gLevel = goReadPot(gPot);
  bLevel = goReadPot(bPot);

  //Calculate targed and difference values.
  //Formula takes current desired mix level
  //mixes back each channel in ratio to maintain (close enough) to
  // the desired colour.  Example below.
  //@ 100%    r = 200, g = 150, b = 100
  //@ 50%     r = 150, g = 112, b = 75
  //@ 0%      r = 100, g = 75,  b = 50
  rTar = rLevel * 0.5;
  gTar = gLevel * 0.5;
  bTar = bLevel * 0.5;
  rDif = rLevel - rTar;
  gDif = gLevel - gTar;
  bDif = bLevel - bTar;

  //fade back to target
  for (byte dn = 100; dn > 0; dn = dn - 2) {
    rVal = rTar + ((dn * rDif) / 100);
    gVal = gTar + ((dn * gDif) / 100);
    bVal = bTar + ((dn * bDif) / 100);
    goWriteLEDs (rVal, gVal, bVal);
    delay(10);
  }
  //fade up to normal level
  for (byte up = 0; up < 100; up = up + 2) {
    rVal = rTar + ((up * rDif) / 100);
    gVal = gTar + ((up * gDif) / 100);
    bVal = bTar + ((up * bDif) / 100);
    goWriteLEDs (rVal, gVal, bVal);
    delay(10);
  }
}

void randomColourMix () {
  // Mode 5
  //Using randomised values, mix up and down colours gently.
  digitalWrite (x, HIGH);
  digitalWrite (y, HIGH); 

  //get Random Values
  rLevel = (32 * random(1, 8)) - 1;
  gLevel = (32 * random(1, 8)) - 1;
  bLevel = (32 * random(1, 8)) - 1;

  //fade colour
  goFadeColour(r, rLevel, pRed);
  goFadeColour(g, gLevel, pGrn);
  goFadeColour(b, bLevel, pBlu);

  //set values for next iteration
  pRed = rLevel;
  pGrn = gLevel;
  pBlu = bLevel;  

}

int goReadPot (int pot) {
  int potValue = analogRead(pot);
  byte mapPotValue = map (potValue, 0, 1023, 0, 255);
  return mapPotValue;
}

void goWriteLEDs (byte wRed, byte wGrn, byte wBlu) {
  analogWrite (r, wRed);
  analogWrite (g, wGrn);
  analogWrite (b, wBlu);
}

void goFadeColour(int pin, byte cL, byte pL) {
  int dL = pL - cL;
  if (dL == 0) {
    //there was nothing to do
    return;
  } 
  else if (dL < 0) {
    //current level > previous level .: going up
    for (byte up = pL; up < cL; up = up + 1) {
      analogWrite (pin, up);
      delay(25);
    }
    return;
  } 
  else if (dL > 0) {
    //current level < previous level .: going down
    for (byte dn = pL; dn > cL; dn = dn - 1) {
      analogWrite (pin, dn);
      delay(25);
    }
    return;
  }
}

void buttonPressed () {
  //The button changed stated
  //Do Stuff - Interrupt the current Routine
  if ((millis() - lastPress) > durDeBounce) {
    countPress = true;  //start counting presses
    presses++;          //increment presses received
    if (presses > 5) presses = 1;
    lastPress = millis(); 
  }
}

void modeChanger () { 
  if (countPress == true) {
    if ((millis() - lastPress) > durCountPress) {
      currentMode = presses;  //set the mode to the number of presses received
      flashAck(currentMode);  //flash to acknowledge mode
      countPress = false;     //turn the flag off to prevent excessive execution of loop
    }
  }
}

