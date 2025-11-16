This is a simple pitch detector vst3 plugin. It is still taking up quite a bit of cpu. I intend to add adjustable polling rates to further allow CPU load 
to be reduced on the FFT processing overhead.  The idea was to set the range at about what MIDI is c0 to g10. The next step is to include logging and the ability to export the log as a .mid file. 
The next step after that is to enable polyphonic detect and more precise fundamantal and harmonic detection which will take a little thinking to figure out what the best method of doing that is 
likely related to spectrum analysis.
