MAKEFLAGS := --silent
CLEAR ?= $(if $(filter false,$(clear)),, )
CC ?= clang
PROD ?=
STRICT ?=
DEBUG_FLAGS_0 ?= -g3 -fsanitize=undefined,address,integer,nullability -fstack-protector
DEBUG_FLAGS_1 ?= -g3 -Wno-unused-parameter -Wno-unused-variable
DEBUG_FLAGS_PROD ?= -g3 -O2 -Wno-unused-parameter -Wno-unused-variable
DEBUG_FLAGS ?= $(if $(DEBUG),$(DEBUG_FLAGS_0),$(if $(PROD),$(DEBUG_FLAGS_PROD),$(DEBUG_FLAGS_1)))
CRASH_FLAGS ?= $(and $(FAST_CRASH),-DFAST_CRASH)
STRICT_FLAGS ?= $(and $(STRICT),-Werror)
COMPILE_FLAGS ?= $(shell printf ' %s' $$(cat compile_flags.txt))
CFLAGS ?= $(and $(PROD),-DPROD) $(STRICT_FLAGS) $(DEBUG_FLAGS) $(CRASH_FLAGS) $(COMPILE_FLAGS)
LOCAL ?= local
SRC ?= comp
GEN ?= generated
ASM_GEN_SRC ?= $(SRC)/asm_gen.c
ASM_GEN_OUT ?= $(SRC)/asm_generated.s
MACH_GEN_SRC ?= mach/mach_exc.defs
MACH_GEN_OUT ?= $(GEN)/mach_exc.c
ALL_SRC ?= $(wildcard *.s *.c *.h **/*.c **/*.h) $(ASM_GEN_SRC)
MAIN_SRC ?= $(SRC)/main.c
MAIN_S ?= astil_s.exe
MAIN_R ?= astil_r.exe
FILE_EXE ?= $(and $(file),$(basename $(file)).exe)
DISASM ?= --disassemble-all --headers --private-headers --reloc --dynamic-reloc --syms --dynamic-syms
INSTALL_DIR ?= /usr/local/bin
WATCH_IGNORE ?= -i=$(GEN) -i=$(ASM_GEN_OUT)
WATCH ?= watchexec $(and $(CLEAR),-c) $(WATCH_IGNORE) -r -d=1ms -n -q
# WATCH ?= watchexec $(and $(CLEAR),-c) $(WATCH_IGNORE) -r -d=1ms -n -q --no-vcs-ignore
WATCH_COMP ?= $(WATCH) -e=c,h,s
WATCH_PROG ?= $(WATCH) -e=f
WATCH_ALL ?= $(WATCH) -e=c,h,s,f
WATCH_IMM ?= $(WATCH) -e=f,exe --no-vcs-ignore

ARTIF ?= $(MAIN_S) $(MAIN_R) *.o *.exe *.dSYM *.plist *.elf *.dbg \
	**/*.o **/*.exe **/*.dSYM **/*.plist **/*.elf **/*.dbg

# Disables some dangerous behaviors. Without this, `$@` sometimes changes from
# the intended target name to something surprising, like `makefile`, resulting
# in weird `cc` build commands that don't work and delete the wrong files.
.SUFFIXES:

# Auto-delete intermediary executables if any.
# Automatically affects `run_file`.
.INTERMEDIATE: $(FILE_EXE)

.PHONY: build
build: $(MAIN_S) $(MAIN_R)

.PHONY: build_w
build_w:
	$(WATCH_COMP) -- $(MAKE) clean build

.PHONY: run
run:
	rlwrap -n sandbox-exec -f sandbox.sb -D MAIN="$(PWD)/$(file)" ./$(file) $(args)

# Stack-CC version.
.PHONY: run_s
run_s:
	$(MAKE) run file=$(MAIN_S)

# Register-CC version.
.PHONY: run_r
run_r:
	$(MAKE) run file=$(MAIN_R)

# Usage example:
#
#   make run_s_w args='forth/lang_s.f forth/test_s.f -'
.PHONY: run_s_w
run_s_w:
	$(WATCH_IMM) -- $(MAKE) run_s

