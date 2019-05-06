#!/usr/bin/env bash

get-deps(){
    rebar get-deps > /dev/null
    wait
    mv deps/thrift deps/t
    mv deps/t/lib/erl deps/thrift
    rm -r deps/t
}

compile() {
    if [[ ! -d deps/thrift ]]; then get-deps; wait; fi
    if [[ ! -d src/gen-erl ]]; then gen; wait; fi
    rebar compile
}

gen() {
    thrift -o src -r --gen erl thrift/bank.thrift
}

run() {
    compile
    wait
    erl -noshell -pa ebin/ deps/*/ebin -eval "application:start(thrift)" -eval "application:start(client)"
}

case $1 in
    -r | --run )     run
                     ;;
    -c | --compile ) compile
                     ;;
    -g | --gen )     gen
                     ;;
    -d | --get-deps) get-deps
                     ;;
    * )              run
esac

