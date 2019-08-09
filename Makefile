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
LINKERS = -L $(LFTDIR) -lft
INCLUDES = -I $(LFTDIR)includes/ -I inc/

ASM_SRCDIR = cmd/asm/
ASM_NAME = asm
ASM_CFILES = main.c
ASM_SRCS = $(addprefix $(ASM_SRCDIR), $(ASM_CFILES))
ASM_OBJS = $(ASM_SRCS:.c=.o)

VM_SRCDIR = cmd/corewar/
VM_NAME = corewar
VM_CFILES = corewar.c cpu.c instructions.c
VM_SRCS = $(addprefix $(VM_SRCDIR), $(VM_CFILES))
VM_OBJS = $(VM_SRCS:.c=.o)

GUI_LDFLAGS = $(LIBGLEW) -L $(dir $(LIBGLFW)) -lglfw3
GUI_FRAMEWORKS = -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
GUI_INCLUDE = -I lib/glfw-3.3/build/include -I lib/glew-2.1.0/include

INTERNAL_SRCDIR = internal/
INTERNAL_CFILES = op.c hashtbl.c util.c lex.c parse.c resolve_labels.c print_args.c globals.c asm_main_funcs.c parse_binary.c
INTERNAL_SRCS = $(addprefix $(INTERNAL_SRCDIR), $(INTERNAL_CFILES))
INTERNAL_OBJS = $(INTERNAL_SRCS:.c=.o)

CFLAGS = $(CCFLAGS) $(INCLUDES)

all: $(LFT) $(ASM_NAME) $(VM_NAME)

$(ASM_NAME): $(LFT) $(ASM_OBJS) $(INTERNAL_OBJS)
	$(CC) $(CFLAGS) $(LINKERS) $(INCLUDES) $(INTERNAL_OBJS) $(ASM_OBJS) -o $(ASM_NAME)

$(VM_NAME): $(VM_OBJS) $(ASM_OBJS) $(INTERNAL_OBJS)
	make deps
	$(CC) $(CFLAGS) $(LINKERS) $(INCLUDES) $(GUI_INCLUDE) $(GUI_LDFLAGS) $(GUI_FRAMEWORKS) $(INTERNAL_OBJS) $(VM_OBJS) -o $(VM_NAME)

deps: $(LFT) $(LIBGLEW) $(LIBGLFW)

$(LIBGLFW):
	mkdir -p $(GLFWBUILDDIR) && cd $(GLFWBUILDDIR) && cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_EXECUTABLE_FORMAT=MACHO -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_INSTALL=OFF -DGLFW_USE_OSMESA=OFF -DGLFW_VULKAN_STATIC=OFF
	make -C $(GLFWBUILDDIR)

$(LIBGLEW):
	make -C $(GLEWDIR)

$(LFT):
	make -C $(LFTDIR)

clean:
	-$(RM) $(ASM_OBJS) $(VM_OBJS) $(INTERNAL_OBJS)

fclean: clean
	-$(RM) $(ASM_NAME) $(VM_NAME)
	-$(RM) -r $(ASM_NAME).dSYM $(VM_NAME).dSYM

depclean:
	make -C $(LIBDIR)glew-2.1.0 clean
	make -C $(LIBDIR)glfw-3.3/build clean
	-$(RM) -r $(LIBDIR)glfw-3.3/build
	make -C $(LIBDIR)libft clean

re: fclean all

er:
	@echo "Make 'er?  I barely know 'er!"
