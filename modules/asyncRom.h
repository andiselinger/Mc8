#include "systemc.h"
#include "global_definitions.h"

extern sc_trace_file *fp;  // Trace file from main.cpp


SC_MODULE(asyncRom)
{

public:
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	//sc_out<sc_bv<8> > dataBus;

	sc_out_rv<8> dataBus;

	SC_CTOR(asyncRom)
	{
		sc_trace (fp, rd, "RD_ROM");
		SC_THREAD(doIt);
		sensitive << rd;
	}

	void
	initMemory (unsigned char *data)
	{
		// copy data to rom
		for (int i = 0; i < ROM_SIZE; i++)
		{
			if (data[i])
				cout << "Initialize ROM Address 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << i << " with 0x" << std::setw (2)
						<< (int) data[i] << endl;
			memory[i] = data[i];
		}
	}

private:

	sc_bv<8> memory[ROM_SIZE];
	unsigned int addressToRead;

	void
	doIt ()
	{
		while (1)
		{
			wait ();  // for event on rd

			if (rd.read () == false)  // neg edge
			{
				addressToRead = addrBus.read ().to_uint ();
				wait (ROM_READ_DELAY_US, SC_NS);
#ifdef TEST
				cout << "ROM here, reading Data: " << memory[addressToRead]
						<< " from Address :" << addressToRead << endl;
#endif
				dataBus.write (memory[addressToRead]);
			}
			else  // pos edge
			{
#ifdef TEST
				std::cout << "ROM: RD pos edge @" << sc_time_stamp () << std::endl;
#endif
				dataBus.write ("zzzzzzzz");
			}
		}
	}
};
