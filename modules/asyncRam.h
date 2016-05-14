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

		/*
		 FILE *pfile;

		 pfile = fopen (filename.c_str(),"r");
		 if (pfile!=NULL)
		 {
		 for(int i; i < SIZE; i++)
		 {
		 int val = fgetc (pfile);

		 if (val == EOF) break;

		 memory[i]=val;
		 }
		 }else
		 {
		 cout << " file error : memory not initialized" << endl;
		 }

		 memory[0x0000] = 0x3A; // test MOV A,label
		 memory[0x0001] = 0x01;
		 memory[0x0002] = 0x30;
		 memory[0x0003] = 0x3C; // inc A
		 memory[0x0004] = 0x32; // test MOV label,A
		 memory[0x0005] = 0x22;
		 memory[0x0006] = 0x3D;
		 memory[0x1001] = 0xFF; // test MOV A,label
		 memory[0x1002] = 0x02;
		 memory[0x1003] = 0x00;
		 memory[0x1004] = 0xC3;
		 memory[0x1006] = 0x05;
		 memory[0x1007] = 0x22;
		 */
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
			dataBus.write ("zzzzzzzz");
			if (!busy)
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
