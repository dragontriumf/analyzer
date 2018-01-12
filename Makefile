### Include local makefile with user customization. This is supposed to be generated by the ./configure script
include config.mk

ifdef USE_ROOT
else
$(error No config.mk file found. Please run the configure script first. Running './configure --help' will give instructions on how to do this)
endif

SHLIBFILE    = $(DRLIB)/libDragon.so
ROOTMAPFILE := $(patsubst %.so,%.rootmap,$(SHLIBFILE))

ifeq ($(USE_ROOT),YES)
$(info --------------- USE_ROOT --------------)
$(info ------------ ROOT Version: ------------)
$(info $(ROOTVERSION))
$(info ---------------------------------------)
ifeq ($(ROOTMAJORVERSION),6)
MAKE_DRAGON_DICT += rootcling -v -f $@ -s $(SHLIBFILE) -rml $(SHLIBFILE) -rmf $(ROOTMAPFILE) -c $(CINTFLAGS) \
-p $(HEADERS) TError.h TNamed.h TObject.h TString.h TTree.h $(CINT)/Linkdef.h
else
MAKE_DRAGON_DICT += rootcint -f $@ -c $(CINTFLAGS) -p $(HEADERS) TTree.h $(CINT)/Linkdef5.h
endif
DRA_DICT          = $(DRLIB)/DragonDictionary.cxx
DRA_DICT_DEP      = $(DRLIB)/DragonDictionary.cxx
ROOTLIBS    += -lXMLParser -lTreePlayer -lSpectrum -lMinuit
ROOTGLIBS   += -lXMLParser -lTreePlayer -lSpectrum -lMinuit
else
USE_ROOTBEER = NO
endif

ifeq ($(USE_MIDAS),YES)
$(info ------------ USE_MIDAS ----------------)
endif

# DEBUG       += -ggdb -O3 -DDEBUG
# CXXFLAGS     = -g -O2 -Wall -Wuninitialized
# CXXFLAGS    += -Wall $(DEBUG) $(INCLUDE)
CXXFLAGS    += $(DEFINITIONS) -DHAVE_ZLIB
CCFLAGS     +=

ifeq ($(USE_ROOTBEER),YES)
$(info ------------ USE_ROOTBEER -------------)
endif

ifeq ($(USE_ROOTANA),YES)
$(info ------------ USE_ROOTANA -------------)
endif

CC        += $(filter-out -std=c++11, $(CXXFLAGS))
CXXFLAGS  += $(INCLUDE)
CINTFLAGS := $(filter-out ($(ROOTCFLAGS)), $(CXXFLAGS))

ifeq ($(NAME),Ubuntu5)
CXX += $(filter-out -std=c++11, $(CXXFLAGS))
else
CXX += $(CXXFLAGS)
endif

LD   = $(CXX) $(LDFLAGS) $(ROOTGLIBS) $(RPATH) -L$(PWD)/lib

