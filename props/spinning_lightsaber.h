// Custom prop file for spinning-activated lightsaber with retraction motors
// For ProffieOS and Proffieboard V3.9
#ifndef PROPS_SPINNING_LIGHTSABER_H
#define PROPS_SPINNING_LIGHTSABER_H

#include "prop_base.h"
#include "sound/hybrid_font.h"
#include "motion/motion_util.h"

#undef PROP_TYPE
#define PROP_TYPE SpinningLightsaberProp

class SpinningLightsaberProp : public PROP_INHERIT_PREFIX PropBase {
public:
  SpinningLightsaberProp() : PropBase() {}
  
  const char* name() override { return "SpinningLightsaber"; }

  // State tracking
  bool is_on_ = false;
  enum SpinState {
    STOPPED,
    SPINNING,
    SLOWING
  };
  SpinState spin_state_ = STOPPED;
  
  // Pin definitions
  static const int LED_STRIP_1_PIN = bladePowerPin;     // LED1 pin for LED strip 1
  static const int LED_STRIP_2_PIN = bladePowerPin2;    // LED2 pin for LED strip 2
  static const int RETRACTION_MOTOR_1_PIN = bladePowerPin3; // LED3 pin for retraction motor 1
  static const int RETRACTION_MOTOR_2_PIN = bladePowerPin4; // LED4 pin for retraction motor 2
  static const int CANE_ROTATION_MOTOR_PIN = bladePowerPin5; // LED5 pin for cane rotation motor
  
  static const int SERVO_PIN = blade5Pin;  // Using Free 1 pin for servo control
  static const int SERVO_LEFT_POS = 2000;  // Servo value for left position
  static const int SERVO_RIGHT_POS = 30000; // Servo value for right position
  
  // Thresholds for spin detection
  const float SPIN_THRESHOLD = 200.0f;  // Angular velocity threshold for activation (deg/s)
  const float SLOW_THRESHOLD = 100.0f;  // Angular velocity threshold for slow spin (deg/s)
  const float STOP_THRESHOLD = 30.0f;   // Angular velocity threshold for stopping (deg/s)
  
  bool rotating_chassis_spin_on_ = false;
  uint32_t servo_return_time_ = 0;
  uint32_t sound_deactivation_time_ = 0;

  void Setup() override {
    PropBase::Setup();
    
    // Initialize pins
    pinMode(LED_STRIP_1_PIN, OUTPUT);
    pinMode(LED_STRIP_2_PIN, OUTPUT);
    pinMode(RETRACTION_MOTOR_1_PIN, OUTPUT);
    pinMode(RETRACTION_MOTOR_2_PIN, OUTPUT);
    pinMode(CANE_ROTATION_MOTOR_PIN, OUTPUT);
    
    // Turn everything off initially
    digitalWrite(LED_STRIP_1_PIN, LOW);
    digitalWrite(LED_STRIP_2_PIN, LOW);
    digitalWrite(RETRACTION_MOTOR_1_PIN, LOW);
    digitalWrite(RETRACTION_MOTOR_2_PIN, LOW);
    digitalWrite(CANE_ROTATION_MOTOR_PIN, LOW);
    
    // Initialize servo
    LSanalogWriteSetup(SERVO_PIN, PWM_USECASE::SERVO);
    LSanalogWrite(SERVO_PIN, SERVO_LEFT_POS); // Start in left (retracted) position
  }

  // Function to check if the saber is currently activated
  bool IsOn() override {
    return is_on_;
  }

  // Main loop function that gets called by ProffieOS
  void Loop() override {
    PropBase::Loop();
    
    // Get gyroscope data from IMU to detect rotation
    float rotation_speed = GetRotationSpeed();
    
    // Check for servo return timing
    if (millis() > servo_return_time_) {
      LSanalogWrite(SERVO_PIN, SERVO_LEFT_POS); // Return to left position
      servo_return_time_ = 0; // Reset timer
    }

    // Check for deactivation sound
    if (sound_deactivation_time_ > 0 && millis() > sound_deactivation_time_) {
    hybrid_font.PlayCommon(&SFX_poweroff); // Play deactivation sound
      sound_deactivation_time_ = 0; // Reset timer
    }
    
    // State machine for saber control
    switch (spin_state_) {
      case STOPPED:
        if (rotation_speed > SPIN_THRESHOLD && !is_on_) {
          // Hilt is spinning fast enough - activate lightsaber
          ActivateSaber();
          spin_state_ = SPINNING;
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
        if (rotation_speed < STOP_THRESHOLD) {
          // Spinning has stopped - turn off saber
          DeactivateSaber();
          spin_state_ = STOPPED;
        }
        break;
    }
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
    hybrid_font.PlayCommon(&SFX_boot);
    
    // Turn on LED strips (simple on/off, no PWM)
    digitalWrite(LED_STRIP_1_PIN, HIGH);
    digitalWrite(LED_STRIP_2_PIN, HIGH);
    
    // Move servo right 5mm
    LSanalogWrite(SERVO_PIN, SERVO_RIGHT_POS);
    
    // Schedule servo to return after 500ms
    servo_return_time_ = millis() + 500;
    
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
    LSanalogWrite(SERVO_PIN, SERVO_LEFT_POS);
  }
  
};

#endif // PROPS_SPINNING_LIGHTSABER_H
