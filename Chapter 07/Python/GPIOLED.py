import RPi.GPIO as GPIO
import time

# Set the GPIO mode to BCM
GPIO.setmode(GPIO.BCM)

# Define the GPIO pin
led_pin = 12
# Set up the GPIO pin as an output
GPIO.setup(led_pin, GPIO.OUT)

try:
    while True:
        # Turn the LED on
        GPIO.output(led_pin, GPIO.HIGH)
        print("LED On")
        time.sleep(1)  # 1 second delay

        # Turn the LED off
        GPIO.output(led_pin, GPIO.LOW)
        print("LED Off")
        time.sleep(1)  # 1 second delay

except KeyboardInterrupt:
    pass

# Clean up the GPIO configuration
GPIO.cleanup()