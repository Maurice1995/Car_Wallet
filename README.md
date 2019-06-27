# STM32F407 CAN bus CAR WALLET

SPI &amp; CAN &amp; I2C test environment and drivers for Car wallet project

## PINOUT 

CAN1 GPIO Configuration    

    PB8     ------> CAN1_RX
    
    PB9     ------> CAN1_TX 
    
SPI2 GPIO Configuration    

    PC2     ------> SPI2_MISO
    
    PC3     ------> SPI2_MOSI
    
    PB10     ------> SPI2_SCK 
    
    PE4      ------> SPI2_SS
    
I2C1 GPIO Configuration    

    PB6     ------> I2C1_SCL
    
    PB7     ------> I2C1_SDA 
    
## SCHEMATIC

   Please refer to the Schematic on Confluence:
   
   https://riddleandcode.atlassian.net/wiki/spaces/DEV/pages/721453097/Car+Wallet+Hardware+Specs

    
### HOW TO RUN 

   Build and flash the code for first board .Change #DEFINE BOARD1 to #DEFINE BOARD2 on top of main, build & flash to the 
   
 second board. Push button interrupt is enabled in such a way that eveytime the button is pushed a counter is incremented and
 
 value of it is transferred over CAN to the pair board. Trasferred number can be seen through user leds on STM32F4 Disco
   
 board.Works both way.CAN filter is enabled to accept message only from the paired nodes. 
    
    
  
  STDID of the first board  = 0xBE
   
  STDID of the second board = 0xEF
  
  
  ### TESTS
  
  Echo test over SPI for Nodemcu and store dummy data , read dummy data and compare test , generate key-pair, SHA256 a random 

string, sign the digest & verify the sign tests are present within the main source file. Red LED will blink in case any of 

those tests fail. Store data test is off by default to prevent overuse since the device has the capability to perform a 

certain number of EEPROM read / writes during it's lifetime. Add #define STORE_TEST to enable store test.
  
  #### TODO
  - Activate Wifi 
  - Perform dummy https post
  - Fix hanging nodemcu during boot when SS pin is high
