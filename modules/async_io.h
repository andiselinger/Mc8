#ifndef ASYNC_IO_H
#define ASYNC_IO_H


#include "global_definitions.h"
#include "systemc.h"



class asyncIO: public sc_module //SC_MODULE(asyncIO)
{

public:

	SC_HAS_PROCESS(asyncIO);

	sc_in<bool> wr;
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	sc_inout_rv<8> dataBus;

	sc_inout_rv<8> connect;

/*	SC_CTOR(asyncIO)
	{
    SC_THREAD(readData);
    SC_THREAD(writeData);


  }*/

	asyncIO(sc_module_name nm, sc_uint<8> address_, bool io_) : sc_module(nm), port_address(address_),io(io_)
	{
		// configure if input or output
		// io = 1 input io = 0 output

		if(io)
		{
			SC_THREAD(readData);
		}else
		{
			SC_THREAD(writeData);
		}
		SC_THREAD(weak);
	}



private:

	sc_bv<8> memory;
	sc_uint<8> port_address;
	bool io;



	void  readData ()
	{
		while (1)
		{
			wait (rd.negedge_event ());

			sc_bv<8> io_addr = addrBus.read ().range(7,0);

			if( io_addr == port_address)	dataBus.write (connect.read());
		}
	}

	void  writeData ()
	{
		while (1)
				{
					wait (rd.negedge_event ());

					sc_bv<8> io_addr = addrBus.read ().range(7,0);

					if( io_addr == port_address)	connect.write (dataBus.read());
				}
	}


	void weak()
	{
		// if there is no reading of the io set the data Output to all z

		while(1)
		{
			wait (rd.posedge_event ());
			dataBus.write("zzzzzzzz");
		}

	}
};



#endif
