# Banana-Pi-BPI-P2-Zero-H3-websocket-server
websocket server to start stepper motor and stop by encoder value
.
this code receives the number of steps for the motor via the websocket, starts the motor rotation and reads data from the encoder. when the number of steps is reached, the motor stops, and continues to listen to the socket.
