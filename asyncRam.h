#include "systemc.h"

#define RAM_READ_DELAY_NS 1
#define RAM_WRITE_DELAY_NS 1

#define SIZE 8191

SC_MODULE(asyncRam)
{

public:

	sc_in<bool> wr;
	sc_in<bool> rd;
	sc_in<sc_bv<16> > addrBus;
	sc_in<sc_bv<8> > dataBus_in;
	sc_out<sc_bv<8> > dataBus_out;

  SC_CTOR(asyncRam)
  {
    SC_THREAD(readData);
    SC_THREAD(writeData);

    busy = false;
  }

  void
  initMemory ()
  {

	  memory[0x0000] = 0x3A;
	  memory[0x0001] = 0x03;
	  memory[0x0002] = 0x30;
	  memory[0x0003] = 0xFF;
	  memory[0x0004] = 0x22;
	  memory[0x0005] = 0x3D;
	  memory[0x1002] = 0xC3;
	  memory[0x1003] = 0x02;
	  memory[0x1004] = 0x00;
	  memory[0x1005] = 0xC3;
	  memory[0x1006] = 0x05;
	  memory[0x1007] = 0x22;

  }

private:

  sc_bv<8> memory[SIZE];    // 8 bit data, 16 bit address
  unsigned int address;
  bool busy;


  void  readData ()
  {
    while (1)
    {
      wait (rd.negedge_event ());

      if(!busy){

    	  busy = true;
    	  address = addrBus.read ().to_uint ();
    	  wait (RAM_READ_DELAY_NS, SC_NS);
    	  dataBus_out.write (memory[address]);
    	  busy = false;
      }

    }
  }

  void  writeData ()
    {
      while (1)
      {
        wait (wr.negedge_event ());

        if(!busy){

          	  busy = true;
          	  address = addrBus.read ().to_uint ();
          	  wait (RAM_WRITE_DELAY_NS, SC_NS);
          	  memory[address] = dataBus_in.read();
          	busy = false;
        }
      }
    }

};
