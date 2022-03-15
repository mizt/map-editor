# map-editor

No Warranty.    
This Application (including the Data) is provided to you “as is,” and you agree to use it at your own risk.

### Requirements

[https://github.com/libjpeg-turbo/libjpeg-turbo/](https://github.com/libjpeg-turbo/libjpeg-turbo/)    
[https://github.com/randy408/libspng/](https://github.com/randy408/libspng/)

### Run Script

```
mkdir -p ${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources
xcrun -sdk macosx metal -c ${SRCROOT}/content.metal -o ${SRCROOT}/content.air
xcrun -sdk macosx metallib ${SRCROOT}/content.air -o ${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/content-macosx.metallib
xcrun -sdk macosx metal -c ${SRCROOT}/guide.metal -o ${SRCROOT}/guide.air
xcrun -sdk macosx metallib ${SRCROOT}/guide.air -o ${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/guide-macosx.metallib
```
