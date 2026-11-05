#!/usr/bin/env python3
"""Generate embedded 2x glyphs and fixed-point clock geometry; no device runtime."""
from PIL import Image, ImageFont, ImageDraw
from pathlib import Path
import math
root=Path(__file__).resolve().parent
specs=[('DejaVuSans.ttf',14),('DejaVuSans.ttf',18),('DejaVuSans.ttf',24),('DejaVuSerif.ttf',28),('DejaVuSans.ttf',72),('DejaVuSans-Bold.ttf',36)]
out=['/* Generated DejaVu glyphs; see FONT-LICENSE.txt. */','typedef struct {int height;const unsigned short *widths;const unsigned int *offsets;const unsigned char *bits;} Face;']
for idx,(name,size) in enumerate(specs):
 f=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/'+name,size*2)
 h=int(size*2*1.35); widths=[]; offsets=[]; data=[]
 for c in range(32,127):
  w=math.ceil(f.getlength(chr(c)));widths.append(w);offsets.append(len(data))
  im=Image.new('1',(w,h));ImageDraw.Draw(im).text((0,0),chr(c),font=f,fill=1)
  for y in range(h):
   for x in range(0,w,8):
    data.append(sum((128>>k) for k in range(8) if x+k<w and im.getpixel((x+k,y))))
 out += [f'static const unsigned short widths{idx}[]={{'+','.join(map(str,widths))+'};',f'static const unsigned int offsets{idx}[]={{'+','.join(map(str,offsets))+'};',f'static const unsigned char bits{idx}[]={{'+','.join(map(str,data))+'};',f'static const Face face{idx}={{{h},widths{idx},offsets{idx},bits{idx}}};']
out+=['static const Face *faces[]={&face0,&face1,&face2,&face3,&face4,&face5};','static const short sinequarter[]={'+','.join(str(round(math.sin(math.radians(i/2))*10000)) for i in range(181))+'};']
(root/'src/ui-assets.h').write_text('\n'.join(out)+'\n')
