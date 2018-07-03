#ifndef HC12_H
#define HC12_H

#include <serialcomm.h>

namespace hc12
{
  enum TRANSMIT_MODE { FU1, FU2, FU3, FU4, NA };
  enum TRANSMIT_NAME { LOWPWR=FU1, VERYLOW=FU2, NORMAL=FU3, HIGHPWR=FU4 };

  class BaseHC12
  {
  private:
    struct _Cmd
    {
      String confirm;
      String value;
      String optional;
    };

    const uint8_t cmd;
    
    _Cmd _send_cmd( const char* atcmd, 
                    //size_t timeout=500, 
                    const bool debug=false );

  /* send AT commands to the hc12, 
      if optional 'value' parameter is set then will return true/false depening on
      if hc12 responds with that */
    bool _confirm_cmd( const char* atcmd, const char* value=nullptr, 
                            //size_t timeout=500, 
                            const bool debug=false );                  

  protected:
    size_t timeout = 5000;
    /* bring the CMD pin low to put the HC12 in command mode, needed so we can configure it */
    void _cmd_mode( const bool onoff );

    /* needed for BaseHC12 behaviour
      you might ask why have _functions rather than just inheriting from
      Stream and overriding the functions properly? well these _functions
      are only used when the hc12 is in command mode as opposed to the 
      overridden functions that are used when it's in transparent mode and
      the behaviour is slightly different */
    virtual void _begin( long ) = 0;
    virtual size_t _write( const char* ) = 0;
    virtual int _read() = 0;
    virtual int _available() = 0;
    virtual  void _flush() = 0;

    BaseHC12( uint8_t _cmd );

  public:
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
    bool set_transmit_mode( TRANSMIT_MODE mode=FU4 );
    bool set_transmit_mode( TRANSMIT_NAME mode=HIGHPWR );

    TRANSMIT_MODE get_transmit_mode();

    bool set_channel( const uint8_t channel );
    uint8_t get_channel();

     /* Trouble is that we can't know on boot what the current settings on the HC12 
      are and therefore what the baud rate is. So try all the supported speeds until 
      we find one that works */
    bool begin(bool debug=false);
  };

  template<typename S>
  class HC12: public Stream, public BaseHC12
  {
  private:
    S& serial;

    // needed by BaseHC12
    void _begin( long speed );
    size_t _write( const char* byte );
    int _read();
    int _available();
    void _flush();

  public:
    // needed by Stream
    size_t write( uint8_t byte );   
    //size_t write( uint8_t byte ); 
    using Print::write;
    int peek();
    int read();
    int available();

    HC12( S& _serial, uint8_t _cmd );
    HC12( S&& _serial, uint8_t _cmd);
  };
};

template<typename S>
void hc12::HC12<S>::_begin( long speed )         { serial.begin( speed ); }
template<typename S>
size_t hc12::HC12<S>::_write( const char* byte ) { return serial.write( byte ); }
template<typename S>
int hc12::HC12<S>::_read()                       { return serial.read(); }
template<typename S>
int hc12::HC12<S>::_available()                  { return serial.available(); }
template<typename S>
void hc12::HC12<S>::_flush()                     { return serial.flush(); }

template<typename S>
int hc12::HC12<S>::peek() 
{ 
  _cmd_mode( false );
  return serial.peek(); 
}

template<typename S>
size_t hc12::HC12<S>::write( uint8_t byte ) 
{ 
  _cmd_mode( false );
  return serial.write( byte ); 
}

/*template<typename S>
size_t hc12::HC12<S>::write( const char* byte ) 
{ 
  _cmd_mode( false );
  return serial.write(byte); 
}*/

template<typename S>
int hc12::HC12<S>::read() 
{ 
  _cmd_mode( false );
  return serial.read(); 
}

template<typename S>
int hc12::HC12<S>::available() 
{ 
  _cmd_mode( false );
  return serial.available(); 
}

template<typename S>
hc12::HC12<S>::HC12( S& _serial, uint8_t _cmd )
  : BaseHC12(_cmd), serial(_serial) 
{}

template<typename S>
hc12::HC12<S>::HC12( S&& _serial, uint8_t _cmd )
  : BaseHC12(_cmd), serial(_serial)
{}

#endif
