#!/usr/bin/env python


from bitarray import bitarray
import numpy
import matplotlib.pyplot as plt

def main():
    with open('dt-Uranglas.txt') as f:
        data = [int(line) for line in f]
    avg = numpy.average(data)
    median = numpy.median(data)
    vmin = numpy.amin(data)
    vmax = numpy.amax(data)
    print "%d\n%d\n%d\n%d\n" % (vmin, vmax, median, avg)
    histo, bins = numpy.histogram(data, bins=512, range=(0, vmax))
    #width = 0.5 * (bins[1] - bins[0])
    #center = (bins[:-1] + bins[1:]) / 2
    #plt.bar(center, histo, align='center')
    #plt.show()
    rand = bitarray(endian='little')
    b = 0
    for line in f:
        dt = int(line)
        rand.append(dt > last_dt)
        last_dt = dt

    with open('rand.dat', 'wb+') as f:
        f.write(rand.tobytes())


if __name__ == '__main__':
    main()
