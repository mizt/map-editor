# map-editor

No Warranty.    
This Application (including the Data) is provided to you “as is,” and you agree to use it at your own risk.

[Demo](https://mizt.github.io/map-editor/)

* Drop JPEG to change image.
* Drop PNG to change map image.
* Right click to download distorted image / map image.

#### Map

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