HEADERS =								\
$(SRC)/midas/*.hxx						\
$(SRC)/midas/*.h						\
$(SRC)/midas/libMidasInterface/*.h		\
$(SRC)/utils/*.hxx						\
$(SRC)/utils/*.h						\
$(SRC)/*.hxx

#### OBJECTS ####
OBJECTS =										\
$(OBJ)/midas/mxml.o								\
$(OBJ)/midas/Odb.o								\
$(OBJ)/midas/Xml.o								\
$(OBJ)/midas/libMidasInterface/TMidasFile.o		\
$(OBJ)/midas/libMidasInterface/TMidasEvent.o	\
$(OBJ)/midas/Event.o							\
$(OBJ)/Unpack.o									\
$(OBJ)/TStamp.o									\
$(OBJ)/Vme.o									\
$(OBJ)/Dragon.o									\
$(OBJ)/Sonik.o									\
$(OBJ)/utils/Uncertainty.o						\
$(OBJ)/utils/ErrorDragon.o

ifeq ($(USE_MIDAS), YES)
OBJECTS += $(OBJ)/midas/libMidasInterface/TMidasOnline.o
endif

ifeq ($(USE_ROOT), YES)
OBJECTS += $(OBJ)/utils/RootAnalysis.o
OBJECTS += $(OBJ)/utils/Selectors.o
OBJECTS += $(OBJ)/utils/Calibration.o
OBJECTS += $(OBJ)/utils/LinearFitter.o
OBJECTS += $(OBJ)/utils/TAtomicMass.o
endif
## END OBJECTS ##

### MID2ROOT LIBRARY ###
MID2ROOT_LIBS = -lDragon $(MIDASLIBS)

MAKE_ALL      = $(SHLIBFILE) $(PWD)/bin/mid2root

all:  $(MAKE_ALL)

libDragon: $(SHLIBFILE)

$(SHLIBFILE): $(DRA_DICT_DEP) $(OBJECTS)
	$(LD) $(DYLIB) $(MIDASLIBS) $(OBJECTS) $(DRA_DICT) \
	-o $@ \

mid2root: $(PWD)/bin/mid2root

$(PWD)/bin/mid2root: src/mid2root.cxx $(SHLIBFILE)
	$(LD) $(MID2ROOT_INC) $(MID2ROOT_LIBS) $< \
	-o $@ \

rbdragon.o: $(OBJ)/rootbeer/rbdragon.o

rbdragon_impl.o: $(OBJ)/rootbeer/rbdragon_impl.o

### OBJECT FILES ###

$(OBJ)/utils/%.o: $(SRC)/utils/%.cxx $(DRA_DICT_DEP)
	$(CXX) -c -o $@ $< \

$(OBJ)/midas/%.o: $(SRC)/midas/%.c $(DRA_DICT_DEP)
	$(CC) -c -o $@ $< \

$(OBJ)/midas/%.o: $(SRC)/midas/%.cxx $(DRA_DICT_DEP)
	$(CXX) -c -o $@ $< \

$(OBJ)/midas/libMidasInterface/%.o: $(SRC)/midas/libMidasInterface/%.cxx $(DRA_DICT_DEP)
	$(CXX) -c -o $@ $< \

$(OBJ)/rootana/%.o: $(SRC)/rootana/%.cxx $(CINT)/rootana/Dict.cxx
	$(CXX) $(ROOTANA_FLAGS) $(ROOTANA_DEFS) -c -o $@ $< \

## Must be last object rule!!
$(OBJ)/%.o: $(SRC)/%.cxx $(DRA_DICT_DEP)
	$(CXX) -c -o $@ $< \

### CINT DICTIONARY ###
dict: $(DRA_DICT)

$(DRA_DICT):  $(HEADERS) $(CINT)/Linkdef.h
	$(MAKE_DRAGON_DICT) \

### FOR ROOTANA ###

$(CINT)/rootana/Dict.cxx: $(ROOTANA_HEADERS) $(DRA_DICT) $(SRC)/rootana/Linkdef.h \
	rootcint -f $@ -c $(CXXFLAGS) $(ROOTANA_FLAGS) \
	-p $(ROOTANA_HEADERS) $(SRC)/rootana/Linkdef.h \

$(CINT)/rootana/CutDict.cxx: $(SRC)/rootana/Cut.hxx $(SRC)/rootana/CutLinkdef.h
	rootcint -f $@ -c $(CXXFLAGS) $(ROOTANA_FLAGS) \
	-p $(SRC)/rootana/Cut.hxx $(SRC)/rootana/CutLinkdef.h \

$(DRLIB)/libRootanaCut.so: $(CINT)/rootana/CutDict.cxx
	$(LD)  $(DYLIB)  $(ROOTANA_FLAGS) $(ROOTANA_DEFS)  \
	-o $@ $< \

libRootanaDragon.so: $(DRLIB)/libDragon.so $(CINT)/rootana/Dict.cxx \
	$(DRLIB)/libRootanaCut.so $(ROOTANA_OBJS)
	$(LD)  $(DYLIB)  $(ROOTANA_FLAGS) $(ROOTANA_DEFS)  \
	-o $@ $<
	$(CINT)/rootana/Dict.cxx $(ROOTANA_OBJS) -lDragon -lRootanaCut -L$(DRLIB) \

$(PWD)/bin/anaDragon: $(SRC)/rootana/anaDragon.cxx $(DRLIB)/libDragon.so \
	$(CINT)/rootana/Dict.cxx $(DRLIB)/libRootanaCut.so \
	$(ROOTANA_OBJS) $(ROOTANA_REMOTE_OBJS) \
	$(LD) $(ROOTANA_FLAGS) $(ROOTANA_DEFS) \
	-o $@
	$< $(CINT)/rootana/Dict.cxx $(ROOTANA_OBJS)
	-lDragon -lRootanaCut -L$(DRLIB) $(ROOTANA_LIBS) $(MIDASLIBS) \

rootana_clean:
	rm -f $(ROOTANA_OBJS) anaDragon libRootanaDragon.so \
	$(CINT)/rootana/* $(DRLIB)/libRootanaCut.so

Dragon: $(OBJ)/Dragon.o

$(CINT)/rootbeer/rootbeerDict.cxx: $(SRC)/rootbeer/rbsymbols.hxx $(DRA_DICT_DEP) $(RB_HOME)/cint/RBDictionary.cxx $(RB_HOME)/cint/MidasDict.cxx
	rootcint -f $@ -c $(CXXFLAGS) $(RBINC) -p $< $(CINT)/rootbeer/rblinkdef.h \

$(OBJ)/rootbeer/rbdragon.o: $(SRC)/rootbeer/rbdragon.cxx $(SRC)/rootbeer/*.hxx $(DRA_DICT_DEP) $(CINT)/rootbeer/rootbeerDict.cxx
	$(CXX) $(RB_DEFS) $(RBINC) -c \
	-o $@ $< \

$(OBJ)/rootbeer/rbsonik.o: $(SRC)/rootbeer/rbsonik.cxx $(SRC)/rootbeer/*.hxx $(DRA_DICT_DEP) $(CINT)/rootbeer/rootbeerDict.cxx
	$(CXX) $(RB_DEFS) $(RBINC) -c \
	-o $@ $< \

$(OBJ)/rootbeer/rbdragon_impl.o: $(SRC)/rootbeer/rbdragon_impl.cxx $(SRC)/rootbeer/*.hxx $(DRA_DICT_DEP) $(CINT)/rootbeer/rootbeerDict.cxx
	$(CXX) $(RB_DEFS) $(RBINC) -c \
	-o $@ $< \

$(OBJ)/rootbeer/rbsonik_impl.o: $(SRC)/rootbeer/rbsonik_impl.cxx $(SRC)/rootbeer/*.hxx $(DRA_DICT_DEP) $(CINT)/rootbeer/rootbeerDict.cxx
	$(CXX) $(RB_DEFS) $(RBINC) -c \
	-o $@ $< \

$(PWD)/bin/rbdragon: $(CINT)/rootbeer/rootbeerDict.cxx $(RB_DRAGON_OBJECTS)
	$(LD) $^ $(RBINC) -L$(PWD)/../../rootbeer/lib  -lDragon -lRootbeer -lrbMidas \
	-o $@ \

$(PWD)/bin/rbsonik: $(CINT)/rootbeer/rootbeerDict.cxx $(RB_SONIK_OBJECTS)
	$(LD) $^ $(RBINC) -L$(PWD)/../../rootbeer/lib  -lDragon -lRootbeer -lrbMidas \
	-o $@ \

Timestamp: $(OBJ)/rootbeer/Timestamp.o

MidasBuffer: $(OBJ)/rootbeer/MidasBuffer.o

DragonEvents: $(OBJ)/rootbeer/DragonEvents.o

DragonRootbeer: $(OBJ)/rootbeer/DragonRootbeer.o


#### REMOVE EVERYTHING GENERATED BY MAKE ####
clean:
	rm -rf $(DRA_DICT) $(PWD)/*.root $(DRLIB)/* $(OBJECTS) $(RB_DRAGON_OBJECTS) $(RB_SONIK_OBJECTS) \
	obj/rootana/*.o obj/*/*.o $(CINT)/*.cxx $(CINT)/rootana/* bin/*

#### FOR DOXYGEN ####

doc::
	cd doc ; doxygen Doxyfile ; cd ..


#### FOR UNIT TESTING ####

mxml.o:           $(OBJ)/midas/libMidasInterface/mxml.o
strlcpy.o:        $(OBJ)/midas/libMidasInterface/strlcpy.o
Odb.o:            $(OBJ)/midas/Odb.o
Xml.o:            $(OBJ)/midas/Xml.o
TMidasFile.o:     $(OBJ)/midas/libMidasInterface/TMidasFile.o
TMidasEvent.o:    $(OBJ)/midas/libMidasInterface/TMidasEvent.o
Event.o:          $(OBJ)/midas/Event.o

TStamp.o:         $(OBJ)/tstamp/TStamp.o

V792.o:           $(OBJ)/vme/V792.o
V1190.o:          $(OBJ)/vme/V1190.o
Io32.o:           $(OBJ)/vme/Io32.o

Bgo.o:            $(OBJ)/dragon/Bgo.o
Mcp.o:            $(OBJ)/dragon/Mcp.o
Dsssd.o:          $(OBJ)/dragon/Dsssd.o
Auxillary.o:      $(OBJ)/dragon/Auxillary.o
IonChamber.o:     $(OBJ)/dragon/IonChamber.o
SurfaceBarrier.o: $(OBJ)/dragon/SurfaceBarrier.o

Head.o:           $(OBJ)/dragon/Head.o
Tail.o:           $(OBJ)/dragon/Tail.o
Scaler.o:         $(OBJ)/dragon/Scaler.o

test/%: test/%.cxx $(DRLIB)/libDragon.so
	$(LD) \
	$< -o $@ \
	-DMIDASSYS -lDragon -L$(DRLIB) $(MIDASLIBS) -DODB_TEST -I$(PWD)/src


odbtest: $(DRLIB)/libDragon.so
	$(LD) src/midas/Odb.cxx \
	-o test/odbtest -DMIDASSYS \
	-lDragon -L$(DRLIB) $(MIDASLIBS) \
	-DODB_TEST -I$(PWD)/src

filltest: test/filltest.cxx $(DRLIB)/libDragon.so
	$(LD) test/filltest.cxx \
	-o bin/filltest \
	-DMIDAS_BUFFERS \
	-lDragon -L$(DRLIB) -I$(PWD)/src \
