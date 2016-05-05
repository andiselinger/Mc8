#include <iostream>
#include "systemc.h"
//#include "SESYD.hpp"

//LibSESYD::FileWriter *myWriter;

#include "clkResGen.h"
#include "asyncRom.h"
#include "mc8.h"

using namespace std;

int
sc_main (int argc, char* argv[])
{

  //initSesydFramework();

  //myWriter = new FileWriter();

  sc_signal<bool> sig_clk;
  sc_signal<bool> sig_tclk;
  sc_signal<bool> sig_res;

  sc_signal<bool> sig_wr;
  sc_signal<bool> sig_rd;
  sc_signal<bool> sig_mreq;
  sc_signal<bool> sig_iorq;

  sc_signal<sc_bv<8> > sig_dataBus;
  sc_signal<sc_bv<16> > sig_addressBus;

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
  myMC8.addressBus (sig_addressBus);

  asyncRom myRom ("myROM");
  myRom.rd (sig_rd);
  myRom.addrBus (sig_addressBus);
  myRom.dataBus (sig_dataBus);
  myRom.initMemory ();

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

//myWriter->setupVCDFile("mc8");

  sc_start (200, SC_MS);	// run the simulation for 100 µ-sec

  //myWriter->closeFile();
//	sc_close_vcd_trace_file(fp);

  return 0;

}
;
