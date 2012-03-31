/// \file Modules.hxx
/// \brief Defines classes that encapsulate the VME modules used to read out data
///  for each DRAGON sub-system (heavy ion and gamma).
#ifndef DRAGON_MODULES_HXX
#define DRAGON_MODULES_HXX
#include "vme/IO32.hxx"
#include "vme/caen/Adc.hxx"
#include "vme/caen/V1190.hxx"

namespace dragon {

namespace gamma {

/// Encapsulates all VME modules used to read out gamma-ray data.
class Modules {
private:
	 /// CAEN v792 qdc (32 channel, integrating).
	 vme::caen::V792 v792;

   /// CAEN v1190b tdc (64 channel).
	 vme::caen::V1190b v1190b;

	 /// I032 FPGA
	 vme::IO32 io32;

public:
	 /// Initialize all modules
	 Modules();

	 /// Nothing to do
	 ~Modules() { }

   /// Unpack Midas event data into module data structures.
	 /// \param [in] event Reference to the MidasEvent object being unpacked
	 void unpack(const TMidasEvent& event);

	 /// Return data from a v792 channel
	 /// \param ch channel numner
	 int16_t v792_data(unsigned ch) const;

	 /// Return data from a v1190b channel
	 /// \param ch channel numne
	 int16_t v1190b_data(unsigned ch) const;

	 /// Return io32 tstamp value
	 int32_t tstamp() const;
};

} // namespace gamma

namespace hion {

/// Encapsulates all VME modules used to read out heavy-ion data.
class Modules {
private:
	 /// Caen v785 adcs (32 channel, peak-sensing, x2)
	 vme::caen::V785 v785[2];

	 /// CAEN v1190b TDC
	 vme::caen::V1190b v1190b;

	 /// IO32 FPGA
	 vme::IO32 io32;

public:
	 /// Initialize all modules
	 Modules();

   /// Nothing to do
	 ~Modules() { }

	 /// Unpack Midas event data into module data structures.
	 /// \param [in] event Reference to the MidasEvent object being unpacked
	 void unpack(const TMidasEvent& event);	 

	 /// Return data from a v785 channel
	 /// \param which The ADC you want data from
	 /// \param ch channel numner
	 int16_t v785_data(unsigned which, unsigned ch) const;

	 /// Return data from a v1190b channel
	 /// \param ch channel numner
	 int16_t v1190b_data(unsigned ch) const;

	 /// Return io32 tstamp value
	 int32_t tstamp() const;
};

} // namespace hion

} // namespace dragon




#endif