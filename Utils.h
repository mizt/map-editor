#import <objc/runtime.h>

namespace Utils {

    typedef struct _Bounds {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    } Bounds;

    void print(Bounds *b, NSString *label=nil) {
        if(label==nil) {
            NSLog(@"%d,%d,%d,%d",b->left,b->top,b->right,b->bottom);
        }
        else {
            NSLog(@"%@ %d,%d,%d,%d",label,b->left,b->top,b->right,b->bottom);
        }
    }

    BOOL isEqString(NSString *a,NSString *b) {
        return [a compare:b]==NSOrderedSame;
    }

    BOOL isEqClassName(id a,NSString *b) {
        return [[a className] compare:b]==NSOrderedSame;
    }

    void addMethod(Class cls, NSString *method, id block, const char *type, bool isClassMethod=false) {
            
        SEL sel = NSSelectorFromString(method);
        int ret = ([cls respondsToSelector:sel])?1:(([[cls new] respondsToSelector:sel])?2:0);
        if(ret) {
            class_addMethod(cls,(NSSelectorFromString([NSString stringWithFormat:@"_%@",(method)])),method_getImplementation(class_getInstanceMethod(cls,sel)),type);
            class_replaceMethod((ret==1)?object_getClass((id)cls):cls,sel,imp_implementationWithBlock(block),type);
        }
        else {
            class_addMethod((isClassMethod)?object_getClass((id)cls):cls,sel,imp_implementationWithBlock(block),type);
        }
    }

    const char *stringWithFormat(const char *format, ...){
        char *str = new char[(long)fmax(1024,(pow(2,(floor(log(strlen(format))/log(2)))+1)))];
        va_list arg;
        va_start(arg,format); vsprintf(str,format,arg); va_end(arg);
        return str;
    }


};
