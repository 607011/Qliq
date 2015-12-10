#!/usr/bin/env python

def main():
    all_bytes = bytearray()
    with open("dt.txt", "r") as f:
        flip_bit = 0
        last_dt = 0
        bit_idx = 0
        current_byte = 0
        for line in f:
            dt = int(line)
            if dt > last_dt:
                bit = flip_bit
            else:
                bit = flip_bit ^ 1
            flip_bit = flip_bit ^ 1
            last_dt = dt
            current_byte = current_byte | (bit << bit_idx)
            bit_idx = bit_idx + 1
            if bit_idx > 7:
                bit_idx = 0
                all_bytes.append(current_byte)
                current_byte = 0
    with open("rand.dat", "wb+") as f:
        f.write(all_bytes)
    print ("%d bytes written.\n" % (len(all_bytes)))

if __name__ == "__main__":
    main()
