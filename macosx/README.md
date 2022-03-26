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
xcrun -sdk macosx metal -c ${SRCROOT}/copy.metal -o ${SRCROOT}/copy.air
xcrun -sdk macosx metallib ${SRCROOT}/copy.air -o ${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/copy-macosx.metallib
xcrun -sdk macosx metal -c ${SRCROOT}/smudge.metal -o ${SRCROOT}/smudge.air
xcrun -sdk macosx metallib ${SRCROOT}/smudge.air -o ${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/smudge-macosx.metallib
```
