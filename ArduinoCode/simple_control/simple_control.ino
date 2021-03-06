/*  Stewart Platform Simple Control Code
 *  Angles can be commanded over serial connection
 *  
 *  
 *  by Scott Christensen
 * 
 *  last modified 3/31/2016
 * 
 *  Part of the stewart platform project at
 *  http://www.github.com/meowFlute/StewartPlatform
 */
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3; 
Servo servo4;
Servo servo5;
Servo servo6;

byte incomingBuffer[24];
float commands[6];
float angles[6];

void setup() 
{
  //attach the servos to their respective pwm pins
  servo1.attach(3);
  servo2.attach(5);
  servo3.attach(6);
  servo4.attach(9);
  servo5.attach(10);
  servo6.attach(11);

  int i;
  for(i = 0; i < 6; i++)
  {
    angles[i] = -0.0464253166312861;
  }
  moveAllServos();
  Serial.begin(115200);
}

void loop() 
{
  if (Serial.available() >= 24) 
  {
    receiveBytes();
    getServoAngles(commands[0], commands[1], commands[2], commands[3], commands[4], commands[5]);
    printVector(&angles[0], 6);
    moveAllServos();
  }
}


/**************************************************************
 * This function takes 6 floats and applies it to the 6 servos
 */
void moveAllServos()
{
  servo1.write((-angles[0]*180/PI)+85);
  servo2.write((angles[1]*180/PI)+75);
  servo3.write((-angles[2]*180/PI)+100);
  servo4.write((angles[3]*180/PI)+90);
  servo5.write((-angles[4]*180/PI)+85);
  servo6.write((angles[5]*180/PI)+90);
}

/**************************************************************
 * This function takes in the desired orientation and computes the needed servo angles 
 */
void getServoAngles(float roll, float pitch, float yaw, float x, float y, float z)
{
  //----------------- CONSTANTS NEEDED FOR COMPUTATION OBTAINED FROM DESIGN STAGE IN MATLAB ----------------------------
  //6 positions of the servo centers of rotation in xyz of base frame
  float b[][6] = {{-0.957555553898723,  0.957555553898723,  1.174615775982385, 0.217060222083663, -0.217060222083662, -1.174615775982386}, 
                  {-0.803484512108174, -0.803484512108174, -0.427525179157086, 1.231009691265260,  1.231009691265260, -0.427525179157086}, 
                  { 0.0,                0.0,                0.0,               0.0,                0.0,                0.0}};
  
  Serial.print("b = \n");
  printVector(b); 
  
  //6 positions of the servo top connections in xyz of platform frame
  float p[][6] = {{-0.147600951016891,  0.147600951016891,  0.798738727668022, 0.651137776651131, -0.651137776651131, -0.798738727668022},
                  {-0.837086590060377, -0.837086590060377,  0.290717121826818, 0.546369468233558,  0.546369468233559,  0.290717121826819},
                  { 0.0,                0.0,                0.0,               0.0,                0.0,                0.0}};
  
  Serial.print("\np = \n");
  printVector(p);   
  
  float s = 4.0;    //length of connection links
  float a = 0.625;  //length of servo horn
  //angle of the servo arm relative to the x axis of the base frame
  float beta[] = {0.0, 3.14159265358979, 2.09439510239320, 5.23598775598299, 4.18879020478639, 7.33038285837618};
  float h0 = 3.9665; //"home" height of the platform relative to the base

  Serial.print("\nbeta = \n");
  printVector(&beta[0], 6);
  //----------------- CONVERT THE ANGLES FROM DEGREES TO RADIANS FOR COMPUTATION ----------------------------
  float phi   = roll*PI/180;
  float theta = pitch*PI/180;
  float psi   = yaw*PI/180;

  Serial.print("\nphi, theta, psi = \n");
  Serial.print(phi);
  Serial.print(theta);
  Serial.print(psi);

  //compute rotation matrix for the planned euler sequence
  float R[][3] = {{cos(psi)*cos(theta),    (-sin(psi)*cos(phi))+(cos(psi)*sin(theta)*sin(phi)),    ( sin(psi)*sin(phi))+(cos(psi)*sin(theta)*cos(phi))},
                  {sin(psi)*cos(theta),    ( cos(psi)*cos(phi))+(sin(psi)*sin(theta)*sin(phi)),    (-cos(psi)*sin(phi))+(sin(psi)*sin(theta)*cos(phi))},
                  {-sin(theta),               cos(theta)*sin(phi),                                                      cos(theta)*cos(phi)}};

  Serial.print("\nR = \n");
  printVector(R); 
  //compute the translation vector
  float T[] = {x, y, (z+h0)};   

  Serial.print("\nT = \n");
  printVector(&T[0], 3); 
  //compute the vector q = T + R*p for the location of the connections in the base frame
  int i;
  float q[3][6];
  for(i = 0; i < 6; i++)
  {
    q[0][i] = T[0] + R[0][0]*p[0][i] + R[0][1]*p[1][i] + R[0][2]*p[2][i];
    q[1][i] = T[1] + R[1][0]*p[0][i] + R[1][1]*p[1][i] + R[1][2]*p[2][i];
    q[2][i] = T[2] + R[2][0]*p[0][i] + R[2][1]*p[1][i] + R[2][2]*p[2][i];     
  }

  Serial.print("\nq = \n");
  printVector(q); 
  
  //compute the necessary leg lengths (the distance from b to q) and save it as l for each servo. Take the norm to do it.
  float l[6];
  for(i = 0; i < 6; i++)
  {
    l[i] = sqrt(((q[0][i] - b[0][i])*(q[0][i] - b[0][i])) + ((q[1][i] - b[1][i])*(q[1][i] - b[1][i])) + ((q[2][i] - b[2][i])*(q[2][i] - b[2][i])));
  }

  Serial.print("\nl = \n");
  printVector(&l[0], 6); 
  //use trigonometry and the other constants you know to extract the angle of each servo
  float L[6];
  float M[6];
  float N[6];
  for(i = 0; i < 6; i++)
  {
    L[i] = (l[i]*l[i]) - ((s*s) - (a*a));
    M[i] = 2*a*((h0+p[2][i]) - b[2][i]);
    N[i] = 2*a*(cos(beta[i])*(p[0][i] - b[0][i]) + sin(beta[i])*(p[1][i] - b[1][i]));
    angles[i] = asin(L[i]/sqrt((M[i]*M[i]) + (N[i]*N[i]))) - atan(N[i]/M[i]);
  }
  Serial.print("\nL = \n");
  printVector(&L[0], 6);
  Serial.print("\nM = \n");
  printVector(&M[0], 6);
  Serial.print("\nN = \n");
  printVector(&N[0], 6);
  Serial.print("\nangles = \n");
}

