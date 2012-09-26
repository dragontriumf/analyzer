//! \file V1190.cxx
//! \author G. Christian
//! \brief Implements V1190.hxx
#include <string>
#include <cassert>
#include "utils/Bits.hxx"
#include "utils/Error.hxx"
#include "midas/Event.hxx"
#include "Constants.hxx"
#include "V1190.hxx"


void vme::caen::V1190::reset()
{
	n_ch = 0;
	count = 0;
	word_count = 0;
	trailer_word_count = 0;
	event_id = 0;
	bunch_id = 0;
	status = 0;
	type = vme::NONE;
	extended_trigger = vme::NONE;
	for (int i=0; i< MAX_CHANNELS; ++i) {
		leading_edge[i].clear();
		trailing_edge[i].clear();
		nleading[i]  = 0;
		ntrailing[i] = 0;
	}
}

bool vme::caen::V1190::unpack_data_buffer(const uint32_t* const pbuffer)
{
	/*!
	 * \param [in] pbuffer Pointer to the data buffer word.
	 *
	 * A data buffer encodes the measurement value (i.e. the pulse time)
	 * for a single TDC measurement. The TDC is multi-hit, so more than one measurement
	 * per channel can be read in a single event.
	 *
	 * See below for bitpacking instructions.
	 */

	type     = (*pbuffer >> 26) & READ1; /// Bit 26 tells the measurement type (leading or trailing)
	int ch   = (*pbuffer >> 19) & READ7; /// Bits 19-25 tell the channel number
	if (ch >= MAX_CHANNELS) {
		err::Error("vme::caen::V1190::unpack_data_buffer")
			<< ERR_FILE_LINE << "Read a channel number (" << ch
			<< ") which is >= the maximum (" << MAX_CHANNELS << "). Skipping...\n";
		return false;
	}

	int32_t measurement = (*pbuffer >> 0) & READ19; /// Bits 0 - 18 encode the measurement value

	if (type == 0) leading_edge[ch].push_back(measurement);
	else if (type == 1) trailing_edge[ch].push_back(measurement);
	else assert (0 && "Impossible!");

	nleading[ch] = leading_edge[ch].size();
	ntrailing[ch] = trailing_edge[ch].size();
	return true;
}

void vme::caen::V1190::unpack_footer_buffer(const uint32_t* const pbuffer, const char* bankName)
{
  /*!
	 * \param [in] pbuffer Pointer to the footer buffer
	 * \param [in] bankName Name of the midas bank being unpacked
	 * \note Footer also contains the GEO information, which we ignore.
	 *
	 * See below for the bitpacking instructions and what we read.
	 */
	word_count = (*pbuffer >> 0) & READ12; /// Bits 0 - 11 are the event counter (word_count)
	int16_t evtId = (*pbuffer >> 12) & READ12; 
	if(evtId != event_id) { /// Bits 12 - 23 are the event id (event_id), check for consistency w/ header
		std::cerr << ERR_FILE_LINE;
		err::Warning("vme::caen::V1190::unpack_footer_buffer")
			<< ERR_FILE_LINE << "Bank name: \"" << bankName << "\": "
			<< "Trailer event id (" << evtId << ") != header event Id (" << event_id << ")\n";
	}
}

