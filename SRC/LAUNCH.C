// Launch program for External Controllers
// This External Control API should work with all "Build" type
// 3DRealm games with little modification.
//
// Specific implementation detail about a controller is left
// for the user to insert code about their particular device
//
// This example controller supports the mouse as its external
// device.  This is just an example
//
// This program was compiled and created with Borland C/C++ 3.1, but just
// about any 16-bit compiler can be used.
//
// If you have any questions on integrating your device with 3DRealms games
// you can contact me, Mark Dochtermann at:
//
// 76746,3357 on CompuServe or
// paradigm @ metronet.com
//
// 02/19/96 Example driver creation.

// include files
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <process.h>
#include <stdarg.h>
#include <bios.h>
#include <time.h>
#include <float.h>

#include "types.h"
#include "external.h"
#include "function.h"

#define CONTROL_STACKSIZE    (2048)
#define CONTROL_SAFETYMARGIN (8)

typedef enum
   {
   axis_up,
   axis_down,
   axis_left,
   axis_right
   } axisdirection;

typedef enum
   {
   analog_turning=0,
   analog_strafing=1,
   analog_lookingupanddown=2,
   analog_elevation=3,
   analog_rolling=4,
   analog_moving=5,
   analog_maxtype
   } analogcontrol;

union REGS inregs, outregs;
struct SREGS segregs;

static ExternalControlInfo control;
static boolean control_started = false;
void interrupt (*oldcontrolvector) (void);
void interrupt CONTROL_ISR (void);
static char * control_stack;
static unsigned short control_stacksegment;
static unsigned short control_stackpointer;
static unsigned short old_stacksegment;
static unsigned short old_stackpointer;

// For parsing configurations.
static const char *configurations[] = {
	"DUKE3D.CFG",
	"SW.CFG",
	"BLOOD.CFG",
	"REDNECK.CFG"
};
static const char *chosen_config = NULL;
static char keydefs[100][100]; // Arbitrary max number of keydefs of arbitrary size, just ensure it's enough.
static int amount_keydefs = 0;

static const char *axis_types[] = {
	"analog_turning",
	"analog_strafing",
	"analog_lookingupanddown",
	"analog_elevation",
	"analog_rolling",
	"analog_moving"
};

static char settings_mouse_button[3][2][100];
static char settings_mouse_axes[2][100];
static char settings_mouse_digital[2][2][100];
static int settings_mouse_aiming = 0;
static int settings_mouse_inverted = 0;
static int32 settings_mouse_sensitivity = 32792L;
static int32 settings_mouse_axis_sensitivity[2] = {65536L, 65536L};
static char buffer[1024];

enum {
    game_unknown,
    game_duke3d,
    game_sw,
    game_blood,
    game_redneck
} static which_game = game_unknown;

// Used for omouse positioning.
static int game_default_inverted = 0;
static int game_mouse_y_threshold = 1;
static int game_mouse_scale[2] = {32, 48};
static int32 mouse_y_leftover = 0;

#define DUKECONTROLID 0xdead
#define MouseInt 0x33
#define ResetMouse       0
#define GetMouseButtons  3
#define GetMouseDelta    11

#define FIRE (1<<0)
#define OPEN (1<<1)
#define AIMUP (1<<2)
#define AIMDOWN (1<<3)
#define RUN (1<<4)
#define INVENTORYLEFT (1<<5)
#define INVENTORYRIGHT (1<<6)
#define INVENTORY (1<<7)

/*
=================
=
= CheckParm
=
= Checks for the given parameter in the program's command line arguments
=
= Returns the argument number (1 to argc-1) or 0 if not present
=
=================
*/

int CheckParm (char *check)
   {
   char  i;
   char  *parm;
   char  tempstring[128];
   char  delims[4] = {'\\','-','/',(char)NULL};

   for (i = 1;i<_argc;i++)
      {
      strcpy(tempstring,_argv[i]);
      parm = strtok(tempstring,&delims[0]);
      if (parm == NULL)
         continue;

      if (!stricmp(check,parm) )
         return i;
      }
   return 0;
   }

//******************************************************************************
//
// MOUSE_GetDelta
//
// Gets the amount that the mouse has moved from the mouse driver.
//
//******************************************************************************

void MOUSE_GetDelta( int32*x, int32*y  )
   {
   inregs.x.ax = GetMouseDelta;
   int86( MouseInt, &inregs, &outregs );

   *x = ( int32 )((int16)outregs.x.cx);
   *y = ( int32 )((int16)outregs.x.dx);
   }

