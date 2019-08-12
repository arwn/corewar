#!/usr/bin/env bash

# Horrendous and ugly bash script ahead, read if you dare.

function print_help {
    echo "Run regression tests for the Corewar Virtual Machine."
    echo "Example: run_test.sh -v and or xor"
    echo ""
    echo "Commands:"
    echo -e "  all\t\t\tenable all tests"
    echo -e "  help\t\t\tdisplay this help output"
    echo -e "  clean\t\t\tremove all output in the test output directory"
    echo ""
    echo "Instruction Tests:"
    echo -e "  live\t\t\tenable live instruction tests"
    echo -e "  ld\t\t\tenable ld instruction tests"
    echo -e "  st\t\t\tenable st instruction tests"
    echo -e "  add\t\t\tenable add instruction tests"
    echo -e "  sub\t\t\tenable sub instruction tests"
    echo -e "  and\t\t\tenable and instruction tests"
    echo -e "  or\t\t\tenable or instruction tests"
    echo -e "  xor\t\t\tenable xor instruction tests"
    echo -e "  zjmp\t\t\tenable zjmp instruction tests"
    echo -e "  ldi\t\t\tenable ldi instruction tests"
    echo -e "  sti\t\t\tenable sti instruction tests"
    echo -e "  fork\t\t\tenable fork instruction tests"
    echo -e "  lld\t\t\tenable lld instruction tests"
    echo -e "  lldi\t\t\tenable lldi instruction tests"
    echo -e "  lfork\t\t\tenable lfork instruction tests"
    echo -e "  aff\t\t\tenable aff instruction tests"
    echo ""
    echo "Miscellaneous Tests:"
    echo -e "  zork\t\t\tenable testing zork.s for 1536 cycles"
    echo -e "  fluttershy\t\tenable testing fluttershy.s for 1536 cycles"
    echo -e "  helltrain\t\tenable testing helltrain.s for 1536 cycles"
    echo -e "  Asombra\t\tenable testing Asombra.s for 1536 cycles"
    echo -e "  overwatch\t\tenable testing overwatch.s for 1536 cycles"
    echo -e "  Gagnant\t\tenable testing Gagnant.s for 1536 cycles"
    echo ""
    echo "Options:"
    echo -e "  -f, --file=FILE\tuse FILE as test assembly"
    echo -e "  -d, --debug\t\tenable debug output"
    echo -e "  -q\t\t\tdecrease verbosity"
    echo -e "  --quiet\t\tset verbosity level to 0"
    echo -e "  --verbose\t\tset verbosity level to 1"
    echo -e "  -v\t\t\tincrease verbosity"
}

declare -A ins_list
ins_list+=(['live']=1)
ins_list+=(['ld']=1)
ins_list+=(['st']=1)
ins_list+=(['add']=1)
ins_list+=(['sub']=1)
ins_list+=(['and']=9)
ins_list+=(['or']=9)
ins_list+=(['xor']=9)
ins_list+=(['zjmp']=3)
ins_list+=(['ldi']=1)
ins_list+=(['sti']=1)
ins_list+=(['fork']=1)
ins_list+=(['lld']=1)
ins_list+=(['lldi']=1)
ins_list+=(['lfork']=1)
ins_list+=(['aff']=1)

declare -A misc_tests
misc_tests+=(['zork']=1)
misc_tests+=(['helltrain']=1)
misc_tests+=(['fluttershy']=1)
misc_tests+=(['overwatch']=1)
misc_tests+=(['Asombra']=1)
misc_tests+=(['Gagnant']=1)

declare -A ins_time
ins_time+=(['live']=90)
ins_time+=(['ld']=90)
ins_time+=(['st']=90)
ins_time+=(['add']=30)
ins_time+=(['sub']=30)
ins_time+=(['and']=30)
ins_time+=(['or']=30)
ins_time+=(['xor']=30)
ins_time+=(['zjmp']=543)            # 543
ins_time+=(['ldi']=90)
ins_time+=(['sti']=90)
ins_time+=(['fork']=870)
ins_time+=(['lld']=30)
ins_time+=(['lldi']=100)
ins_time+=(['lfork']=1050)
ins_time+=(['aff']=2)
ins_time+=(['zork']=57954)           # 57955
ins_time+=(['helltrain']=27438)      # 27439
ins_time+=(['fluttershy']=25092)     # 25093
ins_time+=(['overwatch']=27438)      # 27439
ins_time+=(['Asombra']=30360)        # 30361
ins_time+=(['Gagnant']=4675)        # 26024

out=/tmp
input_file=""

declare -i doall=0 debug=0 verbose=1 experimental=0 status=0
declare -a input_args

while (( $# > 0 )); do
    case "$1"
    in
        "all")
            doall=1
            break
            ;;
        "clean")
            rm -f $out/*.in $out/*.out $out/*.cor $out/in $out/out
            echo "Cleaned output in $out"
            exit 0
            ;;
        "help")
            echo "Usage: run_test.sh [OPTIONS] COMMAND|TEST ..."
            print_help
            exit 0
            ;;
        "-f"|"--file")
            shift
            if [[ -n "$1" ]]; then
                echo "ERROR: -f requires an argument"
                exit 1
            else
                input_file="$1"
            fi
            ;;
        "-d"|"--debug")
            debug=1
            ;;
        "--quiet")
            verbose=0
            ;;
        "-q")
            (( verbose-- ))
            ;;
        "--verbose")
            verbose=1
            ;;
        "-v")
            (( verbose++ ))
            ;;
        "--experimental")
            experimental=1
            ;;
        *)
            input_args+=("$1")
            # echo "WARNING: unknown argument \"$1\""
    esac
    shift
done

if (( debug!=0 )); then
    echo "-----BEGIN DEBUG OUTPUT-----"
    echo "out:          [$out]"
    echo "doall:        [$doall]"
    echo "debug:        [$debug]"
    echo "verbose:      [$verbose]"
    echo "experimental: [$experimental]"
    echo "input_file:   [\"${input_file[*]}\"]"
    echo "input_args:   [\"${input_args[*]}\"]"
    echo "-----END DEBUG OUTPUT-----"
fi


if [[ -z "${input_args[*]}" ]] && (( doall==0 )); then
    echo "Error: no tests specified"
    echo "Usage: run_test.sh [OPTIONS] COMMAND|TEST ..."
    echo "Try 'run_test.sh help' for more information."
    exit 1
fi

if [[ $(pwd) = */tests$ ]]; then
    basedir=..
