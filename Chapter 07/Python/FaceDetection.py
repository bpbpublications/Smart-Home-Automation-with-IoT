#import libraries
import cvzone
from cvzone.FaceDetectionModule import FaceDetector
import cv2
from picamera2 import Picamera2
import json
import paho.mqtt.client as mqtt

#MQTT connection parameters
MQTT_ADDRESS:str = 'smarty.local'
MQTT_TOPIC_SWITCH: str = 'smarty/switchcontrol/mainstudy'
MQTT_CLIENT_ID:str = 'MQTTDataBridge'
MQTT_PORT:int = 1883

#switch to control
switchID = 'MAINSTUDY.Switch4'
switchState = 1

#function to publish MQTT message
def _publish_message(topic, switchID, switchState):
    data = {}
    data['SwitchID'] = switchID
    data['SwitchState'] = switchState
    payload = json.dumps(data)
    print(payload)
    mqtt_client.publish(MQTT_TOPIC_SWITCH, payload)
    return
    
#connect to MQTT Broker
mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
mqtt_client.connect(MQTT_ADDRESS, MQTT_PORT)
mqtt_client.loop_start()  # start the loop


# Initialize the webcam
picam2 = Picamera2()
picam2.preview_configuration.main.size = (980,460)
picam2.preview_configuration.main.format = "RGB888"
picam2.preview_configuration.align()
picam2.configure("preview")
picam2.start()
    
# Initialize the FaceDetector object
# minDetectionCon: Minimum detection confidence threshold
# modelSelection: 0 for short-range detection (2 meters), 1 for long-range detection (5 meters)
detector = FaceDetector(minDetectionCon=0.5, modelSelection=0)

# Run the loop to continually get frames from the webcam
while True:
    # Read the current frame from the webcam
    # success: Boolean, whether the frame was successfully grabbed
    # img: the captured frame
    img = picam2.capture_array()

    # Detect faces in the image
    # img: Updated image
    # bboxs: List of bounding boxes around detected faces
    img, bboxs = detector.findFaces(img, draw=False)

    # Check if any face is detected processing each frame in a loop
    if bboxs:
        # Loop through each bounding box
        for bbox in bboxs:
            # bbox contains 'id', 'bbox', 'score', 'center'

            # ---- Get Data  ---- #
            center = bbox["center"]
            x, y, w, h = bbox['bbox']
            score = int(bbox['score'][0] * 100)
            
            #publish message on face detection when the confidence level is more than 95%
            if score > 95:
                _publish_message(MQTT_TOPIC_SWITCH, switchID, switchState)
            
            # ---- Draw Data  ---- #
            cv2.circle(img, center, 5, (255, 0, 255), cv2.FILLED)
            cvzone.putTextRect(img, f'{score}%', (x, y - 10))
            cvzone.cornerRect(img, (x, y, w, h))

    # Display the image in a window named 'Image'
    cv2.imshow("Image", img)

    if cv2.waitKey(1) & 0xFF==27 :
        break
    
cv2.destroyAllWindows()