//******************************************************************************
//
// MOUSE_GetButtons ()
//
//******************************************************************************

int32 MOUSE_GetButtons( void )
   {
   inregs.x.ax = GetMouseButtons;
   int86( MouseInt, &inregs, &outregs );

   return( (int32)outregs.h.bl );
   }


//******************************************************************************
//
// MOUSE_Init ()
//
// Detects and sets up the mouse.
//
//******************************************************************************

boolean MOUSE_Init( void )
   {
   inregs.x.ax = ResetMouse;
   int86( MouseInt, &inregs, &outregs );

   return( outregs.x.ax == 0xffff );
   }

//******************************************************************************
//
// MOUSE_Shutdown ()
//
// Shutsdown the mouse
//
//******************************************************************************

void MOUSE_Shutdown( void )
   {
   }


/*****************************
 * sMouse functions
 *****************************/

int StrStartsWith(char *str, char *starts_with)
{
	while (*str && *starts_with) {
		if (*str != *starts_with)
			break;
		str++;
		starts_with++;
	}

	return *starts_with == '\0';
}

void StrTruncateOn(char *str, int c)
{
	char *quote = strchr(str, c);
	if (quote != NULL)
		*quote = '\0';
}

int ReadGameConfig(void)
{
    int i;
    FILE *f = NULL;

    memset(settings_mouse_button, 0, sizeof(settings_mouse_button));
    memset(settings_mouse_axes, 0, sizeof(settings_mouse_axes));
    memset(settings_mouse_digital, 0, sizeof(settings_mouse_digital));

    // Prefer using a command-line specified config.
	i = CheckParm("config");
	if (i != 0)
		chosen_config = _argv[i + 1];

   // If a config file was provided through command line use that.
   if (chosen_config != NULL) {
		f = fopen(chosen_config, "r");
		if (f == NULL) {
			printf("ERROR - The specified configuration file \"%s\" could not be opened.\n", chosen_config);
			return -1;
		}
		printf("Reading controls from \"%s\".\n", chosen_config);
   } else {
	   // Otherwise try opening some of the known build game configuration files.
	   for (i = 0; i < sizeof(configurations)/sizeof(*configurations); i++) {
		   f = fopen(configurations[i], "r");
		   if (f != NULL) {
			   printf("Reading controls from \"%s\".\n", configurations[i]);
			   break;
		   }
	   }
	   if (f == NULL) {
		   // Not an error because it might just be an unsupported game and the user might just want to boot it anyway.
		   printf("WARNING - Could not find a suitable configuration file for game. Try specifying one with the \"config\" parameter.\n");
		   printf("    Tried looking for:");
		   for (i = 0; i < sizeof(configurations)/sizeof(*configurations); i++) {
				printf(" %s", configurations[i]);
		   }
		   printf("\n");
	   }
   }

	// If a config was found then load all the mouse settings for use with this driver.
	if (f != NULL) {
		int grabbing_keydefs = 0;
		while (fgets(buffer, sizeof(buffer), f) != NULL) {
			// Lazy assumption that a whole line has been read here.
			if (grabbing_keydefs) {
				if (buffer[0] == '[') {
					grabbing_keydefs = 0;
				} else if (amount_keydefs < 100) {
					char dummy;
					if (sscanf(buffer, "%s = %c", keydefs[amount_keydefs], &dummy) == 2) {
						amount_keydefs++;
					}
				}
			} else {
				if (StrStartsWith(buffer, "[KeyDefinitions]")) {
					grabbing_keydefs = 1;
				} else if (StrStartsWith(buffer, "Mouse")) {
					// Find mouse bindings and axes
					char binding_name[100];
					int binding_index = 0;
					int binding_subindex = 0;
					int32 sensitivity = 0;
					if (sscanf(buffer, "MouseButton%d = \"%s", &binding_index, binding_name) == 2) {
						StrTruncateOn(binding_name, '"');
						strcpy(settings_mouse_button[binding_index][0], binding_name);
					} else if (sscanf(buffer, "MouseButtonClicked%d = \"%s", &binding_index, binding_name) == 2) {
						StrTruncateOn(binding_name, '"');
						strcpy(settings_mouse_button[binding_index][1], binding_name);
					} else if (sscanf(buffer, "MouseAnalogAxes%d = \"%s", &binding_index, binding_name) == 2) {
						StrTruncateOn(binding_name, '"');
						strcpy(settings_mouse_axes[binding_index], binding_name);
					} else if (sscanf(buffer, "MouseDigitalAxes%d_%d = \"%s", &binding_index, &binding_subindex, binding_name) == 3) {
						StrTruncateOn(binding_name, '"');
						strcpy(settings_mouse_digital[binding_index][binding_subindex], binding_name);
					} else if (sscanf(buffer, "MouseAiming = %d", &settings_mouse_aiming) == 1) {
					} else if (sscanf(buffer, "MouseAimingFlipped = %d", &settings_mouse_inverted) == 1) {
					} else if (sscanf(buffer, "MouseSensitivity = %ld", &settings_mouse_sensitivity) == 2) {
					} else if (sscanf(buffer, "MouseAnalogScale%d = %ld", &binding_index, &sensitivity) == 2) {
						settings_mouse_axis_sensitivity[binding_index] = sensitivity;
					}
				}
			}
		}
		fclose(f);
	}

    return 0;
}

