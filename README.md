# Corewar

## Resources

- Subject pdfs in resources

- Sample assembler, VM, champions in resources/vm_champs

- [42 Corewar cookbook](https://github.com/VBrazhnik/Corewar/wiki)

## Build

```sh
make deps
make
```

## Assembler

### Usage

```sh
./asm file.s        # assembles asm file and writes to file.cor
./asm -d file.cor   # disassembles binary and writes to file.s
```

## Virtual Machine

[vm in action]: corewar.png

### Usage

```sh
./corewar champ1.cor champ2.cor
```

### Debug

Debug window.  Shows processes, registers, cycle, etc. as processes run

### Open

Opens *.s files in edit.  Loads *.cor files in debug

### Edit

Built in editor for assembly source files.  Able to compile and load files

### Graph

Shows which commands are being run and how often
