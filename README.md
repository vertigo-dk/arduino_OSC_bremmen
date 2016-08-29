
# OSC requests and responses
| Arduino | direction | PC or MAC | action |
| :---: | :---: | :---: | :---: |
| ''/ping powerON'' | -> | ''/ping 2016'' | turn on whatever and respond when ready |
| ''/ping powerOFF'' | -> | ''/ping 2016'' | turn of whatever and shutdown computer |
| ''/button/[1-5] [0-1]'' | -> | no response necessary | map value to something |
| no response | <- | ''/LED/[1-5] [0-1]'' | Arduino sets state of LED |

