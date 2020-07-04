# Arduino Data Logger

## Description

The `Arduino Data Logger` consists of two programs:

1. An arduino sketch for the Arduino: 

    - Receives commands from the client side program, to start/stop logging for a sensor or request data from it.
    - Has simulated sensors as I didn't have any actual sensors to test with. 
      Right now they don't continuously generate new data but that would be easy to change.

2. A client side program written in Python: 

    - Communicates with the arduino via serial
    - Switch between sensors, start/stop logging, request data, print data
    - Parse the response data for further processing or printing it out.

Right now the unit of data being generated and transferred is called a `data point` which consists of a `header` and a `body`, both of which
are integers, but I tried to write it in a way so extending it to accept any number of integer fields hopefully isn't too much
work.

## Use

According to my *very limited* tests it works, but further testing would very likely reveal bugs and certainly reveal
limitations in how I've done it, but if you're doing exactly this maybe it can be useful, if nothing else to serve as
inspiration for your own solution.

Tried to leave some hopefully useful comments in.

<a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/">Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License</a>.
