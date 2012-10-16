#ifndef ROOTANA_DIRECTORY_HXX
#define ROOTANA_DIRECTORY_HXX
#include <stdint.h>
#include <map>
#include <list>
#include <string>
#include <memory>
#include <algorithm>
#include <TDirectory.h>

namespace rootana {

class HistBase;

/// Abstract rootana directory class.
/*!
 * Implementations serve the function of a standard ROOT TDirectory by wrapping
 * an instance of TDirectory* and directing it's actions. In addition, this class
 * also manages rootana histograms; in particular, it creates all desired histograms
 * from the user's definition file and provides public methods to call rootana::HistBase
 * functions for all histograms (or all histograms of a given "event" type). This class
 * also internally handles the work of "net" exporting the internal directories, allowing
 * histograms to be viewed in roody.
 */
class Directory {
public:
	/// Container sorting lists of HistBase pointers, each keyed by the event ID.
	typedef std::map<uint16_t, std::list<rootana::HistBase*> > Map_t;

private:
	/// Container for all rootana histograms.
	/*! See Map_t typedef */
	Map_t fHistos;

	/// Internal ROOT directory
	/*!
	 * \note Derived classes may set this to a specific type using the Reset() method.
	 * \warning The default ROOT behavior for histograms owned by a TDirectory is for
	 * the TDirectory to take care of their destruction, i.e. 
	 */
	std::auto_ptr<TDirectory> fDir;

public:
	/// Initializes fDir
	Directory(TDirectory* dir = 0):	fDir(dir) { }

	/// Removes
	~Directory();
	bool IsOpen() const	{ return fDir.get() && !fDir->IsZombie(); }
	void Reset (TDirectory* newDir) { fDir.reset(newDir); }
	const char* GetName() const
		{
			if (IsOpen()) return fDir->GetName();
			return "< OfflineDirectory:: Unopened file >";
		}
	const char* ClassName() const
		{
			if (IsOpen()) return fDir->ClassName();
			return "TDirectory";
		}
	void AddHist(rootana::HistBase* hist, const char* path, uint16_t eventId);
	virtual bool Open(Int_t runnum, const char* defFile) = 0;
	virtual void Close() = 0;

protected:
	void CreateHists(const char* definitionFile);
	void DeleteHists();
	void NetDirExport(const char* name);
	void StartNetDirServer(Int_t tcp);
	Int_t Write (const char* name = 0, Int_t i = 0, Int_t j = 0)
		{ return fDir->Write(name, i, j); }
	Int_t Write (const char* name = 0, Int_t i = 0, Int_t j = 0) const
		{ return fDir->Write(name, i, j); }

private:
	TDirectory* CreateSubDirectory(const char* path);
	Directory(const Directory& other) { }
	Directory& operator= (const Directory& other) { return *this; }

public:
	template <class R> void CallForAll( R (HistBase::*f)(), int32_t id = -1 )
		{
			Map_t::iterator itFirst, itLast;
			if (id < 0) {
				itFirst = fHistos.begin();
				itLast  = fHistos.end();
			}
			else {
				itFirst = itLast = fHistos.find(static_cast<uint16_t>(id));
			}

			for (Map_t::iterator it = itFirst; it != itLast; ++it) {
				std::for_each(it->second.begin(), it->second.end(), std::mem_fun(f));
			}
		}
};

class OfflineDirectory: public Directory {
private:
	const std::string fOutputPath;
public:
	OfflineDirectory(const char* outPath);
	~OfflineDirectory();
	bool Open(Int_t runnum, const char* defFile);
	void Close();
};

class OnlineDirectory: public Directory {
public:
	OnlineDirectory();
	~OnlineDirectory();
	bool Open(Int_t tcp, const char* defFile);
	void Close();
};

} // namespace rootana



#endif
