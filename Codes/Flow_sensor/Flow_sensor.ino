unsigned long lastFlow = 0;

void setupFlow() {
  attachInterrupt(
    digitalPinToInterrupt(FLOW_PIN),
    countFlowPulse,
    FALLING
  );
  lastFlow = millis();
}

void updateFlow() {
  unsigned long now = millis();

  if (now - lastFlow >= 1000) {
    unsigned long elapsed = now - lastFlow;

    noInterrupts();
    unsigned long pulses = flowPulses;
    flowPulses = 0;
    interrupts();

    float frequency = pulses * 1000.0 / elapsed;

    flowRate = frequency / FLOW_FACTOR;
    totalLiters += flowRate * elapsed / 60000.0;

    filteredFlow = 0.8 * filteredFlow + 0.2 * flowRate;
    lastFlow = now;
  }
}