void SetupMouseParameters(void)
{
    int32 i;

    // Determine what game is being run by finding a unique key definition.
    for (i = 0; i < amount_keydefs; i++) {
        if (strcmp(keydefs[i], "Holo_Duke") == 0) {
            which_game = game_duke3d;
            break;
        } else if (strcmp(keydefs[i], "Smoke_Bomb") == 0) {
            which_game = game_sw;
            break;
        } else if (strcmp(keydefs[i], "BeastVision") == 0) {
            which_game = game_blood;
            break;
        } else if (strcmp(keydefs[i], "Yeehaa") == 0) {
            which_game = game_redneck;
            break;
        }
    }

    game_default_inverted = (which_game == game_duke3d || which_game == game_blood);
    if (game_default_inverted)
        settings_mouse_inverted = !settings_mouse_inverted;

    i = CheckParm("mparams");
    if (i != 0) {
        // Read mouse paramters from command line.
        game_mouse_scale[0] = atoi(_argv[i + 1]);
        game_mouse_scale[1] = atoi(_argv[i + 2]);
        game_mouse_y_threshold = atoi(_argv[i + 3]);
        printf("Setting up mouse parameters from command line. (%d %d %d)\n", game_mouse_scale[0], game_mouse_scale[1], game_mouse_y_threshold);
    } else {
        FILE *f;
        int from_file = 0;

        // Check if there is a file containing mouse parameters.
        f = fopen("mparams.txt", "r");
        if (f != NULL) {
            from_file = fscanf(f, "%d %d %d", &game_mouse_scale[0], &game_mouse_scale[1], &game_mouse_y_threshold) == 3;
            fclose(f);
        }

        if (from_file) {
            printf("Read parameters from \"mparams.txt\".\n");
        } else {
            // Set up mouse parameters based on game.
            switch (which_game) {
                case game_duke3d:
                    // Tested using Duke Nukem 3D Classic from Steam.
                    game_mouse_scale[0] = 32;
                    game_mouse_scale[1] = 96;
                    game_mouse_y_threshold = 128;
                    printf("Setting up mouse parameters for Duke Nukem 3D.\n");
                    break;
                case game_sw:
                    // Tested using Shadow Warrior Classic from Steam.
                    game_mouse_scale[0] = 32;
                    game_mouse_scale[1] = 48;
                    game_mouse_y_threshold = 1;
                    printf("Setting up mouse parameters for Shadow Warrior.\n");
                    break;
                case game_blood:
                    // Tested using One Unit Whole Blood from GOG.
                    game_mouse_scale[0] = 32;
                    game_mouse_scale[1] = 48;
                    game_mouse_y_threshold = 512;
                    printf("Setting up mouse parameters for Blood.\n");
                    break;
                case game_redneck:
                    // Tested using Redneck Rampeg from GOG.
                    game_mouse_scale[0] = 32;
                    game_mouse_scale[1] = 96;
                    game_mouse_y_threshold = 128;
                    printf("Setting up mouse parameters for Redneck Rampage.\n");
                    break;
                default:
                    printf("WARNING - Unable to determine which game is running and no mouse parameters were otherwise passed, mouse smoothing will be disabled.\n");
                    break;
            }
        }
    }
}

