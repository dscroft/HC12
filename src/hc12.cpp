#include "hc12.h"

hc12::BaseHC12::BaseHC12( uint8_t _cmd ) : cmd( _cmd )
{
  pinMode( cmd, OUTPUT );
  digitalWrite( cmd, HIGH );
}

void hc12::BaseHC12::_cmd_mode( const bool onoff )
{
  if( digitalRead( cmd ) != ( onoff ? LOW : HIGH ) )
  {
      digitalWrite( cmd, onoff ? LOW : HIGH );
      delay( 200 );
      _flush();
  }
}

hc12::BaseHC12::_Cmd hc12::BaseHC12::_send_cmd( const char* atcmd, 
                //size_t timeout, 
                const bool debug )
{
  const unsigned long start = millis();

  _cmd_mode( true );

  // bookend the statement correctly
  _write( "AT+" );
  _write( atcmd );
  _write( "\r\n" );

  _Cmd result;
  String *buffer = &(result.confirm);    

  while( millis()-start <= timeout )
  {
#ifdef wdt_reset
    wdt_reset();
#endif

    if( _available() )
    {
      char c = _read();
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


bool hc12::BaseHC12::_confirm_cmd( const char* atcmd, const char* value, 
                          //size_t timeout, 
                          const bool debug )
{
  if( debug )
  {
    Serial.println();
    Serial.println( "DEBUG confirm cmd" );
    Serial.print( "DEBUG atcmd: " ); Serial.println( atcmd );
    if( value != nullptr )
    { 
      Serial.print( "DEBUG value: " ); Serial.println( value ); 
    }
  }

  _Cmd result = _send_cmd( atcmd, debug );

  if( debug )
  {
    Serial.print( "DEBUG confirm:  " ); Serial.println( result.confirm );
    Serial.print( "DEBUG value:    " ); Serial.println( result.value );
    Serial.print( "DEBUG optional: " ); Serial.println( result.optional );
  }

  if( result.optional.startsWith( "B" ) )
  {
    size_t baud = result.optional.substring(1).toInt();
    if( debug ) 
    { 
      Serial.print( "DEBUG change baud: " ); Serial.println( baud ); 
    }
    _begin( baud );

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

bool hc12::BaseHC12::set_transmit_mode( hc12::TRANSMIT_MODE mode )
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

bool hc12::BaseHC12::set_transmit_mode( hc12::TRANSMIT_NAME mode )
{
  return set_transmit_mode( (hc12::TRANSMIT_MODE)mode );
}

hc12::TRANSMIT_MODE hc12::BaseHC12::get_transmit_mode()
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


bool hc12::BaseHC12::set_channel( const uint8_t channel )
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

uint8_t hc12::BaseHC12::get_channel()
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

void hc12::BaseHC12::transparent()
{
  _cmd_mode( false );
}

bool hc12::BaseHC12::begin(bool debug)
{
  static const int bauds = 8;
  static const uint32_t baudRates[bauds] = 
    {9600,1200,2400,4800,19200,38400,57600,115200}; // in order of liklihood
  static const char *atcmd = "RB";
  char okcmd[] = "B000000";

  for( int i=0; i<bauds; ++i )
  {
    _begin( baudRates[i] );
    _cmd_mode( false );

    snprintf( okcmd, sizeof(okcmd), "B%d", baudRates[i] );
    if( _confirm_cmd( atcmd, okcmd, debug ) ) 
      return true;
  }

  return false;
}

bool hc12::BaseHC12::sleep()
{
  static const char* cmd = "SLEEP";
  return _confirm_cmd( cmd, cmd );
}

bool hc12::BaseHC12::wake()
{
  return _confirm_cmd( "", "", true );
} 

bool hc12::BaseHC12::restore_defaults()
{
  static const char* cmd = "DEFAULT";
  return _confirm_cmd( cmd, cmd );
}

/*HC12::HC12( uint8_t RX, uint8_t TX, uint8_t CMD ) : 
  ss( RX, TX ), sc( ss ), cmd( CMD )
{
  pinMode( cmd, OUTPUT );
  digitalWrite( cmd, HIGH );
}*/


