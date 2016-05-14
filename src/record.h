/*
 * record.h
 *
 *  Created on: 14. Mai 2016
 *      Author: wolfgang
 */

#ifndef RECORD_H_
#define RECORD_H_


#include <string>
#include <sstream>

using namespace std;


class record {

public:
	 record(string line)
{
		this->line = line;
	/*	if(init_record() != 0)
			{
			this -> ~record();
			}*/
		data = NULL;
		init_record();

	}
	virtual ~record()
	{
		if  ( data != NULL ) delete [] data;
	}

	string line;
	int Byte_count;
	int Address;
	enum e_types{
		DATA = 0,
		END_OF_FILE,
		EXTENDED_SEGMENT_ADDRESS,
		START_SEGMENT_ADDRESS,
		EXTENDED_LINEAR_ADDRESS,
		START_LINEAR_ADDRESS
	}Record_type;
	unsigned char *data;


private :

	const size_t DATA_LEN_START = 1;
	const size_t DATA_START = 9;
	const size_t ADDRESS_START = 3;
	const size_t RECORD_TYPE_START = 7;

	int init_record()
	{

		if (line[0]!=':' )
		{
			cout << "Error: Line not starting with ':'." << endl;
					return -1;
		}
#ifdef TEST
		cout << "Found record : ..." << endl;
#endif

		// get Byte_count
		{
			Byte_count = strtoul(line.substr(DATA_LEN_START,2).c_str(), NULL, 16);
#ifdef TEST
			cout << "Byte_count : " << Byte_count << " File : " << line.substr(DATA_LEN_START,2) << endl;
#endif
		}

		// get Address
		{

			Address = strtoul(line.substr(ADDRESS_START,4).c_str(), NULL, 16);
#ifdef TEST
			cout << "Address : " << Address << " File : " << line.substr(ADDRESS_START,4) << endl;
#endif
		}

		// get Record Type
		{
			int rec_type = strtoul(line.substr(RECORD_TYPE_START,2).c_str(), NULL, 16);
#ifdef TEST
			cout << "Record Type : " << rec_type << " File : " << line.substr(RECORD_TYPE_START,2) << endl;
#endif

			if(rec_type < 0 || rec_type > 5)
			{
				cout << "Error: Unsupported dataset type  : " << line.substr(3,4) << endl;
				return -1;
			}

			Record_type = (e_types)rec_type;
		}



		switch(Record_type)
		{
		case DATA: // get the data of the records

			data = new unsigned char [Byte_count];

			for ( int i = 0 ; i < Byte_count ; i ++)
			{
				data[i] = strtoul(line.substr((DATA_START+i*2),2).c_str(), NULL, 16);
#ifdef TEST
				cout << "Data Byte "<< i << " Data : " << (int)data[i] << endl;
#endif
			}

			break;

		case END_OF_FILE: // maybe another day

			break;
		case EXTENDED_SEGMENT_ADDRESS: // maybe another day
			cout << "Error: EXTENDED_SEGMENT_ADDRESS not supported yet" << endl;
			return -1;
			break;
		case START_SEGMENT_ADDRESS: // maybe another day
			cout << "Error: START_SEGMENT_ADDRESS  not supported yet"<< endl;
			return -1;
			break;
		case EXTENDED_LINEAR_ADDRESS: // maybe another day
			cout << "Error: EXTENDED_LINEAR_ADDRESS  not supported yet" << endl;
			return -1;
			break;
		case START_LINEAR_ADDRESS: // maybe another day
			cout << "Error: START_LINEAR_ADDRESS  not supported yet" << endl;
			return -1;
			break;

		}


		return 0;
	}

};

#endif /* RECORD_H_ */
