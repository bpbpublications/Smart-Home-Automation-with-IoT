#import libraries  
import cv2
from picamera2 import Picamera2

#initialize pi camera
picam2 = Picamera2()
picam2.preview_configuration.main.size = (1280,720)
picam2.preview_configuration.main.format = "RGB888"
picam2.preview_configuration.align()
picam2.configure("preview")

#start camera to capture video
picam2.start()

#process video
while True:
    #capture the frames from the video feed
    frame = picam2.capture_array()

    #show the video preview in a window
    cv2.imshow("Camera", frame)
    
    # Break the loop if 'Esc' key is pressed
    if cv2.waitKey(1) & 0xFF == 27:
        break

cv2.destroyAllWindows()
