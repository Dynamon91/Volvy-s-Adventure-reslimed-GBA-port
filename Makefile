.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment, e.g. export DEVKITARM=/opt/devkitpro/devkitARM")
endif

include $(DEVKITARM)/gba_rules

TARGET := volvy_reslimed
BUILD := build
SOURCES := source source/core source/platform assets/converted assets/converted/tilesets assets/converted/rooms
INCLUDES := include include/core include/platform assets/converted assets/converted/tilesets assets/converted/rooms
DATA := assets/converted/ui

ARCH := -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb -mthumb-interwork
CFLAGS := -g -Wall -Wextra -O2 -std=gnu11 $(ARCH) -ffunction-sections -fdata-sections
CFLAGS += $(INCLUDE)
LDFLAGS = -g $(ARCH) -Wl,-Map,$(OUTPUT).map -Wl,--gc-sections

LIBS := -lgba
LIBDIRS := $(LIBGBA)

ifeq ($(strip $(GAME_TITLE)),)
GAME_TITLE := VOLVYRESLIME
endif
GAME_CODE := VLVY
MAKER_CODE := 00

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
export HFILES := $(shell find $(CURDIR)/include -name '*.h') \
                 $(wildcard $(CURDIR)/assets/converted/*.h) \
                 $(wildcard $(CURDIR)/assets/converted/tilesets/*.h) \
                 $(wildcard $(CURDIR)/assets/converted/rooms/*.h) \\
                 $(addsuffix .h,$(subst .,_,$(BINFILES)))

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                 $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                 -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).gba $(OUTPUT).map

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).gba: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES): $(HFILES)

# Link the walkthrough-derived binary asset with devkitPro's standard bin2o rule.
%.bin.o %_bin.h : %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif
