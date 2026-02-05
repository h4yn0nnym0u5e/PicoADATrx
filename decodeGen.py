packet = "xxxxxxxxxx1uuuu1"
sch='abcdefgh'
for i in range(0,8):
    nbl = sch[i]*4+'1'
    for j in range(0,6):
        packet += nbl
# print(len(packet))

def readBuffer(packet):
    result = packet[:32]
    packet = packet[32:]
    return (result,packet)

pkt = packet
while pkt != "":
    result, pkt = readBuffer(pkt)
    print(f"// {result}")

print("""
\tuint32_t* pOut = buffer;
\tuint32_t out = 0, tmp,
\t         mask = 0xF000'0000;

\ttmp = (*buffer++) << 11;
      
""")

pkt = packet
tmp, pkt = readBuffer(pkt)
shift = 20
tmp = tmp[11:]
out = "."*20
ch = 0
while pkt != "" or tmp != "":
    avail = len(tmp)
    out += tmp[:4]
    tmp = tmp[5:] # get rid of forced transition
    op = " " if shift == 0 else "|"
    if avail > 5: # data followed by transition
        print(f"\tout {op}= (tmp & mask) >> {shift}; tmp <<= 5; // {tmp}")
        shift += 4
    else:
        mask = "mask" if avail > 3 else f"0x{'8CE'[avail-1]}000'0000"
        print(f"\tout {op}= (tmp & {mask}) >> {shift};")
        if pkt != "":
            tmp, pkt = readBuffer(pkt)
            print(f"\ttmp = *buffer++; // {tmp}")
            if avail == 4: # starts with forced transition
                tmp = tmp[1:]
                print(f"\ttmp <<= 1; // {tmp}")
        if avail < 4: # partial nibble: complete from next word
            tmp = tmp[5 - avail:]
            mask = "mask" if shift >= 20 else f"0x{'EC8'[avail-1]}000'0000"
            print(f"\tout |= (tmp & {mask}) >> {shift+avail}; // {tmp}")
        shift +=4

    if shift >= 24:
        print(f"\t*pOut++ = out; // {out}........")
        out = ""
        shift = 0
        ch += 1
        if tmp != "":
            print(f"\n\t// Channel {ch}")

