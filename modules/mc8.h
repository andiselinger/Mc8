#include "systemc.h"

typedef enum state_e {
	FETCH_INSTR, FETCH_TWO_BYTE_ADDRESS, DECODE, EXECUTE, HALT
} state_t;

typedef enum fetch_state_e {
	SET_ADDRESS, SET_CONTROL_SIG, READ_TO_INSTR_REG, RESET_CONTROL_SIG
} fetch_state_t;

typedef enum fetch_instr_state_e {
	SET_ADDRESS_LO,
	SET_CONTROL_SIG_LO,
	READ_TO_ZR_LO,
	RESET_CONTROL_SIG_LO,
	SET_ADDRESS_HI,
	SET_CONTROL_SIG_HI,
	READ_TO_ZR_HI,
	RESET_CONTROL_SIG_HI
} fetch_instr_state_t;

SC_MODULE(mc8) {

public:
	sc_in<bool> clk;
	sc_in<bool> tclk;
	sc_in<bool> res;

	sc_out<bool> wr;
	sc_out<bool> rd;
	sc_out<bool> mreq;
	sc_out<bool> iorq;

	sc_in<sc_bv<8> > dataBus;
	sc_out<sc_bv<16> > addressBus;

	state_t state;

private:

	// Registers
	sc_bv<16> pc;
	sc_bv<16> zr;
	sc_bv<16> ix;
	sc_bv<16> hl;
	sc_bv<8> ir;
	sc_bv<8> a;
	sc_bv<8> b;
	sc_bv<8> c;

	// Flags
	bool flag_c;
	bool flag_z;
	bool flag_pv;
	bool flag_s;
	sc_bv<4> tempFlags;	// Stores flags directly after computation, before corresponding flags are written

	// ALU
	sc_bv<8> alu1;
	sc_bv<8> alu2;
	sc_bv<8> aluRes;

	fetch_state_t fetchState;
	fetch_instr_state_t fetchInstrState;
	unsigned int execCount;	// Indicates the current execution step of instruction
	unsigned int twoByteInstrB1;		// First Byte of two byte instruction
public:

	SC_HAS_PROCESS(mc8);

	mc8(sc_module_name name) :
			sc_module(name) {

		/*		myWriter->registerBool("CLK");
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
		resetRegAndInternals();
		SC_METHOD(ProcCycle);
		sensitive << clk.pos() << res;
		SC_METHOD(traceIt);
		sensitive << tclk.pos();

		dont_initialize();
	}

private:

	void ProcCycle() {

		//async reset
		if (res.read() == false) {
			resetProcSignals();
			resetRegAndInternals();
		} else if (clk.read() == true) {
			switch (state) {
			case FETCH_INSTR:
				switch (fetchState) {
				case SET_ADDRESS:
					std::cout << "FETCH INSTRUCTION ... [START] " << std::endl;
					std::cout << "Will fetch instruction at address "
							<< pc.to_uint() << std::endl;
					addressBus.write(pc);
					fetchState = SET_CONTROL_SIG;
					break;
				case SET_CONTROL_SIG:
					wr.write(true);
					rd.write(false);
					mreq.write(false);
					iorq.write(true);
					fetchState = READ_TO_INSTR_REG;
					break;
				case READ_TO_INSTR_REG:
					ir = dataBus.read();
					std::cout << "Got instruction " << ir.to_uint()
							<< std::endl;
					fetchState = RESET_CONTROL_SIG;
					break;
				case RESET_CONTROL_SIG:
					wr.write(true);
					rd.write(true);
					mreq.write(true);
					iorq.write(true);
					pc = pc.to_uint() + 1;
					fetchState = SET_ADDRESS;
					state = DECODE;
					std::cout << "FETCH INSTRUCTION ... [END] " << std::endl;
					break;
				default:
					fetchState = SET_ADDRESS;
				}
				break; //FETCH_INSTR

			case FETCH_TWO_BYTE_ADDRESS:

				switch (fetchInstrState) {
				case SET_ADDRESS_LO:
					std::cout << "FETCH TWO BYTE ADDRESS ... [START] "
							<< std::endl;
					std::cout << "Will fetch label low byte at address "
							<< pc.to_uint() << std::endl;
					addressBus.write(pc);
					fetchInstrState = SET_CONTROL_SIG_LO;
					break;
				case SET_CONTROL_SIG_LO:
					wr.write(true);
					rd.write(false);
					mreq.write(false);
					iorq.write(true);
					fetchInstrState = READ_TO_ZR_LO;
					break;
				case READ_TO_ZR_LO:
					zr = (zr & 0xff00) | (0x00FF & dataBus.read());
					std::cout << "Got low byte " << dataBus.read() << std::endl;
					fetchInstrState = RESET_CONTROL_SIG_LO;
					break;
				case RESET_CONTROL_SIG_LO:
					wr.write(true);
					rd.write(true);
					mreq.write(true);
					iorq.write(true);
					pc = pc.to_uint() + 1;
					fetchInstrState = SET_ADDRESS_HI;
					break;
				case SET_ADDRESS_HI:
					std::cout << "Will fetch label hi byte at address "
							<< pc.to_uint() << std::endl;
					addressBus.write(pc);
					fetchInstrState = SET_CONTROL_SIG_HI;
					break;
				case SET_CONTROL_SIG_HI:
					wr.write(true);
					rd.write(false);
					mreq.write(false);
					iorq.write(true);
					fetchInstrState = READ_TO_ZR_HI;
					break;
				case READ_TO_ZR_HI:
					zr = (dataBus.read() << 8) | (zr & 0x00FF);
					std::cout << "Got hi byte " << dataBus.read() << std::endl;
					fetchInstrState = RESET_CONTROL_SIG_HI;
					break;
				case RESET_CONTROL_SIG_HI:
					wr.write(true);
					rd.write(true);
					mreq.write(true);
					iorq.write(true);
					pc = pc.to_uint() + 1;
					fetchInstrState = SET_ADDRESS_LO;
					state = EXECUTE;
					std::cout << "FETCH TWO BYTE ADDRESS... [END] "
							<< std::endl;
					break;

				default:
					fetchInstrState = SET_ADDRESS_LO;
				}

				break; //FETCH_TWO_BYTE_ADDRESS

			case DECODE:
				std::cout << "DECODE INSTRUCTION ... [START] " << std::endl;
				decodeInstr(ir);
				std::cout << "DECODE INSTRUCTION ... [END] " << std::endl;

				break; //DECODE

			case EXECUTE:
				switch (ir.to_uint()) {
				case 0x00:
					// NOP
					std::cout << "EXECUTE NOP ... [START] " << std::endl;
					state = FETCH_INSTR;
					std::cout << "EXECUTE NOP ... [END] " << std::endl;
					break;

				case 0x3E:
					// MOV A,dat_8
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE MOV A,dat_8 ... [START] "
								<< std::endl;
						// Read dat8 from Mem, set address
						addressBus.write(pc);
						execCount++;
						break;
					case 1:
						// Set control sigs for mem read
						wr.write(true);
						rd.write(false);
						mreq.write(false);
						iorq.write(true);
						execCount++;
						break;
					case 2:
						// Read dat_8 to A
						a = dataBus.read();
						std::cout << "Read dat_8 and copy to A "
								<< dataBus.read() << std::endl;
						execCount++;
						break;
					case 3:
						// Reset control signals
						wr.write(true);
						rd.write(true);
						mreq.write(true);
						iorq.write(true);
						pc = pc.to_uint() + 1;
						std::cout << "EXECUTE MOV A,dat_8 ... [END] "
								<< std::endl;
						execCount = 0;
						state = FETCH_INSTR;
						break;
					default:
						break;
					}
					break; // MOV A,dat_8
				case 0x06:
					// MOV B,dat_8
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE MOV B,dat_8 ... [START] "
								<< std::endl;
						// Read dat8 from Mem, set address
						addressBus.write(pc);
						execCount++;
						break;
					case 1:
						// Set control sigs for mem read
						wr.write(true);
						rd.write(false);
						mreq.write(false);
						iorq.write(true);
						execCount++;
						break;
					case 2:
						// Read dat_8 to B
						b = dataBus.read();
						std::cout << "Read dat_8 and copy to B "
								<< dataBus.read() << std::endl;
						execCount++;
						break;
					case 3:
						// Reset control signals
						wr.write(true);
						rd.write(true);
						mreq.write(true);
						iorq.write(true);
						pc = pc.to_uint() + 1;
						std::cout << "EXECUTE MOV B,dat_8 ... [END] "
								<< std::endl;
						execCount = 0;
						state = FETCH_INSTR;
						break;
					default:
						break;
					}
					break; // MOV B,dat_8
				case 0x0E:
					// MOV C,dat_8
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE MOV C,dat_8 ... [START] "
								<< std::endl;
						// Read dat8 from Mem, set address
						addressBus.write(pc);
						execCount++;
						break;
					case 1:
						// Set control sigs for mem read
						wr.write(true);
						rd.write(false);
						mreq.write(false);
						iorq.write(true);
						execCount++;
						break;
					case 2:
						// Read dat_8 to C
						c = dataBus.read();
						std::cout << "Read dat_8 and copy to c "
								<< dataBus.read() << std::endl;
						execCount++;
						break;
					case 3:
						// Reset control signals
						wr.write(true);
						rd.write(true);
						mreq.write(true);
						iorq.write(true);
						pc = pc.to_uint() + 1;
						std::cout << "EXECUTE MOV C,dat_8 ... [END] "
								<< std::endl;
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

					// INCREMENTS and DECREMENTS
				case 0x3C:
					// INC A
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE INC A ... [START] " << std::endl;
						// Load A to ALU
						alu1 = a;
						execCount++;
						std::cout << "Load " << a.to_int() << std::endl;
						break;
					case 1:
						// ALU inc
						aluRes = alu1.to_uint() + 1;
						execCount++;
						break;
					case 2:
						// Set flags and Store Result
						if (aluRes.to_uint() == 0)
							flag_z = true;
						else
							flag_z = false;
						if (alu1.to_uint() == 0x7F)
							flag_pv = true;
						else
							flag_pv = false;
						if (aluRes[7])
							flag_s = true;
						else
							flag_s = false;

						// Store Result
						a = aluRes;
						std::cout << "Write back result " << a.to_int()
								<< std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE INC B ... [START] " << std::endl;
						// Load B to ALU
						alu1 = b;
						execCount++;
						std::cout << "Load " << b.to_int() << std::endl;
						break;
					case 1:
						// ALU inc
						aluRes = alu1.to_uint() + 1;
						execCount++;
						break;
					case 2:
						// Set flags and Store Result
						if (aluRes.to_uint() == 0)
							flag_z = true;
						else
							flag_z = false;
						if (alu1.to_uint() == 0x7F)
							flag_pv = true;
						else
							flag_pv = false;
						if (aluRes[7])
							flag_s = true;
						else
							flag_s = false;

						// Store Result
						b = aluRes;
						std::cout << "Write back result " << b.to_int()
								<< std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE INC C ... [START] " << std::endl;
						// Load C to ALU
						alu1 = c;
						execCount++;
						std::cout << "Load " << c.to_int() << std::endl;
						break;
					case 1:
						// ALU inc
						aluRes = alu1.to_uint() + 1;
						execCount++;
						break;
					case 2:
						// Set flags and Store Result
						if (aluRes.to_uint() == 0)
							flag_z = true;
						else
							flag_z = false;
						if (alu1.to_uint() == 0x7F)
							flag_pv = true;
						else
							flag_pv = false;
						if (aluRes[7])
							flag_s = true;
						else
							flag_s = false;

						// Store Result
						c = aluRes;
						std::cout << "Write back result " << c.to_int()
								<< std::endl;
						std::cout << "EXECUTE INC C ... [END] " << std::endl;
						execCount = 0;
						state = FETCH_INSTR;
						break;
					default:
						break;
					}
					break;

				case 0x23:
					if (twoByteInstrB1 == 0xDD) {
						// INC IX
						std::cout << "EXECUTE INC IX ... [START] " << std::endl;
						ix = ix.to_uint() + 1;
						std::cout << "EXECUTE INC IX ... [END] " << std::endl;
					} else {
						// INC HL
						std::cout << "EXECUTE INC HL ... [START] " << std::endl;
						hl = hl.to_uint() + 1;
						std::cout << "EXECUTE INC HL ... [END] " << std::endl;
					}
					state = FETCH_INSTR;
					twoByteInstrB1 = 0;
					break;

				case 0x3D:
					// DEC A
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE DEC A ... [START] " << std::endl;
						// Load A to ALU
						alu1 = a;
						execCount++;
						std::cout << "Load " << a.to_int() << std::endl;
						break;
					case 1:
						// ALU dec
						aluRes = alu1.to_uint() - 1;
						execCount++;
						break;
					case 2:
						// Set flags and Store Result
						if (aluRes.to_uint() == 0)
							flag_z = true;
						else
							flag_z = false;
						if (alu1.to_uint() == 0x80)
							flag_pv = true;
						else
							flag_pv = false;
						if (aluRes[7])
							flag_s = true;
						else
							flag_s = false;

						// Store Result
						a = aluRes;
						std::cout << "Write back result " << a.to_int()
								<< std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE DEC B ... [START] " << std::endl;
						// Load B to ALU
						alu1 = b;
						execCount++;
						std::cout << "Load " << b.to_int() << std::endl;
						break;
					case 1:
						// ALU dec
						aluRes = alu1.to_uint() - 1;
						execCount++;
						break;
					case 2:
						// Set flags and Store Result
						if (aluRes.to_uint() == 0)
							flag_z = true;
						else
							flag_z = false;
						if (alu1.to_uint() == 0x80)
							flag_pv = true;
						else
							flag_pv = false;
						if (aluRes[7])
							flag_s = true;
						else
							flag_s = false;

						// Store Result
						b = aluRes;
						std::cout << "Write back result " << b.to_int()
								<< std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE DEC C ... [START] " << std::endl;
						// Load C to ALU
						alu1 = c;
						execCount++;
						std::cout << "Load " << c.to_int() << std::endl;
						break;
					case 1:
						// ALU dec
						aluRes = alu1.to_uint() - 1;
						execCount++;
						break;
					case 2:
						// Set flags and Store Result
						if (aluRes.to_uint() == 0)
							flag_z = true;
						else
							flag_z = false;
						if (alu1.to_uint() == 0x80)
							flag_pv = true;
						else
							flag_pv = false;
						if (aluRes[7])
							flag_s = true;
						else
							flag_s = false;

						// Store Result
						c = aluRes;
						std::cout << "Write back result " << c.to_int()
								<< std::endl;
						std::cout << "EXECUTE DEC C ... [END] " << std::endl;
						execCount = 0;
						state = FETCH_INSTR;
						break;
					default:
						break;
					}
					break;

				case 0x2B:
					if (twoByteInstrB1 == 0xDD) {
						// DEC IX
						std::cout << "EXECUTE DEC IX ... [START] " << std::endl;
						ix = ix.to_uint() - 1;
						std::cout << "EXECUTE DEC IX ... [END] " << std::endl;
					} else {
						// DEC HL
						std::cout << "EXECUTE DEC HL ... [START] " << std::endl;
						hl = hl.to_uint() - 1;
						std::cout << "EXECUTE DEC HL ... [END] " << std::endl;
					}
					twoByteInstrB1 = 0;
					state = FETCH_INSTR;
					break;

					// Arithmetic Operations
				case 0x87:
					// ADD A
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE ADD A ... [START] " << std::endl;
						// Load A to alu1
						alu1 = a;
						std::cout << "Load " << alu1.to_uint() << " to alu1"
								<< std::endl;
						execCount++;
						break;
					case 1:
						// Load A to alu2
						alu2 = a;
						std::cout << "Load " << alu2.to_uint() << " to alu2"
								<< std::endl;
						execCount++;
						break;
					case 2:
						// Execute Computation
						aluRes = add(alu1, alu2);
						execCount++;
						break;
					case 3:
						// Set flags and store result
						flag_c = tempFlags[0];
						flag_z = tempFlags[1];
						flag_pv = tempFlags[2];
						flag_s = tempFlags[3];
						a = aluRes;
						std::cout << "New Value in A: " << a << std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE ADD B ... [START] " << std::endl;
						// Load A to alu1
						alu1 = a;
						std::cout << "Load A = " << alu1.to_uint() << " to alu1"
								<< std::endl;
						execCount++;
						break;
					case 1:
						// Load B to alu2
						alu2 = b;
						std::cout << "Load B = " << alu2.to_uint() << " to alu2"
								<< std::endl;
						execCount++;
						break;
					case 2:
						// Execute Computation
						aluRes = add(alu1, alu2);
						execCount++;
						break;
					case 3:
						// Set flags and store result
						flag_c = tempFlags[0];
						flag_z = tempFlags[1];
						flag_pv = tempFlags[2];
						flag_s = tempFlags[3];
						a = aluRes;
						std::cout << "New Value in A: " << a << std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE ADD C ... [START] " << std::endl;
						// Load A to alu1
						alu1 = a;
						std::cout << "Load A = " << alu1.to_uint() << " to alu1"
								<< std::endl;
						execCount++;
						break;
					case 1:
						// Load C to alu2
						alu2 = c;
						std::cout << "Load C =  " << alu2.to_uint() << " to alu2"
								<< std::endl;
						execCount++;
						break;
					case 2:
						// Execute Computation
						aluRes = add(alu1, alu2);
						execCount++;
						break;
					case 3:
						// Set flags and store result
						flag_c = tempFlags[0];
						flag_z = tempFlags[1];
						flag_pv = tempFlags[2];
						flag_s = tempFlags[3];
						a = aluRes;
						std::cout << "New Value in A: " << a << std::endl;
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
					switch (execCount) {
					case 0:
						std::cout << "EXECUTE ADD dat_8 ... [START] "
								<< std::endl;
						// Read dat8 from Mem, set address
						addressBus.write(pc);
						execCount++;
						break;
					case 1:
						// Load A to ALU
						alu1 = a;
						std::cout << "Load A = " << a.to_uint() << " to alu1"
								<< std::endl;
						execCount++;
						break;
					case 2:
						// Set control sigs for mem read
						wr.write(true);
						rd.write(false);
						mreq.write(false);
						iorq.write(true);
						execCount++;
						break;
					case 3:
						// Read dat_8 to ALU
						alu2 = dataBus.read();
						std::cout << "Read dat_8 to ALU " << dataBus.read()
								<< std::endl;
						execCount++;
						break;
					case 4:
						// Execute Computation
						aluRes = add(alu1, alu2);
						execCount++;
						break;
					case 5:
						// set flags, store result and reset control signals
						flag_c = tempFlags[0];
						flag_z = tempFlags[1];
						flag_pv = tempFlags[2];
						flag_s = tempFlags[3];
						a = aluRes;			// Store result
						wr.write(true);
						rd.write(true);
						mreq.write(true);
						iorq.write(true);
						pc = pc.to_uint() + 1;
						std::cout << "New Value in A: " << a << std::endl;
						std::cout << "EXECUTE ADD dat_8 ... [END] "
								<< std::endl;
						execCount = 0;
						state = FETCH_INSTR;
						break;
					default:
						break;
					}
					break; // ADD dat_8

				case 0xC2:
					// JPNZ label
					std::cout << "EXECUTE JPNZ label ... [START] " << std::endl;
					if (flag_z == true)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPNZ label ... [END] " << std::endl;
					break;

				case 0xCA:
					// JPZ label
					std::cout << "EXECUTE JPZ label ... [START] " << std::endl;
					if (flag_z == false)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPZ label ... [END] " << std::endl;
					break;

				case 0xD2:
					// JPNC label
					std::cout << "EXECUTE JPNC label ... [START] " << std::endl;
					if (flag_c == true)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPNC label ... [END] " << std::endl;
					break;

				case 0xDA:
					// JPC label
					std::cout << "EXECUTE JPC label ... [START] " << std::endl;
					if (flag_c == false)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPC label ... [END] " << std::endl;
					break;

				case 0xE2:
					// JPNO label
					std::cout << "EXECUTE JPNO label ... [START] " << std::endl;
					if (flag_pv == true)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPNO label ... [END] " << std::endl;
					break;

				case 0xEA:
					// JPO label
					std::cout << "EXECUTE JPO label ... [START] " << std::endl;
					if (flag_pv == false)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPO label ... [END] " << std::endl;
					break;

				case 0xF2:
					// JPNS label
					std::cout << "EXECUTE JPNS label ... [START] " << std::endl;
					if (flag_s == true)
						pc = zr;
					state = FETCH_INSTR;
					std::cout << "EXECUTE JPNS label ... [END] " << std::endl;
					break;

				case 0xFA:
					// JPS label
					std::cout << "EXECUTE JPS label ... [START] " << std::endl;
					if (flag_s == false)
						pc = zr;
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
					if (twoByteInstrB1 == 0xDD) {
						// JP [IX]
						std::cout << "EXECUTE JP [IX] ... [START] "
								<< std::endl;
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

	sc_bv<8> add(sc_bv<8> a, sc_bv<8> b) {
		sc_bv<8> sum;
		sc_bv<8> carry;

		sum[0] = a[0] ^ b[0];
		carry[0] = a[0] && b[0];
		for (int i = 1; i < 8; ++i) {
			sum[i] = (a[i] ^ b[i]) ^ carry[i - 1];
			carry[i] = (a[i] && b[i] && carry[i - 1]) || (a[i] && b[i])
					|| (a[i] && carry[i - 1]) || (b[i] && carry[i - 1]);
		}
		tempFlags[0] = carry[7];					// crry flag
		tempFlags[1] = sum.to_uint() == 0;			// zero flag
		tempFlags[2] = (carry[6] ^ carry[7]) == 1;// overflow flag; Bauer, Kap. 2.5: "Ein Overflow tritt auf, wenn
												  // ein Übertrag in den letzten Volladdierer hinein - oder aus
												  // ihm heraus geschoben wird; nicht aber dann, wenn der Übertrag
												  // 'durch ihn durch' geschoben wird."
		tempFlags[3] = sum[7];						// sign flag
		return sum;
	}

	void traceIt() {
		/*		myWriter->traceBool(clk.read(), "CLK", sc_time_stamp().to_seconds());
		 myWriter->traceBool(res.read(), "RES", sc_time_stamp().to_seconds());

		 myWriter->traceBool(wr.read(), "WR", sc_time_stamp().to_seconds());
		 myWriter->traceBool(rd.read(), "RD", sc_time_stamp().to_seconds());
		 myWriter->traceBool(mreq.read(), "MREQ", sc_time_stamp().to_seconds());
		 myWriter->traceBool(iorq.read(), "IORQ", sc_time_stamp().to_seconds());

		 myWriter->traceWire16(addressBus.read().to_uint(), "ABus",
		 sc_time_stamp().to_seconds());
		 myWriter->traceWire8(dataBus.read().to_uint(), "DBus",
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

	void resetProcSignals() {

		wr.write(true);
		rd.write(true);
		mreq.write(true);
		iorq.write(true);
		addressBus.write(false);
	}

	void resetRegAndInternals() {
		// Reset Prog counter
		pc = 0x0000;
		zr = 0xFFFF;
		ix = 0x0000;
		hl = 0x0000;
		a = 0x00;
		fetchState = SET_ADDRESS;
		execCount = 0;
		state = FETCH_INSTR;
	}

	void decodeInstr(sc_bv<8> i) {
		switch (i.to_uint()) {
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

			// Inkrement und Dekrement
		case 0x3C:
			std::cout << "Instruction -- INC A" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x4:
			std::cout << "Instruction -- INC B" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0xC:
			std::cout << "Instruction -- INC C" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x23:
			if (twoByteInstrB1 == 0xDD) {
				std::cout << "Instruction -- INC IX" << std::endl;
			} else {
				std::cout << "Instruction -- INC HL" << std::endl;
				twoByteInstrB1 = 0;
			}
			state = EXECUTE;
			break;
		case 0x3D:
			std::cout << "Instruction -- DEC A" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x5:
			std::cout << "Instruction -- DEC B" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0xD:
			std::cout << "Instruction -- DEC C" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x2B:
			if (twoByteInstrB1 == 0xDD) {
				std::cout << "Instruction -- DEC IX" << std::endl;
			} else {
				std::cout << "Instruction -- DEC HL" << std::endl;
				twoByteInstrB1 = 0;
			}
			state = EXECUTE;
			break;
			// Arithmetic Operations
		case 0x87:
			std::cout << "Instruction -- ADD A" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x80:
			std::cout << "Instruction -- ADD B" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x81:
			std::cout << "Instruction -- ADD C" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0xC6:
			std::cout << "Instruction -- ADD dat_8" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x97:
			std::cout << "Instruction -- SUB A" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x90:
			std::cout << "Instruction -- SUB B" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0x91:
			std::cout << "Instruction -- SUB C" << std::endl;
			state = EXECUTE;
			twoByteInstrB1 = 0;
			break;
		case 0xD6:
			std::cout << "Instruction -- SUB dat_8" << std::endl;
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
			if (twoByteInstrB1 == 0xDD) {
				std::cout << "Instruction -- JP [IX]" << std::endl;
				state = EXECUTE;
			}
			break;

			// CPU Steuerbefehele
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
			std::cout << "2 Byte Instruction" << std::endl;
			twoByteInstrB1 = i.to_uint();
			state = FETCH_INSTR;
			break;

		default:
			std::cout << "Instruction not valid" << std::endl;
		}
	}

}
;
