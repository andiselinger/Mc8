/*
 * global_definitions.h
 *
 *  Created on: 11. Mai 2016
 *      Author: wolfgang
 */

#ifndef GLOBAL_DEFINITIONS_H_
#define GLOBAL_DEFINITIONS_H_

#define ADDRESSABLE_AREA 0xFFFF

#define ROM_SIZE 0x1FFF

#define RAM_SIZE (ADDRESSABLE_AREA - ROM_SIZE)

#define RAM_START (ROM_SIZE+1)

#define RAM_READ_DELAY_NS 1
#define RAM_WRITE_DELAY_NS 1


#define ROM_READ_DELAY_US 5

#endif /* GLOBAL_DEFINITIONS_H_ */