/**************************************************************
 * This function reads 24 bytes off of the input buffer and then processes them 
 */
void receiveBytes()
{
  while(Serial.available() > 24)
    {
      char wastedChar = Serial.read();
    }
    // read the incoming byte:
    unsigned int bytesRecieved;
    bytesRecieved = Serial.readBytes(incomingBuffer, 24);
    int i;
    int command = 0;
    for(i = 0; i<24; i=i+4)
    {
      byte tempBytes[] = {incomingBuffer[i], incomingBuffer[i+1], incomingBuffer[i+2], incomingBuffer[i+3]};

      union {
          float float_variable;
          byte temp_array[4];
        } u;
      
      memcpy(u.temp_array, tempBytes, 4);
      commands[command] = u.float_variable;
      command++;
    }
    /*
    Serial.print("Roll = ");
    Serial.print(commands[0]);
    Serial.print('\n');
    Serial.print("Pitch = ");
    Serial.print(commands[1]);
    Serial.print('\n');
    Serial.print("Yaw = ");
    Serial.print(commands[2]);
    Serial.print('\n');
    Serial.print("X = ");
    Serial.print(commands[3]);
    Serial.print('\n');
    Serial.print("Y = ");
    Serial.print(commands[4]);
    Serial.print('\n');
    Serial.print("Z = ");
    Serial.print(commands[5]);
    Serial.print('\n');
    Serial.print('\n');
    */
}

void printVector(float myArray[3][6])
{
  int i, j;
  for(i = 0; i < 3; i++)
  {
    for(j = 0; j < 6; j++)
    {
      Serial.print(myArray[i][j]);
      Serial.print(", ");
    }
    Serial.print('\n');
  }
  Serial.print('\n');
}

void printVector(float myArray[3][3])
{
  int i, j;
  for(i = 0; i < 3; i++)
  {
    for(j = 0; j < 3; j++)
    {
      Serial.print(myArray[i][j]);
      Serial.print(", ");
    }
    Serial.print('\n');
  }
  Serial.print('\n');
}

void printVector(float* myArray, int rows)
{
  int i;
  for(i = 0; i < rows; i++)
  {
    Serial.print(myArray[i]);
    Serial.print(", ");
  }
  Serial.print('\n');
}
