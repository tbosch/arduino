# TODOs


- Remote control works a bit:
  * GET requests seem to be slow! (About 500ms between update of left and right).
    ==> Send them in 1 request
    ==> add multiple request parameters, not just 1 key/value pair.
  
  * Somehow the motors seem to switch direction while controlling??
    ==> print values again, something is wrong here!
  

- Try to allocate the memory not via malloc,
  But via a global variable
  ==> as we seem to require PSRam, which slows down the decode!
  Globale Variablen verwenden 66116 Bytes (20%) des dynamischen Speichers,
  261564 Bytes für lokale Variablen verbleiben. Das Maximum sind 327680 Bytes.
  ==> should work!!
