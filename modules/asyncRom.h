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
		SC_THREAD(weak);
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

		/*memory[0] = 0x3A;
		 memory[1] = 0x03;
		 memory[2] = 0x00;
		 memory[3] = 0xFF;
		 memory[4] = 0x22;
		 memory[0x2205] = 0x3D;
		 memory[0x2206] = 0xC3;
		 memory[0x2207] = 0x02;
		 memory[0x2208] = 0x00;
		 memory[0x2209] = 0xC3;
		 memory[0x220A] = 0x05;
		 memory[0x220B] = 0x22;*/

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
			cout << "ROM here, reading Data: " << memory[addressToRead]
					<< " from Address :" << addressToRead << endl;
#endif
			dataBus.write (memory[addressToRead]);
		}
	}

	void
	weak ()
	{
		while (1)
		{
			wait (rd.posedge_event ());
			dataBus.write ("zzzzzzzz");
		}
	}

};
