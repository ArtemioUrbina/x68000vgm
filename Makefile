# Names of the compiler and friends
APP_BASE 	= /opt/toolchains/x68k/bin/human68k
AS 		= $(APP_BASE)-as
CC 		= $(APP_BASE)-gcc
LD 		= $(APP_BASE)-ld
OBJCOPY		= $(APP_BASE)-objcopy
STRIP 		= $(APP_BASE)-strip

# Where to copy the output exe files to
DEST		= "release"

$(shell mkdir -p build bin $(DEST))

# libraries and paths
LIBS	 	= -ldos -lm
GFXLIBS		= $(LIBS)
TEXTLIBS	= $(LIBS) 
INCLUDES 	= -I.

# Compiler flags
ASM_FLAGS 	= -m68000 -mtune=68000 --register-prefx-optional
LDFLAGS 	=
CFLAGS 		= -m68000 -mtune=68000 -std=c99 -fomit-frame-pointer -Wall -Wno-unused-function -Wno-unused-variable -O3
LDSCRIPT 	=
OCFLAGS		= -O xfile

# What our application is named
TARGET		= vgm
EXE		= $(TARGET).X

# Detect tinf subfolder
TINF_DIR	= tinf
TINF_EXISTS	:= $(wildcard $(TINF_DIR)/tinf.h)

ifneq ($(TINF_EXISTS),)
    CFLAGS   += -DVGZ_SUPPORT -I$(TINF_DIR)
    OBJFILES += build/tinflate.o build/tinfgzip.o \
                build/crc32.o build/adler32.o build/tinfzlib.o
endif

# Core objects always compiled
OBJFILES = build/vgm.o build/hardware.o build/gd3.o build/vgz.o

ifneq ($(TINF_EXISTS),)
    OBJFILES += build/tinflate.o \
                build/tinfgzip.o \
                build/crc32.o    \
                build/adler32.o  \
                build/tinfzlib.o
    $(info tinf found -- enabling VGZ support)
else
    $(info tinf not found -- disabling VGZ support)
endif

all: $(EXE)

# The main application
$(EXE): $(OBJFILES)
	@echo ""
	@echo "========================================"
	@echo " -= $(EXE) =-"
	@echo ""
	@echo Linking ....
	$(CC) $(LDFLAGS) $(OBJFILES) $(LIBS) -o bin/$(TARGET)
	@echo ""
	@echo Dumping executable object ....
	$(OBJCOPY) $(OCFLAGS) bin/$(TARGET) bin/$(EXE)
	@echo ""
	@echo Moving application binary to X68000 shared folder ....
	mv bin/$(EXE) $(DEST)
	
################################
#
# Assembly stuff
#
################################

################################
#
# Main code
#
################################

build/vgm.o: src/vgm.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/vgm.o
build/hardware.o: src/hardware.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/hardware.o
build/gd3.o: src/gd3.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/gd3.o
build/vgz.o: src/vgz.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/vgz.o
build/tinflate.o: $(TINF_DIR)/tinflate.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/tinflate.o
build/tinfgzip.o: $(TINF_DIR)/tinfgzip.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/tinfgzip.o
build/crc32.o: $(TINF_DIR)/crc32.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/crc32.o
build/adler32.o: $(TINF_DIR)/adler32.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/adler32.o
build/tinfzlib.o: $(TINF_DIR)/tinfzlib.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o build/tinfzlib.o

###############################
#
# Clean up
#
###############################
clean:
	rm -f build/*.o bin/$(EXE) bin/$(TARGET) release/*.x
