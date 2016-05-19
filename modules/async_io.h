#ifndef ASYNC_IO_H
#define ASYNC_IO_H

#include "global_definitions.h"
#include "systemc.h"

class asyncIO : public sc_module //SC_MODULE(asyncIO)
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

	asyncIO (sc_module_name nm, sc_uint<8> address_, bool io_) :
			sc_module (nm), port_address (address_), io (io_)
	{
		// configure if input or output
		// true: input, false: output

		if (io)
		{
			SC_THREAD(readData);
		}
		else
		{
			SC_THREAD(writeData);
		}

	}

private:

	sc_bv<8> memory;
	sc_uint<8> port_address;
	bool io;

	void
	readData ()
	{
		while (1)
		{
			wait (rd.negedge_event ());
			wait(1,SC_NS);
			sc_bv<8> io_addr = addrBus.read ().range (7, 0);

			if (io_addr == port_address)
				dataBus.write (connect.read ());

			wait (rd.posedge_event ());
			wait(1,SC_NS);
									dataBus.write ("zzzzzzzz");
		}
	}

	void
	writeData ()
	{
		while (1)
		{
			wait (wr.negedge_event ());
			wait(1,SC_NS);
			sc_bv<8> io_addr = addrBus.read ().range (7, 0);

			if (io_addr == port_address)
				connect.write (read_databus_resolved());


		}
	}



	sc_bv<8> read_databus_resolved ()
			{
				sc_bv<8> input_data_bv;
				sc_lv<8> input_data_lv = dataBus.read ();

				if (input_data_lv == "zzzzzzzz" || input_data_lv == "xxxxxxxx")
				{
					cout << "Module asyncIO: Error reading dataBus : undefined value" << endl;
					input_data_bv = "00000000";
					exit (-1);
				}
				else
				{
					input_data_bv = input_data_lv.to_uint ();
				}

				return input_data_bv;
			}
};

#endif
