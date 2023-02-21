-*- fill-column: 100 -*-

I'll describe this in terms of tasks, but frequently the tasks can be integrated the main loop, as
they are in IDF.

scani2c disappears, probably.  It can be incorporated in a separate diagnostic firmware?

Things that are going on:

- wifi client up and down
- mqtt communication (TLS)
- time service communication (TCP)
- monitoring window:
  - wait for warmup
  - measure motion and sound on independent tasks
  - take readings, integrate them
  - report back
- rest
- slideshow
- button presses
- switch from monitoring to slideshow and back
- switch from operating mode to ap mode and back?
- "sleep": power down and up peripherals, block until time or button press

The list gives rise to modes and a flow chart between modes, but some modes are overlapping, in
particular, slideshow mode and wifi connection overlaps several others.

**NOTE ===>** It's actually an open question whether spinning down the wifi and letting the device
rest will be enough to give the temperature sensor a chance to do a proper reading.

IMO the temperature sensor really needs to be offboard from the CPU.  But since that sensor also is
light and UV, it needs to be "exposed".  So really we want a design where the CPU is on a different
board.  Consider taking a second v1.1 mainboard and mounting it below the current one with the CPU
on the bottom; or mounting it with the CPU on top but with long standoffs.  Either way there will be
some wires, but the CPU becomes separate from the sensors fairly well.

Communication will be a problem because there's more than I2C - we also have the PIR and MEMS
signals.  These sensor can sit on the CPU board but it's awkward .




We can have a *monitoring task* that waits for a startup event.  During the monitoring window it
captures values into a private SnappySenseData structure, and posts clock ticks to itself.  When the
time is right it receives a stop event, at which point it posts an event back to the main event loop
with the data.  THE SENSORS ARE PRIVATE TO THE MONITORING TASK.

What about WiFi?  Is there a task that maintains the wifi connection so that we don't need to bring
it up and down repeatedly?

There is a *time service task* that coordinates with the time server.  When run, it attempts to
connect, to configure time, and so on, and when it's done it sends a message to the main event queue
that it got the time or that it failed (maybe).

There can be an *upload task* that performs data uploads.  It is sent a startup event

The main loop runs the UI and orchestrates other operations.

Basically, 






diff --git a/firmware-arduino/src/main.cpp b/firmware-arduino/src/main.cpp
index 0a104cf..5214074 100644
--- a/firmware-arduino/src/main.cpp
+++ b/firmware-arduino/src/main.cpp
@@ -192,6 +192,7 @@ void setup() {
 #ifdef MQTT_UPLOAD
   sched_microtask_after(new StartMqttTask, 0);
   sched_microtask_after(new MqttCommsTask, 0);
+  // FIXME: The period of this task must change when we switch mode
   sched_microtask_periodically(new CaptureSensorsForMqttTask, mqtt_capture_interval_s() * 1000);
 #endif
 #ifdef WEB_UPLOAD
@@ -241,6 +242,39 @@ again:
     goto again;
   case EV_BUTTON_UP:
     log("Button up\n");
+
+    // Short press < 2s
+    // Long press > 3s
+    //
+    // If short press:
+    //   If we're in slideshow mode:
+    //     Enter monitoring mode, that is
+    //      - set the slideshow_mode flag to false
+    //      - block the slideshow task
+    //      - the device will power itself down when the next timer interrupt arrives, no action necessary
+    //      - update the display: "MONITORING MODE"
+    //   Otherwise, we're in monitoring mode:
+    //     Enter the slideshow mode:
+    //      - set the slideshow_mode flag to true
+    //      - unblock the slideshow task; this will be set to run immediately
+    //      - the device will power itself up if it was powered down, no action necessary
+    //
+    //   NOTES
+    //
+    //     How do we want the display to be handled during monitoring mode?
+    //      - no display?
+    //      - display when we read the sensors, about once per hr?
+    //      - run the display "every so often" independent of the sensors?
+    //
+    //     How long is monitoring active?  We need a good sample, but this means warmup.  Probably
+    //     there is something about the sensor task that needs to change.
+    //
+    //     In fact, lots of tasks need to change their frequency, this is not great.  And they are on
+    //     the run queue!  So this is a mess that's unhandled.
+    //
+    // If long press:
+    //   Want to enter config mode somehow, but this is not necessary yet
+
     goto again;
   default:
     panic("Unknown event");
