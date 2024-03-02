======================================================
=                                                    =
= Controlling 3DRealms Games with an External Device =
= Version 1.0                                        =
= February 19, 1996                                  =
= Written by Mark Dochtermann                        =
=                                                    =
======================================================

INTRODUCTION

A number of 3DRealms games use the same external control interface.  The
following specifications describe how the the control sysem works, and how
you can interace to it.

GENERAL

All communication between an external driver and the game program is done
through a shared data structure.  This data structure is passed into the
game program by its flat linear address.  This structure is called the
CONTROL structure and from here on will be referred to as such:

MAXEXTERNALAXES is defined as 6
MAXEXTERNALBUTTONS is defined as 32

typedef struct
   {

   // Each 3DRealms game has a unique CONTROL id.

   word  id;

   // Game executes an interrupt to communicate with driver

   word  intnum;

   // the current state of each axis
   // Only the number of axes supported in your device should have
   // values, otherwise these should be 0

   int32 axes[MAXEXTERNALAXES];

   // the current button state of your device

   int32 buttonstate;

   // the button mapping of your device
   // 0 - single clicked button mappings
   // 1 - double clicked button mappings

   byte  buttonmap[MAXEXTERNALBUTTONS][2];

   // the analog mapping of your axes
   // any axes which are not supported should be left undefined

   byte  analogaxesmap[MAXEXTERNALAXES];

   // the command to send to the external driver

   word  command;

   // the digital mapping of your axes
   // any axes which are not supported should be left undefined

   byte  digitalaxesmap[MAXEXTERNALAXES][2];
   } ExternalControlInfo;

CONTROL contains all information necessary to communicate between the
game and the driver.  When the CONTROL structure is initially passed
into a game it must have certain critical information set in it.  This
information includes: button mappings, analog axis mapping and digital
axis mapping.

MAPPING BUTTONS

Buttons are mapped in a very simple manner.  Each button can have both a
single-clicked effect as well as a double-clicked effect.  The
determination of whether or not a button has been double-clicked or not is
taken care of by the control system.  So the only thing the control system
needs from an external device is the current state of the buttons.  The
mapping of a button is as follows, for each button defined, place a
gamefunction in the first array element for a single-clicked mapping and a
gamefunction in the second array element for a double-clicked mapping.  A
gamefunction is an enumerated type that changes from game to game.  A
complete list of gamefunctions can be found in the file FUNCTION.H.  Now
let's look at an example.  Suppose we have a three button mouse and we
went to set up the buttons as follows:

  LMB single-clicked -> Fire
  RMB single-clicked -> Strafe
  MMB single-clicked -> Move_Forward
  RMB double-clicked -> Open

All we would need to fill in to the button mapping array is :

  buttonmap[0][0] = gamefunc_Fire;
  buttonmap[1][0] = gamefunc_Strafe;
  buttonmap[2][0] = gamefunc_Move_Forward;
  buttonmap[1][1] = gamefunc_Open;

Notice that the order of the mouse buttons went left,right,middle.  This
is just a peculiarity of the mouse interface, not the control system.  It
is important that all buttons that do not have mappings are setup to have
an undefined mapping defined by EXTERNALBUTTONUNDEFINED.


MAPPING ANALOG AXES

Axes can have any analog function assigned to them.  The analog functions
are:

typedef enum
   {
   analog_turning=0,
   analog_strafing=1,
   analog_lookingupanddown=2,
   analog_elevation=3,
   analog_rolling=4,
   analog_moving=5
   } analogcontrol;

To assign a particular axis to an analog function simply place the analog
function enumerated type in the analogaxismap for that axis.  For example,
to setup the mappings for the mouse:

The mouse has two degrees of freedom or two axes X and Y.
Let Axis 0 be X.
Let Axis 1 be Y.

  analogaxesmap[0] = analog_turning;
  analogaxesmap[1] = analog_moving;

Axes which are undefined should have the value EXTERNALAXISUNDEFINED.


MAPPING DIGITAL AXES

Each axis is digitized internally in the control system.  It's digital
value is then used to assert digital axis mappings.  These mappings are
defined exactly the same way as the button mappings.  In order to specify
which axis extreme you want to assign a gamefunction to use the following
enumerated type:

typedef enum
   {
   axis_up,
   axis_down,
   axis_left,
   axis_right
   } axisdirection;

Here's an example.  Let's assume we are setting up mappings for a Gravis
GamePad.  Since the GamePad is essentially a digital device, we want to
ignore it's analog components and just use it's digital ones.  These
mappings would look as follows:

The GamePad has two degrees of freedom or two axes X and Y.
Let Axis 0 be X.
Let Axis 1 be Y.

  digitalaxesmap[0][axis_left] = gamefunc_Turn_Left;
  digitalaxesmap[0][axis_right] = gamefunc_Turn_Right;
  digitalaxesmap[1][axis_up] = gamefunc_Move_Forward;
  digitalaxesmap[1][axis_down] = gamefunc_Move_Backward;

Axes which are undefined should have the value EXTERNALAXISUNDEFINED.


NOTE

Since analog and digital axis mappings are concurrent.  It is recommended
that you get rid of an analog mapping for a specific axis if you also have
digital mappings for that same axis.


DURING THE GAME

The external driver will be queried once per frame for any new control
information, at that time, the driver should place updated buttonstate
information and axis information in CONTROL.  The control system limits
all analog controls to +/- 32767.  Please refer to game specific
documentation to find out how controls are limited inside the game.


It is recommended that you manage your own stack when calling your
driver code since the transfer stack provided by the dos extender may not
be large enough for your driver.

Please Refer to game specific documentation for idiosyncracies of a
certain game.

Happy Programming,

Mark D
PARADIGM @ METRONET.COM
