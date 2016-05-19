#include "systemc.h"
#include "global_definitions.h"
#include <cstdio>

#define TEST
SC_MODULE(asyncRam)
{

public:

	sc_in<bool> wr;
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	sc_out_rv<8> dataBus;

	SC_CTOR(asyncRam)
	{
		SC_THREAD(readData);
		sensitive << rd;
		SC_THREAD(writeData);
		sensitive << wr;

		busy = false;
	}

	void
	initMemory (unsigned char *data)
	{
		// copy ramfile to ram memory
		for (int i = 0; i < RAM_SIZE; i++)
		{
			if (data[i])
				cout << "Initialize RAM Address 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << i << " with 0x" << std::setw (2)
						<< (int) data[i] << endl;
			memory[i] = data[i];
		}
	}

private:

	sc_bv<8> memory[RAM_SIZE];    // 8 bit data, 16 bit address
	unsigned int address;
	bool busy;

	void
	readData ()
	{
		while (1)
		{
			wait ();
			if (!busy && rd.read () == false)  // neg edge event
			{
				busy = true;
				address = addrBus.read ().to_uint ();
				wait (RAM_READ_DELAY_NS, SC_NS);
				dataBus.write (memory[address]);
#ifdef TEST
				cout << "RAM here, reading Data: " << memory[address]
						<< " from Address :" << address << endl;
#endif
				busy = false;
			}
			else if (rd.read () == true)  // pos edge event
			{
				dataBus.write ("zzzzzzzz");
			}
		}
	}

	void
	writeData ()
	{
		while (1)
		{
			wait ();  // wait for wr event
			dataBus.write ("zzzzzzzz");
			if (!busy && wr.read () == false)  // neg edge
			{
				busy = true;
				address = addrBus.read ().to_uint ();
				wait (RAM_WRITE_DELAY_NS, SC_NS);
				memory[address] = dataBus.read ();
#ifdef TEST
				cout << "RAM here, writing Data: " << memory[address] << " to Address :"
						<< address << endl;
#endif
				busy = false;
			}
		}
	}

};
