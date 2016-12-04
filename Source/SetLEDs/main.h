/*  setleds for Mac
    http://github.com/damieng/setledsmac
    Copyright 2015 Damien Guard. GPL 2 licenced.
 */

#ifndef SetLEDs_main_h
#define SetLEDs_main_h

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>
#include <fnmatch.h>

const int maxLeds = 3;
const char* ledNames[] = { "num", "caps", "scroll" };
// Elements in `ledNames` is ordered the way it is because:
// kHIDUsage_LED_NumLock    = 0x01
// kHIDUsage_LED_CapsLock   = 0x02
// kHIDUsage_LED_ScrollLock = 0x03
const char* stateSymbol[] = {"-", "+" };
typedef enum { Off=0, On=1, Toggle, NoChange} LedState;

void explainUsage();
void setAllKeyboards(LedState changes[]);

#endif
