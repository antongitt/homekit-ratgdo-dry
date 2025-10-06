/****************************************************************************
 * RATGDO HomeKit
 * https://ratcloud.llc
 * https://github.com/PaulWieland/ratgdo
 *
 * Copyright (c) 2023-25 David A Kerr... https://github.com/dkerr64/
 * All Rights Reserved.
 * Licensed under terms of the GPL-3.0 License.
 *
 * Contributions acknowledged from
 * Thomas Hagan... https://github.com/tlhagan
 *
 */

#if defined(ESP8266) || !defined(USE_GDOLIB)
// RATGDO project includes
#include "ratgdo.h"
#include "config.h"
#include "comms.h"
#include "drycontact.h"

// Logger tag
static const char *TAG = "ratgdo-drycontact";

static bool drycontact_setup_done = false;

void onOpenSwitchPress();
void onCloseSwitchPress();
void onOpenSwitchRelease();
void onCloseSwitchRelease();

// Define OneButton objects for open/close pins
OneButton buttonOpen(DRY_CONTACT_OPEN_PIN, true, true); // Active low, with internal pull-up
OneButton buttonClose(DRY_CONTACT_CLOSE_PIN, true, true);
bool dryContactDoorOpen = false;
bool dryContactDoorClose = false;
bool previousDryContactDoorOpen = false;
bool previousDryContactDoorClose = false;

void setup_drycontact()
{
    if (drycontact_setup_done)
        return;

    ESP_LOGI(TAG, "=== Setting up dry contact protocol");

    if (doorControlType == 0)
        doorControlType = userConfig->getGDOSecurityType();

    pinMode(DRY_CONTACT_OPEN_PIN, INPUT_PULLUP);
    pinMode(DRY_CONTACT_CLOSE_PIN, INPUT_PULLUP);

    buttonOpen.setDebounceMs(userConfig->getDCDebounceDuration());
    buttonClose.setDebounceMs(userConfig->getDCDebounceDuration());

    // Attach OneButton handlers
    buttonOpen.attachPress(onOpenSwitchPress);
    buttonClose.attachPress(onCloseSwitchPress);
    buttonOpen.attachLongPressStop(onOpenSwitchRelease);
    buttonClose.attachLongPressStop(onCloseSwitchRelease);

    drycontact_setup_done = true;
}

void drycontact_loop()
{
    if (!drycontact_setup_done)
        return;

    // Poll OneButton objects
    buttonOpen.tick();
    buttonClose.tick();

    if (doorControlType == 3)
    {
        // For dry contact mode with reed switches, read pin states directly
        // Pins are active low (INPUT_PULLUP), so LOW = switch closed/active
        int openPinState = digitalRead(DRY_CONTACT_OPEN_PIN);
        int closePinState = digitalRead(DRY_CONTACT_CLOSE_PIN);

        bool openPinActive = (openPinState == LOW);
        bool closePinActive = (closePinState == LOW);

        static GarageDoorCurrentState lastReportedState = GarageDoorCurrentState::UNKNOWN;
        GarageDoorCurrentState newState = doorState;

        if (openPinActive)
        {
            newState = GarageDoorCurrentState::CURR_OPEN;
        }
        else if (closePinActive)
        {
            newState = GarageDoorCurrentState::CURR_CLOSED;
        }
        // If neither pin is active, maintain last known state

        if (newState != lastReportedState)
        {
            ESP_LOGI(TAG, "Door state changed to: %d (0=open, 1=closed, 2=opening, 3=closing, 4=stopped, 5=unknown)", newState);
            lastReportedState = newState;
        }
        doorState = newState;
        garage_door.current_state = newState;  // Update the actual state used by status.json
        garage_door.active = true;  // Mark door as active so status.json reports the state
    }
    else if (userConfig->getDCOpenClose())
    {
        // Dry contacts are repurposed as optional door open/close when we
        // are using Sec+ 1.0 or Sec+ 2.0 door control type
        if (dryContactDoorOpen)
        {
            open_door();
            dryContactDoorOpen = false;
        }

        if (dryContactDoorClose)
        {

            close_door();
            dryContactDoorClose = false;
        }
    }
}

/*************************** DRY CONTACT CONTROL OF DOOR ***************************/
// Functions for sensing GDO open/closed
void onOpenSwitchPress()
{
    dryContactDoorOpen = true;
    ESP_LOGI(TAG, "Open switch pressed");
}

void onCloseSwitchPress()
{
    dryContactDoorClose = true;
    ESP_LOGI(TAG, "Close switch pressed");
}

void onOpenSwitchRelease()
{
    dryContactDoorOpen = false;
    ESP_LOGI(TAG, "Open switch released");
}

void onCloseSwitchRelease()
{
    dryContactDoorClose = false;
    ESP_LOGI(TAG, "Close switch released");
}
#endif // not USE_GDOLIB
