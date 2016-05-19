#include "systemc.h"
#include "global_definitions.h"

SC_MODULE(asyncRom)
{

public:
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	//sc_out<sc_bv<8> > dataBus;

	sc_out_rv<8> dataBus;

	SC_CTOR(asyncRom)
	{
		SC_THREAD(readData);

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
	readData ()
	{
		while (1)
		{
			wait (rd.negedge_event ());
			addressToRead = addrBus.read ().to_uint ();
			wait (ROM_READ_DELAY_US, SC_NS);
#ifdef TEST
			cout << "ROM here, reading Data: " << "0x" << hex << memory[addressToRead]
			 << " from Address :"  << "0x" << hex << addressToRead << endl;
#endif
			dataBus.write (memory[addressToRead]);

			wait (rd.posedge_event ());
			wait(1,SC_NS);
			dataBus.write ("zzzzzzzz");

		}
	}



};
