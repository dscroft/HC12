#include "hc12.h"

void HC12::_cmd_mode( const bool onoff )
{
  if( digitalRead( cmd ) != ( onoff ? LOW : HIGH ) )
  {
      digitalWrite( cmd, onoff ? LOW : HIGH );
      delay( onoff ? 40 : 80 );
  }
}

HC12::_Cmd HC12::_send_cmd( const char* atcmd, 
                size_t timeout, 
                const bool debug )
{
  const unsigned long start = millis();

  _cmd_mode( true );

  // bookend the statement correctly
  ss.write( "AT+" );
  ss.write( atcmd );
  ss.write( "\r\n" );

  _Cmd result;
  String *buffer = &(result.confirm);    

  while( millis()-start <= timeout )
  {
    if( ss.available() )
    {
      char c = ss.read();
      switch( c )
      {
        case '+': buffer = &(result.value); continue;
        case ',': buffer = &(result.optional); continue;
        case '\r': break;
        case '\n': return result;
        default: *buffer += c; break;
      }
    }
  }

  return result;
}

bool HC12::_confirm_cmd( const char* atcmd, const char* value, 
                          size_t timeout, 
                          const bool debug )
{
  if( debug )
  {
    Serial.print( "atcmd: " ); Serial.println( atcmd );
    if( value != nullptr )
    { Serial.print( "value: " ); Serial.println( value ); }
  }

  _Cmd result = _send_cmd( atcmd, timeout, debug );

  if( debug )
  {
    Serial.println( result.confirm );
    Serial.println( result.value );
    Serial.println( result.optional );
  }

  if( result.optional.startsWith( "B" ) )
  {
    size_t baud = result.optional.substring(1).toInt();
    if( debug ) { Serial.print( "Change baud to: " ); Serial.println( baud ); }
    ss.begin( baud );

    // HC12 has changed baud rate so force an exit from cmd_mode
    _cmd_mode( false ); 
  }

  // expected value was not returned
  if( result.value != nullptr && strcmp(value, result.value.c_str()) != 0 ) 
    return false;
  
  // at command was not confirmed 
  if( strcmp("OK", result.confirm.c_str()) != 0 ) 
    return false;

  return true;      
}

bool HC12::set_transmit_mode( UART_MODE mode )
{
  String atcmd = "FU";

  switch( mode )
  {
    case FU1: atcmd += "1"; break;
    case FU2: atcmd += "2"; break;
    case FU3: atcmd += "3"; break;
    case FU4: atcmd += "4"; break;
    default: return false;
  }

  const bool result = _confirm_cmd( atcmd.c_str(), atcmd.c_str() );

  return result;
}

bool HC12::set_transmit_mode( UART_NAME mode )
{
  return set_transmit_mode( (UART_MODE)mode );
}

HC12::UART_MODE HC12::get_transmit_mode()
{
  _Cmd response = _send_cmd( "RF" );

  // at command was not confirmed 
  if( response.confirm != "OK" ) 
    return NA;

        if( response.value == "FU1" ) return FU1;
  else if( response.value == "FU2" ) return FU2;
  else if( response.value == "FU3" ) return FU3;
  else if( response.value == "FU4" ) return FU4;
  
  return NA;
}


bool HC12::set_channel( const uint8_t channel )
{
  /*AT+Cxxx
Change wireless communication channel, selectable from 001 to 127 (for wireless
channels exceeding 100, the communication distance cannot be guaranteed). The
default value for the wireless channel is 001, with a working frequency of
433.4MHz. The channel stepping is 400KHz, and the working frequency of channel 
100 is 473.0MHz*/
  if( channel < 1 || channel > 127 ) return false;

  char atcmd[] = "C000";
	snprintf( atcmd, sizeof(atcmd), "C%03d", channel );

  return _confirm_cmd( atcmd, atcmd );
}

uint8_t HC12::get_channel()
{
  /* Send command “AT+RC” to the module, and if the module returns “OK+RC001” it is
confirmed that the communication channel of the module is 001. */
  uint8_t channel = 0;
  static const char* atcmd = "RC";

  _Cmd response = _send_cmd( atcmd );
  if( response.confirm == "OK" && 
      response.confirm.startsWith( atcmd ) )
  {
    channel = response.value.toInt();
  }

  return channel;
}

void HC12::transparent()
{
  _cmd_mode( false );
}


bool HC12::sendMessage( byte action, bool ack, const char *fmt, ... )
{
  _cmd_mode( false );

  bool sent = false;
  va_list args;
  for( size_t attempt=0; !sent; ++attempt )
  {
    sent = sc.sendMessage( action, ack, fmt, args );
  }

  return sent;
}

bool HC12::sendMessage( byte action, bool ack )
{
  return sc.sendMessage( action, ack );
}

bool HC12::sendAck( const char *fmt, ... )
{
  _cmd_mode( false );

  va_list args;
  return sc.sendAck( fmt, args ); 
}

bool HC12:: sendAck()
{
  _cmd_mode( false );
  return sc.sendAck();
}

bool HC12::init()
{
  static const int bauds = 8;
  static const uint32_t baudRates[bauds] = 
    {9600,1200,2400,4800,19200,38400,57600,115200}; // in order of liklihood
  static const char *atcmd = "RB";
  char okcmd[] = "B000000";

  for( int i=0; i<bauds; ++i )
  {
    ss.begin( baudRates[i] );

    snprintf( okcmd, sizeof(okcmd), "B%d", baudRates[i] );
    if( _confirm_cmd( atcmd, okcmd ) ) 
      return true;
  }

  return false;
}

bool HC12::sleep()
{
  static const char* cmd = "SLEEP";
  return _confirm_cmd( cmd, cmd );
}

bool HC12::wake()
{
  return _confirm_cmd( "", "" );
} 

bool HC12::restore_defaults()
{
  static const char* cmd = "DEFAULT";
  return _confirm_cmd( cmd, cmd );
}

HC12::HC12( uint8_t RX, uint8_t TX, uint8_t CMD ) : 
  ss( RX, TX ), sc( ss ), cmd( CMD )
{
  pinMode( cmd, OUTPUT );
  digitalWrite( cmd, HIGH );
}
