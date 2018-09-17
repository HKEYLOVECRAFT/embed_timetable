#include <GxEPD.h>
#include <GxGDE0213B1/GxGDE0213B1.cpp>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// init spi-bus
GxIO_Class io(SPI, SS, D3, D4);
GxEPD_Class display(io);

void setup() {
  display.init();
  display.setTextColor(GxEPD_BLACK);
  
  display.setCursor(30, 60);
  display.print("Hello World");
  display.update();
}

void loop() {}