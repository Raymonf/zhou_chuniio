#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

/* Initialize JVS-based input. This function will be called before any other
   chuni_io_jvs_*() function calls. Errors returned from this function will
   manifest as a disconnected JVS bus.

   All subsequent calls may originate from arbitrary threads and some may
   overlap with each other. Ensuring synchronization inside your IO DLL is
   your responsibility. */

_declspec(dllexport) HRESULT chuni_io_jvs_init(void);

/* Poll JVS input.

   opbtn returns the cabinet test/service state, where bit 0 is Test and Bit 1
   is Service.

   beam returns the IR beams that are currently broken, where bit 0 is the
   lowest IR beam and bit 5 is the highest IR beam, for a total of six beams.

   Both bit masks are active-high.

   Note that you cannot instantly break the entire IR grid in a single frame to
   simulate hand movement; this will be judged as a miss. You need to simulate
   a gradual raising and lowering of the hands. Consult the proof-of-concept
   implementation for details. */

_declspec(dllexport) void chuni_io_jvs_poll(uint8_t *opbtn, uint8_t *beams);

/* Read the current state of the coin counter. This value should be incremented
   for every coin detected by the coin acceptor mechanism. This count does not
   need to persist beyond the lifetime of the process. */

_declspec(dllexport) void chuni_io_jvs_read_coin_counter(uint16_t *total);

/* Set the state of the coin blocker. Parameter is true if the blocker is
   disengaged (i.e. coins can be inserted) and false if the blocker is engaged
   (i.e. the coin slot should be physically blocked). */

_declspec(dllexport) void chuni_io_jvs_set_coin_blocker(bool open);

/* Initialize touch slider emulation. This function will be called before any
   other chuni_io_slider_*() function calls.

   All subsequent calls may originate from arbitrary threads and some may
   overlap with each other. Ensuring synchronization inside your IO DLL is
   your responsibility. */

_declspec(dllexport) HRESULT chuni_io_slider_init(void);

/* Chunithm touch slider layout:

                               ^^^ Toward screen ^^^

----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 31 | 29 | 27 | 25 | 23 | 21 | 19 | 17 | 15 | 13 | 11 |  9 |  7 |  5 |  3 |  1 |
----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 32 | 30 | 28 | 26 | 24 | 22 | 20 | 18 | 16 | 14 | 12 | 10 |  8 |  6 |  4 |  2 |
----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+

   There are a total of 32 regions on the touch slider. Each region can return
   an 8-bit pressure value. The operator menu allows the operator to adjust the
   pressure level at which a region is considered to be pressed; the factory
   default value for this setting is 20. */

/* Callback function supplied to your IO DLL. This must be called with a
   pointer to a 32-byte array of pressure values, one byte per slider cell.
   See above for layout and pressure threshold information.

   The callback will copy the pressure state data out of your buffer before
   returning. The pointer will not be retained. */

_declspec(dllexport) typedef void (*chuni_io_slider_callback_t)(const uint8_t *state);

/* Start polling the slider. Your DLL must start a polling thread and call the
   supplied function periodically from that thread with new input state. The
   update interval is up to you, but if your input device doesn't have any
   preferred interval then 1 kHz is a reasonable maximum frequency.

   Note that you do have to have to call the callback "occasionally" even if
   nothing is changing, otherwise the game will raise a comm timeout error. */

_declspec(dllexport) void chuni_io_slider_start(chuni_io_slider_callback_t callback);

/* Stop polling the slider. You must cease to invoke the input callback before
   returning from this function.

   This *will* be called in the course of regular operation. For example,
   every time you go into the operator menu the slider and all of the other I/O
   on the cabinet gets restarted.

   Following on from the above, the slider polling loop *will* be restarted
   after being stopped in the course of regular operation. Do not permanently
   tear down your input driver in response to this function call. */

_declspec(dllexport) void chuni_io_slider_stop(void);

/* Update the RGB lighting on the slider. A pointer to an array of 32 * 3 = 96
   bytes is supplied. The illuminated areas on the touch slider are some
   combination of rectangular regions and dividing lines between these regions
   but the exact mapping of this lighting control buffer is still TBD. */

_declspec(dllexport) void chuni_io_slider_set_leds(const uint8_t *rgb);

//_declspec(dllexport) int chuni_io_getcardstatue(void);