void SetupMouseMappings(void)
{
    int32 i;
    int32 j;
    int32 k;

    // Set up mouse bindings based on what was found in the config file.
    /*printf("Found keydefs:"); // DEBUG
    for (i = 0; i < amount_keydefs; i++) {
        printf(" %s,", keydefs[i]);
    }
    printf("\n");*/

    // Buttons
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            for (k = 0; k < amount_keydefs; k++) {
                if (strcmp(settings_mouse_button[i][j], keydefs[k]) == 0) {
                    // printf("Mouse button %ld (%s): %s\n", i, j ? "double" : "single", keydefs[k]); // DEBUG
                    control.buttonmap[i][j] = k;
                }
            }
        }
    }

    // Digital axis mapping
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            for (k = 0; k < amount_keydefs; k++) {
                if (strcmp(settings_mouse_digital[i][j], keydefs[k]) == 0) {
                    //const char *name = i ? (j ? "Backwards" : "Forward") : (j ? "Right" : "Left");
                    // printf("Mouse digital axis %s: %s\n", name, keydefs[k]); // DEBUG
                    if (game_default_inverted && i == 1)
                        control.digitalaxesmap[i][1 - j] = k;
                    else
                        control.digitalaxesmap[i][j] = k;
                }
            }
        }
    }

    //Axes
    for (i = 0; i < 2; i++) {
        for (j = 0; j < sizeof(axis_types)/sizeof(*axis_types); j++) {
            if (strcmp(settings_mouse_axes[i], axis_types[j]) == 0) {
                //printf("Mouse axis %ld: %s\n", i, axis_types[j]); // DEBUG
                control.analogaxesmap[i] = j;
            }
        }
    }
}

/*
=================
=
= InitializeDevice
=
= This function intializes your device.
=
= Returns 0 upon success, any other number on failure
=
=================
*/



int32 InitializeDevice( void )
{
    boolean status;

    if (ReadGameConfig())
        return -1;
    SetupMouseParameters();
    SetupMouseMappings();

    // see if our device is available

    status = MOUSE_Init();
    if (status)
    {
        printf("Mouse has been initialized\n");
        return 0;
    }
    printf("ERROR - Mouse not found or unavailable\n");
    return -1;
}

/*
=================
=
= ShutdownDevice
=
= This function shutsdown your device.
=
=================
*/
void ShutdownDevice( void )
   {
   MOUSE_Shutdown();
   }

/*
=================
=
= GetDeviceInput
=
= This function querys your device and gets it's input
=
=================
*/
void GetDeviceInput( void )
   {
	int i;

   //
   // get movement info
   //
   MOUSE_GetDelta( &control.axes[0], &control.axes[1] );

	for (i = 0; i < 2; i++) {
		control.axes[i] *= game_mouse_scale[i];
		control.axes[i] *= settings_mouse_sensitivity;
		control.axes[i] /= 32792L;
		control.axes[i] *= settings_mouse_axis_sensitivity[i];
		control.axes[i] /= 65536L;
	}

	control.axes[1] += mouse_y_leftover;
	if (control.axes[1] > 0) {
		mouse_y_leftover = control.axes[1] % game_mouse_y_threshold;
	} else {
		mouse_y_leftover = -(-control.axes[1] % game_mouse_y_threshold);
	}

	if (settings_mouse_inverted)
		control.axes[1] = -control.axes[1];

   //
   // get button info
   //
   control.buttonstate = MOUSE_GetButtons();
   }

/*
=============
=
= SetupCONTROL
=
=============
*/

void SetupCONTROL ( void )
   {
	unsigned char far *vectorptr;
   short vector;
   char * topofstack;
   int i;


	if ((i = CheckParm("-vector")) != 0)
      {
      vector = sscanf ("0x%x",_argv[i+1]);
      goto gotit;
      }

 	/* Get an interrupt vector */
   for (vector = 0x60 ; vector <= 0x66 ; vector++)
      {
		vectorptr = *(unsigned char far * far *)(vector*4);
		if ( !vectorptr || *vectorptr == 0xcf )
			break;
	   }
   if (vector == 0x67)
	   {
      printf ("Warning: no NULL or iret interrupt vectors were found between 0x60 and 0x65\n"
              "You can specify a vector with the -vector 0x<num> parameter.\n"
              "Press a key to continue.\n");
	   getch ();
      printf ("Using default vector 0x66\n");
      vector = 0x66;
		}
gotit:
	control.intnum = vector;

   // allocate the gamecontrol stack

   control_stack = malloc(CONTROL_STACKSIZE);

   // check to see if the malloc worked
   if (!control_stack)
      {
      printf("ERROR: could not malloc stack\n");
      }

   // Calculate top of stack

   topofstack = control_stack + CONTROL_STACKSIZE - CONTROL_SAFETYMARGIN;

   // Determine stack segment and pointer

   control_stacksegment = FP_SEG( (char huge *)topofstack );
   control_stackpointer = FP_OFF( (char huge *)topofstack );

   // hook the vector for the game

	oldcontrolvector = getvect (control.intnum);
	setvect (control.intnum, CONTROL_ISR);
   control_started = true;
   }


