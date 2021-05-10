This is the solution of the Driver Assignment in my Embedded Programming Course. This assignment tasked us to create a 'Misc' Driver to Flash Morse Code.
The encoding is taken from https://en.wikipedia.org/wiki/Morse_code  
<br />
This driver supports read and writing so when you write a message to the drivers nodes, done via echo it will translate the message into morse code
and then flash the message to the LEDS using the morse-code led-trigger created by the driver. So please set the LED trigger on your beaglebone to 'morse-code' to see the propper flashing occur!

<br />
Sample input and output   <br />
(bbg)$ echo 'SOS' | sudo tee /dev/morse-code   <br />
(bbg)$ sudo cat /dev/morse-code <br />
...---...    <br />
   <br />
This was tested under with Target Kernel 5.3.7  
To load the driver: instmod morsecode.ko  
Ensure your botted kernel version matches the vermagic field of the driver with: modinfo morsecode.ko  
To remove the module: rmmod morsecode.ko  
  
    
