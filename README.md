# OSCbox
OSC interface for remote controle

# OSC requests and responses
<<<<<<< HEAD
Arduino | direction | PC or MAC | action  
|---|---|---|---|
''/ping powerON'' |    ->     | ''/ping 2016'' | turn on whatever and respond when ready
''/ping powerOFF'' |    ->     | ''/ping 2016'' | turn of whatever and shutdown computer
''/button/[1-5] [0-1]'' |    ->     | no response necessary | map value to something
no response |    <-     | ''/LED/[1-5] [0-1]'' | Arduino sets state of LED
=======
Arduino               | direction |   PC or MAC           | action  
/ping powerON         |    ->     | /ping 2016            | turn on whatever and respond when ready
/ping powerOFF        |    ->     | /ping 2016            | turn of whatever and shutdown computer
/button/[1-5] [0-1]   |    ->     | no response necessary | map value to something
no response           |    <-     | /LED/[1-5] [0-1]      | Arduino sets state of LED
>>>>>>> 9cb60a1f41e54434c816be4cdd4b47501c314d1d
