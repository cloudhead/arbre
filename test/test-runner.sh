#!/usr/bin/env bash
#
# test-runner.sh
#
# run arbre test-cases
#
# (c) 2011, Alexis Sellier
#
# usage:
#
#     ./test-runner.sh FILE...
#

main () {

    FAILED=0
    PASSED=0

    COLS=$(tput cols)

    for file in $@; do
        run_test $file
        case $? in
            0)
                echo -ne "\033[$(expr $COLS - 2)G"
                echo -e "$BOLD${GREEN}OK$CLEAR"
                PASSED=$(expr $PASSED + 1)
                ;;
            1)
                echo
                cat $TMP.result.expected   | sed 's/^/'`echo -e "$BOLD$RED-$CLEAR"`' /'
                cat $TMP.result.unexpected | sed 's/^/'`echo -e "$BOLD$RED+$CLEAR"`' /'
                FAILED=$(expr $FAILED + 1)
                ;;
            139)
                echo -e "$BOLD${RED}SEGFAULT$CLEAR"
                FAILED=$(expr $FAILED + 1)
                ;;
            *)
                echo -e "$BOLD${RED}FAILED$CLEAR"
                ;;
        esac
    done

    echo

    if [ $FAILED -eq 0 ]; then
        echo -ne "$BOLD${GREEN}OK$CLEAR $PASSED"
    else
        echo -ne "$BOLD${RED}FAILED$CLEAR $FAILED"
    fi

    echo -e "/$(expr $FAILED + $PASSED) tests."

    rm -rf "/tmp/arbre.test.$$"
}

arbre () {
    bin/arbre $@
}

run_test () {

    FILE=$1

    TMP="/tmp/arbre.test.$$/$(basename $FILE)"

    mkdir -p $TMP

    # Extract compiler command
    CMD=$(cat $FILE | head -n 1 | sed 's/^--! //')
    CMD=$(eval "echo $CMD")

    log "executing" $CMD

    # Run the compiler and save the relevant output to a file
    $CMD > /dev/null 2>$TMP.errors

    local R=$?

    if [ ! $? -eq 0 ]; then
        return $R
    fi

    # Parse errors
    cat $TMP.errors                                     |
        egrep -e $FILE                                  |
        sed   -e 's|^\('$FILE':[0-9]\+\):[0-9]\+ |\1:|' > $TMP.actual

    # Parse the test-case
    cat $FILE                                           |
        egrep -noe '-- (error|warning):.*'              |
        sed     -e 's|^|'$FILE':|'                      |
        sed     -e 's/--//'                             > $TMP.expected

    grep -vFf $TMP.actual   $TMP.expected > $TMP.result.expected   && R=1
    grep -vFf $TMP.expected $TMP.actual   > $TMP.result.unexpected && R=1

    return $R
}

ESC="\033"
RED="$ESC[31m"; GREEN="$ESC[32m"
BOLD="$ESC[1m"; CLEAR="$ESC[0m"

log () {
    local header=$1
    shift
    echo -ne "$BOLD$header$CLEAR $@: " ; return 0
}

main "$@"

