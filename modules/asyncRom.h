#include "systemc.h"

#define ROM_READ_DELAY_NS 500

SC_MODULE(asyncRom) {

public:
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	sc_out<sc_bv<8> > dataBus;

	SC_CTOR(asyncRom) {
		SC_THREAD(readData);

	}

	void initMemory() {
		/*
		memory[0] = 0x3E;
		memory[1] = 0x01;
		memory[2] = 0xC3;
		memory[3] = 0x09;
		memory[4] = 0x22;
		memory[0x2205] = 0x3D;
		memory[0x2206] = 0xC3;
		memory[0x2207] = 0x02;
		memory[0x2208] = 0x00;
		memory[0x2209] = 0xC3;
		memory[0x220A] = 0x05;
		memory[0x220B] = 0x22;
*/
		memory[0] = 0x21;
		memory[1] = 0x88;
		memory[2] = 0x44;
		//memory[3] = 0xFF;
		//memory[4] = 0xA8;
		//memory[5] = 0x03;
		//memory[6] = 0x80;

	}

private:

	sc_bv<8> memory[65536];			// 16 bit address
	unsigned int addressToRead;

	void readData() {
		while (1) {
			wait(rd.negedge_event());
			addressToRead = addrBus.read().to_uint();
			wait(ROM_READ_DELAY_NS, SC_US);
			dataBus.write(memory[addressToRead]);
		}
	}

};
