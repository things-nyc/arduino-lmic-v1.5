/*********************************************************************
 *
 * Copy this to "test_node_secrets.h" and fill your private details
 *
 * See 
 *
 * http://staging.thethingsnetwork.org/wiki/Backend/ttnctl/QuickStart
 *
 * In particular you need a local executable for ttnctl
 *
 * once you have that, ttnctl-HOST-CPU devices devices info DEVADDR  
 *
 * will give you the node detials to fill in below 
 *
 **********************************************************************/

// LoRaWAN Application identifier (AppEUI)
#define MY_APPEUI { INSERT 8 HEX BYTES HERE WITH COMMAS }

// LoRaWAN DevEUI, unique device ID (LSBF)
#define MY_DEVEUI { INSERT 8 HEX BYTES HERE WITH COMMAS }

//NwkSKey
#define MY_NWKSKEY { INSERT 16 HEX BYTES HERE WITH COMMAS }
           
//AppSKey
#define MY_APPSKEY { INSERT 16 HEX BYTES HERE WITH COMMAS }

//put 32-bit hex value here as a single number, run together, no commans
#define MY_DEVADDR 0x........ 

