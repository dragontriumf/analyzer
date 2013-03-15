///\file Banks.cxx
///\author G. Christian
///\brief Implements Banks.hxx
#include <cstring>
#include <string>
#include <vector>
#include "midas/Database.hxx"
#include "ErrorDragon.hxx"
#include "Banks.hxx"


void dragon::utils::Banks::Set(dragon::utils::Banks::Name_t bkName, const char* from)
{
	/*!
	 * Sets a new bank name, checking to make sure the length is correct.
	 * \param [out] bkName Bank name to set
	 * \param [in] from Desired new value of \e bkName
	 */
	int fromLen = strlen(from);
	if (fromLen >= 4) {
		strncpy(bkName, from, 4);
		bkName[4] = '\0';
		if (fromLen > 4) {
			dragon::utils::err::Warning("dragon::utils::Banks::Set")
				<< "Source string longer than 4: truncating bank name to " << bkName
				<< DRAGON_ERR_FILE_LINE;
		}
	}
	else {
		strncpy(bkName, from, fromLen);
		for(int i= fromLen; i < 4; ++i) bkName[i] = '0';
		bkName[4] = '\0';
		dragon::utils::err::Warning("dragon::utils::Banks::Set")
			<< "Source string shorter than 4: extending bank name to " << bkName
			<< DRAGON_ERR_FILE_LINE;
	}
}

bool dragon::utils::Banks::OdbSet(dragon::utils::Banks::Name_t bkName, const midas::Database& odb, const char* path)
{
	std::string temp;
	bool success = !odb.IsZombie();
	if(success) success = odb.ReadValue(path, temp);
	if(success) dragon::utils::Banks::Set(bkName, temp.c_str());
	
	return success;
}

bool dragon::utils::Banks::OdbSetArray(dragon::utils::Banks::Name_t* bkName, int length,
																			 const midas::Database& odb, const char* path)
{
	std::vector<std::string> temp(length);
	bool success = !odb.IsZombie();

	if(success) success = odb.ReadArray(path, &temp[0], length);
	if(success) {
		for(int i=0; i< length; ++i) {
			dragon::utils::Banks::Set(bkName[i], temp[i].c_str());
		}
	}

	return success;
}

		
