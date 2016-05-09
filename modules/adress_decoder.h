#ifndef ADRESS_DECODER
#define ADRESS_DECODER

#include "systemc.h"

SC_MODULE(address_decoder)
{
public :

/*	sc_inout<sc_bv<8> > data_cpu;
	sc_inout<sc_bv<8> > data_ram;
	sc_inout<sc_bv<8> > data_rom;
	sc_inout<sc_bv<8> > data_io;*/

	sc_in<sc_bv<16> > address_bus_in;
	sc_in< bool > WR_in;
	sc_in< bool > RD_in;
	sc_in< bool > M_in;
	sc_in< bool > IO_in;

	sc_out<sc_bv<16> > address_bus_ram;
	sc_out<sc_bv<16> > address_bus_rom;
	sc_out<sc_bv<8> > address_bus_io;

	sc_out< bool > WR_io;
	sc_out< bool > WR_ram;

	sc_out< bool > RD_io;
	sc_out< bool > RD_ram;
	sc_out< bool > RD_rom;


	SC_CTOR(address_decoder)
	{
		SC_METHOD(decode);

		sensitive << M_in;
		sensitive << IO_in;
	}

private :

	void decode()
	{
		sc_uint<16> address = address_bus_in.read();

		if(IO_in.read() == true && M_in.read() == false )
		{
			//forward control signals to ram
			if(address >= 0x2000 && address <=0x3FFF)
			{
				address_bus_ram.write(address - 0x2000);
				WR_ram.write(WR_in.read());
				RD_ram.write(RD_in.read());
			}

			//forward control signals to rom
			else if(address >= 0x0000 && address <0x2000)
			{
				address_bus_ram.write(address);
				RD_rom.write(RD_in.read());
			}

		}else if(IO_in.read() == false && M_in.read() == true )
		{
			WR_io.write(WR_in.read());
			RD_io.write(RD_in.read());
		}else if(IO_in.read() == true && M_in.read() == true )
		{
			WR_ram.write(true);
			RD_ram.write(true);
			RD_rom.write(true);
			WR_io.write(true);
			RD_io.write(true);
		}
		else
		{
			cout << "memory decoder error";
		}



	}

};


#endif