/*
=============
=
= ShutdownCONTROL
=
=============
*/

void ShutdownCONTROL ( void )
   {
	if (control_started)
      {
      control_started = false;
		setvect (control.intnum,oldcontrolvector);
      free ( control_stack );
      }
   }

/*
=============
=
= CONTROL_ISR
=
=============
*/

#define GetStack(a,b) \
   {                  \
   *a = _SS;          \
   *b = _SP;          \
   }
#define SetStack(a,b) \
   {                  \
   _SS=a;             \
   _SP=b;             \
   }

void interrupt CONTROL_ISR (void)
	{
   //
   // Get current stack
   //

   GetStack( &old_stacksegment, &old_stackpointer );

   //
   // Set the local stack
   //

   SetStack( control_stacksegment, control_stackpointer );

   if (control.command == EXTERNAL_GetInput)
      {
      GetDeviceInput();
      }

   //
   // Restore the old stack
   //

   SetStack( old_stacksegment, old_stackpointer );
   }

/*
=============
=
= LaunchGAME
=
=============
*/

void LaunchGAME ( int pause )
{
	char	*newargs[39];
   char  *launchname;
	char	adrstring[10];
	long  	flatadr;
	int argnum = 1;
	int i,j;

	i = CheckParm("launch");
	if (i != 0)
      {
      i++;
      launchname = _argv[i];
      i++;
      }
   else
      {
      printf("No program to launch\n");
      return;
      }

	SetupCONTROL ();

   //
   // if we did not successfully set up our interrupt get out of here
   //
   if (!control_started)
      return;

   // build the argument list for the game
   // adding all previous command lines to be passed to the main game

   // i initialized from above
   for (;i<_argc;i++)
      {
      newargs [argnum++] = _argv[i];
      }

	newargs [argnum++] = "-" EXTERNALPARM;

	/* Add address of gamecontrol structure */

	flatadr = (long)_DS*16 + (unsigned)&control;
	sprintf (adrstring,"%lu",flatadr);
	newargs [argnum++] = adrstring;

	newargs [argnum] = NULL;

// Make sure arg 0 is correct, if we want to dump the arguments

	newargs [0] = launchname;

   if (pause==1)
      {
      for (i = 0; i < argnum; i++)
         printf ("  arg %d = %s\n", i, newargs [i]);

      printf ("\nPress ESC to abort, or any other key to continue...");
      if (getch () == 0x1b)
         {
         printf ("\n\n");
         ShutdownCONTROL();
         return;
         }
      }
   if (spawnvp ( P_WAIT, launchname, newargs) != 0)
      {
      if (errno!=0)
         printf("%s\n%d\n",strerror(errno),errno);
      }
	printf ("\nReturned from %s\n\n",launchname);

   ShutdownCONTROL();
	}

/*
=============
=
= main
=
=============
*/

void main( void )
   {
   int32 i;
   FILE *f;

   // welcome the user
	printf("sMouse version 1.0 for 3DRealms games.\n");
	printf("A modified version of External Launch for 3DRealms games version 1.0,\n");
	printf("originally written by Mark Dochtermann.\n");

   // clear out control structure
   memset(&control,0,sizeof(control));

   control.id=DUKECONTROLID;

   //
   // clear all assigments
   //

   // clear all button mappings
   for (i=0;i<MAXEXTERNALBUTTONS;i++)
      {
      control.buttonmap[i][0] = EXTERNALBUTTONUNDEFINED;
      control.buttonmap[i][1] = EXTERNALBUTTONUNDEFINED;
      }

   // clear all axis mappings
   for (i=0;i<MAXEXTERNALAXES;i++)
      {
      control.analogaxesmap[i] = EXTERNALAXISUNDEFINED;
      control.digitalaxesmap[i][0] = EXTERNALAXISUNDEFINED;
      control.digitalaxesmap[i][1] = EXTERNALAXISUNDEFINED;
      }

   // try to initialize the device
   if (InitializeDevice())
      {
      exit(0);
      }

   // launch the game
   LaunchGAME ( 0 );
   // we are back from the game, so clean up and exit
   ShutdownDevice();
   }
