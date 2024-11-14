// Arduino code for Colin's glider simulator

// https://github.com/MHeironimus/ArduinoJoystickLibrary 
#include <Joystick.h>


#define BUTTON_COUNT 8

#define BASE_POT A1
#define BRAKE_AXIS A0
#define BRAKE_BUTTON 7
#define BASE_PULL 10 
#define BASE_TOGGLE 6
#define GEAR_DOWN 4
#define GEAR_UP 5
#define TOLERANCE 3

// Initialize Joystick with specific settings
Joystick_ Joystick(//0x88, // The joystick and airbrakes
  JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_GAMEPAD, //JOYSTICK_TYPE_MULTI_AXIS,
  BUTTON_COUNT, //buttons
  0, //hat switches
  false, false, false,  // x y z = brake, vario
  true, true, false, // rx ry rz =  brake, pot, null
  false, false, false, false, false); // rudder throttle accel brake steer


// Button states and other control variables
float brake_min = 450.0;
float brake_max = 1024.0;

int lastButtonState[BUTTON_COUNT] = {0};
int currentButtonState[BUTTON_COUNT] = {0};
int last_toggle_state = -1;
int last_read = -1;
int trim_value = 512;
int last_gear_up_state = 0;
int last_gear_down_state = 0;

long time_both_buttons_pressed = -1; 
int function_mode_selection = -1;
int trim_axis_alone = 0;
int gear_buttons_transitory = 0;
int pot_does_buttons = 0;

//#define testing 1
#ifdef testing
  int loops = 0;
#endif

void setup() {
  pinMode(BASE_TOGGLE, INPUT_PULLUP);
  pinMode(BASE_PULL, INPUT_PULLUP);
  pinMode(BRAKE_BUTTON, INPUT_PULLUP);
  pinMode(GEAR_DOWN, INPUT_PULLUP);
  pinMode(GEAR_UP, INPUT_PULLUP);

#ifdef testing
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Init");
#endif

  Joystick.begin();
  last_toggle_state = digitalRead(BASE_TOGGLE);
}

/* This is for changing settings on the device.
 *  Not that useful for no_stick.
 *  But we can calibrate the brakes.
 */
void function_mode() {
  // Function for setting mode based on button states
  brake_min = 2000;
  brake_max = -100;
  #ifdef testing
  Serial.println("Config mode");
  #endif
  
  while(1) {
    int pull_switch = !digitalRead(BASE_PULL);
    int toggle = digitalRead(BASE_TOGGLE);
    int gear_down = !digitalRead(GEAR_DOWN);
    int gear_up = !digitalRead(GEAR_UP); 
    int brake_button = !digitalRead(BRAKE_BUTTON); 
    int brake  = analogRead(BRAKE_AXIS);
    if(brake < brake_min) {
      brake_min = brake;
    }
    if(brake > brake_max) {
      brake_max = brake;
    }
    
    if(brake_button) {
      gear_buttons_transitory = !gear_buttons_transitory;
      break;
    } else if(pull_switch) {
      pot_does_buttons = !pot_does_buttons;
      break;
    }
    delay(50);  
  }
  time_both_buttons_pressed = -1;
}

void loop() {
  // Process analog inputs
  int potvalue  = analogRead(BASE_POT);
  int brake  = analogRead(BRAKE_AXIS);

  // Process digital inputs
  int pull_switch = !digitalRead(BASE_PULL);
  int toggle = digitalRead(BASE_TOGGLE);
  int gear_down = !digitalRead(GEAR_DOWN);
  int gear_up = !digitalRead(GEAR_UP); 
  int brake_button = !digitalRead(BRAKE_BUTTON); 
  int pot_button1 = 0;
  int pot_button2 = 0;

  if(gear_down && gear_up && time_both_buttons_pressed == -1) {
    time_both_buttons_pressed = millis();
  } else if(time_both_buttons_pressed > 500) {
    function_mode();
    return;
  } else {
    time_both_buttons_pressed = -1;
  }

  int toggle_a = 0;
  int toggle_b = 0;
  if (toggle != last_toggle_state) {
    toggle_a = !toggle;
    toggle_b = toggle;    
  }
  last_toggle_state = toggle; 

  if(gear_buttons_transitory) {
    int lgd = gear_down;
    int lgu = gear_up;
    
    if(gear_down == last_gear_down_state) gear_down = 0;
    if(gear_up == last_gear_up_state) gear_up = 0;
    last_gear_up_state = lgu;
    last_gear_down_state = lgd;
  }

  currentButtonState[0] = brake_button;
  currentButtonState[1] = gear_up;
  currentButtonState[2] = gear_down;
  currentButtonState[3] = pull_switch;
  currentButtonState[4] = toggle_a;
  currentButtonState[5] = toggle_b;
    
  if(pot_does_buttons) {
    #ifdef testing
    Serial.println("BUTTON");
    #endif
     if (potvalue - last_read > TOLERANCE)
        pot_button1 = 1;
     else if (potvalue - last_read < -TOLERANCE)
        pot_button2 = 1;
     last_read = potvalue; 
  }
  currentButtonState[6] = pot_button1;
  currentButtonState[7] = pot_button2;

  // Update button states
  for (int index = 0; index < BUTTON_COUNT; index++) {
    if (currentButtonState[index] != lastButtonState[index]) {
      Joystick.setButton(index, currentButtonState[index]);
      lastButtonState[index] = currentButtonState[index];
    }
  }
  
  // Calculate and set the brake value
  float brk = (brake*1.0 - brake_min) / (brake_max - brake_min);
  brake = (int)(brk * 1023);
  Joystick.setRxAxis(brake);
  Joystick.setRyAxis(1023 - potvalue);

#ifdef testing
  Serial.print("Brake axis: ");
  Serial.println(brake);
  Serial.print("Pot axis: ");
  Serial.println(1023 - potvalue);
  Serial.print("Buttons: ");
  for(int i = 0; i < BUTTON_COUNT; i++) {
    Serial.print(" ");
    Serial.print(currentButtonState[i]);
  }
  Serial.println("///");
#endif

  delay(50);
#ifdef testing
  delay(100);
#endif
}
