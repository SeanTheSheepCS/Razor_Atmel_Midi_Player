/**********************************************************************************************************************
File: user_app1.c

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app1 as a template:
 1. Copy both user_app1.c and user_app1.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app1" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP1" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app1.c file template

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserApp1Initialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserApp1RunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"
#include "music.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_UserApp1"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserApp1Flags;                       /* Global state flags */

/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */
extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */

extern u32 G_u32AntApiCurrentMessageTimeStamp;                            // From ant_api.c
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;            // From ant_api.c
extern u8 G_au8AntApiCurrentMessageBytes[ANT_APPLICATION_MESSAGE_BYTES];  // From ant_api.c
extern AntExtendedDataType G_sAntApiCurrentMessageExtData;                // From ant_api.c


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp1_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type UserApp1_StateMachine;            /* The state machine function pointer */
static u32 UserApp1_u32Timeout;                      /* Timeout counter used across states */
static AntAssignChannelInfoType UserApp1_sChannelInfo = {0};
static u8* UserApp1_au8MessageFail = "A failure has occurred, most likely with ant!\n";
static u32 UserApp1_u32DataMsgCount = 0; /* ANT_DATA packets recieved */
static u32 UserApp1_u32TickMsgCount = 0;

static u8 lastInstruction;
static u8 charsOnScreen;
static u8 UserApp1_au8NoteNumbers[] =   {48  ,49  ,50  ,51  ,52  ,53  ,54  ,55  ,56  ,57  ,58  ,59  ,60  ,61  ,62  ,63  ,64  ,65  ,66  ,67  ,68  ,69  ,70  ,71  ,72  ,73  ,74  ,75  ,76  ,77  ,78  ,79  ,80  ,81  ,82  ,83  ,84  ,85  ,86  ,87  ,88  ,89  ,90  ,91  ,92  ,93  ,94  ,95  };
static u16 UserApp1_au16Frequencies[] = {C3  ,C3S ,D3  ,D3S ,E3  ,F3  ,F3S ,G3  ,G3S ,A3  ,A3S ,B3  ,C4  ,C4S ,D4  ,D4S ,E4  ,F4  ,F4S ,G4  ,G4S ,A4  ,A4S ,B4  ,C5  ,C5S ,D5  ,D5S ,E5  ,F5  ,F5S ,G5  ,G5S ,A5  ,A5S ,B5  ,C6  ,C6S ,D6  ,D6S ,E6  ,F6  ,F6S ,G6  ,G6S ,A6  ,A6S ,B6  };

/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function: UserApp1Initialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  -
*/
void UserApp1Initialize(void)
{

  LcdClearScreen();
  CapTouchOn();

  LedOn(RED0);

  /* Configure ANT for this application */
  UserApp1_sChannelInfo.AntChannel          = ANT_CHANNEL_USERAPP;
  UserApp1_sChannelInfo.AntChannelType      = ANT_CHANNEL_TYPE_USERAPP;
  UserApp1_sChannelInfo.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
  UserApp1_sChannelInfo.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;

  UserApp1_sChannelInfo.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
  UserApp1_sChannelInfo.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
  UserApp1_sChannelInfo.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  UserApp1_sChannelInfo.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  UserApp1_sChannelInfo.AntFrequency        = ANT_FREQUENCY_USERAPP;
  UserApp1_sChannelInfo.AntTxPower          = ANT_TX_POWER_USERAPP;

  UserApp1_sChannelInfo.AntNetwork = ANT_NETWORK_DEFAULT;

  for( u8 i = 0; i < ANT_NETWORK_NUMBER_BYTES; i++)
  {
    UserApp1_sChannelInfo.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
  }

  if( AntAssignChannel(&UserApp1_sChannelInfo))
  {
    //Channel has been assigned, change the led colour to purple.
    LedOn(BLUE0);
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
  }
  else
  {
    DebugPrintf(UserApp1_au8MessageFail);
    LedBlink(RED0, LED_4HZ);
    UserApp1_StateMachine = UserApp1SM_Error;
  }
} /* end UserApp1Initialize() */



