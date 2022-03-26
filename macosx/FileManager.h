namespace FileManager {
    
    bool exists(NSString *path) {
        static const NSFileManager *fileManager = [NSFileManager defaultManager];
        NSError *err = nil;
        [fileManager attributesOfItemAtPath:path error:&err];
        return (err)?false:true;
    }
    
    bool extension(NSString *str, NSString *ext) {
        return ([[str pathExtension] compare:ext]==NSOrderedSame)?true:false;
    }

    NSString *replace(NSString *str, NSString *from,NSString *to) {
        return [str stringByReplacingOccurrencesOfString:from withString:to];
    }
    
    NSString *replace(NSString *str,NSArray<NSString *> *from,NSString *to) {
        NSMutableString *mstr = [NSMutableString stringWithString:str];
        for(int n=0; n<[from count]; n++) {
            mstr.string = replace(mstr,from[n],to);
        }
        return [NSString stringWithString:mstr];
    }
    
    NSString *concat(NSString *a, NSString *b) {
        if(a==nil) {
            return b;
        }
        else {
            return replace([NSString stringWithFormat:@"%@/%@",a,b],@"//",@"/");
        }
    }
    
    NSString *removePlatform(NSString *str) {
        NSString *extension = [NSString stringWithFormat:@".%@",[str pathExtension]];
        return FileManager::replace(str,@[
            [NSString stringWithFormat:@"-macosx%@",extension],
            [NSString stringWithFormat:@"-iphoneos%@",extension],
            [NSString stringWithFormat:@"-iphonesimulator%@",extension]],
            extension);
    }
    
    NSString *addPlatform(NSString *str) {
        NSString *extension = [NSString stringWithFormat:@".%@",[str pathExtension]];
#if TARGET_OS_OSX
        return FileManager::replace(FileManager::removePlatform(str),extension,[NSString stringWithFormat:@"-macosx%@",extension]);
#elif TARGET_OS_SIMULATOR
        return FileManager::replace(FileManager::removePlatform(str),extension,[NSString stringWithFormat:@"-iphonesimulator%@",extension]);
#elif TARGET_OS_IPHONE
        return FileManager::replace(FileManager::removePlatform(str),extension,[NSString stringWithFormat:@"-iphoneos%@",extension]);
#else
        return nil;
#endif
    }

    NSString *path(NSString *filename,NSString *identifier=nil) {
        if(identifier==nil) {
#if TARGET_OS_OSX
            return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] bundlePath],filename];
#else
            return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] resourcePath],filename];
#endif
        }
        else {
            return [NSString stringWithFormat:@"%@/%@",[[NSBundle bundleWithIdentifier:identifier] resourcePath],filename];
        }
    }
};
