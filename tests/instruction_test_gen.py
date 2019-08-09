#!/usr/bin/env python3

import sys
from itertools import product as prod

base_arg_types = ["REG", "DIR", "IND"]


class vmOp:
    def __init__(
        self,
        name="nop",
        argc=0,
        arg_types=[],
        opcode=0,
        cycles=1,
        arg_enc=0,
        direct_size=0,
    ):
        self.name = name
        self.argc = argc
        if self.argc == 1 and len(arg_types) == 1:
            self.arg_types = arg_types[0]
        elif self.argc == 2 and len(arg_types) == 2:
            self.arg_types = [ii for ii in prod(arg_types[0], arg_types[1])]
        elif self.argc == 3 and len(arg_types) == 3:
            self.arg_types = [ii for ii in prod(arg_types[0], arg_types[1], arg_types[2])]
        else:
            self.arg_types = arg_types
        self.opcode = opcode
        self.cycles = cycles
        self.arg_enc = arg_enc
        self.direct_size = direct_size

    def get_size(self, argt):
        ret = 1 + self.arg_enc
        for tt in argt:
            if tt == "REG":
                ret += 1
            elif tt == "DIR":
                if self.direct_size:
                    ret += 2
                else:
                    ret += 4
            elif tt == "IND":
                ret += 2
        return ret

    def get_name(self, argt):
        ret = self.name
        if len(argt) == 1:
            ret = ret + "_" + argt
        else:
            for ii in argt:
                ret = ret + "_" + ii
        return ret


vops = [
    vmOp(
        name="live",
        argc=1,
        arg_types=[base_arg_types[1]],
        opcode=int("0x01", 16),
        cycles=10,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="ld",
        argc=2,
        arg_types=[base_arg_types[1:], base_arg_types[:-2]],
        opcode=int("0x02", 16),
        cycles=5,
        arg_enc=0,
        direct_size=0,
    ),
    vmOp(
        name="st",
        argc=2,
        arg_types=[base_arg_types[:-2], base_arg_types[::2]],
        opcode=int("0x03", 16),
        cycles=5,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="add",
        argc=3,
        arg_types=[base_arg_types[:-2]] * 3,
        opcode=int("0x04", 16),
        cycles=10,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="sub",
        argc=3,
        arg_types=[base_arg_types[:-2]] * 3,
        opcode=int("0x05", 16),
        cycles=10,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="and",
        argc=3,
        arg_types=[base_arg_types, base_arg_types, base_arg_types[:-2]],
        opcode=int("0x06", 16),
        cycles=6,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="or",
        argc=3,
        arg_types=[base_arg_types, base_arg_types, base_arg_types[:-2]],
        opcode=int("0x07", 16),
        cycles=6,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="xor",
        argc=3,
        arg_types=[base_arg_types, base_arg_types, base_arg_types[:-2]],
        opcode=int("0x08", 16),
        cycles=6,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="zjmp",
        argc=1,
        arg_types=[base_arg_types[-2:-1]],
        opcode=int("0x09", 16),
        cycles=20,
        arg_enc=0,
        direct_size=1,
    ),
    vmOp(
        name="ldi",
        argc=3,
        arg_types=[base_arg_types, base_arg_types[:-1], base_arg_types[:-2]],
        opcode=int("0x0a", 16),
        cycles=25,
        arg_enc=1,
        direct_size=1,
    ),
    vmOp(
        name="sti",
        argc=3,
        arg_types=[base_arg_types[:-2], base_arg_types, base_arg_types[:-1]],
        opcode=int("0x0b", 16),
        cycles=25,
        arg_enc=1,
        direct_size=1,
    ),
    vmOp(
        name="fork",
        argc=1,
        arg_types=[base_arg_types[-2:-1]],
        opcode=int("0x0c", 16),
        cycles=800,
        arg_enc=0,
        direct_size=1,
    ),
    vmOp(
        name="lld",
        argc=2,
        arg_types=[base_arg_types[1:], base_arg_types[:-2]],
        opcode=int("0x0d", 16),
        cycles=10,
        arg_enc=1,
        direct_size=0,
    ),
    vmOp(
        name="lldi",
        argc=3,
        arg_types=[base_arg_types, base_arg_types[:1]],
        opcode=int("0x0e", 16),
        cycles=50,
        arg_enc=1,
        direct_size=1,
    ),
    vmOp(
        name="lfork",
        argc=1,
        arg_types=[base_arg_types[-2:-1]],
        opcode=int("0x0f", 16),
        cycles=1000,
        arg_enc=0,
        direct_size=1,
    ),
    vmOp(
        name="aff",
        argc=1,
        arg_types=[base_arg_types[:-2]],
        opcode=int("0x10", 16),
        cycles=2,
        arg_enc=1,
        direct_size=0,
    ),
]

def gen_tests(op):
    print("Stop trying to use this")
    sys.exit(1)
    for ii in range(len(op.arg_types)):
        file_contents = ""
        file_name = op.name + str(ii+1) + "BAD.s"
        file = open(file_name, "w")
        regnum = 2
        file_contents += ".name \"" + op.get_name(op.arg_types[ii]) + "\"\n"
        # print("size: " + str(op.get_size(op.arg_types[ii])))
        file_contents += ".comment \"h\"\n"
        if (op.arg_types[ii][0] == "REG"):
            file_contents += "\n\tld %16909060,r2\n"
        file_contents += "\t"+op.name+" "
        jj = 0
        while jj < op.argc:
            if op.arg_types[ii][jj] == "REG":
                file_contents += "r"+str(regnum)
                regnum += 1
            elif op.arg_types[ii][jj] == "DIR":
                file_contents += "%270544960"
            elif op.arg_types[ii][jj] == "IND":
                file_contents += "0"
            if jj != op.argc - 1:
                file_contents += ","
            else:
                file_contents += "\n"
            #     if op.arg_types[ii][jj] == "REG":
            #         print(",r"+str(regnum))
            #     elif op.arg_types[ii][jj] == "DIR":
            #         print(",%")
            #     elif op.arg_types[ii][jj] == "IND":
            #         print(",29")
            jj += 1
        file_contents += "\tst r"+str(regnum-1)+",7\n"
        file.write(file_contents)
        file.close()

if __name__ == "__main__":
    for op in vops:
        for args in op.arg_types:
            print(op.get_name(args) + ": size(" + str(op.get_size(args))+")")
    # gen_tests(vops[9])
    print("Currently broken, do not attempt to use")
