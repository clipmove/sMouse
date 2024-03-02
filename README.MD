### Note

sMouse was created by Rickard Westerlund

This is a mirror of https://bitbucket.org/rwesterlund/smouse/src/master/

# sMouse

Version 1.0

## Introduction

This is an external controller for Build engine games that is primarily
meant to deal with the poor mouse aiming in Blood.

One of the reasons the mouse controls poorly is in part
due to a filtering function that only moves the aim along the dominant axis
which is also addressed by bMouse. Another issue is that at low sensitivities the
mouse would become jerky in vertical movements which this implementation
aims to fix as well.

It appears that most Build engine games divide the input of the vertical mouse
axis by a somewhat large number which causes a loss of precision. This number
was found by manually testing each game and then used in this implementation
to store the lower bits that would get lost and then reuse them the next frame.
The result being smooth mouse movement, to a certain degree. With Blood the
vertical aim will still skeep a few pixels when it actually moves which
is probably caused by another compensation done in the game that
most likely cannot be addressed using an external controller implementation.

## Features

  * Support for Duke Nukem 3D, Shadow Warrior, Blood and Redneck Rampage.
    Other Build engine games that allows for external controller can also be
    supported, see Advanced Usage.
  * Provides a smooth mouse aiming in contrast to the default controls.

## Building from Source

Because the original source comes without a license the source modifications
have to be distributed indirectly via patching. Note that this package already
comes with a prebuilt binary and you only need to do this if you want
to tinker yourself.

You can compile the source file using any old DOS compiler.
I use Turbo C++ 3.0 running in DOSBox.

## Installation

  1. Extract SMOUSE.EXE to the game folder.
  2. Run SETUP.EXE
  3. Navigate to `Controller Setup -> Controller Type` and set it to
     "Keyboard and External".
  4. Optionally, navigate to `Controller Setup -> Setup External` and change it
     to `SMOUSE.EXE`. This will allow you to run the game with sMouse from the
     setup program when you choose `Save and launch Game`.
  5. Save and exit setup.

It is also possible to tweak the sensitivities in the setup program afterwards.

## Usage

Assuming the executable for the game is called `GAME.EXE` you would then type
`SMOUSE.EXE LAUNCH GAME.EXE` to run it with sMouse active and if you have set
the controller type to external it should work.

If you have a GOG installation you may want to change the file called
something like `dosboxGAMENAME_single.conf` and `dosboxGAMENAME_addon.conf`
to reflect this as well. The line(s) in the `[autoexec]` section of
these files which launch the game needs to be changed to run the game
through sMouse in the same fashioned mentioned earlier.

## Advanced Usage

sMouse has additional parameters that can be provided on the command line.
For each of these it is important that they appear before the `LAUNCH`
parameter as anything that comes after it will be interpreted as parameters
for the game itself.

  * `CONFIG`: This parameter will specify an explicit config file to use when
    reading mouse settings. If this parameter is not provided the program will
    try looking for the default config for the different games until it
    finds one.
    Example: `SMOUSE.EXE CONFIG CONF.CFG LAUNCH BLOOD.EXE`
  * `MPARAMS`: Specifies and override for the mouse smoothing parameters used
    by the program. It consists of three values, the first two are scaling
    values for the horizontal and vertical axis respectively. The third value
    is the divisor applied on the vertical axis to get the remainder for the
    next frame.
    Example: `SMOUSE.EXE MPARAMS 32 48 512 LAUNCH BLOOD.EXE`

The `MPARAMS` parameter can also be provided as a file in the working directory
called `mparams.txt`. It should contain the three integer values also used
in the command line form, separated by space.