# Copyright 2019 Stephen Farrell. All Rights Reserved.
#
# Licensed under the OpenSSL license (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html
#
# An OpenSSL-based HPKE implementation following draft-irtf-cfrg-hpke
#
# I plan to use this for my ESNI-enabled OpenSSL build (https://github.com/sftcd/openssl)
# when the time is right.

OSSL=${HOME}/code/openssl
INCL=${HOME}/code/openssl/include
# There are testvectors for this - see comments in hpketv.h
# if you don't want to compile in test vector checks then
# comment out the next line
testvectors=-D TESTVECTORS

CFLAGS=-g ${testvectors}

CC=gcc

all: hpkemain

# do a test run
test: hpkemain
	- LD_LIBRARY_PATH=${OSSL} ./hpkemain

hpke.o: hpke.c hpke.h
	${CC} ${CFLAGS} -I ${INCL} -c $<

hpkemain.o: hpkemain.c hpke.h
	${CC} ${CFLAGS} -I ${INCL} -c $<

ifdef testvectors
hpketv.o: hpketv.c hpketv.h hpke.h
	${CC} ${CFLAGS} -I ${INCL} -c $<
endif

ifdef testvectors
hpkemain: hpkemain.o hpke.o hpketv.o
	${CC} ${CFLAGS} -o $@ hpkemain.o hpke.o hpketv.o -L ${OSSL} -lssl -lcrypto
else
hpkemain: hpkemain.o hpke.o 
	${CC} ${CFLAGS} -o $@ hpkemain.o hpke.o -L ${OSSL} -lssl -lcrypto
endif

doc: 
	- doxygen hpke.doxy
	- (cd doxy/latex; make; mv refman.pdf ../../hpke-api.pdf )

docclean:
	- rm -rf doxy

clean:
	- rm -f hpkemain.o hpke.o hpketv.o hpkemain 
