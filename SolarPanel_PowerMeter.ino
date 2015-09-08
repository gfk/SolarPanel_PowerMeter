#include <LowPower.h>
#include <U8glib.h>
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);	// Display which does not send ACK

const bool debug = false;
const byte voltsPin = 1;
const byte ampsPin = 2;
const int numReadings = 10;

bool redraw = true;
float volts[numReadings];
float amps[numReadings];
int index = 0;                  // the index of the current reading
float totalV = 0;                  // the running total
float totalA = 0;

void sleep(period_t period)
{
  LowPower.idle(period, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
                SPI_OFF, USART0_OFF, TWI_OFF);
}

void draw(float volts, float amps) {
  // Watts  
  if (debug) {
    u8g.setFont(u8g_font_helvB24n);
  } else {
    u8g.setFont(u8g_font_helvB24);
  }
  u8g.setFontPosTop();
  float watts = volts * amps;
  if (watts > 0.5) {
    String wattsStr = String(watts, 1) + "W";
    int wLen = u8g.getStrWidth(wattsStr.c_str());
    u8g.drawStr( (128 - wLen)/2, 10, wattsStr.c_str());
  }

  // Volts & Amps
  u8g.setFont(u8g_font_unifont);
  u8g.setFontPosBottom();
  String voltsStr= String(volts, 2) + "V";

  String ampsStr;
  //if (amps < 1) {
  //  // Miliamps
  //  ampsStr = "    " + String(amps * 1000, 0) + "mA";
  //} else {
    ampsStr = "    " + String(amps, 2) + "A";
  //}
  
  String voltsAmps = voltsStr + " " + ampsStr;
  u8g.drawStr( 0, 64, voltsAmps.c_str());
}

void setup(void) {
  u8g.setRot180();
  u8g.setColorIndex(1);

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    volts[thisReading] = 0;
    amps[thisReading] = 0;
  }

  pinMode(voltsPin, INPUT);
  pinMode(ampsPin,  INPUT);

  if (debug) Serial.begin(9600);
}

void loop(void) {
  // Rolling average (part 1)
  totalV -= volts[index];
  totalA -=  amps[index];

  int voltRaw = analogRead(voltsPin);
  int ampRaw  = analogRead(ampsPin);
  if (ampRaw <= 512) ampRaw = 512; // No negative amp values

  volts[index] = (float)voltRaw / 41.05;
  amps[index]  = (((float)ampRaw / 1024.0 * 5000) - 2500) / 100;

  if (debug) {
    Serial.print("VoltRaw=");
    Serial.println(voltRaw);
    Serial.print("AmpRaw=");
    Serial.println(ampRaw);
  }

  // Rolling average (part 2)
  totalV += volts[index]; 
  totalA +=  amps[index];
  float avgAmps = totalA / numReadings;
  float avgVolts = totalV / numReadings;

  if (++index >= numReadings) index = 0;
  
  if (redraw) {
    u8g.firstPage();
    do {
      draw(avgVolts, avgAmps);
    } while( u8g.nextPage() );
  }
  
  // rebuild the picture after some delay
  if (debug || avgVolts * avgAmps > 0.05 || avgVolts < 12) {
    sleep(SLEEP_500MS);
    redraw = !redraw; // Redraw every second (50% of the time)
  } else {
    // Night: Low power, sleep 8 seconds
    sleep(SLEEP_8S);
    redraw = true;
  }
}

