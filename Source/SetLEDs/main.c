/*  setleds for Mac
    http://github.com/damieng/setledsmac
    Copyright 2015 Damien Guard. GPL 2 licenced.

    clang -o setleds main.c  -framework Foundation  -framework Carbon  -framework IOKit

    To be able to run `setleds` as normal user:
    chown root setleds
    chmod 4755 setleds
*/

#include "main.h"

Boolean verbose = false;
const char *nameMatch;

int main(int argc, const char * argv[]) {
    if (argc == 1)
        explainUsage();

    LedState changes[] = { NoChange, NoChange, NoChange, NoChange };       // changes[0] not used.
    Boolean  nextIsName = false;

    for (int i = 1; i < argc; i++)                                         {
        if (strcasecmp(argv[i], "-v") == 0)
            verbose = true                                                 ;
        else if (strcasecmp(argv[i], "-h") == 0)
            explainUsage()                                                 ;
        else if (strcasecmp(argv[i], "-k") == 0)
            nextIsName = true                                              ;

        else if (strcasecmp(argv[i], "+n") == 0)
            changes[kHIDUsage_LED_NumLock] = On                            ;
        else if (strcasecmp(argv[i], "-n") == 0)
            changes[kHIDUsage_LED_NumLock] = Off                           ;
        else if (strcasecmp(argv[i], "/n") == 0)
            changes[kHIDUsage_LED_NumLock] = Toggle                        ;

        else if (strcasecmp(argv[i], "+c") == 0)
            changes[kHIDUsage_LED_CapsLock] = On                           ;
        else if (strcasecmp(argv[i], "-c") == 0)
            changes[kHIDUsage_LED_CapsLock] = Off                          ;
        else if (strcasecmp(argv[i], "/c") == 0)
            changes[kHIDUsage_LED_CapsLock] = Toggle                       ;

        else if (strcasecmp(argv[i], "+s") == 0)
            changes[kHIDUsage_LED_ScrollLock] = On                         ;
        else if (strcasecmp(argv[i], "-s") == 0)
            changes[kHIDUsage_LED_ScrollLock] = Off                        ;
        else if (strcasecmp(argv[i], "/s") == 0)
            changes[kHIDUsage_LED_ScrollLock] = Toggle                     ;

        else                                                               {
            if (nextIsName)                                                {
                nameMatch  = argv[i]                                       ;
                nextIsName = false                                         ;}
            else                                                           {
                fprintf(stderr, "Unknown arg: %s\n", argv[i])              ;
                explainUsage()                                             ;}}}

    setAllKeyboards(changes)                                               ;
    return 0                                                               ;}


void explainUsage()                                                              {
    printf("Usage:\tsetleds [-h] [-v] [-k KEYBOARD_NAME] [[+|-|/][n|c|s]]\n")    ;
    exit(1)                                                                      ;}

Boolean isKeyboardDevice(struct __IOHIDDevice *device)                                   {
    return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard) ;}

CFMutableDictionaryRef getKeyboardDictionary() {
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    if (!result) return result;

    UInt32 inUsagePage = kHIDPage_GenericDesktop;
    UInt32 inUsage = kHIDUsage_GD_Keyboard;

    CFNumberRef page = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsagePage)     ;
    if (page)                                                                                  {
        CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), page)                        ;
        CFRelease(page)                                                                        ;

        CFNumberRef usage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsage)    ;
        if (usage)                                                                             {
            CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usage)                   ;
            CFRelease(usage)                                                                   ;}}
    return result                                                                              ;}

void setKeyboard(struct __IOHIDDevice *device, CFDictionaryRef keyboardDictionary, LedState changes[])        {
    CFStringRef deviceNameRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey))                       ;
    if (!deviceNameRef)  return;

    const char * deviceName = CFStringGetCStringPtr(deviceNameRef, kCFStringEncodingUTF8);
    if (nameMatch && fnmatch(nameMatch, deviceName, 0))  return;

    if (verbose)
        printf("[%s]\n", deviceName);

    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, keyboardDictionary, kIOHIDOptionsTypeNone);
    if (elements)                                                                                             {
        uint32_t elementCount = CFArrayGetCount(elements)                                                     ;
        for (CFIndex elementIndex = 0; elementIndex < elementCount; elementIndex++)                           {
            IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, elementIndex)        ;
            if (element && kHIDPage_LEDs == IOHIDElementGetUsagePage(element))                                {
                uint32_t led = IOHIDElementGetUsage(element)                                                  ;

                if (led > maxLeds)  continue;

                // Get current keyboard led state.
                IOHIDValueRef ledCurrentState = 0;
                IOHIDDeviceGetValue(device, element, &ledCurrentState);
                long ledCurrentOnOffBit = IOHIDValueGetIntegerValue(ledCurrentState);
                CFRelease(ledCurrentState);

                // Do not set the led if already the state.
                if (changes[led] != NoChange && changes[led] != ledCurrentOnOffBit)                                                {
                    uint32_t ledNewOnOffBit                                                                                        ;
                    if (changes[led] == Toggle)
                        ledNewOnOffBit = 1 - ledCurrentOnOffBit                                                                    ;
                    else
                        ledNewOnOffBit = changes[led]                                                                              ;
                    IOHIDValueRef ledNewState = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, 0, ledNewOnOffBit)  ;
                    if (ledNewState)                                                                                               {
                        IOReturn changeResult = IOHIDDeviceSetValue(device, element, ledNewState)                                  ;
                        if (kIOReturnSuccess != changeResult)
                            printf("x  %s%-10s error code=0x%x", stateSymbol[ledNewOnOffBit], ledNames[led], changeResult)         ;
                        CFRelease(ledNewState)                                                                                     ;}}}}
        CFRelease(elements)                                                                                                        ;}}

void setAllKeyboards(LedState changes[])                                                       {
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone)   ;
    if (!manager)                                                                              {
        fprintf(stderr, "Failed to create IOHID manager.\n")                                   ;
        return                                                                                 ;}

    CFDictionaryRef keyboard = getKeyboardDictionary()                                         ;
    if (!keyboard)                                                                             {
        fprintf(stderr, "Failed to get dictionary usage page for kHIDUsage_GD_Keyboard.\n")    ;
        return                                                                                 ;}

    IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    IOHIDManagerSetDeviceMatching(manager, keyboard);

    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    if (devices)                                                                               {
        CFIndex deviceCount = CFSetGetCount(devices)                                           ;
        if (deviceCount == 0)                                                                  {
            fprintf(stderr, "Could not find any keyboard devices.\n")                          ;}
        else                                                                                   {
            IOHIDDeviceRef *deviceRefs = malloc(sizeof(IOHIDDeviceRef) * deviceCount)          ;
            if (deviceRefs)                                                                    {
                CFSetGetValues(devices, (const void **) deviceRefs)                            ;
                for (CFIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
                    if (isKeyboardDevice(deviceRefs[deviceIndex]))
                        setKeyboard(deviceRefs[deviceIndex], keyboard, changes)                ;
                free(deviceRefs)                                                               ;}}
        CFRelease(devices)                                                                     ;}
    CFRelease(keyboard)                                                                        ;}
