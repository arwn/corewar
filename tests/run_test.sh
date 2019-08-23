#!/usr/bin/env bash

# Horrendous and ugly bash script ahead, read if you dare.

function print_help {
    echo "Run regression tests for the Corewar Virtual Machine."
    echo "Example: run_test.sh -v and or xor"
    echo ""
    echo "Commands:"
    echo -e "  all\t\t\trun all tests"
    echo -e "  help\t\t\tdisplay this help output"
    echo -e "  clean\t\t\tremove all output in the test output directory"
    echo ""
    echo "Instruction Tests:"
    echo -e "  live\t\t\trun live instruction tests"
    echo -e "  ld\t\t\trun ld instruction tests"
    echo -e "  st\t\t\trun st instruction tests"
    echo -e "  add\t\t\trun add instruction tests"
    echo -e "  sub\t\t\trun sub instruction tests"
    echo -e "  and\t\t\trun and instruction tests"
    echo -e "  or\t\t\trun or instruction tests"
    echo -e "  xor\t\t\trun xor instruction tests"
    echo -e "  zjmp\t\t\trun zjmp instruction tests"
    echo -e "  ldi\t\t\trun ldi instruction tests"
    echo -e "  sti\t\t\trun sti instruction tests"
    echo -e "  fork\t\t\trun fork instruction tests"
    echo -e "  lld\t\t\trun lld instruction tests"
    echo -e "  lldi\t\t\trun lldi instruction tests"
    echo -e "  lfork\t\t\trun lfork instruction tests"
    echo -e "  aff\t\t\trun aff instruction tests"
    echo ""
    echo "Miscellaneous Tests:"
    echo -e "  zork\t\t\trun testing zork.s for 57955 cycles"
    echo -e "  fluttershy\t\trun testing fluttershy.s for 27439 cycles"
    echo -e "  helltrain\t\trun testing helltrain.s for 25093 cycles"
    echo -e "  Asombra\t\trun testing Asombra.s for 27439 cycles"
    echo -e "  overwatch\t\trun testing overwatch.s for 30361 cycles"
    echo -e "  Gagnant\t\trun testing Gagnant.s for 26024 cycles"
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
ins_list+=(['lld']=3)
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
ins_time+=(['lld']=45)
ins_time+=(['lldi']=100)
ins_time+=(['lfork']=1050)
ins_time+=(['aff']=7)
ins_time+=(['zork']=57954)           # 57955
ins_time+=(['helltrain']=27438)      # 27439
ins_time+=(['fluttershy']=25092)     # 25093
ins_time+=(['overwatch']=27438)      # 27439
ins_time+=(['Asombra']=30361)        # 30361
ins_time+=(['Gagnant']=26024)        # 26024

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
        "-e"|"--experimental")
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
    $outvm -d "$tim" "$out/$bn.cor" > /tmp/out
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
            $outvm -a -v 31 -d "$tim" "$out/$ins$i.cor" | awk '{$1=$1};1' > "$out/$ins$i.out"
        else
            timeout 5 $outvm -d "$tim" "$out/$ins$i.cor" > "$out/out.tmp"
            status=$?
            if (( status!=124 )); then
                grep -E '^0x0[[:xdigit:]]{2}0 : ' "$out/out.tmp" | awk '{$1=$1};1' > "$out/$ins$i.out"
            fi
        fi
        if (( status==124 )); then
            if [[ $verbose -gt 0 ]]; then
                printf "\e[33mTIMEOUT\e[0m\n"
            fi
        elif ! diff -q "$out/$ins$i.in" "$out/$ins$i.out" >/dev/null; then
            if [[ $verbose -gt 0 ]]; then
                if [[ $verbose -gt 1 ]]; then
                    diff "$out/$ins$i.in" "$out/$ins$i.out"
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