void vme::caen::V1190::handle_error_buffer(const uint32_t* const pbuffer, const char* bankName)
{
	/*!
	 * Error encoding is handled with a bitmask, bits 0 - 13. Here we print the
	 * appropriate messages as given in the V1190 manual.
	 * \param [in] pbuffer Pointer to the error buffer longword
	 * \param [in] bankName Name of the MIDAS bank containing the data in question
	 */
	std::string errors[15] = {
		"Hit lost in group 0 from read-out FIFO overflow.",
		"Hit lost in group 0 from L1 buffer overflow",
		"Hit error have been detected in group 0.",
		"Hit lost in group 1 from read-out FIFO overflow.",
		"Hit lost in group 1 from L1 buffer overflow",
		"Hit error have been detected in group 1.",
		"Hit data lost in group 2 from read-out FIFO overflow.",
		"Hit lost in group 2 from L1 buffer overflow",
		"Hit error have been detected in group 2.",
		"Hit lost in group 3 from read-out FIFO overflow.",
		"Hit lost in group 3 from L1 buffer overflow",
		"Hit error have been detected in group 3.",
		"Hits rejected because of programmed event size limit",
		"Event lost (trigger FIFO overflow).",
		"Internal fatal chip error has been detected."
	};
	err::Error error("vme::caen::handle_error_buffer");
	error << ERR_FILE_LINE << "Bank name: \"" << bankName << 
		"\": TDC Error buffer: error flags:\n";

	int flag;
	for(int i=0; i< 14; ++i) {
		flag = (*pbuffer >> i) & READ1;
		if(flag) {
			 error << "[" << i << "]: " << errors[i] << "\n";
		}
	}
}

bool vme::caen::V1190::unpack_buffer(const uint32_t* const pbuffer, const char* bankName)
{
  /*!
	 * \param [in] pbuffer Pointer to the buffer data
	 * \param [in] bankName Name of the midas bank being unpacked
	 * \returns True if the buffer was successfully unpacked, false otherwise
	 *
	 * V1190 buffers are 32 bit words. Bits 27-31 specify the type of data contained
	 * in the buffer. In this function, we read the buffer type and then handle appropriately.
	 */
	bool success = true;
	uint32_t type = (*pbuffer >> 27) & READ5;

	switch (type) {
	case GLOBAL_HEADER:  /// case GLOBAL_HEADER: read event counter from bits 5 - 26
		count = (*pbuffer >> 5) & READ22;
		break;
	case GLOBAL_TRAILER: /// case GLOBAL_TRAILER: read status word from bits 24-26 and word count from bits 5-21
		status = (*pbuffer >> 24) & READ3;
		trailer_word_count = (*pbuffer >> 5) & READ16;
		break;
	case EXTENDED_TRIGGER_TIME: /// case EXTENDED_TRIGGER_TIME: read extended trigger from bits 0 - 26
		extended_trigger = (*pbuffer >> 0) & READ27;
		break;
	case TDC_HEADER: /// case TDC_HEADER: read bunch id from bits 0 - 12, event id from bits 12 - 23
		bunch_id = (*pbuffer >> 0)  & READ12;
		event_id = (*pbuffer >> 12) & READ12;
		break;
	case TDC_MEASUREMENT: /// case TDC_MEASUREMENT: See unpack_data_buffer()
		success = unpack_data_buffer(pbuffer);
		break;
	case TDC_ERROR:   /// case TDC_ERROR: See handle_error_buffer()
		handle_error_buffer(pbuffer, bankName);
		success = false;
		break;
	case TDC_TRAILER: /// case TDC_TRAILER: See unpack_footer_buffer()
		unpack_footer_buffer(pbuffer, bankName);
		break;
	default: /// Bail out if we read an unknown buffer code
		err::Error("vme::caen::V1190::unpack_buffer")
			<< ERR_FILE_LINE << "Bank name: \"" << bankName
			<< "\": Unknown TDC buffer code: 0x" << std::hex << type << ". Skipping...\n";
		success = false;
		break;
	}
	return success;
}

bool vme::caen::V1190::unpack(const midas::Event& event, const char* bankName, bool reportMissing)
{
  /*!
	 * \param [in] event The midas event to unpack
	 * \param [in] bankName Name of the bank to unpack
   * \param [in] reportMissing False specifies to silently return if \e bankName isn't found in
	 *             the event. True specifies to print a warning message if this is the case.
	 * \returns True if the event was successfully unpacked, false otherwise
	 */
  int bank_len;
	uint32_t* pbank32 =
		event.GetBankPointer<uint32_t>(bankName, &bank_len, reportMissing, true);

  // Loop over all data words in the bank
	bool ret = true;
  for (int i=0; i< bank_len; ++i) {
    bool success = unpack_buffer(pbank32++, bankName);
		if(!success) ret = false;
  }
  return ret;
}
