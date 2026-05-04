// ===== CONFIG =====
const int muxSig = A0;

// MULTIPLEXER select pins
const int S0 = 0;
const int S1 = 1;
const int S2 = 2;
const int S3 = 3;

// Ground control pins
const int groundPins[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const int NUM_GROUNDS = 10;

// Channel → node mapping
const int muxChannels[] = {
  0,  // source (before 1k)
  1,  // node 0
  2,  // node 4
  3,  // node 8
  4,  // node 12
  5,  // node 13
  6,  // node 14
  7,  // node 15
  8,  // node 11
  9,  // node 7
 10,  // node 3
 11,  // node 2
 12   // node 1
};

const int NUM_CHANNELS = sizeof(muxChannels)/sizeof(muxChannels[0]);

const float VREF = 5.0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  analogReadResolution(14); //changes ADC to 14-bit resolution

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  for (int i = 0; i < NUM_GROUNDS; i++) {
    pinMode(groundPins[i], OUTPUT);
    digitalWrite(groundPins[i], LOW);
  }
}

// ===== MULTIPLEXER SELECTOR =====
void setMuxChannel(int ch) {
  digitalWrite(S0, ch & 1);
  digitalWrite(S1, (ch >> 1) & 1);
  digitalWrite(S2, (ch >> 2) & 1);
  digitalWrite(S3, (ch >> 3) & 1);
}

// ===== READ VOLTAGE =====
float readVoltage(int ch) {
  setMuxChannel(ch);
  delayMicroseconds(50);

  long sum = 0;
  const int samples = 50;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(muxSig);
  }

  float raw = sum / (float)samples;
  return (raw / 16383.0) * VREF;
}

// ===== MAIN LOOP =====
void loop() {

  for (int g = 0; g < NUM_GROUNDS; g++) {

    // Turn OFF all grounds
    for (int i = 0; i < NUM_GROUNDS; i++) {
      digitalWrite(groundPins[i], LOW);
    }

    // Enable the specified ground MOSFET
    digitalWrite(groundPins[g], HIGH);

    delay(50); // settle the array before measuring

    Serial.print("EXP ");
    Serial.println(g);

    for (int i = 0; i < NUM_CHANNELS; i++) {
      float v = readVoltage(muxChannels[i]);

      Serial.print(v, 4);
      if (i < NUM_CHANNELS - 1) Serial.print(",");
    }

    Serial.println();
    delay(200);
  }

  Serial.println("DONE\n");
  delay(2000);
}
