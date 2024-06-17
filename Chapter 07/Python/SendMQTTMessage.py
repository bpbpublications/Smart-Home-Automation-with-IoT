#import libraries
import json
import paho.mqtt.client as mqtt

#define connection parameters for MQTT
MQTT_ADDRESS:str = 'smarty.local'
MQTT_TOPIC_SWITCH: str = 'smarty/switchcontrol/mainstudy'
MQTT_CLIENT_ID:str = 'MQTTDataBridge'
MQTT_PORT:int = 1883

#define switch ID and switch state
switchID = 'MAINSTUDY.Switch4'
switchState = 1


#function to call on connect
def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC_SWITCH)


#function to call when a message to the subscribed topic is received from MQTT Broker
def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    print(msg.topic + ' ' + str(msg.payload))


#function to publish MQTT message    
def _publish_message(topic, switchID, switchState):
    
    #specify the data object to create JSON
    data = {}
    data['SwitchID'] = switchID
    data['SwitchState'] = switchState
    
    #create a JSON from the data 
    payload = json.dumps(data)
    
    #publish MQTT message 
    mqtt_client.publish(MQTT_TOPIC_SWITCH, payload)
    return
 
#connect to MQTT broker
mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
mqtt_client.connect(MQTT_ADDRESS, MQTT_PORT)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.loop_start()  # start the loop

#publish a MQTT message
_publish_message(MQTT_TOPIC_SWITCH, switchID, switchState)
