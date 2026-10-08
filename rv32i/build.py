from pathlib import Path
import re, json

root=Path(__file__).resolve().parent
out=root
out.mkdir(exist_ok=True)
src=[[1,4,2,0,3,5,6],[0,1,2,4,5,6,3],[0,2,5,3,1,4,6]]
tw=[[1,2,0,2,1,0,0],[0,0,0,1,2,1,2],[0]*7]
maps=[];twists=[]
for face in range(3):
    p=list(range(7));o=[0]*7
    for turn in range(3):
        p=[p[i] for i in src[face]]
        o=[(o[src[face][i]]+tw[face][i])%3 for i in range(7)]
        maps.append(p);twists.append(o)
def rows(arr):
    return '\n    '.join('.byte '+','.join(map(str,v+[0])) for v in arr)
header=(root/'pdb_tables.h').read_text(encoding='utf-8-sig')
pdb=[];sizes={}
for name,n,vals in re.findall(r'static const uint8_t (\w+)\[(\d+)\]\s*=\s*\{(.*?)\}',header,re.S):
    vals=[int(v) for v in re.findall(r'\d+',vals)]
    assert len(vals)==int(n)
    sizes[name]=len(vals)
    pdb.append(name+':\n'+'\n'.join('    .byte '+', '.join(map(str,vals[i:i+32])) for i in range(0,len(vals),32)))
faces=[([7,4,0,1],[0,0,0,0],9,0),
       ([7,0,6,3],[1,2,2,1],0,7),
       ([0,1,3,2],[1,2,2,1],9,7),
       ([1,4,2,5],[1,2,2,1],18,7),
       ([4,7,5,6],[1,2,2,1],27,7),
       ([3,2,6,5],[0,0,0,0],9,14)]
descriptors=[]
for corners,slots,x,y in faces:
    for i in range(4):descriptors.append([corners[i],slots[i],x+(i%2)*4,y+(i//2)*3])
facelets='\n    '.join('.byte '+','.join(map(str,v)) for v in descriptors)
renderer=(root/'renderer.s').read_text().replace('@FACELETS@',facelets)
core=(root/'core.s').read_text()
core=core.replace('@MOVE_SOURCE@',rows(maps)).replace('@MOVE_TWIST@',rows(twists))
core=core.replace('@PDB@','\n'.join(pdb)).replace('@TRANSITIONS@',(root/'transitions.inc').read_text())
for name,render in [('minirubik_rv32i.s','render_cube:\n    ret'),('minirubik_rv32i_led.s',renderer)]:
    text=core.replace('@RENDERER@',render)
    # Ripes resolves .word references immediately, so pointer tables must come
    # after the data they reference. Instruction references can point forward.
    table=re.search(r'move_tables:\n(?:    \.word [^\n]+\n)+',text).group()
    text=text.replace(table,'')
    # Emit one text section and one data section. Also allow the user's current
    # CLI settings, where .text and .data both default to address zero: reserve
    # 4 KiB before initialized data so it cannot overwrite the instruction bytes.
    sections={'text':[],'data':[]};kind='text'
    for line in text.splitlines():
        if line.strip() in ('.text','.data'):
            kind=line.strip()[1:];continue
        if line.strip().startswith('.equ '):
            sections['text'].append(line);continue
        sections[kind].append(line)
    data='\n'.join(sections['data']).replace('static_data_end: .byte 0','')
    # Ripes .align N uses bytes, unlike GNU .align on RISC-V (2**N).
    data=data.replace('.align 4\nsoftware_stack:','.align 16\nsoftware_stack:')
    data=data.replace('.align 2\n','.align 4\n')
    text='.text\n'+'\n'.join(sections['text'])+'\n.data\n'
    text+='# Reserve 4096 bytes: compatible even if data/text bases are both zero.\n.zero 4096\n'
    text+=data+'\n.align 4\n'+table+'static_data_end: .byte 0\n'
    assert not re.search(r'^\s*(mul\w*|div\w*|rem\w*|fl\w*|fs\w*)\s',text,re.M)
    assert '@' not in text
    (out/name).write_text(text,encoding='ascii')
    print(name,len(text),'source bytes')
(root/'render_descriptors.json').write_text(json.dumps({'source':maps,'twist':twists,'facelets':descriptors}))
