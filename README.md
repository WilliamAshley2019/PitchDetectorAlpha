This is a simple pitch detector vst3 plugin. It is still taking up quite a bit of cpu. I intend to add adjustable polling rates to further allow CPU load 
to be reduced on the FFT processing overhead.  The idea was to set the range at about what MIDI is c0 to g10. The next step is to include logging and the ability to export the log as a .mid file. 
The next step after that is to enable polyphonic detect and more precise fundamantal and harmonic detection which will take a little thinking to figure out what the best method of doing that is likely related to spectrum analysis.

I will do a full readme once this project has developed a bit more. 

JUCE v8.09  plugin basics + dsp.


Likely need to get some craft way of determining the fundamental of dynamic samples... currently seems to work well with sin wave osc up to about c7 then it starts getting strange results I am thinking perhaps the detection pattern needs to somehow be scaled with pitch to detect correctly the same is true with very low frequnecies I need to see if there is some type of processing ratio that should be applied to low and high frequencies.


Added a basic log - the intent is to match it to midi ticks and do real time recording with it so that the midi can be exported as a standard .mid file. 
Monophinic detection.. and it has an averaging of sorts of the frequencies. I think at some point I will need to detect spectrum strength to have more precise analysis but need to look into that a bit more.

Q. How can we extend the clear detection range without adding significant computational complexity.

Q. How much extra processing is added by analyzing seperate spectrum ranges ex. low mid high frequency detection bands. 

Q. What gradient of power exists for generalized human undertsanding of the fundamental versus harmonics - for power ratios - human detection of the fundamnetal needs to be represented clearly rather than only the accoustical / physical model of the frequencies if it is going to be an effective tool, what methods can be used to introduce a psychoaccoustic element to 
note detection.
