/// \file Coinc.cxx
/// \author G. Christian
/// \brief Implements Coinc.hxx
#include "utils/Valid.hxx"
#include "Coinc.hxx"


// ========== Class dragon::Coinc ========== //

dragon::Coinc::Coinc():
	xtof(dragon::NO_DATA), head(), tail()
{
	reset();
}

dragon::Coinc::Coinc(const dragon::Head& head, const dragon::Tail& tail):
	xtof(dragon::NO_DATA)
{
	read_event(head, tail);
}

void dragon::Coinc::reset()
{
	head.reset();
	tail.reset();
	reset_data(xtof);
}

void dragon::Coinc::set_variables(const char* odb)
{
	/*!
	 * \param [in] odb_file Name of the ODB file (*.xml or *.mid) from which to read
	 * \note Passing \c "online" looks at the experiment's ODB, if connected
	 */
	head.set_variables(odb);
	tail.set_variables(odb);
}

void dragon::Coinc::read_event(const dragon::Head& head_, const dragon::Tail& tail_)
{
	/*!
	 * \param [in] head_ Head (gamma-ray) event
	 * \param [in] tail_ Tail (heavy-ion) event
	 */
	head = head_;
	tail = tail_;
}

void dragon::Coinc::unpack(const midas::CoincEvent& coincEvent)
{
	head.unpack( *(coincEvent.fGamma) );
	tail.unpack( *(coincEvent.fHeavyIon) );
}

void dragon::Coinc::calculate_coinc()
{
	/// \todo Implement dragon::Coinc::calculate() properly
	xtof = dragon::NO_DATA ; // head - tail //
}	

void dragon::Coinc::calculate()
{
	/*!
	 * Does calculation for internal head and tail fields first,
	 * then calculates coincidence parameters.
	 */
	head.calculate();
	tail.calculate();
	calculate_coinc();
}































// DOXYGEN MAINPAGE //

/*!
	\mainpage
	\authors G. Christian
	\authors C. Stanford
	\authors K. Olchanski

	\section intro Introduction

	\section using For Users
	\section test Test

	\section developers For Developers


 */
