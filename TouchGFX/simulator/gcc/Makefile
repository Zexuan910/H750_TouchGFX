# Helper macros to convert spaces into question marks and back again
e := 
sp := $(e) $(e)
qs = $(subst ?,$(sp),$1)
sq = $(subst $(sp),?,$1)

# Get name of this Makefile (avoid getting word 0 and a starting space)
makefile_name := $(wordlist 1,1000,$(MAKEFILE_LIST))

# Get path of this Makefile
makefile_path := $(call qs,$(dir $(call sq,$(abspath $(call sq,$(makefile_name))))))

# Get path where the Application is
application_path := $(call qs,$(abspath $(call sq,$(makefile_path)../..)))

msys_shell := D:/TouchGFX/4.26.1/env/MinGW/msys/1.0/bin/sh.exe
mingw_bin := D:/TouchGFX/4.26.1/env/MinGW/bin
msys_bin := D:/TouchGFX/4.26.1/env/MinGW/msys/1.0/bin
ruby_bin := D:/TouchGFX/4.26.1/env/MinGW/msys/1.0/Ruby30-x64/bin
simulator_path := $(mingw_bin);$(ruby_bin);$(msys_bin);$(PATH)

.PHONY: clean assets all

ifneq ($(words $(makefile_path))$(words $(MAKEFILE_LIST)),11)
all clean assets:
$(error Spaces not allowed in path)
else

ADDITIONAL_SOURCES := 
ADDITIONAL_INCLUDE_PATHS := 
ADDITIONAL_LIBRARY_PATHS := 
ADDITIONAL_LIBRARIES :=
export ADDITIONAL_SOURCES ADDITIONAL_INCLUDE_PATHS ADDITIONAL_LIBRARY_PATHS ADDITIONAL_LIBRARIES

all: $(filter assets,$(MAKECMDGOALS))
all assets: $(filter clean,$(MAKECMDGOALS))
all clean assets:
	@$(MAKE) -r -f generated/simulator/gcc/Makefile -s $(MFLAGS) SHELL="$(msys_shell)" PATH="$(simulator_path)" c_compiler="$(mingw_bin)/g++.exe" cpp_compiler="$(mingw_bin)/g++.exe" linker="$(mingw_bin)/g++.exe" $@ -C "$(application_path)"
endif

