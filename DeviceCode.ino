#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

#define STATUS_COLOR  LGRAYBLUE
#define COLOR1  BRRED

// LED Sensor setup
int ledpins[4] = {2,3,4,5};
String nmValues[4] = {"670nm", "810nm", "850nm", "950nm"};

// Optical Reader  Setup
const int reader = A3; // A3 port will recieve data from OPT101 Optical Sensor (with built in Trans-Impedance Amplifier)

// If set to true debug messages will be printed on the Serial Monitor
boolean debug = false;

// Variable to store the difference in the high and low voltage signal
// AC voltage = Max Voltage - Min Voltage 
double highLowDiff[4];

// Average of min volt signal using 40 values (to increase data accuracy)
int min[4];


// Arduino Setup function
void setup()
{
    Serial.begin(9600); // Bluetooth will communicate on 9600 
    // Restart button logic
    pinMode(A5, INPUT_PULLUP);
  
    // Setup 4 ports using array ledpins, as output ports for leds
    for (int i=0; i<4; ++i)
    {
      pinMode(ledpins[i], OUTPUT);
    }
    
    // Initialize the screen
    Config_Init();
    LCD_Init();
    // Function to draw initial screen
    drawScreen();
}


// Arduino loop function
void loop()
{
    // first initialize the screen 
    Paint_DrawString_EN(54, 150, "             " , &Font16,  BLACK, STATUS_COLOR);
    Paint_DrawString_EN(50, 90, "Calculating.. ", &Font16,  BLACK, COLOR1);
    // Wait for 1 sec for system to complete the initialization 
    delay(1000);
  
    // Read the values from the OPT101 sensor when 4 leds are turned on (one by one) for 4 seconds each.
    readValues();
    
    //updateScreen;
    Paint_DrawString_EN(52, 150, "Processing.." , &Font16,  BLACK, STATUS_COLOR);
    
    // Calculate and display the results
    calculateResult();
}


int readValues()
{
   // Reading 4 values using 670nm, 810nm, 850nm, and 950nm LED lights
   long int totalValue;
   int count;
 
  // Turn on each led one by one and read 40 values for each LED (670nm,810nm,850nm,950nm)
  for ( int i = 0; i < 4; ++i) 
  {
      int high = 0;
      int low = 1023;
      //totalValue = 0;
      count=0;
      
      int pinNumber = ledpins[i]; // get the pin number one by one from the ledpins array 
      digitalWrite(pinNumber, HIGH); //Turn on one led at a time

      // Display on the LCD screen which led is on
      char disp[14];
      (nmValues[i]+ " Source").toCharArray(disp, 14);
      Paint_DrawString_EN(52, 150, disp , &Font16,  BLACK, STATUS_COLOR);
  
      // Keep repeating until 40 values are read
      while(true)
      {
         int value = analogRead(reader);
         // Find the Min and Max value for calculating coefficient
         if (high < value)
         {
            high = value;
         }
         if (low > value)
         {
            low = value;
         }
  
         if (count >= 40) // collect 40 values, it will take approx 2 seconds
         {          
           highLowDiff[i]= high - low;
           min[i] = low;
           break;
         }
         else
         {
            count++;
            delay(100); 
         }
       }
        // Turn off the current LED and wait for 1 sec before turning-on next LED
        digitalWrite(pinNumber, LOW);
        delay(1000); 
    }
}

String calculateResult()
{
    // Well-known values of extinction coefficients of oxy-hemoglobin and deoxy-hemoglobin
    // are used and then the inverse of these values is calculated and added to the 
    // following arrays
    double deoxy_coefficient[4] = {2795.12, 717.08,691.32,602.24};
    double oxy_coefficient[4] = {294, 864,1054,1204};
  
    // Display strings for severity of anemia 
    String values[] = {" All Good :)", " Mild Anemia ", " Mod. Anemia ", "Severe Anemia", "Extre. Anemia"};
  
    double hValue = 0; // variable to store final hemoglobin value
    String displayValue = "";
    int displayColor = 0;
    double Hb = 0;
    double HbO2 = 0;
   
    for ( int i = 0; i < 4; ++i) 
    {
        // Calculate PPG ration for each wavelength 
        double PPG_Ratio = log(highLowDiff[i]/min[i]);
        
        Hb = Hb + (PPG_Ratio*deoxy_coefficient[i]);
        HbO2 = HbO2 + (PPG_Ratio*oxy_coefficient[i]);
    }
    // Final hemoglobin value is Hb + HbO2
    hValue = Hb + HbO2;

    // Display string as per the value of hemoglobin 
    if (hValue >= 12.0)
    {
      displayValue = values[0];
      displayColor = GREEN;
    }
    else if (hValue < 12.0 && hValue >= 10.0)
    {
      displayValue = values[1];
      displayColor = YELLOW;
    }
    else if (hValue < 10.0 && hValue >= 8.0)
    {
      displayValue = values[2];
      displayColor = YELLOW;
    }
    else if (hValue < 8.0 && hValue >= 6.5)
    { 
      displayValue = values[3];
      displayColor = RED;
    }
    else
    {
      displayValue = values[4];
      displayColor = RED;
    }

     if (debug)
     {
          Serial.println(" hValue:" + String(hValue));
          Serial.println(" displayValue:" + displayValue);
     }
  
    // Display the value of hemoglobin on device's LCD screen in grams/deciliter
    char disp1[14];
    (String(hValue)+" g/dl").toCharArray(disp1, 14);
    char disp[14];
    displayValue.toCharArray(disp, 14);
    Paint_DrawString_EN(50, 150, "             " , &Font16,  BLACK, STATUS_COLOR);
    Paint_DrawString_EN(50, 90, "              ", &Font16,  BLACK, COLOR1);

    // Color code the string based on if the Hemoglobin is normal, low or too low
    Paint_DrawString_EN(70, 90, disp1, &Font16,  BLACK, COLOR1);
    Paint_DrawString_EN(50, 150, disp , &Font16,  BLACK, displayColor);

    // Prepare the output string and send to Mobile App using bluetooth  
    String output = String(hValue) +" g/dl" +" , "+ displayValue; 
    Serial.println(output);
    delay(200);
  
    while (true)
    {
      // Measure Again when push button is pressed
      if (digitalRead(A5) == LOW)
      {
        loop();
      }
      else
      {
        // Wait 
        Serial.println(output);
        delay(200);
      }
    }
}


void drawScreen()
{
  LCD_SetBacklight(1000);
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, BLACK);
  Paint_Clear(BLACK);
  Paint_DrawCircle(120, 120, 120, CYAN , DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  //Paint_DrawString_EN(60, 30, "Hemo", &Font24, BLACK,  COLOR1);
  Paint_DrawString_EN(40, 60, "Hemoglobin", &Font24,  BLACK, COLOR1);
  Paint_DrawString_EN(75, 90, "-----", &Font24,  BLACK, COLOR1);
  Paint_DrawRectangle( 40, 130, 200, 190, WHITE , DOT_PIXEL_2X2, DRAW_FILL_EMPTY );
  Paint_DrawString_EN(52, 150, "Initializing" , &Font16,  BLACK, STATUS_COLOR);
  delay(1000);
}

