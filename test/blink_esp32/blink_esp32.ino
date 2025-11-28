    void setup() {
      pinMode(8, OUTPUT);
    }

    void loop() {
      digitalWrite(8, HIGH);
      delay(1000);
      digitalWrite(8, LOW);
      delay(1000);
    }

    /*
    cd ..
    arduino-cli compile --fqbn esp32:esp32:esp32c3 blink_esp32
    arduino-cli upload -b esp32:esp32:esp32c3 -p /dev/ttyACM0 blink_esp32
    */