/*----------------------------------------------------------------------------------------------------------------------
Function UserApp1RunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserApp1RunActiveState(void)
{
  UserApp1_StateMachine();

  //Make the LED blink

} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/* Wait for any ANT channel assignment */
static void UserApp1SM_AntChannelAssign(void)
{
  u8 u8status = AntRadioStatusChannel(ANT_CHANNEL_USERAPP);
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_CONFIGURED)
  {
    /*Channel assignment was successful!*/
    DebugPrintf("Channel Assignment Successful!");
    AntOpenChannelNumber(ANT_CHANNEL_USERAPP);
    UserApp1_StateMachine = UserApp1SM_Idle;
  }

  if(IsTimeUp(&UserApp1_u32Timeout, 5000))
  {
    DebugPrintf(UserApp1_au8MessageFail);
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for ??? */
static void UserApp1SM_Idle(void)
{
  if(WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);
    AntOpenChannelNumber(ANT_CHANNEL_USERAPP);
    LedOff(RED0);
    LedOff(BLUE0);
    LedBlink(GREEN0, LED_2HZ);
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
  }
} /* end UserApp1SM_Idle() */

static void UserApp1SM_WaitChannelOpen(void)
{
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_OPEN)
  {
    LedOn(GREEN0);
    UserApp1_StateMachine = UserApp1SM_ChannelOpen;
  }

  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE))
  {
    LedOff(GREEN0);
    LedOn(BLUE0);
    LedOn(RED0);
    UserApp1_StateMachine = UserApp1SM_Idle;
  }
}

static void UserApp1SM_ChannelOpen(void)
{
  static u8 u8LastState = 0xff;
  static u8 au8TickMessage[] = "EVENT x\n\r"; /* "x" at index [6] will be replaced by the current code */
  static u8 au8DataContent[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  static u8 au8DataContentForPrinting[] = "xxxxxxxxxxxxxxxx";
  static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  static u8 au8TestMessage[] = {0,0,0,0,0xA5,0,0,0};
  bool bGotNewData;

  /* Channel stays open until the button is pressed again */
  if(WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);

    /* Queues closing the channel. Initializes the u8LastState variable and changes the
     * LED to blinking green to indicate the channel is closing
     */
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);
    u8LastState = 0xff;
    LedOff(GREEN0);
    LedBlink(GREEN0, LED_2HZ);

    /* Changes state and sets the timer */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    PWMAudioOff(BUZZER1);
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  }

  /* The slave channel may have closed on its own! Has that happened yet? */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) != ANT_OPEN)
  {
    LedOff(GREEN0);
    LedBlink(GREEN0, LED_2HZ);
    u8LastState = 0xff;

    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  }

  /* Check to see if an ANT message has arrived */
  if(AntReadAppMessageBuffer())
  {
    if(G_eAntApiCurrentMessageClass == ANT_DATA) /* Is this a new data message? */
    {
      UserApp1_u32DataMsgCount++;
      /* Linked with a device = solid blue! */
      LedOff(GREEN0);
      LedOn(BLUE0);

      bGotNewData = FALSE;
      for(u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {
        if(G_au8AntApiCurrentMessageBytes[i] != au8LastAntData[i])
        {
          bGotNewData = TRUE;
          au8LastAntData[i] = G_au8AntApiCurrentMessageBytes[i];
          au8DataContent[i] = G_au8AntApiCurrentMessageBytes[i];
          au8DataContentForPrinting[2*i] = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] / 16); // ORIGINAL : ...CurrentData[i]
          au8DataContentForPrinting[2*i+1] = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] % 16); // ORIGINAL: ...CurrentData[i]
        }
      }
      /* Print the new data */
      DebugPrintf(au8DataContentForPrinting);
      interpretData(au8DataContent);
      DebugLineFeed();

    }
    else if(G_eAntApiCurrentMessageClass == ANT_TICK) /* Is this a tick message? */
    {
      UserApp1_u32TickMsgCount++;
      if( u8LastState != G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        u8LastState = G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX];
      }
      au8TickMessage[6] = HexToASCIICharUpper(u8LastState);
      DebugPrintf(au8TickMessage);

      switch(u8LastState)
      {
        /* Paired but missing messages gives a blue blink */
        case EVENT_RX_FAIL:
        {
          LedOff(GREEN0);
          LedBlink(BLUE0, LED_2HZ);
          break;
        }
        /* If you stopped searching, turn the LED green */
        case EVENT_RX_FAIL_GO_TO_SEARCH:
        {
          LedOff(BLUE0);
          LedOn(GREEN0);
          break;
        }
        /* The channel automatically closes if the search times out */
        case EVENT_RX_SEARCH_TIMEOUT:
        {
          DebugPrintf("Search timeout\r\n");
          break;
        }
        default:
        {
          LedOff(BLUE0);
          LedOn(GREEN0);
          DebugPrintf("Unexpected Event \r\n");
          break;
        }
      }
    }
  }
}

