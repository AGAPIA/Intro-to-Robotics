// declare all the segments pins
const int pinA = 12;
const int pinB = 10;
const int pinC = 9;
const int pinD = 8;
const int pinE = 7;
const int pinF = 6;
const int pinG = 5;
const int pinDP = 4;
const int segSize = 8;
int index = 0;
bool commonAnode = false; // modify if you have common anode
byte state = HIGH;
int segments[segSize] = {
pinA, pinB, pinC, pinD, pinE, pinF, pinG, pinDP
};
void setup() {
for (int i = 0; i < segSize; i++) {
pinMode(segments[i], OUTPUT);
}
if (commonAnode == true) {
state = !state;
}
}
void loop() {
digitalWrite(segments[index], state);
index++;
delay(500);
if (index >= segSize) {
index = 0;
state = !state;
}
}