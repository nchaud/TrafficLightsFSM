Traffic Lights FSM
=======================

This is an embedded system to control traffic lights using a Moore FSM (Finite State Machine).

It's fairly simple (but could be scaled up easily enough) with one set of traffic lights to control Eastbound traffic and one set to control Northbound traffic. There's also a Pedestrian Walking function to stop cars if there's someone waiting. It does cool stuff like rotate between the 2 traffic streams and walking if all 3 sensors detect input. In this case the 'sensors' are switches connected to ports PE0-2 and the traffic lights are outputs connected to ports PB0-5 and the walk/dont walk light is connected to ports PF1, PF3.

Entry point and FSM implementation is in TableTrafficLight.c

Built using the Keil IDE and targets the [TI TM4C123G board](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c123gxl.html?keyMatch=TM4C123g&tisearch=Search-EN). To run this on the board, you'll also need

- 3 switches as described above
- 8 LEDs (2x traffic lights + (dont) walk)
- 3.3V to power rails
- Breadboard
- and of course, Wiring

