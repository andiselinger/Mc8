#include <iostream>
#include "systemc.h"
//#include "SESYD.hpp"

//LibSESYD::FileWriter *myWriter;

#include "clkResGen.h"
#include "asyncRom.h"
#include "asyncRam.h"
#include "mc8.h"
#include "adress_decoder.h"

using namespace std;



int sc_main(int argc, char* argv[]) {

	//initSesydFramework();

//	myWriter = new FileWriter();


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

	sc_signal<sc_bv<8> > sig_dataBus_cputomem;
	sc_signal<sc_bv<8> > sig_dataBus_memtocpu;
	sc_signal<sc_bv<16> > sig_addressBus_cpu;
	sc_signal<sc_bv<16> > sig_addressBus_ram;
	sc_signal<sc_bv<16> > sig_addressBus_rom;
	sc_signal<sc_bv<8> > sig_addressBus_io;



	// Components
	clkResGen crg("CLKRES");
	crg.clk(sig_clk);
	crg.tclk(sig_tclk);
	crg.res(sig_res);

	mc8 myMC8("myMC8");
	myMC8.clk(sig_clk);
	myMC8.tclk(sig_tclk);
	myMC8.res(sig_res);
	myMC8.wr(sig_wr);
	myMC8.rd(sig_rd);
	myMC8.mreq(sig_mreq);
	myMC8.iorq(sig_iorq);
	myMC8.dataBus_in(sig_dataBus_memtocpu);
	myMC8.dataBus_out(sig_dataBus_cputomem);
	myMC8.addressBus(sig_addressBus_cpu);


/*
	asyncRom myRom("myROM");
	myRom.rd(sig_rd_rom);
	myRom.addrBus(sig_addressBus_rom);
	myRom.dataBus(sig_dataBus_memtocpu);
	myRom.initMemory();
	*/

	asyncRam myRam("myRAM");
	myRam.rd(sig_rd_ram);
	myRam.wr(sig_wr_ram);
	myRam.addrBus(sig_addressBus_ram);
	myRam.dataBus_in(sig_dataBus_cputomem);
	myRam.dataBus_out(sig_dataBus_memtocpu);
	myRam.initMemory();

	address_decoder myAddressDecoder("myAddressDecoder");
	myAddressDecoder.address_bus_in(sig_addressBus_cpu);
	myAddressDecoder.address_bus_ram(sig_addressBus_ram);
	myAddressDecoder.address_bus_rom(sig_addressBus_rom);
	myAddressDecoder.address_bus_io(sig_addressBus_io);
	myAddressDecoder.RD_in(sig_rd);
	myAddressDecoder.WR_in(sig_wr);
	myAddressDecoder.IO_in(sig_iorq);
	myAddressDecoder.M_in(sig_mreq);
	myAddressDecoder.RD_ram(sig_rd_ram);
	myAddressDecoder.WR_ram(sig_wr_ram);
	myAddressDecoder.RD_rom(sig_rd_rom);
	myAddressDecoder.WR_io(sig_wr_io);
	myAddressDecoder.RD_io(sig_rd_io);


//	sc_trace_file *fp;
//	fp=sc_create_vcd_trace_file("wave");
//
//	sc_trace(fp,sig_clk,"clk");
//	sc_trace(fp,sig_res,"res");
//	sc_trace(fp,sig_wr,"wr");
//	sc_trace(fp,sig_rd,"rd");
//	sc_trace(fp,sig_mreq,"mreq");
//	sc_trace(fp,sig_iorq,"iorq");
//	sc_trace(fp,sig_dataBus,"dataBus");
//	sc_trace(fp,sig_addressBus,"addressBus");

//	myWriter->setupVCDFile("mc8");

	sc_start();	// run the simulation for 100 µ-sec

	//myWriter->closeFile();
//	sc_close_vcd_trace_file(fp);

	return 0;

}
;
