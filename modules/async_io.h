#ifndef ASYNC_IO_H
#define ASYNC_IO_H

#if 0

#include "systemc.h"

#define RAM_READ_DELAY_NS 5
#define RAM_WRITE_DELAY_NS 5

SC_MODULE(asyncIO)
{

public:

	sc_in<bool> wr;
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	sc_in<sc_bv<8> > dataBus_in;
	sc_out<sc_bv<8> > dataBus_out;


	/*SC_CTOR(asyncIO)
  {
    SC_THREAD(readData);
    SC_THREAD(writeData);

    address =
  }*/

	asyncIO(sc_module_name nm, sc_uint<8> address_) : sc_module(nm), address(address_)
	{
		busy = false;

		 SC_THREAD(readData);
		 SC_THREAD(writeData);
	}



private:

  sc_bv<8> memory[65536];    // 8 bit data, 16 bit address
  sc_uint<8> address;

  bool busy;


  void  readData ()
  {
    while (1)
    {
      wait (rd.negedge_event ());

      if(!busy){

    	  busy = true;
    	  address = addrBus.read ().to_uint ();
    	  wait (RAM_READ_DELAY_NS, SC_NS);
    	  dataBus_out.write (memory[address]);
    	  busy = false;
      }

    }
  }

  void  writeData ()
    {
      while (1)
      {
        wait (wr.negedge_event ());

        if(!busy){

          	  busy = true;
          	  address = addrBus.read ().to_uint ();
          	  wait (RAM_WRITE_DELAY_NS, SC_NS);
          	  memory[address] = dataBus_in.read();
          	busy = false;
        }
      }
    }

};

#endif

#endif
