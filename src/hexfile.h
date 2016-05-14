/*
 * hexfile.h
 *
 *  Created on: 14. Mai 2016
 *      Author: wolfgang
 */

#ifndef HEXFILE_H_
#define HEXFILE_H_

//#define TEST

#include <vector>
#include <fstream>
#include <istream>
#include <string>
#include "record.h"

using namespace std;

class hex_file {
public:

	vector<record *> records;

	hex_file(string filename)
	{
		file = new ifstream(filename);


		if ( read_hex_file() != 0)
		{
			cout << "Error in HEX File : " << filename << endl;

		}

		file->close();
	}

	virtual ~hex_file()
	{
		delete(&records);

	}

	int copy_data_to_memory(unsigned char * data_out, int memlen)
	{
		for(unsigned int i=1 ; i < records.size() ; i++)
		{
			record *act_rec = records.at(i);

			if(act_rec->Address < memlen)
			{
				// copy Data of actual record to memory address
				for(int addr_cnt = 0 ; addr_cnt < act_rec->Byte_count ; addr_cnt ++)
				{
					data_out[act_rec->Address + addr_cnt] = act_rec->data[addr_cnt];
#ifdef TEST
					cout << "Copy " << act_rec->data[addr_cnt] << " to Address : " << act_rec->Address + addr_cnt << endl;
#endif
				}

			}
			else
			{
				cout << "Address out of memory range!" << endl;
				return -1;
			}

		}


		return 0;
	}

private :

	ifstream *file;

	int read_hex_file()
	{
		// init data to all 0xFF


		if(file->is_open())
		{
			while(!file->eof())
			{
				string line;
				getline(*file,line);

				record *new_rec = new record(line);

				if(new_rec != NULL)
					{
					if(new_rec->Record_type == record::e_types::DATA) records.push_back(new_rec);

					}
				else
				{
					cout << "Error creating record!" << endl;
					return -1;
				}

			}
		}else
		{

			return -1;
		}


		return 0;
	}

};

#endif /* HEXFILE_H_ */
