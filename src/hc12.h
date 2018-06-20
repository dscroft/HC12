#ifndef HC12_H
#define HC12_H

#include <SoftwareSerial.h>
#include "serialcomm.h"

class HC12
{
public:
  SoftwareSerial ss;
  SerialComm sc;
  const uint8_t cmd;

  /* bring the CMD pin low to put the HC12 in command mode, needed so we can configure it */
  /*inline*/ void _cmd_mode( const bool onoff );

  /* send AT commands to the hc12, 
    if optional 'result' parameter is set then will return true/false depening on
    if hc12 responds with value of result */
  struct _Cmd
  {
    String confirm;
    String value;
    String optional;
  };

  _Cmd _send_cmd( const char* atcmd, 
                  size_t timeout=500, 
                  const bool debug=false );

  bool _confirm_cmd( const char* atcmd, const char* value=nullptr, 
                          size_t timeout=500, 
                          const bool debug=false );

public:
  enum UART_MODE { FU1, FU2, FU3, FU4, NA };
  enum UART_NAME { LOWPWR=FU1, VERYLOW=FU2, NORMAL=FU3, HIGHPWR=FU4 };

  /* puts it into low power mode, can't send or recieve in this mode but
    power usage should drop to about ??? */
  bool sleep();

  /* technically any command will bring it out of sleep mode but seperate
    function for clarity */
  bool wake();

  void transparent();

  bool restore_defaults();

  /* default is FU4/HIGHPWR since if you're using a HC12 I assume it's because 
    you want long range communications */ 
  bool set_transmit_mode( UART_MODE mode=FU4 );
  bool set_transmit_mode( UART_NAME mode=HIGHPWR );

  UART_MODE get_transmit_mode();

  bool set_channel( const uint8_t channel );
  uint8_t get_channel();

  bool sendMessage( byte action, bool ack, const char *fmt, ... );
  bool sendMessage( byte action, bool ack );

  bool sendAck( const char *fmt, ... );
  bool sendAck();

  /* Trouble is that we can't know on boot what the current settings on the HC12 
    are and therefore what the baud rate is. So try all the supported speeds until 
    we find one that works */
  bool init();

  HC12( uint8_t RX, uint8_t TX, uint8_t CMD );
};

#endif
