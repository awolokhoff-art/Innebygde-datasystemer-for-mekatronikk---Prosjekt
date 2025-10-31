#include "canbushandler.h"

CanBusHandler::CanBusHandler(FlexCAN_T4 <CAN0, RX_SIZE_256, TX_SIZE_16>& can, uint8_t groupNumber)
  : can_{can}
  , groupNumber_{groupNumber}
  {}


void CanBusHandler::sendPaddlePosition(uint8_t paddlePosition)
{
    CAN_message_t msg;
    msg.id = groupNumber_ + 20;
    msg.len = 1;
    msg.buf[0] = paddlePosition;
    can_.write(msg);
}

void CanBusHandler::sendBallPosition(uint8_t xBall, uint8_t yBall)
{
    CAN_message_t msg;
    msg.id = groupNumber_ + 50; 
    msg.len = 2;
    msg.buf[0] = xBall;
    msg.buf[1] = yBall;
    can_.write(msg);
}