static void UserApp1SM_WaitChannelClose(void)
{
  /* Has the channel been closed yet? */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_CLOSED)
  {
    LedOff(GREEN0);
    LedOn(BLUE0);
    LedOn(RED0);

    UserApp1_StateMachine = UserApp1SM_Idle;
  }
  /* Have we timed out yet? */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE))
  {
    LedOff(GREEN0);
    LedBlink(RED0, LED_4HZ);
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}


static void interpretData(u8* au8DataContent)
{
  static bool taskInProgress = FALSE;
  static u8 bytesLeftInMessage = 0;
  if(taskInProgress)
  {
    if(lastInstruction == 0x03) //Track name
    {
      DebugPrintf("GETS HERE!");
      int i = 0;
      while(bytesLeftInMessage != 0 && i <= 7)
      {
        printLetterOnScreen(au8DataContent[i]);
        bytesLeftInMessage--;
        i++;
      }
      if(bytesLeftInMessage!=0)
      {
        taskInProgress = TRUE;
        lastInstruction = 0x03;
      }
      else
      {
        taskInProgress = FALSE;
      }
    }
  }
  else
  {
    if((au8DataContent[0] & 0xf0) == 0x90)
    {
      u8 nodeNumber = au8DataContent[1];
      u16 frequency = 0;
      for(u8 i = 0; i < NUMBER_OF_POSSIBLE_NOTE_NUMBERS; i++)
      {
        if(nodeNumber == UserApp1_au8NoteNumbers[i])
        {
          frequency = UserApp1_au16Frequencies[i];
          break;
        }
      }
      if(frequency == 0)
      {
        //This note is too low to be represented with our current set of frequencies!
        DebugPrintf("The entered note was outside our frequency range. Consider putting implementation into the C# program to shift the song one octave up if the song has notes like this.");
      }
      else
      {
        PWMAudioSetFrequency(BUZZER1,frequency);
        PWMAudioOn(BUZZER1);
      }
    }
    else if((au8DataContent[0] & 0xf0) == 0x80)
    {
      PWMAudioOff(BUZZER1); //Assumes only one note was being played at one time!
    }
    else if(au8DataContent[0] == 0xFF)
    {
      //This is a meta event
      if(au8DataContent[1] == 0x03)
      {
        bytesLeftInMessage = au8DataContent[2];
        int i = 3;
        clearLetters();
        while(bytesLeftInMessage != 0 && i <= 7)
        {
          printLetterOnScreen(au8DataContent[i]);
          bytesLeftInMessage--;
          i++;
        }
        if(bytesLeftInMessage!=0)
        {
          taskInProgress = TRUE;
          lastInstruction = 0x03;
        }
      }
      else if(au8DataContent[1] ==  0x2F)
      {
        //End of track
        clearLetters();
      }
      else
      {
        DebugPrintf("Unrecognized meta event.");
      }
    }
    else
    {
      DebugPrintf("Unrecognized command");
    }
  }
}

static void clearLetters(void)
{
  LcdClearScreen();
  charsOnScreen = 0;
  /*
  static u8 u8letterSpace[7][1] = {{0x1F},{0x01},{0x01},{0x0F},{0x01},{0x01},{0x1F}};
  const u8* u8pAddress = &(u8letterSpace[0][0]);
  u8 i = 11;
  while(i != 0)
  {
    PixelBlockType PBTbarInfo = {0,(i*8),7, (1*8)};
    LcdLoadBitmap(u8pAddress,&PBTbarInfo);
    i--;
  }
  */
}

