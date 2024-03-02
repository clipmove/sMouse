// function.h

// file created by makehead.exe
// these headers contain default key assignments, as well as
// default button assignments and game function names
// axis defaults are also included


#ifndef _function_public_
#define _function_public_

#define NUMGAMEFUNCTIONS 49

extern char * gamefunctions[];

enum
   {
   gamefunc_Move_Forward,
   gamefunc_Move_Backward,
   gamefunc_Turn_Left,
   gamefunc_Turn_Right,
   gamefunc_Strafe,
   gamefunc_Fire,
   gamefunc_Open,
   gamefunc_Run,
   gamefunc_AutoRun,
   gamefunc_Jump,
   gamefunc_Crouch,
   gamefunc_Look_Up,
   gamefunc_Look_Down,
   gamefunc_Look_Left,
   gamefunc_Look_Right,
   gamefunc_Strafe_Left,
   gamefunc_Strafe_Right,
   gamefunc_Aim_Up,
   gamefunc_Aim_Down,
   gamefunc_Weapon_1,
   gamefunc_Weapon_2,
   gamefunc_Weapon_3,
   gamefunc_Weapon_4,
   gamefunc_Weapon_5,
   gamefunc_Weapon_6,
   gamefunc_Weapon_7,
   gamefunc_Weapon_8,
   gamefunc_Weapon_9,
   gamefunc_Weapon_10,
   gamefunc_Inventory,
   gamefunc_Inventory_Left,
   gamefunc_Inventory_Right,
   gamefunc_Holo_Duke,
   gamefunc_Jetpack,
   gamefunc_NightVision,
   gamefunc_MedKit,
   gamefunc_TurnAround,
   gamefunc_SendMessage,
   gamefunc_Map,
   gamefunc_Shrink_Screen,
   gamefunc_Enlarge_Screen,
   gamefunc_Center_View,
   gamefunc_Holster_Weapon,
   gamefunc_Show_Opponents_Weapon,
   gamefunc_Map_Follow_Mode,
   gamefunc_See_Coop_View,
   gamefunc_Mouse_Aiming,
   gamefunc_Toggle_Crosshair,
   gamefunc_Steroids,
   };
#endif
