#ifndef CANBUSHANDLER_H
#define CANBUSHANDLER_H

#include <Arduino.h>
#include <FlexCAN_T4.h>

class CanBusHandler
{
  public:
  CanBusHandler(FlexCAN_T4 <CAN0, RX_SIZE_256, TX_SIZE_16>& can, uint8_t groupNumber);

  void sendPaddlePosition(uint8_t paddlePosition);
  void sendBallPosition(uint8_t xBall, uint8_t yBall);

  private:
  FlexCAN_T4 <CAN0, RX_SIZE_256, TX_SIZE_16>& can_;
  uint8_t groupNumber_;

};
#endif