static void printLetterOnScreen(u8 u8Letter)
{
  static u8 u8letterA[7][1] = {{0x0E},{0x11},{0x11},{0x1F},{0x11},{0x11},{0x11}};
  static u8 u8letterB[7][1] = {{0x0F},{0x11},{0x11},{0x0F},{0x11},{0x11},{0x0F}};
  static u8 u8letterC[7][1] = {{0x0E},{0x11},{0x01},{0x01},{0x01},{0x11},{0x1E}};
  static u8 u8letterD[7][1] = {{0x07},{0x09},{0x11},{0x11},{0x11},{0x09},{0x07}};
  static u8 u8letterE[7][1] = {{0x1F},{0x01},{0x01},{0x0F},{0x01},{0x01},{0x1F}};
  static u8 u8letterF[7][1] = {{0x1F},{0x01},{0x01},{0x0F},{0x01},{0x01},{0x01}};
  static u8 u8letterG[7][1] = {{0x0E},{0x11},{0x01},{0x1D},{0x11},{0x11},{0x0E}};
  static u8 u8letterH[7][1] = {{0x11},{0x11},{0x11},{0x1F},{0x11},{0x11},{0x11}};
  static u8 u8letterI[7][1] = {{0x1F},{0x04},{0x04},{0x04},{0x04},{0x04},{0x1F}};
  static u8 u8letterJ[7][1] = {{0x1C},{0x08},{0x08},{0x08},{0x08},{0x09},{0x06}};
  static u8 u8letterK[7][1] = {{0x11},{0x09},{0x05},{0x03},{0x05},{0x09},{0x11}};
  static u8 u8letterL[7][1] = {{0x01},{0x01},{0x01},{0x01},{0x01},{0x01},{0x1F}};
  static u8 u8letterM[7][1] = {{0x11},{0x1B},{0x15},{0x15},{0x11},{0x11},{0x11}};
  static u8 u8letterN[7][1] = {{0x11},{0x11},{0x13},{0x15},{0x19},{0x11},{0x11}};
  static u8 u8letterO[7][1] = {{0x0E},{0x11},{0x11},{0x11},{0x11},{0x11},{0x0E}};
  static u8 u8letterP[7][1] = {{0x0F},{0x11},{0x11},{0x0F},{0x01},{0x01},{0x01}};
  static u8 u8letterQ[7][1] = {{0x0E},{0x11},{0x11},{0x11},{0x15},{0x09},{0x16}};
  static u8 u8letterR[7][1] = {{0x0F},{0x11},{0x11},{0x0F},{0x05},{0x09},{0x11}};
  static u8 u8letterS[7][1] = {{0x1E},{0x01},{0x01},{0x0E},{0x10},{0x10},{0x0F}};
  static u8 u8letterT[7][1] = {{0x1F},{0x04},{0x04},{0x04},{0x04},{0x04},{0x04}};
  static u8 u8letterU[7][1] = {{0x11},{0x11},{0x11},{0x11},{0x11},{0x11},{0x0E}};
  static u8 u8letterV[7][1] = {{0x11},{0x11},{0x11},{0x11},{0x11},{0x0A},{0x04}};
  static u8 u8letterW[7][1] = {{0x11},{0x11},{0x11},{0x11},{0x15},{0x15},{0x0E}};
  static u8 u8letterX[7][1] = {{0x11},{0x11},{0x0A},{0x04},{0x0A},{0x11},{0x11}};
  static u8 u8letterY[7][1] = {{0x11},{0x11},{0x11},{0x0A},{0x04},{0x04},{0x04}};
  static u8 u8letterZ[7][1] = {{0x1F},{0x10},{0x08},{0x04},{0x02},{0x01},{0x1F}};
  static u8 u8letterLowercaseA[7][1] = {{0x00},{0x00},{0x0E},{0x10},{0x1E},{0x11},{0x1E}};
  static u8 u8letterLowercaseB[7][1] = {{0x01},{0x01},{0x0F},{0x11},{0x11},{0x11},{0x0F}};
  static u8 u8letterLowercaseC[7][1] = {{0x00},{0x00},{0x0E},{0x01},{0x01},{0x11},{0x0E}};
  static u8 u8letterLowercaseD[7][1] = {{0x10},{0x10},{0x16},{0x19},{0x11},{0x11},{0x1E}};
  static u8 u8letterLowercaseE[7][1] = {{0x00},{0x00},{0x0E},{0x11},{0x1F},{0x01},{0x0E}};
  static u8 u8letterLowercaseF[7][1] = {{0x0C},{0x12},{0x02},{0x07},{0x02},{0x02},{0x02}};
  static u8 u8letterLowercaseG[7][1] = {{0x00},{0x1E},{0x11},{0x11},{0x1E},{0x10},{0x0E}};
  static u8 u8letterLowercaseH[7][1] = {{0x01},{0x01},{0x0D},{0x13},{0x11},{0x11},{0x11}};
  static u8 u8letterLowercaseI[7][1] = {{0x04},{0x00},{0x06},{0x04},{0x04},{0x04},{0x0E}};
  static u8 u8letterLowercaseJ[7][1] = {{0x08},{0x00},{0x0C},{0x08},{0x08},{0x09},{0x06}};
  static u8 u8letterLowercaseK[7][1] = {{0x01},{0x01},{0x09},{0x05},{0x03},{0x05},{0x09}};
  static u8 u8letterLowercaseL[7][1] = {{0x06},{0x04},{0x04},{0x04},{0x04},{0x04},{0x0E}};
  static u8 u8letterLowercaseM[7][1] = {{0x00},{0x00},{0x0B},{0x15},{0x15},{0x11},{0x11}};
  static u8 u8letterLowercaseN[7][1] = {{0x00},{0x00},{0x0D},{0x13},{0x11},{0x11},{0x11}};
  static u8 u8letterLowercaseO[7][1] = {{0x00},{0x00},{0x0E},{0x11},{0x11},{0x11},{0x1E}};
  static u8 u8letterLowercaseP[7][1] = {{0x00},{0x00},{0x0F},{0x11},{0x0F},{0x01},{0x01}};
  static u8 u8letterLowercaseQ[7][1] = {{0x00},{0x00},{0x16},{0x19},{0x1E},{0x10},{0x10}};
  static u8 u8letterLowercaseR[7][1] = {{0x00},{0x00},{0x0D},{0x13},{0x01},{0x01},{0x01}};
  static u8 u8letterLowercaseS[7][1] = {{0x00},{0x00},{0x0E},{0x01},{0x0E},{0x10},{0x0F}};
  static u8 u8letterLowercaseT[7][1] = {{0x02},{0x02},{0x07},{0x02},{0x02},{0x12},{0x0C}};
  static u8 u8letterLowercaseU[7][1] = {{0x00},{0x00},{0x11},{0x11},{0x11},{0x19},{0x16}};
  static u8 u8letterLowercaseV[7][1] = {{0x00},{0x00},{0x11},{0x11},{0x11},{0x0A},{0x04}};
  static u8 u8letterLowercaseW[7][1] = {{0x00},{0x00},{0x11},{0x11},{0x11},{0x15},{0x0A}};
  static u8 u8letterLowercaseX[7][1] = {{0x00},{0x00},{0x11},{0x0A},{0x04},{0x0A},{0x11}};
  static u8 u8letterLowercaseY[7][1] = {{0x00},{0x00},{0x11},{0x11},{0x1E},{0x10},{0x0E}};
  static u8 u8letterLowercaseZ[7][1] = {{0x00},{0x00},{0x1F},{0x08},{0x04},{0x02},{0x1F}};
  static u8 u8letterSpace[7][1] = {{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00}};

  const u8* u8pAddress;
  bool noLetterAssignmentMade = FALSE;
  switch(u8Letter)
  {
  case 'A':
    u8pAddress = &(u8letterA[0][0]);
    break;
  case 'B':
    u8pAddress = &(u8letterB[0][0]);
    break;
  case 'C':
    u8pAddress = &(u8letterC[0][0]);
    break;
  case 'D':
    u8pAddress = &(u8letterD[0][0]);
    break;
  case 'E':
    u8pAddress = &(u8letterE[0][0]);
    break;
  case 'F':
    u8pAddress = &(u8letterF[0][0]);
    break;
  case 'G':
    u8pAddress = &(u8letterG[0][0]);
    break;
  case 'H':
    u8pAddress = &(u8letterH[0][0]);
    break;
  case 'I':
    u8pAddress = &(u8letterI[0][0]);
    break;
  case 'J':
    u8pAddress = &(u8letterJ[0][0]);
    break;
  case 'K':
    u8pAddress = &(u8letterK[0][0]);
    break;
  case 'L':
    u8pAddress = &(u8letterL[0][0]);
    break;
  case 'M':
    u8pAddress = &(u8letterM[0][0]);
    break;
  case 'N':
    u8pAddress = &(u8letterN[0][0]);
    break;
  case 'O':
    u8pAddress = &(u8letterO[0][0]);
    break;
  case 'P':
    u8pAddress = &(u8letterP[0][0]);
    break;
  case 'Q':
    u8pAddress = &(u8letterQ[0][0]);
    break;
  case 'R':
    u8pAddress = &(u8letterR[0][0]);
    break;
  case 'S':
    u8pAddress = &(u8letterS[0][0]);
    break;
  case 'T':
    u8pAddress = &(u8letterT[0][0]);
    break;
  case 'U':
    u8pAddress = &(u8letterU[0][0]);
    break;
  case 'V':
    u8pAddress = &(u8letterV[0][0]);
    break;
  case 'W':
    u8pAddress = &(u8letterW[0][0]);
    break;
  case 'X':
    u8pAddress = &(u8letterX[0][0]);
    break;
  case 'Y':
    u8pAddress = &(u8letterY[0][0]);
    break;
  case 'Z':
    u8pAddress = &(u8letterZ[0][0]);
    break;
  case 'a':
    u8pAddress = &(u8letterLowercaseA[0][0]);
    break;
  case 'b':
    u8pAddress = &(u8letterLowercaseB[0][0]);
    break;
  case 'c':
    u8pAddress = &(u8letterLowercaseC[0][0]);
    break;
  case 'd':
    u8pAddress = &(u8letterLowercaseD[0][0]);
    break;
  case 'e':
    u8pAddress = &(u8letterLowercaseE[0][0]);
    break;
  case 'f':
    u8pAddress = &(u8letterLowercaseF[0][0]);
    break;
  case 'g':
    u8pAddress = &(u8letterLowercaseG[0][0]);
    break;
  case 'h':
    u8pAddress = &(u8letterLowercaseH[0][0]);
    break;
  case 'i':
    u8pAddress = &(u8letterLowercaseI[0][0]);
    break;
  case 'j':
    u8pAddress = &(u8letterLowercaseJ[0][0]);
    break;
  case 'k':
    u8pAddress = &(u8letterLowercaseK[0][0]);
    break;
  case 'l':
    u8pAddress = &(u8letterLowercaseL[0][0]);
    break;
  case 'm':
    u8pAddress = &(u8letterLowercaseM[0][0]);
    break;
  case 'n':
    u8pAddress = &(u8letterLowercaseN[0][0]);
    break;
  case 'o':
    u8pAddress = &(u8letterLowercaseO[0][0]);
    break;
  case 'p':
    u8pAddress = &(u8letterLowercaseP[0][0]);
    break;
  case 'q':
    u8pAddress = &(u8letterLowercaseQ[0][0]);
    break;
  case 'r':
    u8pAddress = &(u8letterLowercaseR[0][0]);
    break;
  case 's':
    u8pAddress = &(u8letterLowercaseS[0][0]);
    break;
  case 't':
    u8pAddress = &(u8letterLowercaseT[0][0]);
    break;
  case 'u':
    u8pAddress = &(u8letterLowercaseU[0][0]);
    break;
  case 'v':
    u8pAddress = &(u8letterLowercaseV[0][0]);
    break;
  case 'w':
    u8pAddress = &(u8letterLowercaseW[0][0]);
    break;
  case 'x':
    u8pAddress = &(u8letterLowercaseX[0][0]);
    break;
  case 'y':
    u8pAddress = &(u8letterLowercaseY[0][0]);
    break;
  case 'z':
    u8pAddress = &(u8letterLowercaseZ[0][0]);
    break;
  case ' ':
    u8pAddress = &(u8letterSpace[0][0]);
    break;
  default:
    noLetterAssignmentMade = TRUE;
    break;
  }
  if(!noLetterAssignmentMade)
  {
    PixelBlockType PBTbarInfo = {0,(charsOnScreen*8),7, (1*8)};
    LcdLoadBitmap(u8pAddress,&PBTbarInfo);
    charsOnScreen++;
  }
}
static void UserApp1SM_Error(void)
{

} /* end UserApp1SM_Error() */

/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
