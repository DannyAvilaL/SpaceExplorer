/*
   Programa por Daniela Avila Luna
   contacto: daniela.aluna@outlook.com

   Este programa permite utilizar el GY-521 para un programa
   en Unity que lee los valores de Pitch-Yaw-Roll y que
   mueve la cámara con esos valores.

*/

//Se incluyen librerías necesarias para poder trabajar con el GY-521
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//Se crea una variable mpu.
MPU6050 mpu;

//----------------------------------------------------------------------
// quaternion components in a [w, x, y, z] format (not best for parsing
// on a remote host such as Processing or something though)

//Utilizar en caso de querer eviar valores quaterniones.
//#define OUTPUT_READABLE_QUATERNION
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//Valores de Yaw - Pitch - Roll [yaw, pitch, roll]
//Sufren de Gimbal lock- es decir, pierde un eje de rotación
#define OUTPUT_READABLE_YAWPITCHROLL
//----------------------------------------------------------------------


#define INTERRUPT_PIN 2  // use pin 2 on Arduino Uno & most boards
#define LED_PIN 13 // (Arduino is 13, Teensy is 11, Teensy++ is 6)
bool blinkState = false;

//se definen los pines donde están conectados los botones.
#define teclaESC 4
#define teclaENTER 5
#define teclaUP 6
#define teclaDOWN 8
#define teclaRIGHT 9
#define teclaLEFT 10

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// packet structure for InvenSense teapot demo
uint8_t teapotPacket[14] = { '$', 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x00, '\r', '\n' };

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady()
{
  mpuInterrupt = true;
}

void girsocopios() {
  //funcion que lee los valores del sensor de la manera indicada, ya sea Yaw-Pitch-Roll o
  //con quaterniones.
  if (!dmpReady) return;
  // read a packet from FIFO
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet
    
#ifdef OUTPUT_READABLE_QUATERNION
    // display quaternion values in easy matrix form: w x y z
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    //Serial.print("quat\t");
    Serial.print(q.w);
    Serial.print(",");//Serial.print("\t");
    Serial.print(q.x);
    Serial.print(",");//Serial.print("\t");
    Serial.print(q.y);
    Serial.print(",");//Serial.print("\t");
    Serial.println(q.z);
#endif

#ifdef OUTPUT_READABLE_YAWPITCHROLL
    // display Euler angles in degrees
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    Serial.print(ypr[0] * 180 / M_PI);
    Serial.print(",");
    Serial.print(ypr[1] * 180 / M_PI);
    Serial.print(",");
    Serial.print(ypr[2] * 180 / M_PI);
    Serial.println();
#endif
  }
}


void botones() {
  //funcion que detecta la interrupción de los botones y envía
  //de manera escrita qué tecla representan.
  if (digitalRead(teclaESC) == LOW) { //ESC
    Serial.println("ESC");
    delay(1000);
  }
  if (digitalRead(teclaENTER) == LOW)
  {
    Serial.println("ENTER");
    //delay(100);
  }
  if (digitalRead(teclaUP) == LOW) { //UP
    Serial.println("U");
    //delay(1000);
  }
  if (digitalRead(teclaDOWN) == LOW) { //DOWN
    Serial.println("D");
    //delay(1000);
  }
  if (digitalRead(teclaRIGHT) == LOW) { //RIGHT
    Serial.println("R");
    //delay(1000);
  }
  if (digitalRead(teclaLEFT) == LOW) { //LEFT
    Serial.println("L");
    //delay(1000);
  }
}

void setup() {
  //declaracion de botones que utiliza el joystick
  pinMode(teclaESC, INPUT_PULLUP);
  pinMode(teclaENTER, INPUT_PULLUP);
  pinMode(teclaUP, INPUT_PULLUP);
  pinMode(teclaDOWN, INPUT_PULLUP);
  pinMode(teclaRIGHT, INPUT_PULLUP);
  pinMode(teclaLEFT, INPUT_PULLUP);


#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  Serial.begin(9600);
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);
  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
  devStatus = mpu.dmpInitialize();
  // CAMBIAR ESTOS OFFSETS DE ACUERDO CON LOS QUE SE OBTIENEN DEL ARCHIVO DE CALIBRACIÓN.
  mpu.setXGyroOffset(-1);
  mpu.setYGyroOffset(7);
  mpu.setZGyroOffset(9);
  mpu.setZAccelOffset(4402);

  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
    Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
    Serial.println(F(")..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  // configure LED for output
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  //Función que se repite por siempre y que envía
  //la información del sensor y de los botones por
  //medio del puerto serial.
  girsocopios();
  botones();
}