.PHONY: run_r_w
run_r_w:
	$(WATCH_IMM) -- $(MAKE) run_r

$(MAIN_S): $(ALL_SRC) $(ASM_GEN_OUT)
	$(CC) $(CFLAGS) $(MAIN_SRC) -o $@

$(MAIN_R): $(ALL_SRC) $(ASM_GEN_OUT)
	$(CC) $(CFLAGS) -DNATIVE_CALL_CONV $(MAIN_SRC) -o $@

# Usage example:
#
#   make run_file file=some_file.c
.PHONY: run_file
run_file: $(FILE_EXE) $(ALL_SRC)
	./$(FILE_EXE)

.PHONY: run_file_w
run_file_w:
	$(WATCH_COMP) -- $(MAKE) run_file

# For executables from arbitrary C files. This is possible because our C files
# specify all their dependencies with `#include`, without needing the build
# system to describe the dependencies.
#
# Even on Unix, using `.exe` is convenient. It makes this recipe
# possible, allows to use `.INTERMEDIATE` for auto-cleanup, and
# allows `make clean` to delete these executables by wildcard.
%.exe: %.c $(ALL_SRC)
	$(CC) $(CFLAGS) -x c $< -o $@

**/%.exe: %.c $(ALL_SRC)
	$(CC) $(CFLAGS) -x c $< -o $@

$(ASM_GEN_OUT): $(ASM_GEN_SRC) $(ALL_SRC)
	$(CC) $(CFLAGS) -S $< -o - | awk '/^[[:space:]]*\.set[[:space:]]+[A-Z_]+[[:space:]]*,[[:space:]]*[[:digit:]]/' > $(ASM_GEN_OUT)

.PHONY: debug
debug:
	lldb \
		--source-quietly \
		--one-line "settings set show-statusline false" \
		--one-line "settings set target.disable-aslr true" \
		$(and $(DEBUG),--one-line "settings set target.env-vars DEBUG=true") \
		--file $(file) -- $(args)

# .PHONY: debug_run
# debug_run:
# 	lldb --batch \
# 		--source-quietly \
# 		--one-line "settings set show-statusline false" \
# 		--one-line "process handle SIGSEGV --notify true --pass true --stop false" \
# 		--one-line "process handle SIGBUS  --notify true --pass true --stop false" \
# 		--one-line "process handle SIGILL  --notify true --pass true --stop false" \
# 		--one-line "process handle SIGABRT --notify true --pass true --stop false" \
# 		--one-line "settings set target.disable-aslr true" \
# 		$(and $(DEBUG),--one-line "settings set target.env-vars DEBUG=true") \
# 		--one-line "run" \
# 		--one-line "quit" \
# 		--file $(file) -- $(args)

.PHONY: debug_run
debug_run:
	lldb --batch \
		--source-quietly \
		--one-line "settings set show-statusline false" \
		--one-line "settings set target.disable-aslr true" \
		$(and $(DEBUG),--one-line "settings set target.env-vars DEBUG=true") \
		--one-line "run" \
		--one-line "quit" \
		--file $(file) -- $(args)

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
	llvm-objdump $(DISASM) $(or $(file),$(MAIN_R)) > $(LOCAL)/out.s

# .PHONY: install
# install: build
# 	ln -sf $(MAIN_R) "$(INSTALL_DIR)/astil"

.PHONY: clean
clean:
	rm -rf $(GEN) $(wildcard $(ARTIF))

# MIG is even worse than this.
$(MACH_GEN_OUT): $(MACH_GEN_SRC)
	mkdir -p $(GEN)
	xcrun mig -server $(GEN)/tmp.c -user /dev/null -header /dev/null $(MACH_GEN_SRC) \
		&& cat mach/mach_pre.txt $(GEN)/tmp.c mach/mach_suf.txt \
			| sed -e 's/__attribute__((unused))//g' -e 's/__attribute__((__unused__))//g' \
			| clang-format \
			> $(MACH_GEN_OUT) \
		; rm -rf $(GEN)/tmp.c
