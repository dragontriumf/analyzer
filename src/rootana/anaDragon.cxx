//
// ROOT analyzer
//
// K.Olchanski
//
// $Id: analyzer.cxx 64 2008-12-22 03:14:11Z olchansk $
//
#include "midas.h"
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "TMidasOnline.h"
#include "TMidasEvent.h"
#include "TMidasFile.h"
#include "XmlOdb.h"
#include "libNetDirectory/netDirectoryServer.h"

#include <TSystem.h>
#include <TROOT.h>
#include <TApplication.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TFolder.h>
#include <TH1D.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

#include "utils/definitions.h"
#include "utils/Error.hxx"
#include "midas/Database.hxx"
#include "midas/Event.hxx"
#include "dragon/Coinc.hxx"
#include "Timestamp.hxx"
#include "RootanaDragon.hxx"
#include "HistParser.hxx"
#include "Histos.hxx"
#include "Events.hxx"
#include "Timer.hxx"
#include "Globals.h"


// Global Variables
int  gRunNumber = 0;
bool gIsRunning = false;
bool gIsPedestalsRun = false;
bool gIsOffline = false;
bool gDebugEnable = false;
int  gEventCutoff = 0;

TDirectory* gOnlineHistDir = NULL;
TFile* gOutputFile = NULL;
VirtualOdb* gOdb = NULL;

double gQueueTime = 10e6;


static bool gEnableShowMem = false;

int ShowMem(const char* label)
{
  if (!gEnableShowMem)
    return 0;

  FILE* fp = fopen("/proc/self/statm","r");
  if (!fp)
    return 0;

  int mem = 0;
  fscanf(fp,"%d",&mem);
  fclose(fp);

  if (label)
    printf("memory at %s is %d\n", label, mem);

  return mem;
}
 
void help()
{
  printf("\nUsage:\n");
  printf("\n./analyzer.exe [-h] [-Hhostname] [-Eexptname] [-eMaxEvents] [-P9091] [-p9090] [-m] [-g] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-T: test mode - start and serve a test histogram\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
	printf("\t-qQueueTime: Set timestamp matching queue time in microseconds (default: 10e6)\n");
  printf("\t-P: Start the TNetDirectory server on specified tcp port (for use with roody -Plocalhost:9091)\n");
  printf("\t-e: Number of events to read from input data files\n");
  printf("\t-m: Enable memory leak debugging\n");
  printf("\t-g: Enable graphics display when processing data files\n");
  printf("\t-d: Enable user debug statements\n");
  printf("\n");
  printf("Example1: analyze online data: ./analyzer.exe -P9091\n");
  printf("Example2: analyze existing data: ./analyzer.exe /data/alpha/current/run00500.mid\n");
  exit(1);
}

// Main function call

int main(int argc, char *argv[])
{
	setbuf(stdout,NULL);
	setbuf(stderr,NULL);
 
	signal(SIGILL,  SIG_DFL);
	signal(SIGBUS,  SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
 
	std::vector<std::string> args;
	for (int i=0; i<argc; i++)
	{
		if (strcmp(argv[i],"-h")==0)
			help(); // does not return
		args.push_back(argv[i]);
	}

	rootana::App* app = new rootana::App("rootana", &argc, argv);

	bool forceEnableGraphics = false;
	bool testMode = false;
	int  oldTcpPort = 0;
	int  tcpPort = 0;
	const char* hostname = NULL;
	const char* exptname = NULL;

	for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
	{
		const char* arg = args[i].c_str();
		//printf("argv[%d] is %s\n",i,arg);
	   
		if (strncmp(arg,"-e",2)==0)  // Event cutoff flag (only applicable in offline mode)
			gEventCutoff = atoi(arg+2);
		else if (strncmp(arg,"-m",2)==0) // Enable memory debugging
			gEnableShowMem = true;
		else if (strncmp(arg,"-p",2)==0) // Set the histogram server port
			oldTcpPort = atoi(arg+2);
		else if (strncmp(arg,"-P",2)==0) // Set the histogram server port
			tcpPort = atoi(arg+2);
		else if (strcmp(arg,"-T")==0)
			testMode = true;
		else if (strcmp(arg,"-d")==0)
			gDebugEnable = true;
		else if (strcmp(arg,"-g")==0)
			forceEnableGraphics = true;
		else if (strncmp(arg,"-H",2)==0)
			hostname = strdup(arg+2);
		else if (strncmp(arg,"-E",2)==0)
			exptname = strdup(arg+2);
		else if (strcmp(arg,"-h")==0)
			help(); // does not return
		else if (strcmp(arg,"-q")==0)
			gQueueTime = atof(arg+2);
		else if (arg[0] == '-')
			help(); // does not return
	}

	if (!tcpPort) tcpPort = 9091;
    
	gROOT->cd();
	gOnlineHistDir = new TDirectory("rootana", "rootana online plots");

	if (tcpPort)
		StartNetDirectoryServer(tcpPort, gOnlineHistDir);
	 
	gIsOffline = false;

	for (unsigned int i=1; i<args.size(); i++)
	{
		const char* arg = args[i].c_str();

		if (arg[0] != '-')  
		{  
			gIsOffline = true;
			//gEnableGraphics = false;
			//gEnableGraphics |= forceEnableGraphics;

			app->midas_file(arg);
		}
	}

	if (testMode)
	{
		gOnlineHistDir->cd();
		TH1D* hh = new TH1D("test", "test", 100, 0, 100);
		hh->Fill(1);
		hh->Fill(10);
		hh->Fill(50);

		app->Run(kTRUE);
		return 0;
	}

	// if we processed some data files,
	// do not go into online mode.
	if (gIsOffline)
		return 0;
	   
	gIsOffline = false;
	//gEnableGraphics = true;

#ifdef MIDASSYS
	app->midas_online(hostname, exptname);
#endif
   
	return 0;
}

//end
