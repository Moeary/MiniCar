#ifndef BUTTON_H
#define BUTTON_H

/*
  Description: 
      Key ID enum.
*/
typedef enum
{
    KEY_ID_NONE = 0,
    KEY_ID_S1,
    KEY_ID_S2,
    KEY_ID_USER,
    KEY_ID_GPIO8
} KEY_ID_TYPE;

/*
  Description: 
      Key event type enum.
*/
typedef enum 
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESSED = 1,
    KEY_EVENT_LONG_PRESSED = 2,
    KEY_EVENT_RELEESED = 4
} KEY_EVENT_TYPE;

/*
  Description: 
      Key event callback function pointer type.

  Parameter:
      id --  key id
      event  -- key event type

  Return Value:
      0     -- Success
      other -- Failure
*/
typedef void (*PKeyEventCallback)(KEY_ID_TYPE keyid, KEY_EVENT_TYPE event);

/*
  Description: 
       initialize key event process context.

  Parameter:
      None

  Return Value:
      0     -- Success
      other -- Failure
*/
int KeyEvent_Init(void);

/*
  Description: 
      To register callback functions for a GPIO key.

  Parameter:
      name     -- target GPIO port name for a phisical key
      callback -- callback function for key event
      event    -- the target key event to trigger callback

  Return Value:
      0     -- Success
      other -- Failure
*/
int KeyEvent_Connect(const char* name, PKeyEventCallback callback, unsigned int event);

/*
  Description: 
      To unregister callback functions for a GPIO key.

  Parameter:
      name -- target GPIO port name for a phisical key

  Return Value:
      None
*/
void KeyEvent_Disconnect(const char* name);

/*
  Description: 
      To close key event process context.

  Parameter:
      None

  Return Value:
      None
*/
void KeyEvent_Close(void);

#endif