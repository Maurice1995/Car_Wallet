# STM32F407_CAN_DAIMLER
SPI &amp; CAN &amp; I2C test environment and drivers for daimler project

## PINOUT 

/**CAN1 GPIO Configuration    
    PB8     ------> CAN1_RX
    PB9     ------> CAN1_TX 
    */
/**SPI2 GPIO Configuration    
    PC2     ------> SPI2_MISO
    PC3     ------> SPI2_MOSI
    PB10     ------> SPI2_SCK 
    PE4      ------> SPI2_SS
    */
    
## SCHEMATIC    
    ![screenshot](https://user-images.githubusercontent.com/46520743/58031348-e3aa2e00-7b28-11e9-8bb5-05407df18fe7.png)
    
### HOW TO RUN    
    Build and flash the code for first board .Change #DEFINE BOARD1 to #DEFINE BOARD2 on top of main, build & flash to the second board. Push button interrupt is enabled in such a way that eveytime the button is pushed a counter is incremented and value of it is transferred over CAN to the pair board. Trasferred number can be seen through user leds on STM32F4 Disco board.Works both way.CAN filter is enabled to accept message only from the paired nodes. 
    
   STDID of the first board  = 0xBE;
   STDID of the second board = 0xEF;
