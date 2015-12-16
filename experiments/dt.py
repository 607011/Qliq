#!/usr/bin/env python

from bitarray import bitarray

def main():
    with open('dt-Strumpf.txt', 'r') as f:
        last_dt = 0
        rand = bitarray(endian='little')
        b = 0
        while True:
            line1 = f.readline()
            line2 = f.readline()
            if len(line2) == 0:
                break;
            dt0 = int(line1)
            dt1 = int(line2)
            rand.append(dt1 > dt0)

    with open('rand.dat', 'wb+') as f:
        f.write(rand.tobytes())

if __name__ == '__main__':
    main()
