# source: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
MODEREF = '''
0  m[1:0],g2[4],b2[4],b3[4],r0[9:0],g0[9:0],b0[9:0],r1[4:0],g3[4],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
1  m[1:0],g2[5],g3[4],g3[5],r0[6:0],b3[0],b3[1],b2[4],g0[6:0],b2[5],b3[2],g2[4],b0[6:0],b3[3],b3[5],b3[4],r1[5:0],g2[3:0],g1[5:0],g3[3:0],b1[5:0],b2[3:0],r2[5:0],r3[5:0]
2  m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[4:0],r0[10],g2[3:0],g1[3:0],g0[10],b3[0],g3[3:0],b1[3:0],b0[10],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
6  m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10],g3[4],g2[3:0],g1[4:0],g0[10],g3[3:0],b1[3:0],b0[10],b3[1],b2[3:0],r2[3:0],b3[0],b3[2],r3[3:0],g2[4],b3[3]
10 m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10],b2[4],g2[3:0],g1[3:0],g0[10],b3[0],g3[3:0],b1[4:0],b0[10],b2[3:0],r2[3:0],b3[1],b3[2],r3[3:0],b3[4],b3[3]
14 m[4:0],r0[8:0],b2[4],g0[8:0],g2[4],b0[8:0],b3[4],r1[4:0],g3[4],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
18 m[4:0],r0[7:0],g3[4],b2[4],g0[7:0],b3[2],g2[4],b0[7:0],b3[3],b3[4],r1[5:0],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[5:0],r3[5:0]
22 m[4:0],r0[7:0],b3[0],b2[4],g0[7:0],g2[5],g2[4],b0[7:0],g3[5],b3[4],r1[4:0],g3[4],g2[3:0],g1[5:0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
26 m[4:0],r0[7:0],b3[1],b2[4],g0[7:0],b2[5],g2[4],b0[7:0],b3[5],b3[4],r1[4:0],g3[4],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[5:0],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
30 m[4:0],r0[5:0],g3[4],b3[0],b3[1],b2[4],g0[5:0],g2[5],b2[5],b3[2],g2[4],b0[5:0],g3[5],b3[3],b3[5],b3[4],r1[5:0],g2[3:0],g1[5:0],g3[3:0],b1[5:0],b2[3:0],r2[5:0],r3[5:0]
3  m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[9:0],g1[9:0],b1[9:0]
7  m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[8:0],r0[10],g1[8:0],g0[10],b1[8:0],b0[10]
11 m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[7:0],r0[10:11],g1[7:0],g0[10:11],b1[7:0],b0[10:11]
15 m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10:15],g1[3:0],g0[10:15],b1[3:0],b0[10:15]
'''

INDENT = '    '

FORMAT = 'colors[{endpoint}].c.{channel} |= TCTEX_H_GETBITS(bdata, {offset}, {numbits}) << {firstbit};'

import re

print('switch(mode)')
print('{')
for line in MODEREF.splitlines():
    line = line.strip()
    if not line:
        continue
    mode, assigns = line.split()
    mode = int(mode)
    assigns = assigns.split(',')

    print('case %u:' % mode)
    offset = 0
    for assign in assigns:
        m = re.match(r'(\w)(\d?)\[(\d+)(?::(\d+))?\]', assign)
        assert m

        data = {}
        data['channel'] = m.group(1)
        data['offset'] = offset
        head = int(m.group(3))
        tail = int(m.group(4)) if m.group(4) else head
        if head >= tail:
            reverse = False
            head, tail = tail, head
        else:
            reverse = True
        numbits = tail - head + 1
        if m.group(1) == 'm':
            offset += numbits
            continue
        data['endpoint'] = int(m.group(2))
        data['numbits'] = numbits
        data['firstbit'] = head
        if reverse:
            data['numbits'] = 1
            print(INDENT + '/* begin reverse read */')
            for i in range(tail - head + 1):
                data['offset'] = offset + i
                data['firstbit'] = tail - i
                print(INDENT + FORMAT.format(**data))
            print(INDENT + '/* end reverse read */')
        else:
            print(INDENT + FORMAT.format(**data))
        offset += numbits
    print(INDENT + 'break;')

print('}')
