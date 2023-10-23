# Project Name
TARGET = MyProj

# Sources
CPP_SOURCES = MyProj.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/
LIBS += -lleaf
LIBDIR += -L ../../../LEAF/leaf/build
C_INCLUDES += -I../../../LEAF/leaf/Src -I../../../LEAF/leaf/Inc/  -I../../../LEAF/leaf/ 
# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