else
    basedir=.
fi

invm=$basedir/resources/vm_champs/corewar
inasm=$basedir/resources/vm_champs/asm
outvm=$basedir/corewar
testdir=$basedir/tests

if [[ $input_file != "" ]]; then
    bn=$(basename "$input_file")
    if [[ ! -f "$out/$bn.cor" ]]; then
        if [[ ! -f "$input_file" ]]; then
            echo "Error: $input_file: no such file or directory."
            return 1
        fi
        $inasm "$input_file" 1>/dev/null
        mv "$input_file" "$out/$bn.cor"
    fi
    $invm -d "$tim" "$out/$bn.cor" > /tmp/in
    grep -E '^0x0[[:xdigit:]]{2}0 : ' /tmp/in | awk '{$1=$1};1'> "$out/$bn.in"
    $outvm -r -d "$tim" "$out/$bn.cor" > /tmp/out
    grep -E '^0x0[[:xdigit:]]{2}0 : ' /tmp/out | awk '{$1=$1};1' > "$out/$bn.out"
    if ! diff -q "$out/$bn.in" "$out/$bn.out"; then
        diff "$out/$bn.in" "$out/$bn.out"
        exit 1
    fi
fi

if [[ ! -x $invm ]]; then
    echo "ERROR: invm path invalid: \"$invm\""
    exit 1
elif [[ ! -x $inasm ]]; then
    echo "ERROR: inasm path invalid: \"$inasm\""
    exit 1
elif [[ ! -x $outvm ]]; then
    echo "ERROR: outvm path invalid: \"$outvm\""
    exit 1
fi

if [[ $doall -eq 1 ]]; then
    read -r -a input_args <<< "${!ins_list[*]}"
fi

for ins in "${input_args[@]}"; do
    j=${ins_list["$ins"]}
    if [[ -z $j ]]; then
        j=1
    fi
    tim=${ins_time["$ins"]}
    for i in $(seq 1 "$j"); do
        if [[ $verbose -gt 0 ]]; then
            printf "Testing [%s%d]:  \t" "$ins" "$i"
        fi
        if [[ ! -f "$out/$ins$i.cor" ]]; then
            if [[ ! -f "$testdir/$ins$i.s" ]]; then
                if [[ $verbose -gt 0 ]]; then
                    printf "\nError: %s/%s%d.s: no such file or directory.\n" "$out" "$ins" "$i"
                fi
                break
            fi
            $inasm "$testdir/$ins$i.s" 1>/dev/null
            mv "$testdir/$ins$i.cor" "$out/$ins$i.cor"
        fi
        if [[ $experimental -eq 1 ]]; then
            $invm -a -v 31 -d "$tim" "$out/$ins$i.cor" | awk '{$1=$1};1' > "$out/$ins$i.in"
        else
            $invm -d "$tim" "$out/$ins$i.cor" | grep -E '^0x0[[:xdigit:]]{2}0 : ' | awk '{$1=$1};1' > "$out/$ins$i.in"
        fi
        if [[ $experimental -eq 1 ]]; then
            timeout 2 $outvm -r -v 31 -d "$tim" "$out/$ins$i.cor" > "$out/out.tmp"
            status=$?
            if (( status!=124 )); then
                cat "$out/out.tmp" | awk '{$1=$1};1' > "$out/$ins$i.out"
            fi
            # $outvm -r -v 31 -d "$tim" "$out/$ins$i.cor" | awk '{$1=$1};1' > "$out/$ins$i.out"
        else
            timeout 2 $outvm -r -d "$tim" "$out/$ins$i.cor" > "$out/out.tmp"
            status=$?
            if (( status!=124 )); then
                grep -E '^0x0[[:xdigit:]]{2}0 : ' "$out/out.tmp" | awk '{$1=$1};1' > "$out/$ins$i.out"
            fi
            # $outvm -r -d "$tim" -f "$out/$ins$i.cor" | grep -E '^0x0[[:xdigit:]]{2}0 : ' | awk '{$1=$1};1' > "$out/$ins$i.out"
        fi
        if (( status==124 )); then
            if [[ $verbose -gt 0 ]]; then
                printf "\e[33mTIMEOUT\e[0m\n"
            fi
        elif ! diff -q "$out/$ins$i.in" "$out/$ins$i.out" >/dev/null; then
            if [[ $verbose -gt 0 ]]; then
                if [[ $verbose -gt 1 ]]; then
                    diff -U 3 "$out/$ins$i.in" "$out/$ins$i.out"
                else
                    printf "\e[31mKO\e[0m\n"
                fi
            fi
        else
            if [[ $verbose -gt 0 ]]; then
                printf "\e[32mOK\e[0m\n"
            fi
        fi
    done
done
exit 0
