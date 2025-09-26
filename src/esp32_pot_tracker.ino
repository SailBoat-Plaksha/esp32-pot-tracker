/*
================================================================================
ESP32 Potentiometer Tracker with Averaging
- Shows detailed output in Serial Monitor (raw, min, max, normalized).
- Prints clean averaged normalized values for Serial Plotter on the same line.
================================================================================
*/

const int pinsADC[] = {32, 33, 34, 35};
const int N = sizeof(pinsADC)/sizeof(pinsADC[0]);

const int BITS = 12;
const float FULL = 4095.0f;

const float THRESHOLD_PCT = 4.0f;
const float ASSIGN_INTEGRAL_PCT = 10.0f;
const float EMA_ALPHA = 0.3f;

const uint16_t UPDATE_MS = 250;   // 4 updates/sec
const int AVG_SAMPLES = 4;        // average this many samples

float emaVal[16], baseline[16], accumDeltaPct[16];
int   assignedRankOfChan[16];
int   chanForRank[16];
int   nextRank = 1;
int   rawMinByRank[16], rawMaxByRank[16];

unsigned long tPrint = 0;

static inline float clampf(float x, float a, float b) {
  return (x < a) ? a : (x > b) ? b : x;
}

void resetMinMax() {
  for (int r = 0; r < N; r++) {
    rawMinByRank[r] = 4095;
    rawMaxByRank[r] = 0;
  }
}

void resetAll() {
  nextRank = 1;
  for (int r = 0; r < N; r++) chanForRank[r] = -1;
  resetMinMax();
  for (int i = 0; i < N; i++) {
    assignedRankOfChan[i] = -1;
    accumDeltaPct[i] = 0.0f;
    emaVal[i] = analogRead(pinsADC[i]);
    baseline[i] = emaVal[i];
  }
}

void assignChannel(int i) {
  if (nextRank <= N && assignedRankOfChan[i] == -1) {
    int r = nextRank++;
    assignedRankOfChan[i] = r;
    chanForRank[r-1] = i;
    rawMinByRank[r-1] = 4095;
    rawMaxByRank[r-1] = 0;
    Serial.printf("Assigned: GPIO%d -> Pot%d\n", pinsADC[i], r);
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(BITS);
  analogSetAttenuation(ADC_11db);

  delay(200);
  for (int i = 0; i < N; i++) {
    emaVal[i] = analogRead(pinsADC[i]);
    baseline[i] = emaVal[i];
    assignedRankOfChan[i] = -1;
    accumDeltaPct[i] = 0.0f;
  }
  for (int r = 0; r < N; r++) chanForRank[r] = -1;
  resetMinMax();

  Serial.println("ESP32 Pot Tracker with Monitor + Plotter");
}

void loop() {
  // Detect movement
  for (int i = 0; i < N; i++) {
    float raw = analogRead(pinsADC[i]);
    emaVal[i] = EMA_ALPHA*raw + (1.0f-EMA_ALPHA)*emaVal[i];

    float pctNow  = (emaVal[i] / FULL) * 100.0f;
    float pctBase = (baseline[i] / FULL) * 100.0f;
    float dAbs    = fabsf(pctNow - pctBase);

    if (assignedRankOfChan[i] == -1) {
      if (dAbs >= THRESHOLD_PCT) {
        accumDeltaPct[i] += dAbs;
        if (accumDeltaPct[i] >= ASSIGN_INTEGRAL_PCT) assignChannel(i);
      }
    }
  }

  // Update min/max
  for (int r = 1; r <= N; r++) {
    int idx = chanForRank[r-1];
    if (idx >= 0) {
      int raw = analogRead(pinsADC[idx]);
      if (raw < rawMinByRank[r-1]) rawMinByRank[r-1] = raw;
      if (raw > rawMaxByRank[r-1]) rawMaxByRank[r-1] = raw;
    }
  }

  // Print every 250 ms
  unsigned long now = millis();
  if (now - tPrint >= UPDATE_MS) {
    tPrint = now;

    // First, detailed monitor output
    for (int r = 1; r <= N; r++) {
      int idx = chanForRank[r-1];
      if (idx < 0) {
        Serial.printf("Pot%d: (unassigned)\n", r);
      } else {
        long sum = 0;
        for (int k = 0; k < AVG_SAMPLES; k++) {
          sum += analogRead(pinsADC[idx]);
          delay(2);
        }
        int raw = sum / AVG_SAMPLES;

        int   rmin = rawMinByRank[r-1];
        int   rmax = rawMaxByRank[r-1];
        float pct  = (raw / FULL) * 100.0f;
        float pmin = (rmin / FULL) * 100.0f;
        float pmax = (rmax / FULL) * 100.0f;

        float norm = 0.0f;
        int range = rmax - rmin;
        if (range > 4) {
          norm = ((float)(raw - rmin) / (float)range) * 100.0f;
          norm = clampf(norm, 0.0f, 100.0f);
        }

        Serial.printf(
          "Pot%d [GPIO%d]  RAW:%4d (%.1f%%)  MIN:%4d (%.1f%%)  MAX:%4d (%.1f%%)  NORMALIZED:%5.1f%%\n",
          r, pinsADC[idx],
          raw, pct,
          rmin, pmin,
          rmax, pmax,
          norm
        );
      }
    }
    Serial.println();

    // Then, a compact line for Serial Plotter (just normalized values)
    for (int r = 1; r <= N; r++) {
      int idx = chanForRank[r-1];
      float norm = 0.0f;
      if (idx >= 0) {
        int raw = analogRead(pinsADC[idx]);
        int rmin = rawMinByRank[r-1];
        int rmax = rawMaxByRank[r-1];
        int range = rmax - rmin;
        if (range > 4) {
          norm = ((float)(raw - rmin) / (float)range) * 100.0f;
          norm = clampf(norm, 0.0f, 100.0f);
        }
      }
      Serial.print(norm, 1);
      if (r < N) Serial.print(" ");
    }
    Serial.println();
  }
}
