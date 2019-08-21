RM = rm -f
CCFLAGS = -Wall -Wextra -Werror -g

LIBDIR = lib/

GLEWDIR = $(LIBDIR)glew-2.1.0/
LIBGLEW = $(GLEWDIR)lib/libGLEW.a

GLFWDIR = $(LIBDIR)glfw-3.3/
GLFWBUILDDIR = $(GLFWDIR)build/
LIBGLFW = $(GLFWBUILDDIR)src/libglfw3.a

LFTDIR = $(LIBDIR)libft/
LFT = $(LFTDIR)libft.a
INCDIR = inc
CWLINKERS = -L $(LFTDIR) -lft
CWINCLUDES = -I $(LFTDIR)includes/ -I $(INCDIR)

ASM_SRCDIR = cmd/asm/
ASM_NAME = asm
ASM_CFILES = main.c
ASM_SRCS = $(addprefix $(ASM_SRCDIR), $(ASM_CFILES))
ASM_OBJS = $(ASM_SRCS:.c=.o)

VM_SRCDIR = cmd/corewar/
VM_NAME = corewar
VM_CFILES = corewar.c cpu.c instructions.c colors.c
VM_SRCS = $(addprefix $(VM_SRCDIR), $(VM_CFILES))
VM_OBJS = $(VM_SRCS:.c=.o)

CHAMP_SRCDIR = champ/
CHAMP_NAME = champ.cor
CHAMP_CFILES = champ.s
CHAMP_SRCS = $(addprefix $(CHAMP_SRCDIR), $(CHAMP_CFILES))

GUI_LDFLAGS = $(LIBGLEW) -L $(dir $(LIBGLFW)) -lglfw3
GUI_FRAMEWORKS = -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -Wl,-w
GUI_INCLUDE = -I lib/glfw-3.3/build/include -I lib/glew-2.1.0/include

INTERNAL_SRCDIR = internal/
INTERNAL_CFILES = \
	op.c hashtbl.c util.c lex.c parse.c resolve_labels.c print_args.c \
	globals.c asm_main_funcs.c parse_binary.c
INTERNAL_SRCS = $(addprefix $(INTERNAL_SRCDIR), $(INTERNAL_CFILES))
INTERNAL_OBJS = $(INTERNAL_SRCS:.c=.o)
INTL_HEADERS = colors.h libasm.h op.h hashtbl.h util.h asm.h instructions.h

COREWAR_HEADERS = $(addprefix ./inc/, libasm.h op.h hashtbl.h util.h asm.h )
VM_HEADERS = $(addprefix $(VM_SRCDIR), colors.h instructions.h cpu.h)

CFLAGS = $(CCFLAGS) $(INCLUDES)

all: $(CHAMP_NAME) $(ASM_NAME) $(VM_NAME)

$(INTERNAL_OBJS): CFLAGS = $(CCFLAGS) $(CWINCLUDES)

$(ASM_NAME): LINKERS = $(CWLINKERS)
$(ASM_NAME): INCLUDES = $(CWINCLUDES)
$(ASM_NAME): CFLAGS = $(CCFLAGS) $(INCLUDES) $(LINKERS)
$(ASM_NAME): $(LFT) $(INTERNAL_OBJS)
$(ASM_NAME): $(ASM_OBJS) $(INTERNAL_OBJS)
$(ASM_NAME): $(LFT) $(ASM_OBJS) $(INTERNAL_OBJS)
	$(CC) $(CFLAGS) $(INTERNAL_OBJS) $(ASM_OBJS) -o $(ASM_NAME)

$(ASM_OBJS): CFLAGS = $(CCFLAGS) $(CWINCLUDES)

$(VM_NAME): LINKERS = $(CWLINKERS) $(GUI_LDFLAGS) $(GUI_FRAMEWORKS)
$(VM_NAME): INCLUDES = $(CWINCLUDES) $(GUI_INCLUDE)
$(VM_NAME): CFLAGS = $(CCFLAGS) $(INCLUDES) $(LINKERS)
$(VM_NAME): $(LFT) $(INTERNAL_OBJS)
$(VM_NAME): $(LFT) $(ASM_OBJS)
$(VM_NAME): $(LIBGLFW) $(LIBGLEW)
$(VM_NAME): $(VM_OBJS) $(ASM_OBJS) $(INTERNAL_OBJS)
	$(CC) $(CFLAGS) $(INTERNAL_OBJS) $(VM_OBJS) -o $(VM_NAME)

$(VM_OBJS): CFLAGS = $(CCFLAGS) $(CWINCLUDES) $(GUI_INCLUDE)

$(CHAMP_NAME): $(addprefix $(CHAMP_SRCDIR), $(CHAMP_NAME))
	cp $(addprefix $(CHAMP_SRCDIR), $(CHAMP_NAME)) ./

$(addprefix $(CHAMP_SRCDIR), %.cor): $(addprefix $(CHAMP_SRCDIR), %.s) $(ASM_NAME)
	./$(ASM_NAME) $<

.PHONY: deps
deps: $(LFT) $(LIBGLEW) $(LIBGLFW)

$(LIBGLFW):
	mkdir -p $(GLFWBUILDDIR)
	cd $(GLFWBUILDDIR) && \
	cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=MinSizeRel \
	-DCMAKE_EXECUTABLE_FORMAT=MACHO -DGLFW_BUILD_DOCS=OFF \
	-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF \
	-DGLFW_INSTALL=OFF -DGLFW_USE_OSMESA=OFF -DGLFW_VULKAN_STATIC=OFF
	$(MAKE) -C $(GLFWBUILDDIR)

$(LIBGLEW):
	$(MAKE) SYSTEM=darwin -C $(GLEWDIR) glew.lib.static

$(LFT):
	$(MAKE) -C $(LFTDIR) all

.PHONY: clean
clean:
	-$(RM) $(ASM_OBJS) $(VM_OBJS) $(INTERNAL_OBJS)

.PHONY: fclean
fclean: clean
	-$(RM) $(ASM_NAME) $(VM_NAME)
	-$(RM) -r $(ASM_NAME).dSYM $(VM_NAME).dSYM
	-$(RM) $(CHAMP_NAME)
	-$(RM) $(CHAMP_SRCDIR)$(CHAMP_NAME)

.PHONY: depclean
depclean:
	-$(MAKE) -C $(LIBDIR)glew-2.1.0 clean
	-$(MAKE) -C $(LIBDIR)glfw-3.3/build clean
	-$(RM) -r $(LIBDIR)glfw-3.3/build
	-$(MAKE) -C $(LIBDIR)libft fclean

.PHONY: format
format:
	clang-format -i --style=LLVM $(COREWAR_HEADERS) \
		$(INTERNAL_SRCS) $(VM_SRCS) $(ASM_SRCS) $(VM_HEADERS)

re: fclean all

.PHONY: er
er:
	@printf "Make 'er?"
	@sleep 2
	@printf "\rI barely know 'er!\n"
