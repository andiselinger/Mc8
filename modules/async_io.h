#ifndef ASYNC_IO_H
#define ASYNC_IO_H

#include "global_definitions.h"
#include "systemc.h"

extern sc_trace_file *fp;  // Trace file from main.cpp

class asyncIO : public sc_module //SC_MODULE(asyncIO)
{

public:

	SC_HAS_PROCESS(asyncIO);

	sc_in<bool> wr;
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	sc_inout_rv<8> dataBus;

	sc_inout_rv<8> connect;


	asyncIO (sc_module_name nm, sc_uint<8> address_, bool io_) :
			sc_module (nm), port_address (address_), io (io_)
	{
		// configure if input or output
		// true: input, false: output


		sc_trace (fp, rd, "RD_IO");
		if (io)
		{
			SC_THREAD(readData);
			sensitive << rd;
		}
		else
		{
			SC_THREAD(writeData);
		}
		//SC_THREAD(weak);
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
			wait ();  // for rd event
			if (rd.read () == false)  // neg edge
			{
				std::clog << "READ DATA FROM IO" << std::endl;
				sc_bv<8> io_addr = addrBus.read ().range (7, 0);

				if (io_addr == port_address)
					dataBus.write (connect.read ());
			}
			else   // pos edge
			{
				sc_bv<8> io_addr = addrBus.read ().range (7, 0);

				std::clog << "IO: Write zzzzzzzz to data Bus @" << sc_time_stamp ()
						<< std::endl;
				dataBus.write ("zzzzzzzz");
			}

		}
	}

	void
	writeData ()
	{
		while (1)
		{
			wait (wr.negedge_event ());
			std::clog << "WRITE DATA TO IO @ " << sc_time_stamp () << std::endl;
			sc_bv<8> io_addr = addrBus.read ().range (7, 0);
#ifdef DEBUG
			std::cout << "IO_mod: io addr: 0x" << io_addr.to_uint ()
					<< ", port address 0x" << port_address.to_uint () << std::endl;
#endif
			if (io_addr == port_address)
			{
#ifdef DEBUG
				std::cout << "IO_mod: Write 0x" << std::hex << std::setw (2)
						<< dataBus.read () << "to output" << std::endl;
#endif
				connect.write (dataBus.read ());
			}
		}
	}
};

#endif
