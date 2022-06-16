# TODOs

- Try to allocate the memory not via malloc,
  But via a global variable
  ==> as we seem to require PSRam, which slows down the decode!
  Globale Variablen verwenden 66116 Bytes (20%) des dynamischen Speichers,
  261564 Bytes für lokale Variablen verbleiben. Das Maximum sind 327680 Bytes.
  ==> should work!!

- When the car is switched off and on, then the remote doesn't work anymore.
  Is the reason on the car or on the remote?
- The sonic sensors are really flaky.
  E.g. when approaching a wall, the back sensor also starts to show something, which it souldn't.

- Add safety stop back, directly on the car.
  For case when remote gives us a wrong signal / somebody interferes
