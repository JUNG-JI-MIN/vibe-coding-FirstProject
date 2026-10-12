from pathlib import Path
import zipfile, xml.etree.ElementTree as E, json

root = Path(__file__).parent
ns = {'s': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
with zipfile.ZipFile(root / 'UI 기획안.xlsx') as z:
    shared = []
    if 'xl/sharedStrings.xml' in z.namelist():
        shared = [''.join(x.itertext()) for x in E.fromstring(z.read('xl/sharedStrings.xml')).findall('s:si', ns)]
    workbook = E.fromstring(z.read('xl/workbook.xml'))
    print('SHEETS', [(x.get('name'), x.get('sheetId')) for x in workbook.findall('s:sheets/s:sheet', ns)])
    for file in z.namelist():
        if file.startswith('xl/worksheets/sheet') and file.endswith('.xml'):
            print('\nWORKSHEET', file)
            for row in E.fromstring(z.read(file)).findall('s:sheetData/s:row', ns):
                values = []
                for c in row.findall('s:c', ns):
                    v = c.find('s:v', ns)
                    value = v.text if v is not None else ''.join(c.find('s:is', ns).itertext()) if c.find('s:is', ns) is not None else ''
                    if c.get('t') == 's' and value:
                        value = shared[int(value)]
                    if value:
                        values.append(c.get('r') + ': ' + value)
                if values:
                    print(' | '.join(values))
    media = [x for x in z.namelist() if x.startswith('xl/media/')]
    print('MEDIA', media)
    if media:
        out = root / 'asset_inspection'
        out.mkdir(exist_ok=True)
        for name in media:
            (out / Path(name).name).write_bytes(z.read(name))

vertices = []
groups = {}
current = None
for line in (root / 'SimpleGame/Res/Station_Early_Model.obj').read_text().splitlines():
    fields = line.split()
    if not fields:
        continue
    if fields[0] == 'v':
        vertices.append(tuple(map(float, fields[1:4])))
    elif fields[0] in ('o', 'g'):
        current = ' '.join(fields[1:])
        groups.setdefault(current, {'faces': 0, 'vertices': set(), 'materials': set()})
    elif fields[0] == 'usemtl':
        if current:
            groups[current]['materials'].add(' '.join(fields[1:]))
    elif fields[0] == 'f' and current:
        groups[current]['faces'] += 1
        for f in fields[1:]:
            i = int(f.split('/')[0])
            groups[current]['vertices'].add(i - 1 if i > 0 else len(vertices) + i)
result = {}
for name, g in groups.items():
    points = [vertices[i] for i in g['vertices']]
    result[name] = {'faces': g['faces'], 'materials': sorted(g['materials']),
                    'min': [round(min(p[i] for p in points), 3) for i in range(3)] if points else [],
                    'max': [round(max(p[i] for p in points), 3) for i in range(3)] if points else []}
print('\nOBJ', len(vertices), 'vertices', len(groups), 'objects')
print(json.dumps(result, ensure_ascii=False, indent=2))
(root / 'asset_manifest.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
