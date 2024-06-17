import RPi.GPIO as GPIO
import time

# Define the GPIO pins
enable1_pin = 18
enable2_pin = 12
in1_pin = 27
in2_pin = 23
in3_pin = 24
in4_pin = 25

# Set the GPIO mode to BCM
GPIO.setmode(GPIO.BCM)

# Set up the GPIO pins
GPIO.setup(enable1_pin, GPIO.OUT)
GPIO.setup(enable2_pin, GPIO.OUT)
GPIO.setup(in1_pin, GPIO.OUT)
GPIO.setup(in2_pin, GPIO.OUT)
GPIO.setup(in3_pin, GPIO.OUT)
GPIO.setup(in4_pin, GPIO.OUT)

# Create PWM objects for speed control
motor1_pwm = GPIO.PWM(enable1_pin, 100)
motor2_pwm = GPIO.PWM(enable2_pin, 100)

# Start PWM with 0% duty cycle
motor1_pwm.start(0)
motor2_pwm.start(0)

# Function to change the direction of the motor
def set_motor_direction(in1, in2, in3, in4):
    GPIO.output(in1_pin, in1)
    GPIO.output(in2_pin, in2)
    GPIO.output(in3_pin, in3)
    GPIO.output(in4_pin, in4)

# Function to control motor speed
def set_motor_speed(speed):
    motor1_pwm.ChangeDutyCycle(speed)
    motor2_pwm.ChangeDutyCycle(speed)

try:
    while True:
        direction = input("Enter direction (forward, backward, stop): ")
        speed = int(input("Enter speed (0-100): "))

        if direction == "forward":
            set_motor_direction(1, 0, 1, 0)
        elif direction == "backward":
            set_motor_direction(0, 1, 0, 1)
        elif direction == "stop":
            set_motor_direction(0, 0, 0, 0)
        
        set_motor_speed(speed)

except KeyboardInterrupt:
    pass


# Cleanup
motor1_pwm.stop()
motor2_pwm.stop()
GPIO.cleanup()
