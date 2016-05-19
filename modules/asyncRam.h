#include "systemc.h"
#include "global_definitions.h"
#include <cstdio>

SC_MODULE(asyncRam)
{

public:

	sc_in<bool> wr;
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	//sc_in<sc_bv<8> > dataBus_in;
	//sc_out<sc_bv<8> > dataBus_out;

	sc_out_rv<8> dataBus;

	SC_CTOR(asyncRam)
	{
		SC_THREAD(readData);
		SC_THREAD(writeData);

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
			wait (rd.negedge_event ());

			if (!busy)
			{
				busy = true;
				address = addrBus.read ().to_uint ();
				wait (RAM_READ_DELAY_NS, SC_NS);
				dataBus.write (memory[address]);
#ifdef TEST
				cout << "RAM here, reading Data: " << memory[address]
						<< " from Address :" << address << endl;
#endif
				wait (rd.posedge_event ());
				wait (1, SC_NS);
				dataBus.write ("zzzzzzzz");
				busy = false;
			}
		}
	}

	void
	writeData ()
	{
		while (1)
		{
			wait (wr.negedge_event ());

			if (!busy)
			{
				busy = true;
				address = addrBus.read ().to_uint ();
				wait (RAM_WRITE_DELAY_NS, SC_NS);
				memory[address] = read_databus_resolved ();
#ifdef TEST
				cout << "RAM here, writing Data: " << "0x" << hex << memory[address] << " to Address :"
						<< "0x" << hex << address << endl;
#endif
				busy = false;
			}
		}
	}


	sc_bv<8> read_databus_resolved ()
		{
			sc_bv<8> input_data_bv;
			sc_lv<8> input_data_lv = dataBus.read ();

			if (input_data_lv == "zzzzzzzz" || input_data_lv == "xxxxxxxx")
			{
				cout << "Module asyncRAM: Error reading dataBus : undefined value" << endl;
				input_data_bv = "00000000";
			//	exit (-1);
			}
			else
			{
				input_data_bv = input_data_lv.to_uint ();
			}

			return input_data_bv;
		}

};
