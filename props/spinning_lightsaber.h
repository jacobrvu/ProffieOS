// Custom prop file for spinning-activated lightsaber with retraction motors
// For ProffieOS and Proffieboard V3.9
#ifndef PROPS_SPINNING_LIGHTSABER_H
#define PROPS_SPINNING_LIGHTSABER_H

#include "prop_base.h"
#include "../sound/hybrid_font.h"
#include "../motion/motion_util.h"

#define PROP_TYPE Spinning

class Spinning : public PROP_INHERIT_PREFIX PropBase {
public:
  Spinning() : PropBase() {}
  
  const char* name() override { return "Spinning"; }

  // State tracking
  bool is_on_ = false;
  enum SpinState {
    STOPPED,
    SPINNING,
    SLOWING
  };
  SpinState spin_state_ = STOPPED;
  
  // Pin definitions
  static const int LED_STRIP_1_PIN = bladePowerPin1;     // LED1 pin for LED strip 1
  static const int LED_STRIP_2_PIN = bladePowerPin2;    // LED2 pin for LED strip 2
  static const int RETRACTION_MOTOR_1_PIN = bladePowerPin3; // LED3 pin for retraction motor 1
  static const int RETRACTION_MOTOR_2_PIN = bladePowerPin4; // LED4 pin for retraction motor 2
  static const int CANE_ROTATION_MOTOR_PIN = bladePowerPin5; // LED5 pin for cane rotation motor
  static const int CLUTCH_PIN = bladePowerPin6;  // LED6 pin for clutch control
  
  // Thresholds for spin detection
  const float SPIN_THRESHOLD = 500.0f;  // Angular velocity threshold for activation (deg/s)
  const float SLOW_THRESHOLD = 100.0f;  // Angular velocity threshold for slow spin (deg/s)
  const float STOP_THRESHOLD = 10.0f;   // Angular velocity threshold for stopping (deg/s)
  
  bool rotating_chassis_spin_on_ = false;
  uint32_t clutch_return_time_ = 0;
  uint32_t sound_deactivation_time_ = 0;
  uint32_t activation_buffer_ = 0;
  uint32_t last_check_time_ = 0;

  void Setup() override {
    PropBase::Setup();
    
    // Initialize pins
    pinMode(LED_STRIP_1_PIN, OUTPUT);
    pinMode(LED_STRIP_2_PIN, OUTPUT);
    pinMode(RETRACTION_MOTOR_1_PIN, OUTPUT);
    pinMode(RETRACTION_MOTOR_2_PIN, OUTPUT);
    pinMode(CANE_ROTATION_MOTOR_PIN, OUTPUT);
    pinMode(CLUTCH_PIN, OUTPUT);

    // Turn everything off initially
    digitalWrite(LED_STRIP_1_PIN, LOW);
    digitalWrite(LED_STRIP_2_PIN, LOW);
    digitalWrite(RETRACTION_MOTOR_1_PIN, LOW);
    digitalWrite(RETRACTION_MOTOR_2_PIN, LOW);
    digitalWrite(CANE_ROTATION_MOTOR_PIN, LOW);
    digitalWrite(CLUTCH_PIN, LOW);

  }

  // Main loop function that gets called by ProffieOS
  void Loop() override {
    PropBase::Loop();

    // Get gyroscope data from IMU to detect rotation
    float rotation_speed = GetRotationSpeed();
    
    // Check for servo return timing
    if (millis() > clutch_return_time_ && clutch_return_time_ > 0) {
      digitalWrite(CLUTCH_PIN, LOW); // Return to left position
      clutch_return_time_ = 0; // Reset timer
    }

    // Check for deactivation sound
    if (sound_deactivation_time_ > 0 && millis() > sound_deactivation_time_) {
    SaberBase::TurnOff(SaberBase::OFF_NORMAL); // Play deactivation sound
      sound_deactivation_time_ = 0; // Reset timer
    }

    if (millis() - last_check_time_ >= 500) { 
	last_check_time_ = millis();

    // State machine for saber control
    switch (spin_state_) {
      case STOPPED:
        if (rotation_speed > SPIN_THRESHOLD && !is_on_ && millis() > activation_buffer_) {
          // Hilt is spinning fast enough - activate lightsaber
          ActivateSaber();
          spin_state_ = SPINNING;
	  activation_buffer_ = millis() + 8000;
        }
        break;
        
      case SPINNING:
        if (rotation_speed < SLOW_THRESHOLD && rotation_speed > STOP_THRESHOLD) {
          // Spinning is slowing - start retraction
          BeginRetraction();
          spin_state_ = SLOWING;
        }
        break;
        
      case SLOWING:
        if (rotation_speed < STOP_THRESHOLD && is_on_) {
          // Spinning has stopped - turn off saber
          DeactivateSaber();
          spin_state_ = STOPPED;
        }
        break;
    }
   }
  }

  // Function to check if the saber is currently activated
  bool IsOn() override {
    return is_on_;
  }

  // Helper function to get rotation speed from the IMU
  float GetRotationSpeed() {
    // Use ProffieOS's motion system to get gyroscope data
    Vec3 gyro = fusor.gyro();
    
    // Calculate the magnitude of rotation (in degrees per second)
    return sqrtf(gyro.x * gyro.x + gyro.y * gyro.y + gyro.z * gyro.z);
  }
  
  // Activate the lightsaber
  void ActivateSaber() {
    if (is_on_) return;

    is_on_ = true;
    
    // Play activation sound
    SaberBase::TurnOn();

    // Turn on LED strips (simple on/off, no PWM)
    digitalWrite(LED_STRIP_1_PIN, HIGH);
    digitalWrite(LED_STRIP_2_PIN, HIGH);
    
    // Move clutch right 5mm
    digitalWrite(CLUTCH_PIN, HIGH);
    
    // Schedule clutch to return after 500ms
    clutch_return_time_ = millis() + 500;
    
    // Turn on retraction motors with PWM at 20% power
    analogWrite(RETRACTION_MOTOR_1_PIN, 50);
    analogWrite(RETRACTION_MOTOR_2_PIN, 50);
  }
  
  // Begin retraction sequence when spinning slows
  void BeginRetraction() {
    
    // Schedule deactivation sound after 3000ms
    sound_deactivation_time_ = millis() + 3000;
    
    // Turn on cane rotation motor
    digitalWrite(CANE_ROTATION_MOTOR_PIN, HIGH);
    
    // Turn on both retraction motors at full power
    analogWrite(RETRACTION_MOTOR_1_PIN, 255);
    analogWrite(RETRACTION_MOTOR_2_PIN, 255);
  }
  
  // Deactivate the lightsaber
  void DeactivateSaber() {
    if (!is_on_) return;

    is_on_ = false;
    
    // Turn off LED strips
    digitalWrite(LED_STRIP_1_PIN, LOW);
    digitalWrite(LED_STRIP_2_PIN, LOW);
    
    // Turn off all motors
    digitalWrite(RETRACTION_MOTOR_1_PIN, LOW);
    digitalWrite(RETRACTION_MOTOR_2_PIN, LOW);
    digitalWrite(CANE_ROTATION_MOTOR_PIN, LOW);
    
    // Ensure servo is in left position
    digitalWrite(CLUTCH_PIN, LOW);
  }
  
};

#endif // PROPS_SPINNING_LIGHTSABER_H
