import struct,sys


def unpack(blob):
    assert blob[:4]==b'DVLB' and struct.unpack_from('<I',blob,4)[0]==1
    at=struct.unpack_from('<I',blob,8)[0]
    h=struct.unpack_from('<7I',blob,12)
    assert blob[12:16]==b'DVLP' and blob[at:at+4]==b'DVLE'
    code=list(struct.unpack_from('<%dI'%h[3],blob,12+h[2]))
    desc=[struct.unpack_from('<2I',blob,12+h[4]+8*i) for i in range(h[5])]
    return code,desc,bytearray(blob[at:])


def operand_bits(word):
    op=word>>26
    if op<0x20 or op in (0x2e,0x2f):return 7
    if op>=0x30:return 5
    if op in (0x21,0x22,0x24,0x27,0x28,0x2a,0x2b):return 0
    raise ValueError('Unsupported native shader opcode '+hex(op))


def link(blobs):
    parts=[unpack(b) for b in blobs]
    assert 2<=len(parts)<=8
    descriptors=[]
    for mad in (True,False):
        for code,desc,_ in parts:
            for word in code:
                bits=operand_bits(word)
                if bits and (bits==5)==mad:
                    value=desc[word&((1<<bits)-1)]
                    if value not in descriptors:descriptors.append(value)
        if mad:assert len(descriptors)<=32
    assert len(descriptors)<=128
    code=[];entries=[]
    for instructions,desc,dvle in parts:
        base=len(code)
        for word in instructions:
            op=word>>26;bits=operand_bits(word)
            if bits:
                mask=(1<<bits)-1;index=descriptors.index(desc[word&mask])
                assert index<=mask
                word=(word&~mask)|index
            if op in (0x24,0x27,0x28):
                target=(word>>10)&4095
                assert target<len(instructions)
                if op==0x24:assert (word&255) and target+(word&255)<=len(instructions)
                word=(word&~(4095<<10))|((target+base)<<10)
            code.append(word)
        begin,end=struct.unpack_from('<2I',dvle,8)
        struct.pack_into('<2I',dvle,8,begin+base,end+base)
        entries.append(dvle)
    assert len(code)<512
    program=bytearray(struct.pack('<7I',0x504c5644,0,28,len(code),28+4*len(code),len(descriptors),0))
    program.extend(struct.pack('<%dI'%len(code),*code))
    for d in descriptors:program.extend(struct.pack('<2I',*d))
    result=bytearray(struct.pack('<2I',0x424c5644,len(entries))+bytes(4*len(entries)))
    result.extend(program)
    for i,entry in enumerate(entries):
        result.extend(bytes(-len(result)%4))
        struct.pack_into('<I',result,8+4*i,len(result));result.extend(entry)
    return bytes(result)


def compact(blob):
    count=struct.unpack_from('<I',blob,4)[0];d=8+4*count
    h=struct.unpack_from('<7I',blob,d)
    code=list(struct.unpack_from('<%dI'%h[3],blob,d+h[2]))
    descriptors=blob[d+h[4]:d+h[4]+8*h[5]]
    offsets=struct.unpack_from('<%dI'%count,blob,8)
    entries=[bytearray(blob[x:(offsets[i+1] if i+1<count else len(blob))]) for i,x in enumerate(offsets)]
    calls=[]
    for pc,w in enumerate(code):
        if w>>26==0x24:
            target=(w>>10)&4095;n=w&255
            assert n and target+n<=len(code)
            body=code[target:target+n];fingerprint=[]
            for ins in body:
                assert ins>>26 not in (0x22,0x24), 'No END or nested call inside helper'
                if ins>>26 in (0x27,0x28):
                    dest=(ins>>10)&4095;assert target<=dest<target+n
                    ins=(ins&~(4095<<10))|((dest-target)<<10)
                if ins>>26==0x13:
                    desc,unused=struct.unpack_from('<2I',descriptors,8*(ins&127))
                    assert unused==0
                    fingerprint.append((ins&~127,desc&0x1fff))
                else:fingerprint.append(ins)
            assert body[-1]>>26==0x21,'Helper must end with a retirement NOP'
            calls.append((pc,target,n,tuple(fingerprint)))
    assert len(calls)==4
    owners={};aliases={};removed=set();ranges=[]
    for pc,target,n,key in calls:
        assert all(target+n<=a or a+b<=target for a,b in ranges);ranges.append((target,n))
        if key in owners:
            canonical=owners[key]
            for i in range(n):aliases[target+i]=canonical+i;removed.add(target+i)
        else:owners[key]=target
    assert len(owners)==2,'Expected exactly actor and UI/ambient skin helpers'
    keep=[i for i in range(len(code)) if i not in removed];positions={old:new for new,old in enumerate(keep)}
    def mapped(pc):return positions[aliases.get(pc,pc)]
    final=[]
    for pc in keep:
        w=code[pc]
        if w>>26 in (0x24,0x27,0x28):
            target=(w>>10)&4095
            if target in removed:assert w>>26==0x24,'Only whole CALLs may reference discarded helpers'
            w=(w&~(4095<<10))|(mapped(target)<<10)
        final.append(w)
    for entry in entries:
        begin,end=struct.unpack_from('<2I',entry,8)
        assert begin not in removed
        struct.pack_into('<2I',entry,8,mapped(begin),sum(pc<end for pc in keep))
    program=struct.pack('<7I',0x504c5644,0,28,len(final),28+4*len(final),h[5],0)
    result=bytearray(struct.pack('<2I',0x424c5644,count)+bytes(4*count)+program+struct.pack('<%dI'%len(final),*final)+descriptors)
    for i,entry in enumerate(entries):
        result.extend(bytes(-len(result)%4));struct.pack_into('<I',result,8+4*i,len(result));result.extend(entry)
    return bytes(result),dict(before_instructions=len(code),after_instructions=len(final),removed_instructions=len(removed),calls=len(calls),unique_helpers=len(owners),helper_lengths=[len(k) for k in owners])


def main():
    mode,output,*inputs=sys.argv[1:]
    blobs=[open(p,'rb').read() for p in inputs]
    if mode=='link':result=link(blobs)
    elif mode=='link-compact':result=compact(link(blobs))[0]
    else:raise SystemExit('usage: shbin_link.py link|link-compact OUTPUT INPUT...')
    open(output,'wb').write(result)

if __name__=='__main__':main()
