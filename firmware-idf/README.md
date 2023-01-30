# ESP32-IDF based firmware for SnappySense

This is a work in progress.  It ports the SnappySense firmware from the Arduino framework to the
ESP32-IDF framework, which is lower level, higher quality, and C-based.

I'm not using VS Code for this, just emacs; YMMV.  Do check in anything interesting.

If the esp-idf SDK is installed in `~/esp/esp-idf` then remember to
```
  . ~/esp/esp-idf/export.sh
```
in your shell, then
```
  idf.py help
  idf.py build
  idf.py flash monitor
``
etc.

More documentation will appear.
