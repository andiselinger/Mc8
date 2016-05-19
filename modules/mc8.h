#include "systemc.h"
#include <string>

extern sc_trace_file *fp;  // Trace file from main.cpp

typedef enum state_e
{
	FETCH_INSTR, FETCH_TWO_BYTE_ADDRESS, DECODE, EXECUTE, HALT
} state_t;

typedef enum fetch_state_e
{
	SET_ADDRESS, SET_CONTROL_SIG, READ_TO_INSTR_REG, RESET_CONTROL_SIG
} fetch_state_t;

typedef enum fetch_instr_state_e
{
	SET_ADDRESS_LO,
	SET_CONTROL_SIG_LO,
	READ_TO_ZR_LO,
	RESET_CONTROL_SIG_LO,
	SET_ADDRESS_HI,
	SET_CONTROL_SIG_HI,
	READ_TO_ZR_HI,
	RESET_CONTROL_SIG_HI
} fetch_instr_state_t;

SC_MODULE(mc8)
{

public:
	sc_in<bool> clk;
	sc_in<bool> tclk;
	sc_in<bool> res;

	sc_out<bool> wr;
	sc_out<bool> rd;
	sc_out<bool> mreq;
	sc_out<bool> iorq;

	/*	sc_in<sc_bv<8> > dataBus_in;
	 sc_out<sc_bv<8> > dataBus;*/
	sc_out<sc_bv<16> > addressBus;

	sc_inout_rv<8> dataBus;

	state_t state;

private:

	// Registers
	sc_bv<16> pc;
	sc_bv<16> zr;
	sc_bv<16> ix;
	sc_bv<16> hl;
	sc_bv<16> sp;  // Stack Pointer
	sc_bv<8> ir;
	sc_bv<8> a;
	sc_bv<8> b;
	sc_bv<8> c;

	// Flags
	bool flag_c;
	bool flag_z;
	bool flag_pv;
	bool flag_s;

	// In this vector, the flags are stored directly after the computation,
	// one execution step before corresponding flags are written
	sc_bv<4> tempFlags;

	// ALU
	sc_bv<8> alu1;
	sc_bv<8> alu2;
	sc_bv<8> aluRes;

	fetch_state_t fetchState;
	fetch_instr_state_t fetchInstrState;
	unsigned int execCount; // Indicates the current execution step of instruction
	unsigned int twoByteInstrB1;  // First Byte of two byte instruction
public:

	SC_HAS_PROCESS(mc8);

	mc8 (sc_module_name name) :
			sc_module (name)
	{
		sc_trace (fp, alu1, "alu1_" + (std::string) name); // Add name if you want to use multiple processors
		sc_trace (fp, a, "A_" + (std::string) name);
		sc_trace (fp, b, "B_" + (std::string) name);
		sc_trace (fp, c, "C_" + (std::string) name);
		// ... add any signals you like
		/*
		 myWriter->registerBool("CLK");
		 myWriter->registerBool("RES");

		 myWriter->registerBool("WR");
		 myWriter->registerBool("RD");
		 myWriter->registerBool("MREQ");
		 myWriter->registerBool("IORQ");

		 myWriter->registerWire16("ABus");
		 myWriter->registerWire8("DBus");

		 myWriter->registerWire16("REG_PC");
		 myWriter->registerWire8("REG_IR");
		 myWriter->registerWire16("REG_ZR");
		 myWriter->registerWire16("REG_IX");
		 myWriter->registerWire16("REG_HL");
		 myWriter->registerWire8("REG_A");

		 myWriter->registerWire8("ALU_1");
		 myWriter->registerWire8("ALU_RES");
		 myWriter->registerBool("FLAG_C");
		 myWriter->registerBool("FLAG_Z");
		 myWriter->registerBool("FLAG_PV");
		 myWriter->registerBool("FLAG_S");

		 myWriter->registerWire8("Proc_State");
		 */
		resetRegAndInternals ();
		SC_METHOD(ProcCycle);
		sensitive << clk.pos () << res;
		//SC_METHOD(traceIt);
		//sensitive << tclk.pos ();
		//dont_initialize ();
	}

private:

	void
	ProcCycle ()
	{

		//async reset
		if (res.read () == false)
		{
			resetProcSignals ();
			resetRegAndInternals ();
		}
		else if (clk.read () == true)
		{
			switch (state)
			{
				case FETCH_INSTR:
					switch (fetchState)
					{
						case SET_ADDRESS:
							std::cout << "FETCH INSTRUCTION ... [START] " << std::endl;
							std::cout << "Will fetch instruction at address 0x" << std::hex
									<< std::setw (4) << pc.to_uint () << std::endl;
							addressBus.write (pc);
							fetchState = SET_CONTROL_SIG;
							break;
						case SET_CONTROL_SIG:
							setProcSignals_mem_read ();
							fetchState = READ_TO_INSTR_REG;
							break;
						case READ_TO_INSTR_REG:
							ir = read_databus_resolved ();
							std::cout << "Got instruction 0x" << std::hex << std::setw (2)
									<< ir.to_uint () << std::endl;
							fetchState = RESET_CONTROL_SIG;
							break;
						case RESET_CONTROL_SIG:
							resetProcSignals ();
							pc = pc.to_uint () + 1;
							fetchState = SET_ADDRESS;
							state = DECODE;
							std::cout << "FETCH INSTRUCTION ... [END] " << std::endl;
							break;
						default:
							fetchState = SET_ADDRESS;
					}
					break; //FETCH_INSTR

				case FETCH_TWO_BYTE_ADDRESS:

					switch (fetchInstrState)
					{
						case SET_ADDRESS_LO:
							std::cout << "FETCH TWO BYTE ADDRESS ... [START] " << std::endl;
							std::cout << "Will fetch label low byte at address 0x" << std::hex
									<< std::setw (4) << pc.to_uint () << std::endl;
							addressBus.write (pc);
							fetchInstrState = SET_CONTROL_SIG_LO;
							break;
						case SET_CONTROL_SIG_LO:
							setProcSignals_mem_read ();
							fetchInstrState = READ_TO_ZR_LO;
							break;
						case READ_TO_ZR_LO:
							zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
							std::cout << "Got low byte 0x" << std::hex << std::setw (2)
									<< read_databus_resolved ().to_uint () << std::endl;
							fetchInstrState = RESET_CONTROL_SIG_LO;
							break;
						case RESET_CONTROL_SIG_LO:
							resetControlSignals ();
							pc = pc.to_uint () + 1;
							fetchInstrState = SET_ADDRESS_HI;
							break;
						case SET_ADDRESS_HI:
							std::cout << "Will fetch label hi byte at address 0x" << std::hex
									<< std::setw (4) << pc.to_uint () << std::endl;
							addressBus.write (pc);
							fetchInstrState = SET_CONTROL_SIG_HI;
							break;
						case SET_CONTROL_SIG_HI:
							setProcSignals_mem_read ();
							fetchInstrState = READ_TO_ZR_HI;
							break;
						case READ_TO_ZR_HI:
							zr = (read_databus_resolved () << 8) | (zr & 0x00FF);
							std::cout << "Got hi byte 0x" << std::hex << std::setw (2)
									<< read_databus_resolved ().to_uint () << std::endl;
							fetchInstrState = RESET_CONTROL_SIG_HI;
							break;
						case RESET_CONTROL_SIG_HI:
							resetControlSignals ();
							pc = pc.to_uint () + 1;
							fetchInstrState = SET_ADDRESS_LO;
							state = EXECUTE;
							std::cout << "FETCH TWO BYTE ADDRESS... [END] " << std::endl;
							break;

						default:
							fetchInstrState = SET_ADDRESS_LO;
					}

					break; //FETCH_TWO_BYTE_ADDRESS

				case DECODE:
					std::cout << "DECODE INSTRUCTION ... [START] " << std::endl;
					decodeInstr (ir);
					std::cout << "DECODE INSTRUCTION ... [END] " << std::endl;

					break; //DECODE

				case EXECUTE:
					switch (ir.to_uint ())
					{
						case 0x00:
							// NOP
							std::cout << "EXECUTE NOP ... [START] " << std::endl;
							state = FETCH_INSTR;
							std::cout << "EXECUTE NOP ... [END] " << std::endl;
							break;

						case 0x3E:
							// MOV A,dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE MOV A,dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 2:
									// Read dat_8 to A
									a = read_databus_resolved ();
									std::cout << "Read dat_8 and copy to A "
											<< read_databus_resolved () << std::endl;
									execCount++;
									break;
								case 3:
									// Reset control signals
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									std::cout << "EXECUTE MOV A,dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // MOV A,dat_8

						case 0x06:
							// MOV B,dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE MOV B,dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 2:
									// Read dat_8 to B
									b = read_databus_resolved ();
									std::cout << "Read dat_8 and copy to B "
											<< read_databus_resolved () << std::endl;
									execCount++;
									break;
								case 3:
									// Reset control signals
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									std::cout << "EXECUTE MOV B,dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // MOV B,dat_8
						case 0x0E:
							// MOV C,dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE MOV C,dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 2:
									// Read dat_8 to C
									c = read_databus_resolved ();
									std::cout << "Read dat_8 and copy to C "
											<< read_databus_resolved () << std::endl;
									execCount++;
									break;
								case 3:
									// Reset control signals
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									std::cout << "EXECUTE MOV C,dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // MOV C,dat_8

							// DIRECT ADDRESSING
						case 0x78:
							// MOV A, B
							a = b;
							state = FETCH_INSTR;
							break;
						case 0x79:
							// MOV A, C
							a = c;
							state = FETCH_INSTR;
							break;
						case 0x47:
							// MOV B, A
							b = a;
							state = FETCH_INSTR;
							break;
						case 0x41:
							// MOV B, C
							b = c;
							state = FETCH_INSTR;
							break;
						case 0x4F:
							// MOV C, A
							c = a;
							state = FETCH_INSTR;
							break;
						case 0x48:
							// MOV C, B
							c = b;
							state = FETCH_INSTR;
							break;

						case 0x21:
							if (twoByteInstrB1 == 0xDD)
							{
								// MOV IX, dat_16
								switch (execCount)
								{
									case 0:  // Write PC to address bus
										std::cout << "EXECUTE  MOV IX, dat_16 ... [START]"
												<< std::endl;
										addressBus.write (pc);
										execCount++;
										break;
									case 1:
										// Set control sigs for mem read
										setProcSignals_mem_read ();
										execCount++;
										break;
									case 2:
										// Write low byte to IX
										ix = (ix & 0xff00) | (0x00FF & read_databus_resolved ());
										std::cout << "Got low byte " << read_databus_resolved ()
												<< std::endl;
										execCount++;
										break;
									case 3:
										// Reset control signals and pc++
										resetControlSignals ();
										pc = pc.to_uint () + 1;
										execCount++;
										break;
									case 4:
										// Write PC to address bus
										std::cout << "Will fetch label hi byte at address "
												<< pc.to_uint () << std::endl;
										addressBus.write (pc);
										execCount++;
										break;
									case 5:  // Set signals for second read
										setProcSignals_mem_read ();
										execCount++;
										break;
									case 6:  // Write hi byte to IX
										ix = (read_databus_resolved () << 8) | (ix & 0x00FF);
										std::cout << "Got hi byte " << read_databus_resolved ()
												<< std::endl;
										execCount++;
										break;
									case 7:
										resetControlSignals ();
										pc = pc.to_uint () + 1;
										execCount = 0;
										std::cout << "EXECUTE  MOV IX, dat_16 ... [END]"
												<< std::endl;
										state = FETCH_INSTR;
										twoByteInstrB1 = 0;
								}
							}  // MOV IX, dat_16
							else
							{
								// MOV HL, dat_16
								switch (execCount)
								{
									case 0:  // Write PC to address bus
										std::cout << "EXECUTE  MOV HL, dat_16 ... [START]"
												<< std::endl;
										addressBus.write (pc);
										execCount++;
										break;
									case 1:
										// Set control sigs for mem read
										setProcSignals_mem_read ();
										execCount++;
										break;
									case 2:
										// Write low byte to HL
										hl = (hl & 0xff00) | (0x00FF & read_databus_resolved ());
										std::cout << "Got low byte " << read_databus_resolved ()
												<< std::endl;
										execCount++;
										break;
									case 3:
										// Reset control signals and pc++
										resetControlSignals ();
										pc = pc.to_uint () + 1;
										execCount++;
										break;
									case 4:
										// Write PC to address bus
										std::cout << "Will fetch label hi byte at address "
												<< pc.to_uint () << std::endl;
										addressBus.write (pc);
										execCount++;
										break;
									case 5:  // Set signals for read
										setProcSignals_mem_read ();
										execCount++;
										break;
									case 6:  // Write hi byte to HL
										hl = (read_databus_resolved () << 8) | (hl & 0x00FF);
										std::cout << "Got hi byte " << read_databus_resolved ()
												<< std::endl;
										execCount++;
										break;
									case 7:
										resetControlSignals ();
										pc = pc.to_uint () + 1;
										execCount = 0;
										std::cout << "EXECUTE  MOV HL, dat_16 ... [END]"
												<< std::endl;
										state = FETCH_INSTR;
										twoByteInstrB1 = 0;
								}  // MOV HL, dat_16
							}
							break; // MOV IX or MOV HL

						case 0x31:
							// MOV SP, dat_16
							switch (execCount)
							{
								case 0:  // Write PC to address bus
									std::cout << "EXECUTE  MOV SP, dat_16 ... [START]"
											<< std::endl;
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 2:
									// Write low byte to SP
									sp = (sp & 0xff00) | (0x00FF & read_databus_resolved ());
									std::cout << "Got low byte " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 3:
									// Reset control signals and pc++
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									execCount++;
									break;
								case 4:
									// Write PC to address bus
									std::cout << "Will fetch label hi byte at address "
											<< pc.to_uint () << std::endl;
									addressBus.write (pc);
									execCount++;
									break;
								case 5:  // Set signals for read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 6:  // Write hi byte to SP
									sp = (read_databus_resolved () << 8) | (sp & 0x00FF);
									std::cout << "Got hi byte " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 7:
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									execCount = 0;
									std::cout << "EXECUTE  MOV SP, dat_16 ... [END]" << std::endl;
									state = FETCH_INSTR;
									twoByteInstrB1 = 0;
							} // switch (execCounter)
							break; // MOV SP, dat_16

							// Direkte Adressierung -Register <-> Speicher
							//---------------------------------------------------------------------------

						case 0x3A:	// MOV A, label

							if (execCount == 0)
								std::cout << "EXECUTE  MOV A, label ... [START]" << std::endl;

							if (mov_register_label (&a))
							{
								std::cout << "EXECUTE  MOV A, label ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

						case 0x32:	// MOV label, A
							if (execCount == 0)
								std::cout << "EXECUTE  MOV label, A ... [START]" << std::endl;

							if (mov_label_register (&a))
							{
								std::cout << "EXECUTE  MOV label, A ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

						case 0x22:

							if (twoByteInstrB1 == 0xDD)
							{
								// MOV label,IX
								if (execCount == 0)
									std::cout << "EXECUTE  MOV label, IX ... [START]"
											<< std::endl;

								if (mov_label_register (&ix))
								{
									std::cout << "EXECUTE  MOV label, IX ... [END]" << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									twoByteInstrB1 = 0;
								}
							}
							else
							{
								// MOV label,HL
								if (execCount == 0)
									std::cout << "EXECUTE  MOV label, HL ... [START]"
											<< std::endl;

								if (mov_label_register (&hl))
								{
									std::cout << "EXECUTE  MOV label, HL ... [END]" << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									twoByteInstrB1 = 0;
								}
							}

							break;

						case 0x2A:

							if (twoByteInstrB1 == 0xDD) // MOV IX,label
							{
								if (execCount == 0)
									std::cout << "EXECUTE  MOV IX, label ... [START]"
											<< std::endl;

								if (mov_register_label (&ix))
								{
									std::cout << "EXECUTE  MOV IX, label ... [END]" << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									twoByteInstrB1 = 0;
								}
							}
							else // MOV HL,label
							{
								if (execCount == 0)
									std::cout << "EXECUTE  MOV HL, label ... [START]"
											<< std::endl;

								if (mov_register_label (&hl))
								{
									std::cout << "EXECUTE  MOV HL, label ... [END]" << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									twoByteInstrB1 = 0;
								}
							}
							break;

							// Registerindirekte Adressierung
							//---------------------------------------------------------------------------------------

						case 0x7E: // MOV A,[HL]
							if (execCount == 0)
								std::cout << "EXECUTE  MOV A, [HL] ... [START]" << std::endl;

							if (mov_register_sourceAddressRegister (&a, &hl))
							{
								std::cout << "EXECUTE  MOV A, [HL] ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

						case 0x77: // MOV [HL],A

							if (execCount == 0)
								std::cout << "EXECUTE  MOV [HL], A ... [START]" << std::endl;

							if (mov_destinationAddressRegister_register (&hl, &a))
							{
								std::cout << "EXECUTE  MOV [HL], A ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

							// Stackbefehle
							//---------------------------------------------------------------------------------------

						case 0xF5: // PUSH
							if (execCount == 0)
								std::cout << "EXECUTE  PUSH ... [START]" << std::endl;

							if (push ())
							{
								std::cout << "EXECUTE  IN POP ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

						case 0xF1: // POP
							if (execCount == 0)
								std::cout << "EXECUTE  POP ... [START]" << std::endl;

							if (pop ())
							{
								std::cout << "EXECUTE  POP ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

							// Ein-Ausgabebefehle
							//---------------------------------------------------------------------------------------

						case 0xDB: // IN A, port
							if (execCount == 0)
								std::cout << "EXECUTE  IN A, port ... [START]" << std::endl;

							if (in_a_port ())
							{
								std::cout << "EXECUTE  IN A, port ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

						case 0xD3: // OUT port, A
							if (execCount == 0)
								std::cout << "EXECUTE  OUT port, A ... [START]" << std::endl;

							if (out_port_a ())
							{
								std::cout << "EXECUTE  OUT port, A ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

							// Unterprogrammbefehle
							//---------------------------------------------------------------------------------------

						case 0xCD: // CALL label
							if (execCount == 0)
								std::cout << "EXECUTE  CALL label ... [START]" << std::endl;

							if (call_label ())
							{
								std::cout << "EXECUTE  CALL label ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

						case 0xC9: // RET
							if (execCount == 0)
								std::cout << "EXECUTE  RET ... [START]" << std::endl;

							if (ret ())
							{
								std::cout << "EXECUTE  RET ... [END]" << std::endl;
								execCount = 0;
								state = FETCH_INSTR;
								twoByteInstrB1 = 0;
							}
							break;

							// INCREMENTS and DECREMENTS
							//---------------------------------------------------------------------------------------

						case 0x3C:
							// INC A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE INC A ... [START] " << std::endl;
									// Load A to ALU
									alu1 = a;
									execCount++;
									std::cout << "Load " << a.to_int () << std::endl;
									break;
								case 1:
									// ALU inc
									aluRes = alu1.to_uint () + 1;
									execCount++;
									break;
								case 2:
									// Set flags and Store Result
									if (aluRes.to_uint () == 0)
										flag_z = true;
									else
										flag_z = false;
									if (alu1.to_uint () == 0x7F)
										flag_pv = true;
									else
										flag_pv = false;
									if (aluRes[7])
										flag_s = true;
									else
										flag_s = false;

									// Store Result
									a = aluRes;
									std::cout << "Write back result " << a.to_int () << std::endl;
									std::cout << "EXECUTE INC A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break;

						case 0x4:
							// INC B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE INC B ... [START] " << std::endl;
									// Load B to ALU
									alu1 = b;
									execCount++;
									std::cout << "Load " << b.to_int () << std::endl;
									break;
								case 1:
									// ALU inc
									aluRes = alu1.to_uint () + 1;
									execCount++;
									break;
								case 2:
									// Set flags and Store Result
									if (aluRes.to_uint () == 0)
										flag_z = true;
									else
										flag_z = false;
									if (alu1.to_uint () == 0x7F)
										flag_pv = true;
									else
										flag_pv = false;
									if (aluRes[7])
										flag_s = true;
									else
										flag_s = false;

									// Store Result
									b = aluRes;
									std::cout << "Write back result " << b.to_int () << std::endl;
									std::cout << "EXECUTE INC B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break;

						case 0xC:
							// INC C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE INC C ... [START] " << std::endl;
									// Load C to ALU
									alu1 = c;
									execCount++;
									std::cout << "Load " << c.to_int () << std::endl;
									break;
								case 1:
									// ALU inc
									aluRes = alu1.to_uint () + 1;
									execCount++;
									break;
								case 2:
									// Set flags and Store Result
									if (aluRes.to_uint () == 0)
										flag_z = true;
									else
										flag_z = false;
									if (alu1.to_uint () == 0x7F)
										flag_pv = true;
									else
										flag_pv = false;
									if (aluRes[7])
										flag_s = true;
									else
										flag_s = false;

									// Store Result
									c = aluRes;
									std::cout << "Write back result " << c.to_int () << std::endl;
									std::cout << "EXECUTE INC C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break;

						case 0x23:
							if (twoByteInstrB1 == 0xDD)
							{
								// INC IX
								std::cout << "EXECUTE INC IX ... [START] " << std::endl;
								ix = ix.to_uint () + 1;
								std::cout << "EXECUTE INC IX ... [END] " << std::endl;
							}
							else
							{
								// INC HL
								std::cout << "EXECUTE INC HL ... [START] " << std::endl;
								hl = hl.to_uint () + 1;
								std::cout << "EXECUTE INC HL ... [END] " << std::endl;
							}
							state = FETCH_INSTR;
							twoByteInstrB1 = 0;
							break;

						case 0x3D:
							// DEC A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE DEC A ... [START] " << std::endl;
									// Load A to ALU
									alu1 = a;
									execCount++;
									std::cout << "Load " << a.to_int () << std::endl;
									break;
								case 1:
									// ALU dec
									aluRes = alu1.to_uint () - 1;
									execCount++;
									break;
								case 2:
									// Set flags and Store Result
									if (aluRes.to_uint () == 0)
										flag_z = true;
									else
										flag_z = false;
									if (alu1.to_uint () == 0x80)
										flag_pv = true;
									else
										flag_pv = false;
									if (aluRes[7])
										flag_s = true;
									else
										flag_s = false;

									// Store Result
									a = aluRes;
									std::cout << "Write back result " << a.to_int () << std::endl;
									std::cout << "EXECUTE DEC A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break;

						case 0x5:
							// DEC B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE DEC B ... [START] " << std::endl;
									// Load B to ALU
									alu1 = b;
									execCount++;
									std::cout << "Load " << b.to_int () << std::endl;
									break;
								case 1:
									// ALU dec
									aluRes = alu1.to_uint () - 1;
									execCount++;
									break;
								case 2:
									// Set flags and Store Result
									if (aluRes.to_uint () == 0)
										flag_z = true;
									else
										flag_z = false;
									if (alu1.to_uint () == 0x80)
										flag_pv = true;
									else
										flag_pv = false;
									if (aluRes[7])
										flag_s = true;
									else
										flag_s = false;

									// Store Result
									b = aluRes;
									std::cout << "Write back result " << b.to_int () << std::endl;
									std::cout << "EXECUTE DEC B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break;

						case 0xD:
							// DEC C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE DEC C ... [START] " << std::endl;
									// Load C to ALU
									alu1 = c;
									execCount++;
									std::cout << "Load " << c.to_int () << std::endl;
									break;
								case 1:
									// ALU dec
									aluRes = alu1.to_uint () - 1;
									execCount++;
									break;
								case 2:
									// Set flags and Store Result
									if (aluRes.to_uint () == 0)
										flag_z = true;
									else
										flag_z = false;
									if (alu1.to_uint () == 0x80)
										flag_pv = true;
									else
										flag_pv = false;
									if (aluRes[7])
										flag_s = true;
									else
										flag_s = false;

									// Store Result
									c = aluRes;
									std::cout << "Write back result " << c.to_int () << std::endl;
									std::cout << "EXECUTE DEC C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break;

						case 0x2B:
							if (twoByteInstrB1 == 0xDD)
							{
								// DEC IX
								std::cout << "EXECUTE DEC IX ... [START] " << std::endl;
								ix = ix.to_uint () - 1;
								std::cout << "EXECUTE DEC IX ... [END] " << std::endl;
							}
							else
							{
								// DEC HL
								std::cout << "EXECUTE DEC HL ... [START] " << std::endl;
								hl = hl.to_uint () - 1;
								std::cout << "EXECUTE DEC HL ... [END] " << std::endl;
							}
							twoByteInstrB1 = 0;
							state = FETCH_INSTR;
							break;

							// Arithmetic Operations
						case 0x87:
							// ADD A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE ADD A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load A to alu2
									alu2 = a;
									std::cout << "Load A = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = add (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE ADD A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // ADD A

						case 0x80:
							// ADD B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE ADD B ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = b;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = add (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE ADD B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // ADD B

						case 0x81:
							// ADD C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE ADD C ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load C to alu2
									alu2 = c;
									std::cout << "Load C =  " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = add (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE ADD C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // ADD C

						case 0xC6:
							// ADD dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE ADD dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Load A to ALU
									alu1 = a;
									std::cout << "Load A = " << a.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 3:
									// Read dat_8 to ALU
									alu2 = read_databus_resolved ();
									std::cout << "Read dat_8 to ALU " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 4:
									// Execute Computation
									aluRes = add (alu1, alu2);
									execCount++;
									break;
								case 5:
									// set flags, store result and reset control signals
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;			// Store result
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									printA ();
									printFlags ();
									std::cout << "EXECUTE ADD dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // ADD dat_8

						case 0x97:
							// SUB A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE SUB A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load A to alu2
									alu2 = a;
									std::cout << "Load A = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE SUB A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // SUB A

						case 0x90:
							// SUB B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE SUB A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = b;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE SUB B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // SUB B

						case 0x91:
							// SUB C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE SUB C ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load C to alu2
									alu2 = c;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE SUB C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // SUB C

						case 0xD6:
							// SUB dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE SUB dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Load A to ALU
									alu1 = a;
									std::cout << "Load A = " << a.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 3:
									// Read dat_8 to ALU
									alu2 = read_databus_resolved ();
									std::cout << "Read dat_8 to ALU " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 4:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 5:
									// set flags, store result and reset control signals
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									a = aluRes;    // Store result
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									printA ();
									printFlags ();
									std::cout << "EXECUTE SUB dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // SUB dat_8

							// LOGICAL OPERATIONS
						case 0xA7:
							// AND A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE AND A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load A to alu2
									alu2 = a;
									std::cout << "Load A = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 & alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE AND A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // AND A

						case 0xA0:
							// AND B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE AND B ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = b;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 & alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE AND B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // AND B

						case 0xA1:
							// AND C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE AND C ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = c;
									std::cout << "Load C = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 & alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE AND C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // AND C

						case 0xE6:
							// AND dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE AND dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Load A to ALU
									alu1 = a;
									std::cout << "Load A = " << a.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 3:
									// Read dat_8 to ALU
									alu2 = read_databus_resolved ();
									std::cout << "Read dat_8 to ALU " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 4:
									// Execute Computation
									aluRes = alu1 & alu2;
									execCount++;
									break;
								case 5:
									// set flags, store result and reset control signals
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;			// Store result
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									printA ();
									printFlags ();
									printFlags ();
									std::cout << "EXECUTE AND dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // AND dat_8

						case 0xB7:
							// OR A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE OR A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load A to alu2
									alu2 = a;
									std::cout << "Load A = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 | alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE OR A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // OR A

						case 0xB0:
							// OR B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE OR A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = b;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 | alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE OR B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // OR B

						case 0xB1:
							// OR C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE OR A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load C to alu2
									alu2 = c;
									std::cout << "Load C = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 | alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE OR B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // OR C

						case 0xF6:
							// OR dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE OR dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Load A to ALU
									alu1 = a;
									std::cout << "Load A = " << a.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 3:
									// Read dat_8 to ALU
									alu2 = read_databus_resolved ();
									std::cout << "Read dat_8 to ALU " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 4:
									// Execute Computation
									aluRes = alu1 | alu2;
									execCount++;
									break;
								case 5:
									// set flags, store result and reset control signals
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;    // Store result
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									printA ();
									printFlags ();
									std::cout << "EXECUTE OR dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // OR dat_8

						case 0xAF:
							// XOR A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE XOR A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load A to alu2
									alu2 = a;
									std::cout << "Load A = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 ^ alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE XOR A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // XOR A

						case 0xA8:
							// XOR B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE XOR B ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = b;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 ^ alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE XOR B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // XOR B

						case 0xA9:
							// XOR C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE XOR C ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load C to alu2
									alu2 = c;
									std::cout << "Load C = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = alu1 ^ alu2;
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE XOR C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // XOR C

						case 0xEE:
							// XOR dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE XOR dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Load A to ALU
									alu1 = a;
									std::cout << "Load A = " << a.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 3:
									// Read dat_8 to ALU
									alu2 = read_databus_resolved ();
									std::cout << "Read dat_8 to ALU " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 4:
									// Execute Computation
									aluRes = alu1 ^ alu2;
									execCount++;
									break;
								case 5:
									// set flags, store result and reset control signals
									flag_c = 0;
									flag_z = aluRes.to_uint () == 0;
									flag_pv = parity (aluRes);
									flag_s = aluRes[7];
									a = aluRes;     // Store result
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									printA ();
									printFlags ();
									std::cout << "EXECUTE XOR dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // XOR dat_8

						case 0x27:
							if (twoByteInstrB1 == 0xCB)
							{
								// Shift Left
								switch (execCount)
								{
									case 0:
										std::cout << "EXECUTE SHL ... [START] " << std::endl;
										// Load A to alu1
										alu1 = a;
										std::cout << "Load A = " << alu1.to_uint () << " to alu1"
												<< std::endl;
										execCount++;
										break;
									case 1:
										// Perform Shift
										tempFlags[0] = alu1[7]; // Bit 7 is stored in temp carry flag
										aluRes = alu1 << 1;       // Shift Bits
										execCount++;
										break;
									case 2:
										// Set flags
										flag_c = tempFlags[0];
										flag_z = aluRes.to_uint () == 0;
										flag_pv = parity (aluRes);
										flag_s = aluRes[7];
										execCount++;
										break;
									case 3:
										// Set result
										a = aluRes;
										printA ();
										printFlags ();
										std::cout << "EXECUTE SHL ... [END] " << std::endl;
										execCount = 0;
										twoByteInstrB1 = 0;
										state = FETCH_INSTR;
										break;
								}
							}
							break;  // Shift Left

						case 0x3F:
							if (twoByteInstrB1 == 0xCB)
							{
								// Shift Right
								switch (execCount)
								{
									case 0:
										std::cout << "EXECUTE SHR ... [START] " << std::endl;
										// Load A to alu1
										alu1 = a;
										std::cout << "Load A = " << alu1.to_uint () << " to alu1"
												<< std::endl;
										execCount++;
										break;
									case 1:
										// Perform Shift
										tempFlags[0] = alu1[0];	// Bit 0 is stored in temp carry flag
										aluRes = alu1 >> 1;				// Shift Bits
										execCount++;
										break;
									case 2:
										// Set flags
										flag_c = tempFlags[0];
										flag_z = aluRes.to_uint () == 0;
										flag_pv = parity (aluRes);
										flag_s = aluRes[7];
										execCount++;
										break;
									case 3:
										// Set result
										a = aluRes;
										printA ();
										printFlags ();
										std::cout << "EXECUTE SHR ... [END] " << std::endl;
										execCount = 0;
										twoByteInstrB1 = 0;
										state = FETCH_INSTR;
										break;
								}
							}
							break;  // Shift Right

						case 0x17:
							// Rotate with carry left  - RCL
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE RCL ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Perform Rotation
									tempFlags[0] = alu1[7];  // Bit 7 is stored in temp carry flag
									aluRes = alu1 << 1;      // Shift Bits
									aluRes[0] = flag_c;
									execCount++;
									break;
								case 2:                  // Set flags
									flag_c = tempFlags[0];
									execCount++;
									break;
								case 3:
									// Set result
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE RCL ... [END] " << std::endl;
									execCount = 0;
									twoByteInstrB1 = 0;
									state = FETCH_INSTR;
									break;
							}
							break;  // Rotate with carry left

						case 0x07:
							// Rotate without carry left  - ROL
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE ROL ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
								{ // (Scope for bool temp)
									// Perform Rotation
									bool temp = tempFlags[0] = alu1[7]; // Bit 7 is stored in temp carry flag and in bit 0
									aluRes = alu1 << 1;    // Shift Bits
									aluRes[0] = temp;
									execCount++;
									break;
								}
								case 2:       // Set flags
									flag_c = tempFlags[0];
									execCount++;
									break;
								case 3:
									// Set result
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE ROL ... [END] " << std::endl;
									execCount = 0;
									twoByteInstrB1 = 0;
									state = FETCH_INSTR;
									break;
							}
							break;  // Rotate without carry left

						case 0x1F:
							// Rotate with carry right  - RCR
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE RCR ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Perform Rotation
									tempFlags[0] = alu1[0];  // Bit 0 is stored in temp carry flag
									aluRes = alu1 >> 1;      // Shift Bits
									aluRes[7] = flag_c;
									execCount++;
									break;
								case 2:                  // Set flags
									flag_c = tempFlags[0];
									execCount++;
									break;
								case 3:
									// Set result
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE RCR ... [END] " << std::endl;
									execCount = 0;
									twoByteInstrB1 = 0;
									state = FETCH_INSTR;
									break;
							}
							break;  // Rotate with carry right

						case 0x0F:
							// Rotate without carry right  - ROR
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE ROR ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
								{  // (Scope for bool temp)
									 // Perform Rotation
									bool temp = tempFlags[0] = alu1[0]; // Bit 0 is stored in temp carry flag and in bit 7
									aluRes = alu1 >> 1;    // Shift Bits
									aluRes[7] = temp;
									execCount++;
									break;
								}
								case 2:       // Set flags
									flag_c = tempFlags[0];
									execCount++;
									break;
								case 3:
									// Set result
									a = aluRes;
									printA ();
									printFlags ();
									std::cout << "EXECUTE ROR ... [END] " << std::endl;
									execCount = 0;
									twoByteInstrB1 = 0;
									state = FETCH_INSTR;
									break;
							}
							break;  // Rotate without carry right

							// COMPARISONS  - same as substraction but no storing of result
						case 0xBF:
							// CP A
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE CP A ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load A to alu2
									alu2 = a;
									std::cout << "Load A = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									//a = aluRes;  // Do not save the result
									//std::cout << "New Value in A: " << a << std::endl;
									printFlags ();
									std::cout << "EXECUTE CP A ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // CP A

						case 0xB8:
							// CP B
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE CP B ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load B to alu2
									alu2 = b;
									std::cout << "Load B = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									// a = aluRes;  // Do not save the result
									// std::cout << "New Value in A: " << a << std::endl;
									printFlags ();
									std::cout << "EXECUTE CP B ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // CP B

						case 0xB9:
							// CP C
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE CP C ... [START] " << std::endl;
									// Load A to alu1
									alu1 = a;
									std::cout << "Load A = " << alu1.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 1:
									// Load C to alu2
									alu2 = c;
									std::cout << "Load C = " << alu2.to_uint () << " to alu2"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 3:
									// Set flags and store result
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									//a = aluRes;  // Do not save the result
									//std::cout << "New Value in A: " << a << std::endl;
									printFlags ();
									std::cout << "EXECUTE CP C ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // CP C

						case 0xFE:
							// CP dat_8
							switch (execCount)
							{
								case 0:
									std::cout << "EXECUTE CP dat_8 ... [START] " << std::endl;
									// Read dat8 from Mem, set address
									addressBus.write (pc);
									execCount++;
									break;
								case 1:
									// Load A to ALU
									alu1 = a;
									std::cout << "Load A = " << a.to_uint () << " to alu1"
											<< std::endl;
									execCount++;
									break;
								case 2:
									// Set control sigs for mem read
									setProcSignals_mem_read ();
									execCount++;
									break;
								case 3:
									// Read dat_8 to ALU
									alu2 = read_databus_resolved ();
									std::cout << "Read dat_8 to ALU " << read_databus_resolved ()
											<< std::endl;
									execCount++;
									break;
								case 4:
									// Execute Computation
									aluRes = sub (alu1, alu2);
									execCount++;
									break;
								case 5:
									// set flags, store result and reset control signals
									flag_c = tempFlags[0];
									flag_z = tempFlags[1];
									flag_pv = tempFlags[2];
									flag_s = tempFlags[3];
									//a = aluRes;    // Do not store result
									resetControlSignals ();
									pc = pc.to_uint () + 1;
									//std::cout << "New Value in A: " << a << std::endl;
									printFlags ();
									std::cout << "EXECUTE CP dat_8 ... [END] " << std::endl;
									execCount = 0;
									state = FETCH_INSTR;
									break;
								default:
									break;
							}
							break; // CP dat_8

						case 0xC2:
							// JPNZ label
							std::cout << "EXECUTE JPNZ label ... [START] " << std::endl;
							if (flag_z == false)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPNZ label ... [END] " << std::endl;
							break;

						case 0xCA:
							// JPZ label
							std::cout << "EXECUTE JPZ label ... [START] " << std::endl;
							if (flag_z == true)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPZ label ... [END] " << std::endl;
							break;

						case 0xD2:
							// JPNC label
							std::cout << "EXECUTE JPNC label ... [START] " << std::endl;
							if (flag_c == false)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPNC label ... [END] " << std::endl;
							break;

						case 0xDA:
							// JPC label
							std::cout << "EXECUTE JPC label ... [START] " << std::endl;
							if (flag_c == true)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;

							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPC label ... [END] " << std::endl;
							break;

						case 0xE2:
							// JPNO label
							std::cout << "EXECUTE JPNO label ... [START] " << std::endl;
							if (flag_pv == false)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPNO label ... [END] " << std::endl;
							break;

						case 0xEA:
							// JPO label
							std::cout << "EXECUTE JPO label ... [START] " << std::endl;
							if (flag_pv == true)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPO label ... [END] " << std::endl;
							break;

						case 0xF2:
							// JPNS label
							std::cout << "EXECUTE JPNS label ... [START] " << std::endl;
							if (flag_s == false)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPNS label ... [END] " << std::endl;
							break;

						case 0xFA:
							// JPS label
							std::cout << "EXECUTE JPS label ... [START] " << std::endl;
							if (flag_s == true)
							{
								pc = zr;
								std::cout << "Jump to PC = 0x" << std::hex << std::setw (4)
										<< pc.to_uint () << std::endl;
							}
							else
							{
								std::cout << "Don't jump" << std::endl;
							}
							state = FETCH_INSTR;
							std::cout << "EXECUTE JPS label ... [END] " << std::endl;
							break;

						case 0xC3:
							// JP label
							std::cout << "EXECUTE JP label ... [START] " << std::endl;
							pc = zr;
							state = FETCH_INSTR;
							std::cout << "EXECUTE JP label ... [END] " << std::endl;
							break;

						case 0xE9:
							if (twoByteInstrB1 == 0xDD)
							{
								// JP [IX]
								std::cout << "EXECUTE JP [IX] ... [START] " << std::endl;
								pc = ix;

								std::cout << "EXECUTE JP [IX] ... [END] " << std::endl;
							}
							state = FETCH_INSTR;
							twoByteInstrB1 = 0;
							break;

						case 0x76:
							// HALT
							std::cout << "EXECUTE HALT ... [START] " << std::endl;
							state = HALT;
							std::cout << "EXECUTE HALT ... [END] " << std::endl;
							break;

						default:
							std::cout << "Instruction not valid" << std::endl;
					}

					break;
				case HALT:
					std::cout << "IN HALT STATE ... [START] " << std::endl;
				default:
					break;
			}
		}
	}
	// Addition of two bitvectors
	sc_bv<8>
	add (sc_bv<8> a, sc_bv<8> b)
	{
		sc_bv<8> sum;
		sc_bv<8> carry;

		sum[0] = a[0] ^ b[0];
		carry[0] = a[0] && b[0];
		for (int i = 1; i < 8; ++i)
		{
			sum[i] = (a[i] ^ b[i]) ^ carry[i - 1];
			carry[i] = (a[i] && b[i] && carry[i - 1]) || (a[i] && b[i])
					|| (a[i] && carry[i - 1]) || (b[i] && carry[i - 1]);
		}
		tempFlags[0] = carry[7];                    // carry flag
		tempFlags[1] = sum.to_uint () == 0;         // zero flag
		tempFlags[2] = (carry[6] ^ carry[7]) == 1; // overflow flag; Bauer, Kap. 2.5: "Ein Overflow tritt auf, wenn
		// ein Übertrag in den letzten Volladdierer hinein - oder aus
		// ihm heraus geschoben wird; nicht aber dann, wenn der Übertrag
		// 'durch ihn durch' geschoben wird."
		tempFlags[3] = sum[7];  // sign flag
		return sum;
	}

	// Two's complement of bitvector (needed for substraction)
	sc_bv<8>
	twoComp (sc_bv<8> a)
	{
		for (int i = 0; i < 8; ++i)
			a[i] = !a[i];
		sc_bv<8> one = 1;
		return add (a, one);
	}

	// Difference of two bitvectors
	sc_bv<8>
	sub (sc_bv<8> a, sc_bv<8> b)
	{
		sc_bv<8> result = add (a, twoComp (b));
		tempFlags[0] = !tempFlags[0]; // Carry flag needs to be inverted for substraction (see online MC8 simulator)
		return result;
	}

	bool
	parity (sc_bv<8> a)
	{
		bool result = true;    // Parity is true for an even number of bits in a
		for (int i = 0; i < 8; ++i)
			result ^= a[i];
		return result;
	}

	void
	traceIt ()
	{
		/*myWriter->traceBool(clk.read(), "CLK", sc_time_stamp().to_seconds());
		 myWriter->traceBool(res.read(), "RES", sc_time_stamp().to_seconds());

		 myWriter->traceBool(wr.read(), "WR", sc_time_stamp().to_seconds());
		 myWriter->traceBool(rd.read(), "RD", sc_time_stamp().to_seconds());
		 myWriter->traceBool(mreq.read(), "MREQ", sc_time_stamp().to_seconds());
		 myWriter->traceBool(iorq.read(), "IORQ", sc_time_stamp().to_seconds());

		 myWriter->traceWire16(addressBus.read().to_uint(), "ABus",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire8(read_databus_resolved().to_uint(), "DBus",
		 sc_time_stamp().to_seconds());

		 myWriter->traceWire16(pc.to_uint(), "REG_PC",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire16(zr.to_uint(), "REG_ZR",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire16(ix.to_uint(), "REG_IX",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire16(hl.to_uint(), "REG_HL",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire8(ir.to_uint(), "REG_IR",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire8(a.to_uint(), "REG_A",
		 sc_time_stamp().to_seconds());

		 myWriter->traceWire8(alu1.to_uint(), "ALU_1",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire8(aluRes.to_uint(), "ALU_RES",
		 sc_time_stamp().to_seconds());

		 myWriter->traceBool(flag_c, "FLAG_C", sc_time_stamp().to_seconds());
		 myWriter->traceBool(flag_z, "FLAG_Z", sc_time_stamp().to_seconds());
		 myWriter->traceBool(flag_pv, "FLAG_PV", sc_time_stamp().to_seconds());
		 myWriter->traceBool(flag_s, "FLAG_S", sc_time_stamp().to_seconds());

		 myWriter->traceWire8((unsigned int) state, "Proc_State",
		 sc_time_stamp().to_seconds());
		 */
	}
	void
	printA ()
	{
		std::cout << "New Value in A: 0x" << std::hex << std::setw (2)
				<< std::setfill ('0') << a << std::endl;
	}
	void
	printFlags ()
	{
		std::cout << "FLAGS: c = " << flag_c << " z = " << flag_z << " pv = "
				<< flag_pv << " s = " << flag_s << std::endl;
	}

	void
	resetControlSignals ()
	{
		wr.write (true);
		rd.write (true);
		mreq.write (true);
		iorq.write (true);
	}

	void
	resetProcSignals ()
	{

		wr.write (true);
		rd.write (true);
		mreq.write (true);
		iorq.write (true);
		addressBus.write (false);
	}

	void
	setProcSignals_mem_write ()
	{
		wr.write (false);
		rd.write (true);
		mreq.write (false);
		iorq.write (true);
	}

	void
	setProcSignals_mem_read ()
	{
		// write all z to databus for resolved bus
		dataBus.write ("zzzzzzzz");
		wr.write (true);
		rd.write (false);
		mreq.write (false);
		iorq.write (true);
	}

	void
	setProcSignals_io_read ()
	{
		dataBus.write ("zzzzzzzz");
		wr.write (true);
		rd.write (false);
		mreq.write (true);
		iorq.write (false);
	}

	void
	setProcSignals_io_write ()
	{
		wr.write (false);
		rd.write (true);
		mreq.write (true);
		iorq.write (false);
	}

	void
	resetRegAndInternals ()
	{
		// Reset Prog counter
		pc = 0x0000;
		zr = 0x0000;
		ix = 0x0000;
		hl = 0x0000;
		a = 0x00;
		fetchState = SET_ADDRESS;
		execCount = 0;
		state = FETCH_INSTR;
	}

	//------------------------------------------------
	//------D E C O D E  I N S T R U C T I O N -------
	//------------------------------------------------
	void
	decodeInstr (sc_bv<8> i)
	{
		switch (i.to_uint ())
		{
			// TRANSPORT INSTRUCTIONS
			// Immediate laden
			case 0x3E:
				// MOV A, dat_8
				std::cout << "Instruction -- MOV A, dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x06:
				// MOV B, dat_8
				std::cout << "Instruction -- MOV B, dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x0E:
				// MOV C, dat_8
				std::cout << "Instruction -- MOV C, dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
				// Direct Addressing
			case 0x78:
				// MOV A, B
				std::cout << "Instruction -- MOV A, B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x79:
				// MOV A, C
				std::cout << "Instruction -- MOV A, C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x47:
				// MOV B, A
				std::cout << "Instruction -- MOV B, A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x41:
				// MOV B, C
				std::cout << "Instruction -- MOV B, C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x4F:
				// MOV C, A
				std::cout << "Instruction -- MOV C, A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x48:
				// MOV C, B
				std::cout << "Instruction -- MOV C, B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x21:
				if (twoByteInstrB1 == 0xDD)
				{
					// MOV IX, dat_16
					std::cout << "Instruction -- MOV IX, dat_16" << std::endl;
				}
				else
				{
					// MOV HL, dat_16
					std::cout << "Instruction -- MOV HL, dat_16" << std::endl;
					twoByteInstrB1 = 0;
				}
				state = EXECUTE;
				break;
			case 0x31:
				// MOV SP, dat_16
				std::cout << "Instruction -- MOV SP, dat_16" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Direkte Adressierung -Register <-> Speicher
				//--------------------------------------------------------------------------------
			case 0x3A:
				// MOV A, label
				std::cout << "Instruction -- MOV A,label" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

			case 0x32:
				// MOV label, A
				std::cout << "Instruction -- MOV label, A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

			case 0x2A:

				if (twoByteInstrB1 == 0xDD)
				{
					//MOV IX,label
					std::cout << "Instruction -- MOV IX, label" << std::endl;
					state = EXECUTE;

				}
				else
				{
					// MOV HL, label
					std::cout << "Instruction -- MOV HL, label" << std::endl;
					state = EXECUTE;
					twoByteInstrB1 = 0;
				}
				break;

			case 0x22:

				if (twoByteInstrB1 == 0xDD)
				{
					//MOV label,IX
					std::cout << "Instruction -- MOV label, IX" << std::endl;
					state = EXECUTE;
				}
				else
				{
					// MOV label, HL
					std::cout << "Instruction -- MOV label, HL" << std::endl;
					state = EXECUTE;
					twoByteInstrB1 = 0;
				}
				break;

				// Registerindirekte Adressierung
				//---------------------------------------------------------------------------------------

			case 0x7E: // MOV A,[HL]
				std::cout << "Instruction -- MOV A,[HL]" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

			case 0x77: // MOV [HL],A
				std::cout << "Instruction -- MOV [HL],A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Stackbefehle
				//---------------------------------------------------------------------------------------

			case 0xF5: // PUSH
				std::cout << "Instruction -- PUSH" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

			case 0xF1: // POP
				std::cout << "Instruction -- POP" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Ein-Ausgabebefehle
				//---------------------------------------------------------------------------------------

			case 0xDB: // IN A,port
				std::cout << "Instruction -- IN A,port" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

			case 0xD3: // OUT port,A
				std::cout << "Instruction -- OUT port,A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Unterprogrammbefehle:
				//---------------------------------------------------------------------------------------

			case 0xCD: // CALL label
				std::cout << "Instruction -- CALL label" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

			case 0xC9: // RET
				std::cout << "Instruction -- RET" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Inkrement und Dekrement
				//------------------------------------------------------------------------
			case 0x3C:
				// INC A
				std::cout << "Instruction -- INC A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x4:
				// INC B
				std::cout << "Instruction -- INC B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xC:
				// INC C
				std::cout << "Instruction -- INC C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x23:
				if (twoByteInstrB1 == 0xDD)
				{
					// INC IX
					std::cout << "Instruction -- INC IX" << std::endl;
				}
				else
				{
					// INC HL
					std::cout << "Instruction -- INC HL" << std::endl;
					twoByteInstrB1 = 0;
				}
				state = EXECUTE;
				break;
			case 0x3D:
				// DEC A
				std::cout << "Instruction -- DEC A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x5:
				// DEC B
				std::cout << "Instruction -- DEC B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xD:
				// DEC C
				std::cout << "Instruction -- DEC C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x2B:
				if (twoByteInstrB1 == 0xDD)
				{
					// DEC IX
					std::cout << "Instruction -- DEC IX" << std::endl;
				}
				else
				{
					// DEC HL
					std::cout << "Instruction -- DEC HL" << std::endl;
					twoByteInstrB1 = 0;
				}
				state = EXECUTE;
				break;
				// Arithmetic Operations
			case 0x87:
				// ADD A
				std::cout << "Instruction -- ADD A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x80:
				// ADD B
				std::cout << "Instruction -- ADD B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x81:
				// ADD C
				std::cout << "Instruction -- ADD C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xC6:
				// ADD dat_8
				std::cout << "Instruction -- ADD dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x97:
				// SUB A
				std::cout << "Instruction -- SUB A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x90:
				// SUB B
				std::cout << "Instruction -- SUB B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x91:
				// SUB C
				std::cout << "Instruction -- SUB C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xD6:
				// SUB dat_8
				std::cout << "Instruction -- SUB dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Logic Operations
			case 0xA7:
				// AND A
				std::cout << "Instruction -- AND A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xA0:
				// AND B
				std::cout << "Instruction -- AND B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xA1:
				// AND C
				std::cout << "Instruction -- AND C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xE6:
				// AND dat_8
				std::cout << "Instruction -- AND dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xB7:
				// OR A
				std::cout << "Instruction -- OR A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xB0:
				// OR B
				std::cout << "Instruction -- OR B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xB1:
				// OR C
				std::cout << "Instruction -- OR C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xF6:
				// OR dat_8
				std::cout << "Instruction -- OR dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xAF:
				// XOR A
				std::cout << "Instruction -- XOR A" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xA8:
				// XOR B
				std::cout << "Instruction -- XOR B" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xA9:
				// XOR C
				std::cout << "Instruction -- XOR C" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xEE:
				// XOR dat_8
				std::cout << "Instruction -- XOR dat_8" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
				// SHIFTS
			case 0x27:
				if (twoByteInstrB1 == 0xCB)
				{
					// Shift Left
					std::cout << "Instruction -- SHL" << std::endl;
					state = EXECUTE;
				}
				else
				{
					std::cout << "Instruction not valid!" << std::endl;
				}
				break;
			case 0x3F:
				if (twoByteInstrB1 == 0xCB)
				{
					// Shift Right
					std::cout << "Instruction -- SHR" << std::endl;
					state = EXECUTE;
				}
				else
				{
					std::cout << "Instruction not valid!" << std::endl;
				}
				break;

				// ROTATE INSTRUCTIONS
			case 0x17:
				// RCL
				std::cout << "Instruction -- RCL " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x07:
				// ROL
				std::cout << "Instruction -- ROL " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x1F:
				// RCR
				std::cout << "Instruction -- RCR " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x0F:
				// ROR
				std::cout << "Instruction -- ROR " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// COMPARISONS
			case 0xBF:
				// CP A
				std::cout << "Instruction -- CP A " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xB8:
				// CP B
				std::cout << "Instruction -- CP B " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xB9:
				// CP C
				std::cout << "Instruction -- CP C " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0xFE:
				// CP dat_8
				std::cout << "Instruction -- CP dat_8 " << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// Bedingter Sprung
			case 0xC2:
				std::cout << "Instruction -- JPNZ label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
			case 0xCA:
				std::cout << "Instruction -- JPZ label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
			case 0xD2:
				std::cout << "Instruction -- JPNC label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
			case 0xDA:
				std::cout << "Instruction -- JPC label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
			case 0xE2:
				std::cout << "Instruction -- JPNO label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				break;
			case 0xEA:
				std::cout << "Instruction -- JPO label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
			case 0xF2:
				std::cout << "Instruction -- JPNS label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
			case 0xFA:
				std::cout << "Instruction -- JPS label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				break;
				//Unbedingter Sprung
			case 0xC3:
				std::cout << "Instruction -- JP label" << std::endl;
				state = FETCH_TWO_BYTE_ADDRESS;
				twoByteInstrB1 = 0;
				break;
				//Registerindirekter Sprung
			case 0xE9:
				if (twoByteInstrB1 == 0xDD)
				{
					std::cout << "Instruction -- JP [IX]" << std::endl;
					state = EXECUTE;
				}
				break;

				// CPU Steuerbefehle
			case 0x00:
				std::cout << "Instruction -- NOP" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;
			case 0x76:
				std::cout << "Instruction -- HALT" << std::endl;
				state = EXECUTE;
				twoByteInstrB1 = 0;
				break;

				// 2 byte Befehle
			case 0xDD:
			case 0xCB:
				std::cout << "2 Byte Instruction" << std::endl;
				twoByteInstrB1 = i.to_uint (); // i.to_uint() = 0xDD or i.to_uint() = 0xCB
				state = FETCH_INSTR;
				break;

			default:
				std::cout << "Instruction not valid" << std::endl;
		}
	}

	//------------------------------------------------
	//------M E M O R Y  I N S T R U C T I O N S------
	//------------------------------------------------

	/*
	 bool load_register_from_pcaddress(sc_bv<8> *reg_destination)
	 {
	 static int execCount_internal = 0;
	 bool ready = false;

	 switch (execCount_internal) {

	 case 0:
	 // Write address register to address bus
	 cout << "WRITE PC = " << pc << " to AddressBus" << endl;
	 addressBus.write(pc);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 1:
	 //Set control sigs for mem read
	 wr.write(true);
	 rd.write(false);
	 mreq.write(false);
	 iorq.write(true);
	 cout << "Set control sigs for mem read  wr,rd,m,io : " << wr << " "<< rd << " "<< mreq <<" "<< iorq << endl;
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 2:

	 // test register len
	 cout << "Write data form databus to register" << endl;
	 *reg_destination = read_databus_resolved();
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 3:
	 // Reset control signals and pc++
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 cout << "Reset control sigs for mem read  wr,rd,m,io : " << wr << " "<< rd << " "<< mreq <<" "<< iorq << endl;
	 pc = pc.to_uint() + 1;
	 execCount_internal = 0;
	 execCount++;
	 ready = true;
	 break;

	 }

	 return ready;
	 }

	 bool load_register_from_pcaddress(sc_bv<16> *reg_destination)
	 {
	 static int execCount_internal = 0;
	 bool ready = false;

	 switch (execCount_internal) {
	 case 0:  // Write PC to address bus
	 cout << "WRITE low byte address PC = " << pc.to_uint() << " to AddressBus" << endl;
	 addressBus.write(pc);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;
	 case 1:
	 // Set control sigs for mem read
	 wr.write(true);
	 rd.write(false);
	 mreq.write(false);
	 iorq.write(true);
	 cout << "Set control sigs for mem read  wr,rd,m,io : " << wr << " "<< rd << " "<< mreq <<" "<< iorq << endl;
	 execCount_internal ++;
	 execCount++;
	 ready = false;
	 break;
	 case 2:
	 // Write low byte to SP
	 cout << "Write low byte of data form databus to register" << endl;
	 *reg_destination = (*reg_destination & 0xff00) | (0x00FF & read_databus_resolved());
	 std::cout << "Got low byte " << read_databus_resolved() << std::endl;
	 execCount_internal ++;
	 execCount++;
	 ready = false;
	 break;
	 case 3:
	 // Reset control signals and pc++
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 cout << "Set control sigs for mem read  wr,rd,m,io : " << wr << " "<< rd << " "<< mreq <<" "<< iorq << endl;
	 pc = pc.to_uint() + 1;
	 execCount_internal ++;
	 execCount++;
	 ready = false;
	 break;
	 case 4:
	 // Write PC to address bus
	 cout << "WRITE low byte address PC = " << pc.to_uint() << " to AddressBus" << endl;
	 addressBus.write(pc);
	 execCount_internal ++;
	 execCount++;
	 ready = false;
	 break;
	 case 5:  // Set signals for read
	 wr.write(true);
	 rd.write(false);
	 mreq.write(false);
	 iorq.write(true);
	 execCount_internal ++;
	 execCount++;
	 ready = false;
	 break;
	 case 6:  // Write hi byte to SP
	 *reg_destination = (read_databus_resolved() << 8) | (*reg_destination & 0x00FF);
	 std::cout << "Got hi byte " << read_databus_resolved() << std::endl;
	 execCount_internal ++;
	 execCount++;
	 ready = false;
	 break;
	 case 7:
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 pc = pc.to_uint() + 1;
	 execCount++;
	 ready = true;
	 break;

	 } // switch (execCounter)

	 return ready;

	 }



	 bool read_from_memaddress_to_register(sc_bv<16> *reg_source_addr , sc_bv<16> *reg_destination )
	 {
	 static int execCount_internal = 0;
	 bool ready = false;

	 switch (execCount_internal) {

	 case 0:
	 // Write address register to address bus
	 addressBus.write(reg_source_addr->to_uint());
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 1:
	 //Set control sigs for mem read

	 cout << "Register   : " << reg_source_addr->to_uint() << endl;
	 cout << "AddressBus : " << addressBus.read().to_uint() << endl;

	 wr.write(true);
	 rd.write(false);
	 mreq.write(false);
	 iorq.write(true);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 2:

	 // test register len
	 *reg_destination = read_databus_resolved();
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 3:

	 // Reset control signals and
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 *reg_source_addr = reg_source_addr->to_uint() + 1;
	 execCount++;
	 ready = false;
	 break;

	 case 4:

	 // Write address register to address bus
	 addressBus.write(reg_source_addr->to_uint());
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 5:
	 cout << "Register   : " << reg_source_addr->to_uint() << endl;
	 cout << "AddressBus : " << addressBus.read().to_uint() << endl;

	 wr.write(true);
	 rd.write(false);
	 mreq.write(false);
	 iorq.write(true);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;
	 break;

	 case 6:

	 // write high byte
	 *reg_destination = read_databus_resolved();
	 execCount_internal++;
	 execCount++;
	 ready = true;
	 break;
	 execCount_internal = 0;
	 break;


	 }

	 return ready;

	 }

	 bool write_from_register_to_memaddress(sc_bv<8> *reg_source, sc_bv<16> *reg_destination_addr )
	 {
	 static int execCount_internal = 0;
	 bool ready = false;

	 switch (execCount_internal) {

	 case 0:
	 // Write address register to address bus
	 addressBus.write(*reg_destination_addr);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 1:
	 //Set control sigs for mem read
	 wr.write(false);
	 rd.write(true);
	 mreq.write(false);
	 iorq.write(true);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 2:

	 // test register len
	 dataBus.write(*reg_source);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 3:
	 // Reset control signals and
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 execCount_internal = 0;
	 execCount++;
	 ready = true;
	 break;

	 }

	 return ready;

	 }

	 bool write_from_register_to_address(sc_bv<16> *reg_source, sc_bv<16> *reg_destination_addr )
	 {
	 static int execCount_internal = 0;
	 bool ready = false;

	 switch (execCount_internal) {

	 case 0:
	 // Write address register to address bus
	 addressBus.write(*reg_destination_addr);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 1:
	 //Set control sigs for mem read
	 wr.write(false);
	 rd.write(true);
	 mreq.write(false);
	 iorq.write(true);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 2:

	 // write low byte of source register to databus
	 sc_bv<8> data = *reg_source;


	 dataBus.write(data);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;

	 case 3:
	 // Reset control signals and
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 *reg_destination_addr = reg_destination_addr->to_uint() + 1;
	 execCount++;
	 ready = false;
	 break;

	 case 4:
	 // Write address register to address bus
	 addressBus.write(*reg_destination_addr);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;
	 break;

	 case 5:
	 // write high byte of source register to databus
	 sc_bv<8> data = *reg_source >> 8;


	 dataBus.write(data);
	 execCount_internal++;
	 execCount++;
	 ready = false;
	 break;
	 break;

	 case 6:
	 // Reset control signals and
	 wr.write(true);
	 rd.write(true);
	 mreq.write(true);
	 iorq.write(true);
	 execCount_internal = 0;
	 execCount++;
	 ready = true;
	 break;
	 break;


	 }

	 return ready;
	 }
	 */

	bool
	mov_register_sourceAddressRegister (sc_bv<8> *reg_destination,
																			sc_bv<16> *reg_source_addr)
	{
		bool ready = false;

		switch (execCount)
		{

			case 0:	// Write reg_source_address register to address bus
				addressBus.write (reg_source_addr->to_uint ());
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// read data from bus to destination
				*reg_destination = read_databus_resolved ();
				execCount++;
				break;

			case 3: // reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;

		}

		return ready;
	}

	bool
	mov_destinationAddressRegister_register (sc_bv<16> *reg_destination_addr,
																						sc_bv<8> *reg_source)
	{
		bool ready = false;

		switch (execCount)
		{

			case 0:	// Write reg_source_address register to address bus
				addressBus.write (reg_destination_addr->to_uint ());
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_write ();
				execCount++;
				break;

			case 2:	// read data from bus to destination
				dataBus.write (*reg_source);
				execCount++;
				break;

			case 3: // reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	mov_label_register (sc_bv<16> *reg_source)
	{
		bool ready = false;

		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:	// Write address register to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// Write low byte to ZR
				cout << "Write low byte of data to ZR register" << endl;
				zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved () << std::endl;
				execCount++;
				break;
			case 3:	// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
			case 4:	// Write PC to address bus
				cout << "WRITE address PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;
			case 5:  // Set signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 6:  // Write hi byte to zr
				zr = (read_databus_resolved () << 8) | (zr & 0x00FF);
				std::cout << "Got hi byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved () << std::endl;
				execCount++;
				break;
			case 7: // reset control signals and increment pc
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
				// copy data from memory to register
				//-----------------------------------------------------------------------------------------
				// copy low data byte from source_register to memory
				//-----------------------------------------------------------------------------------------
			case 8: // write address from zr to address bus
				addressBus.write (zr);
				execCount++;
				break;
			case 9: // Set control signals for mem write
				setProcSignals_mem_write ();
				execCount++;
				break;
			case 10: //write data to Memory
			{
				sc_bv<8> lowByte = reg_source->range (7, 0);
				dataBus.write (lowByte);
				execCount++;
				break;
			}
			case 11: // increment zr and reset control signals
				resetProcSignals ();
				zr = zr.to_uint () + 1;
				execCount++;
				break;

				// copy high data byte from source_register to memory
				//-----------------------------------------------------------------------------------------
			case 12: // write zr to addressbus
				addressBus.write (zr);
				execCount++;
				break;
			case 13: // Set control signals for write
				setProcSignals_mem_write ();
				execCount++;
				break;
			case 14: // Write high byte to destination register
			{
				sc_bv<8> highByte = reg_source->range (15, 8);
				dataBus.write (highByte);
				execCount++;
				break;
			}
			case 15: //  reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	mov_label_register (sc_bv<8> *reg_source)
	{
		bool ready = false;
		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:	// Write address register to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// Write low byte to ZR
				cout << "Write low byte of data to ZR register" << endl;
				zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< std::endl;
				execCount++;
				break;
			case 3:	// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
			case 4:	// Write PC to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;
			case 5:  // Set signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 6:  // Write hi byte to zr
				zr = (read_databus_resolved () << 8) | (zr & 0x00FF);
				std::cout << "Got hi byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< std::endl;
				execCount++;
				break;
			case 7: // reset control signals and increment pc
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
				// copy data from memory to register
				//-----------------------------------------------------------------------------------------
			case 8: // write address from zr to address bus
				addressBus.write (zr);
				execCount++;
				break;
			case 9: // Set control signals for read
				setProcSignals_mem_write ();
				execCount++;
				break;
			case 10: //get data from Memory
				dataBus.write (*reg_source);
				execCount++;
				break;
			case 11: //  reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}

		return ready;
	}

	bool
	mov_register_label (sc_bv<8> *reg_destination)
	{
		bool ready = false;
		//std::cout << "Hello from MOV A, label!" << std::endl;
		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:	// Write address register to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// Write low byte to ZR
				cout << "Write low byte from databus to ZR" << endl;
				zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " of ZR" << std::endl;
				execCount++;
				break;
			case 3:	// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
			case 4:	// Write PC to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;
			case 5:  // Set signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 6:  // Write hi byte to zr
				zr = (read_databus_resolved () << 8) | (zr & 0x00FF);
				std::cout << "Got hi byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " of ZR" << std::endl;
				execCount++;
				break;
			case 7: // reset control signals and increment pc
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;

				// copy low data from memory to the destination register
				//-----------------------------------------------------------------------------------------
			case 8: // write address from zr to address bus
				addressBus.write (zr);
				execCount++;
				break;
			case 9: // Set control signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 10: //get data from Memory at address ZR and load it
				*reg_destination = read_databus_resolved ();
				std::cout << "Got byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " from address in ZR" << std::endl;
				execCount++;
				break;
			case 11: //  reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	mov_register_label (sc_bv<16> *reg_destination)
	{
		bool ready = false;
		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:
				// Write address register to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:
				//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:
				// Write low byte to ZR
				cout << "Write low byte of data to ZR register" << endl;
				zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " of ZR" << std::endl;
				execCount++;
				break;
			case 3:
				// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
			case 4:
				// Write PC to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;
			case 5:  // Set signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 6:  // Write hi byte to zr
				zr = (read_databus_resolved () << 8) | (zr & 0x00FF);
				std::cout << "Got hi byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " of ZR" << std::endl;
				execCount++;
				break;
			case 7: // reset control signals and increment pc
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;

				// copy low data from memory to the destination register
				//-----------------------------------------------------------------------------------------
			case 8: // write address from zr to address bus
				addressBus.write (zr);
				execCount++;
				break;
			case 9: // Set control signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 10: //get data from Memory
				*reg_destination = (*reg_destination & 0xff00)
						| (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " from ZR address" << std::endl;
				execCount++;
				break;
			case 11: // increment zr and reset control signals
				resetProcSignals ();
				zr = zr.to_uint () + 1;
				execCount++;
				break;

				// copy high data byte from memory to the destination register
				//-----------------------------------------------------------------------------------------
			case 12: // write zr to addressbus
				addressBus.write (zr);
				execCount++;
				break;
			case 13: // Set control signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 14: // Write high byte to destination register
				*reg_destination = (read_databus_resolved () << 8)
						| (*reg_destination & 0x00FF);
				std::cout << "Got hi byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved ().to_uint ()
						<< " from ZR address" << std::endl;
				execCount++;
				break;
			case 15: //  reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	call_label ()
	{
		bool ready = false;

		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:
				// Write address register to address bus
				cout << "WRITE low byte address 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:
				//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:
				// Write low byte to ZR
				cout << "Write low byte of data to ZR register" << endl;
				zr (7, 0) = dataBus.read ();
				execCount++;
				break;
			case 3:
				// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;
			case 4:
				// Write PC to address bus
				cout << "WRITE high byte address 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << pc.to_uint () << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;
			case 5:  // Set signals for read
				setProcSignals_mem_read ();
				execCount++;
				break;
			case 6:  // Write hi byte to zr
				cout << "WRITE high byte of databus to ZR register" << endl;
				zr (15, 8) = dataBus.read ();
				execCount++;
				break;
			case 7: // reset control signals and increment pc
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;

				// push pc to stack
				//-------------------------------------------------------------------------
			case 8: // decrement stack pointer
				sp = sp.to_uint () - 1;
				execCount++;
				break;

			case 9:	// Write Stack Pointer register to address bus
				cout << "WRITE SP = " << sp << " to AddressBus" << endl;
				addressBus.write (sp);
				execCount++;
				break;

			case 10:	//Set control sigs for mem write
				setProcSignals_mem_write ();
				execCount++;
				break;

			case 11:	// Write high byte to stack
				cout << "Write high Byte of PC to databus " << endl;
				dataBus.write (sc_lv<8> (pc.range (15, 8)));
				execCount++;
				break;

			case 12:	// Reset control signals and and decrement sp
				resetProcSignals ();
				sp = sp.to_uint () - 1;
				execCount++;
				break;

			case 13: // Set control sigs for mem write
				setProcSignals_mem_write ();
				execCount++;
				break;

			case 14: // Write low byte of pc to stack
			{
				cout << "Write low byte of pc to stack " << endl;
				dataBus.write (sc_lv<8> (pc.range (7, 0)));
				execCount++;
			}
				break;

			case 15:	// Reset control signals and and copy zr to pc
				pc = zr;
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	ret ()
	{
		bool ready = false;

		switch (execCount)
		{
			//-------------------------------------------------------------------------
			case 0:	// Write stack pointer register to address bus
				cout << "WRITE SP = " << sp << " to AddressBus" << endl;
				addressBus.write (sp);
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// read PC low byte
				pc (7, 0) = read_databus_resolved ();
				execCount++;
				break;

			case 3:	// Reset control signals and and invrement sp
				resetProcSignals ();
				sp = sp.to_uint () + 1;
				execCount++;
				break;

			case 4:  // Write stack pointer to address bus
				addressBus.write (sp);
				execCount++;
				break;

			case 5: // Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 6: // read PC high byte
				pc (15, 8) = read_databus_resolved ();
				execCount++;
				break;

			case 7:	// Reset control signals and and increment sp
				resetProcSignals ();
				execCount++;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	pop ()
	{
		bool ready = false;

		switch (execCount)
		{

			//-------------------------------------------------------------------------
			case 0:	// Write Stack Pointer register to address bus
				cout << "WRITE SP = " << sp << " to AddressBus" << endl;
				addressBus.write (sp);
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// read status flags
			{
				sc_bv<8> input_data = read_databus_resolved ();

				flag_s = input_data[0];
				flag_pv = input_data[1];
				flag_z = input_data[2];
				flag_c = input_data[3];

				execCount++;
			}
				break;

			case 3:	// Reset control signals and and increment sp
				resetProcSignals ();
				sp = sp.to_uint () + 1;
				execCount++;
				break;

			case 4:  // Write stack pointer to address bus
				addressBus.write (sp);
				execCount++;
				break;

			case 5: // Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 6: // Read accu from mem
				a = read_databus_resolved ();
				execCount++;
				break;

			case 7:	// Reset control signals and and increment sp
				resetProcSignals ();
				sp = sp.to_uint () + 1;
				execCount++;
				ready = true;
				break;
		}

		return ready;
	}

	bool
	push ()
	{
		bool ready = false;
		switch (execCount)
		{

			//-------------------------------------------------------------------------
			case 0: // decrement stack pointer
				sp = sp.to_uint () - 1;
				execCount++;
				break;

			case 1:	// Write Stack Pointer register to address bus
				cout << "WRITE SP = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << sp << " to AddressBus" << endl;
				addressBus.write (sp);
				execCount++;
				break;

			case 2:	//Set control sigs for mem write
				setProcSignals_mem_write ();
				execCount++;
				break;

			case 3:	// Write Accu to mem
				cout << "Write Accu to databus " << endl;
				dataBus.write (a);
				execCount++;
				break;

			case 4:	// Reset control signals and and decrement sp
				resetProcSignals ();
				sp = sp.to_uint () - 1;
				execCount++;
				break;

			case 5:  // Write stack pointer to address bus
				addressBus.write (sp);
				execCount++;
				break;

			case 6: // Set control sigs for mem write
				setProcSignals_mem_write ();
				execCount++;
				break;

			case 7: // Write status Register to mem
			{
				cout << "Write Status regs to databus " << endl;
				sc_lv<8> output_data;
				output_data[0] = flag_s;
				output_data[1] = flag_pv;
				output_data[2] = flag_z;
				output_data[3] = flag_c;
				output_data.range (7, 4) = "0000";

				dataBus.write (output_data);
				execCount++;
			}
				break;

			case 8:	// Reset control signals
				resetProcSignals ();
				execCount++;
				ready = true;
				break;
		}

		return ready;
	}

	bool
	in_a_port ()
	{
		bool ready = false;

		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:  // Write address register to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:  //Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:  // Write low byte to ZR
				std::cout << "Write low byte of data from databus to ZR register"
						<< std::endl;
				zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved () << std::endl;
				execCount++;
				break;
			case 3:	// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;

				// copy data from io to register
				//-----------------------------------------------------------------------------------------
			case 4:  // write address from zr to address bus
				addressBus.write (zr);
				execCount++;
				break;
			case 5:  // Set control signals for read
				setProcSignals_io_read ();
				execCount++;
				break;
			case 6:  //get data from io
				a = read_databus_resolved ();
				execCount++;
				break;
			case 7:  // reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	bool
	out_port_a ()
	{
		bool ready = false;

		switch (execCount)
		{
			// load register ZR
			//-------------------------------------------------------------------------
			case 0:	// Write address register to address bus
				cout << "WRITE PC = 0x" << std::hex << std::setw (4)
						<< std::setfill ('0') << pc << " to AddressBus" << endl;
				addressBus.write (pc);
				execCount++;
				break;

			case 1:	//Set control sigs for mem read
				setProcSignals_mem_read ();
				execCount++;
				break;

			case 2:	// Write low byte to ZR
				std::cout << "Write low byte of data from databus to ZR register"
						<< std::endl;
				zr = (zr & 0xff00) | (0x00FF & read_databus_resolved ());
				std::cout << "Got low byte 0x" << std::hex << std::setw (2)
						<< std::setfill ('0') << read_databus_resolved () << std::endl;
				execCount++;
				break;
			case 3:	// Reset control signals and pc++
				resetProcSignals ();
				pc = pc.to_uint () + 1;
				execCount++;
				break;

				// Copy data from io to register
				//-----------------------------------------------------------------------------------------
			case 4:  // Write address from zr to address bus
				addressBus.write (zr);
				execCount++;
				break;
			case 5:  // Set control signals for write
				setProcSignals_io_write ();
				execCount++;
				break;
			case 6:  // Get data from io
				dataBus.write (a);
				execCount++;
				break;
			case 7:  // Reset control signals
				resetProcSignals ();
				execCount = 0;
				ready = true;
				break;
		}
		return ready;
	}

	sc_bv<8>
	read_databus_resolved ()
	{
		sc_bv<8> input_data_bv;
		sc_lv<8> input_data_lv = dataBus.read ();

		if (input_data_lv == "zzzzzzzz" || input_data_lv == "xxxxxxxx")
		{
			cout << "Module Mc8: Error reading dataBus : undefined value" << endl;
			input_data_bv = "00000000";
			exit (-1);
		}
		else
		{
			input_data_bv = input_data_lv.to_uint ();
		}

		return input_data_bv;
	}
}
;
