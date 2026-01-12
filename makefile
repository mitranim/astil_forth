MAKEFLAGS := --silent
CLEAR ?= $(if $(filter false,$(clear)),, )
CC ?= clang
PROD ?=
STRICT ?=
DEBUG_FLAGS_0 ?= -O0 -g3 -fsanitize=undefined,address,integer,nullability -fstack-protector
DEBUG_FLAGS_1 ?= -g3 -Wno-unused-parameter -Wno-unused-variable
DEBUG_FLAGS ?= $(if $(DEBUG),$(DEBUG_FLAGS_0),$(DEBUG_FLAGS_1))
CRASH_FLAGS ?= $(and $(FAST_CRASH),-DFAST_CRASH)
STRICT_FLAGS ?= $(and $(STRICT),-Werror)
COMPILE_FLAGS ?= $(shell printf ' %s' $$(cat compile_flags.txt))
CFLAGS ?= $(and $(PROD),-DPROD) $(STRICT_FLAGS) $(DEBUG_FLAGS) $(CRASH_FLAGS) $(COMPILE_FLAGS)
LOCAL ?= local
GEN ?= generated
ASM_GEN_SRC ?= c_asm_gen.c
ASM_GEN_OUT ?= $(GEN)/c_asm.s
MACH_GEN_SRC ?= mach/mach_exc.defs
MACH_GEN_OUT ?= $(GEN)/mach_exc.c
ALL_SRC ?= $(wildcard *.s *.c *.h **/*.c **/*.h) $(ASM_GEN_SRC)
MAIN_SRC ?= c_main.c
MAIN_EXE ?= forth.exe
MAIN_EXE_ABS ?= $(PWD)/$(MAIN_EXE)
FILE_EXE ?= $(and $(file),$(basename $(file)).exe)
DISASM ?= --disassemble-all --headers --private-headers --reloc --dynamic-reloc --syms --dynamic-syms
WATCH ?= watchexec $(and $(CLEAR),-c) -i=$(GEN) -r -d=1ms -n -q
# WATCH ?= watchexec $(and $(CLEAR),-c) -i=$(GEN) -r -d=1ms -n -q --no-vcs-ignore
WATCH_COMP ?= $(WATCH) -e=c,h,s
WATCH_PROG ?= $(WATCH) -e=f
WATCH_ALL ?= $(WATCH) -e=c,h,s,f
WATCH_IMM ?= $(WATCH) -e=f,exe --no-vcs-ignore

ARTIF ?= *.o *.exe *.dSYM *.plist *.elf *.dbg \
	**/*.o **/*.exe **/*.dSYM **/*.plist **/*.elf **/*.dbg

# Disables some dangerous behaviors. Without this, `$@` sometimes changes from
# the intended target name to something surprising, like `makefile`, resulting
# in weird `cc` build commands that don't work and delete the wrong files.
.SUFFIXES:

# Auto-delete intermediary executable if any.
.INTERMEDIATE: $(FILE_EXE)

.PHONY: build
build: $(MAIN_EXE)

.PHONY: build_w
build_w:
	$(WATCH_COMP) -- $(MAKE) clean build

.PHONY: run
# run: $(MAIN_EXE)
run:
	rlwrap -n sandbox-exec -f sandbox.sb -D MAIN="$(MAIN_EXE_ABS)" ./$(MAIN_EXE) $(args)

# Usage example:
#
#   make run_w args='f_init.f f_test.f -'
.PHONY: run_w
run_w:
	$(WATCH_IMM) -- $(MAKE) run

$(MAIN_EXE): $(ALL_SRC) $(ASM_GEN_OUT)
	$(CC) $(CFLAGS) $(MAIN_SRC) -o $@

# Usage: `make run_file file=some_file.c`
.PHONY: run_file
run_file: $(FILE_EXE) $(ALL_SRC)
	./$(FILE_EXE)

.PHONY: run_file_w
run_file_w:
	$(WATCH_COMP) -- $(MAKE) run_file

# For executables from arbitrary C files. This is possible because our C files
# specify all their dependencies with `#include`, without needing the build
# system to describe the dependencies.
%.exe: %.c $(ALL_SRC)
	$(CC) $(CFLAGS) -x c $< -o $@

**/%.exe: %.c $(ALL_SRC)
	$(CC) $(CFLAGS) -x c $< -o $@

$(ASM_GEN_OUT): $(ASM_GEN_SRC) $(ALL_SRC)
	mkdir -p $(GEN)
	$(CC) $(CFLAGS) -S $< -o - | awk '/^[[:space:]]*\.set[[:space:]]+[A-Z_]+[[:space:]]*,[[:space:]]*[[:digit:]]/' > $(ASM_GEN_OUT)

.PHONY: debug
debug:
	lldb \
		--source-quietly \
		--one-line "settings set show-statusline false" \
		--one-line "settings set target.disable-aslr true" \
		$(and $(DEBUG),--one-line "settings set target.env-vars DEBUG=true") \
		--file $(MAIN_EXE) -- $(args)

.PHONY: debug_run
debug_run:
	lldb --batch \
		--source-quietly \
		--one-line "settings set show-statusline false" \
		--one-line "process handle SIGSEGV --notify true --pass true --stop false" \
		--one-line "process handle SIGBUS  --notify true --pass true --stop false" \
		--one-line "process handle SIGILL  --notify true --pass true --stop false" \
		--one-line "process handle SIGABRT --notify true --pass true --stop false" \
		--one-line "settings set target.disable-aslr true" \
		$(and $(DEBUG),--one-line "settings set target.env-vars DEBUG=true") \
		--one-line "run" \
		--one-line "quit" \
		--file $(MAIN_EXE) -- $(args)

.PHONY: debug_run_w
debug_run_w:
	$(WATCH_IMM) -- $(MAKE) debug_run

.PHONY: prepro
prepro:
	mkdir -p $(LOCAL)
	$(CC) -E $(CFLAGS) $(or $(file),$(MAIN_SRC)) -o $(LOCAL)/out.c

.PHONY: asm
asm:
	mkdir -p $(LOCAL)
	$(CC) -S $(CFLAGS) $(or $(file),$(MAIN_SRC)) -o $(LOCAL)/out.s

.PHONY: disasm
disasm:
	mkdir -p $(LOCAL)
	llvm-objdump $(DISASM) $(or $(file),$(MAIN_EXE)) > $(LOCAL)/out.s

.PHONY: clean
clean:
	rm -rf $(GEN) $(wildcard $(ARTIF))

$(MACH_GEN_OUT): $(MACH_GEN_SRC)
	mkdir -p $(GEN)
	xcrun mig -server $(GEN)/tmp.c -user /dev/null -header /dev/null $(MACH_GEN_SRC) \
		&& cat mach/mach_pre.txt $(GEN)/tmp.c mach/mach_suf.txt \
			| sed -e 's/__attribute__((unused))//g' -e 's/__attribute__((__unused__))//g' \
			| clang-format \
			> $(MACH_GEN_OUT) \
		; rm -rf $(GEN)/tmp.c
