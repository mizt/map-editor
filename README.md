# map-editor

No Warranty.    
This Application (including the Data) is provided to you “as is,” and you agree to use it at your own risk.

Noncommercial use only.

#### [Demo](https://mizt.github.io/map-editor/)

Works in newer browsers.

##### Usage:
* Dragging  canvas distort map image.
* Drop JPEG to change image.
* Drop PNG to change map image.
* Right click to download distorted image / map image.

#### [Demo (MJPEG)](https://mizt.github.io/map-editor/?mode=MJPEG)
##### Note: 
* Safari on M1 Mac recommended.
* Load [plasticbag.mov](https://github.com/mizt/map-editor/blob/main/docs/plasticbag.mov) that is 25.3MB.

[MJPEG specification](https://mizt.github.io/map-editor/specifications/?id=MJPEG)    
[test](https://mizt.github.io/map-editor/mov.html)

#### Map specification
##### [Demo (PNG Sequence)](https://mizt.github.io/map-editor/?mode=PNG)

##### Note: 

* Safari on M1 Mac recommended.
* Load [map.mov](https://github.com/mizt/map-editor/blob/main/docs/map.mov) that is 10.0MB.

[test](https://mizt.github.io/map-editor/mov.html?mode=PNG)

```
#define MAP_SCALE 8.0
```

```
for(int i=0; i<height; i++) {
	for(int j=0; j<width; j++) {
		unsigned short x = 0x5555+(int)(j*MAP_SCALE);
		unsigned short y = 0x5555+(int)(i*MAP_SCALE);
		mv[i*width+j] = x<<16|y;
	}
}
```


#### [Demo (Loop)](https://mizt.github.io/map-editor/loop.html)

Still image is converted into video using motion vectors.
