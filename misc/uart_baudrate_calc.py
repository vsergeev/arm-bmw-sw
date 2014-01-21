import math

BR = 115200
PCLK = 48e6
PCLK = 24e6

# 13.5.15, pg. 211 of UM10398 LPC111x/LPC11Cxx User manual
# Baudrate = PCLK / (16*DL*(1 + DivAddVal/MulVal))
#   DL is a 16-bit unsigned integer
#   1 <= MulVal <= 15
#   0 <= DivAddVal <= 14
#   DivAddVal < MulVal

configurations = {}

for MULVAL in range(1, 16):
    for DIVADDVAL in range(0, 15):
        if DIVADDVAL >= MULVAL:
            continue

        FR = 1.0 + float(DIVADDVAL)/float(MULVAL)
        DLint = int(PCLK/(16.0*BR*FR))

        if DLint >= 2**16:
            continue

        BRcalc = PCLK/(16.0*DLint*FR)
        error = abs(float(BR) - float(BRcalc))

        configurations[error] = (BRcalc, (DLint >> 8) & 0xff, DLint & 0xff, DIVADDVAL, MULVAL)

for error in sorted(configurations):
    (BRcalc, U0DLM, U0DLL, DIVADDVAL, MULVAL) = configurations[error]
    print "%.2f -- error %.3f / %0.2f%%, U0DLM = %d, U0DLL = %d, DIVADDVAL = %d, MULVAL = %d" % (BRcalc, error, (error/BR)*100, U0DLM, U0DLL, DIVADDVAL, MULVAL)

