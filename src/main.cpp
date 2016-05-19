#include <iostream>
#include <iomanip>
#include <string>
#include "systemc.h"

//#include "SESYD.hpp"
//LibSESYD::FileWriter *myWriter;

sc_trace_file *fp;  // Trace file

#include "clkResGen.h"
#include "asyncRom.h"
#include "asyncRam.h"
#include "async_io.h"
#include "mc8.h"
#include "adress_decoder.h"
#include "global_definitions.h"
#include "hexfile.h"
#include "async_io.h"

using namespace std;

int
sc_main (int argc, char* argv[])
{

	if (argc < 2)
	{
		std::cout << "Please specify hex file as parameter!" << std::endl;
		return -1;
	}

	string filename = argv[1];

	hex_file *hf = new hex_file (filename);

	unsigned char *mem = new unsigned char[ADDRESSABLE_AREA];

	if (hf->copy_data_to_memory (mem, ADDRESSABLE_AREA) != 0)
	{
		cout << "Error with hexfile!" << endl;
		return -1;
	}

	unsigned char *rom_data = mem;
	unsigned char *ram_data = mem + RAM_START;

	sc_signal<bool> sig_clk;
	sc_signal<bool> sig_tclk;
	sc_signal<bool> sig_res;

	sc_signal<bool> sig_wr;
	sc_signal<bool> sig_rd;
	sc_signal<bool> sig_mreq;
	sc_signal<bool> sig_iorq;

	sc_signal<bool> sig_wr_ram;
	sc_signal<bool> sig_wr_io;

	sc_signal<bool> sig_rd_ram;
	sc_signal<bool> sig_rd_rom;
	sc_signal<bool> sig_rd_io;

	sc_signal<sc_bv<16> > sig_addressBus_cpu;
	sc_signal<sc_bv<16> > sig_addressBus_mem;

	sc_signal_rv<8> sig_dataBus;
	sc_signal_rv<8> connect_0;
	sc_signal_rv<8> connect_1;

	// Create trace file
	fp = sc_create_vcd_trace_file ("wave");
	fp->set_time_unit (1, SC_NS);

	// Components
	clkResGen crg ("CLKRES");
	crg.clk (sig_clk);
	crg.tclk (sig_tclk);
	crg.res (sig_res);

	mc8 myMC8 ("myMC8");
	myMC8.clk (sig_clk);
	myMC8.tclk (sig_tclk);
	myMC8.res (sig_res);
	myMC8.wr (sig_wr);
	myMC8.rd (sig_rd);
	myMC8.mreq (sig_mreq);
	myMC8.iorq (sig_iorq);
	myMC8.dataBus (sig_dataBus);
	myMC8.addressBus (sig_addressBus_cpu);

	// constructor needs name, address, bool input or output
	asyncIO io0 ("io0", 0x00, true);  // true: input, false: output
	io0.addrBus (sig_addressBus_cpu);
	io0.dataBus (sig_dataBus);
	io0.wr (sig_wr_io);
	io0.rd (sig_rd_io);
	io0.connect (connect_0);

	asyncIO io1 ("io1", 0x03, false);  // true: input, false: output
	io1.addrBus (sig_addressBus_cpu);
	io1.dataBus (sig_dataBus);
	io1.wr (sig_wr_io);
	io1.rd (sig_rd_io);
	io1.connect (connect_1);

	asyncRom myRom ("myROM");
	myRom.rd (sig_rd_rom);
	myRom.addrBus (sig_addressBus_mem);
	myRom.dataBus (sig_dataBus);
	myRom.initMemory (rom_data);

	asyncRam myRam ("myRAM");
	myRam.rd (sig_rd_ram);
	myRam.wr (sig_wr_ram);
	myRam.addrBus (sig_addressBus_mem);
	myRam.dataBus (sig_dataBus);
	myRam.initMemory (ram_data);

	address_decoder myAddressDecoder ("myAddressDecoder");
	myAddressDecoder.address_bus_in (sig_addressBus_cpu);
	myAddressDecoder.address_bus_out (sig_addressBus_mem);
	myAddressDecoder.RD_in (sig_rd);
	myAddressDecoder.WR_in (sig_wr);
	myAddressDecoder.IO_in (sig_iorq);
	myAddressDecoder.M_in (sig_mreq);
	myAddressDecoder.RD_ram (sig_rd_ram);
	myAddressDecoder.WR_ram (sig_wr_ram);
	myAddressDecoder.RD_rom (sig_rd_rom);
	myAddressDecoder.WR_io (sig_wr_io);
	myAddressDecoder.RD_io (sig_rd_io);

	// Tracing
	sc_trace (fp, sig_clk, "clk");
	sc_trace (fp, sig_res, "res");
	sc_trace (fp, sig_wr, "wr");
	sc_trace (fp, sig_wr_io, "wr_io");
	sc_trace (fp, sig_rd, "rd");
	sc_trace (fp, sig_rd_io, "rd_io");
	sc_trace (fp, sig_mreq, "mreq");
	sc_trace (fp, sig_iorq, "iorq");
	sc_trace (fp, connect_0, "connect_0");
	sc_trace (fp, connect_1, "connect_1");
	sc_trace (fp, sig_addressBus_cpu, "addressBus_cpu");
	sc_trace (fp, sig_addressBus_mem, "addressBus_mem");
	sc_trace (fp, sig_dataBus, "dataBus");

	connect_0.write (0xAF);
	sc_start (400, SC_MS);	// run the simulation for 100 µ-sec

	sc_close_vcd_trace_file (fp);

	delete[] mem;
	return 0;

}

