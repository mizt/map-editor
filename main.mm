#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#import <WebKit/WebKit.h>
#import <objc/runtime.h>

#import "Types.h"

#import "turbojpeg.h"
#import "spng.h"

#import "MultiTrackQTMovieParser.h"

#import "Utils.h"
#import "UI.h"

#import "Plane.h"
#import "MetalBaseLayer.h"

#import "Guide.h"
#import "Content.h"

#import "MTLUtils.h"
#import "MTLReadPixels.h"
#import "ComputeShaderBase.h"

#import "MapRG16Unorm.h"
#import "Smudge.h"

#import "Win.h"

#define FPS 60.0

enum Mode {
    PREVIEW = 0,
    COPY_AND_PASTE = 1,
    SMUDGE = 2
};

namespace Config {
    unsigned int mode = Mode::SMUDGE;
};

class App {
    
    private:
        
        Win *_win;
        NSView *_view;
        Guide *_guide = nullptr;
        Content *_content = nullptr;
        Footer *_footer = new Footer();

        Smudge *_smudge = nullptr;
    
        dispatch_source_t _timer;
    
        bool _isDrop = true;
        bool _isDrag = false;
        bool _isSelected = false;

        NSPoint _point = CGPointMake(0.0,0.0);
        NSPoint _mouse = CGPointMake(0.0,0.0);
        void mouse(float *x, float *y) {
            NSPoint mouseLoc = [NSEvent mouseLocation];
            float px = this->_point.x+(mouseLoc.x-this->_mouse.x);
            float py = this->_point.y+(this->_mouse.y-mouseLoc.y);
            float zoom = 1.0/this->_content->scale();
            float left = this->LEFT();
            float top = (STAGE_HEIGHT-this->TOP());
            *x = (px-left)*zoom;
            *y = (py-top)*zoom;
        }
        
        Utils::Bounds _selected;
        Utils::Bounds _clipped;
        
        unsigned int _type = 0;
        float _wheel = 0.5;
    
        float LEFT() {
            return this->_content->tx()+this->_content->width()*(1.0-this->_content->scale())*0.5;
        }

        float RIGHT() {
            return this->LEFT()+this->_content->width()*this->_content->scale();
        }
    
        float BOTTOM() {
            return this->_content->ty()+this->_content->height()*(1.0-this->_content->scale())*0.5;
        }

        float TOP() {
            return this->BOTTOM()+this->_content->height()*this->_content->scale();
        }

        void clip() {
            this->_clipped = {
                .left=(int)this->LEFT(),
                .top=(int)(STAGE_HEIGHT-this->TOP()),
                .right=(int)this->RIGHT(),
                .bottom=(int)(STAGE_HEIGHT-this->BOTTOM())
            };
            int w = STAGE_WIDTH-1;
            if(this->_clipped.left<0) this->_clipped.left = 0;
            if(this->_clipped.right>w) this->_clipped.right = w;

            int h = (STAGE_HEIGHT-FOOTER_HEIGHT)-1;
            if(this->_clipped.top<0) this->_clipped.top = 0;
            if(this->_clipped.bottom>h) this->_clipped.bottom = h;
        }
        
        void addEventListener(NSString *type,void (^on)(NSNotification *)) {
            [[NSNotificationCenter defaultCenter]
                addObserverForName:type
                object:nil
                queue:[NSOperationQueue mainQueue]
                usingBlock:on
            ];
        }
    
    public:
    
        void exportMap() {
            if(this->_content) this->_content->exportMap();
        }
    
        void resetMap() {
            if(this->_smudge) this->_smudge->resetMap();
            if(this->_content) this->_content->resetMap();
        }
    
        
        App() {
            
            this->_win = new Win();
            
            this->addEventListener(@"RGB",^(NSNotification *){
                this->_type = Type::RGB;
                if(this->_content) this->_content->draw(Type::RGB);
            });
            
            this->addEventListener(@"MAP",^(NSNotification *){
                this->_type = Type::MAP;
                if(this->_content) this->_content->draw(Type::MAP);
            });
            
            if(objc_getClass("DroppableView")==nil) { objc_registerClassPair(objc_allocateClassPair(objc_getClass("NSView"),"DroppableView",0)); }
            Class DroppableView = objc_getClass("DroppableView");
              
            if(DroppableView) {
              
                if(Config::mode!=Mode::PREVIEW) {
                    
                    Utils::addMethod(DroppableView,@"mouseDown:",^(id me,NSEvent *theEvent) {
                        
                        if(this->_content) {
                        
                            this->_mouse = [NSEvent mouseLocation];
                            this->_point = CGPointMake(theEvent.locationInWindow.x,(STAGE_HEIGHT-1)-theEvent.locationInWindow.y);
                            this->_isDrag = true;
                            
                            if(Config::mode==Mode::COPY_AND_PASTE) {
                                
                            }
                            else if(Config::mode==Mode::SMUDGE) {
                                float x = 0;
                                float y = 0;
                                this->mouse(&x,&y);
                                this->_smudge->reset(x,y);
                            }
                            
                        }
                            
                    },"@@:@");
                }
                
                Utils::addMethod(DroppableView,@"otherMouseDragged:",^(id me,NSEvent *theEvent) {
                    if(this->_isDrag==false&&this->_content) {
                        if(theEvent.buttonNumber==2) {
                            this->_content->translate(
                            this->_content->tx()+theEvent.deltaX,
                            this->_content->ty()-theEvent.deltaY);
                            this->_content->transform();
                        }
                    }
                },"@@:@");
                
                if(Config::mode==Mode::COPY_AND_PASTE) {
                    Utils::addMethod(DroppableView,@"rightMouseDown:",^(id me,NSEvent *theEvent) {
                        if(this->_isSelected) {
                            float zoom = 1.0/this->_content->scale();
                            float left = this->LEFT();
                            float top = (STAGE_HEIGHT-this->TOP());
                            float px = (theEvent.locationInWindow.x);
                            float py = ((STAGE_HEIGHT-1)-theEvent.locationInWindow.y);
                            this->_content->copy(&this->_selected,(px-left)*zoom,(py-top)*zoom);
                            this->_content->draw(this->_type);
                        }
                    },"@@:@");
                }
                
                Utils::addMethod(DroppableView,@"scrollWheel:",^(id me,NSEvent *theEvent) {
                    if(this->_isDrag==false&&this->_content) {
                        float divsion = 4.0;
                        this->_wheel+=(theEvent.deltaY/100.0)*8.0;
                        this->_wheel = (this->_wheel<=0.0)?0.0:(this->_wheel>=1.0)?1.0:this->_wheel;
                        if(this->_wheel>0.5) {
                            this->_content->scale(((int)((1.0+(this->_wheel-0.5)*2.0)*divsion))*(1.0/divsion));
                        }
                        else {
                            this->_content->scale(((int)((0.25+(0.75*2.0)*this->_wheel)*divsion))*(1.0/divsion));
                        }
                        this->_content->transform();
                        this->_footer->scale((int)(this->_content->scale()*100.0));
                    }
                },"@@:@");
                
                Utils::addMethod(DroppableView,@"draggingEntered:",^NSDragOperation(id me,id<NSDraggingInfo> sender) {
                    if(!this->_isDrop) return NSDragOperationNone;
                    return NSDragOperationLink;
                },"@@:@");

                Utils::addMethod(DroppableView,@"performDragOperation:",^BOOL(id me,id<NSDraggingInfo> sender) {
                    return this->_isDrop?YES:NO;
                },"@@:@");
                
                Utils::addMethod(DroppableView,@"concludeDragOperation:",^(id me,id<NSDraggingInfo> sender) {
                    
                    this->_isDrop = false;
                    
                    if(this->_content==nil) {
                        NSString *path = [[NSURL URLFromPasteboard:[sender draggingPasteboard]] path];
                        if([path.pathExtension isEqualToString:@"mov"]) {
                            MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(path);
                            if(parser->tracks()==2&&parser->type(Type::RGB)=="jpeg"&&parser->type(Type::MAP)=="png ") {
                                if(parser->length(Type::RGB)>=1&&parser->length(Type::MAP)>=1) {
                                    int w = parser->width(Type::RGB);
                                    int h = parser->height(Type::RGB);
                                    if(w>=1&&h>=1&&w==parser->width(Type::MAP)&&h==parser->height(Type::MAP)) {
                                        
                                        NSString *path = FileManager::addPlatform(FileManager::path(@"smudge.metallib",[[NSBundle mainBundle] bundleIdentifier]));

                                        if(this->_smudge) delete[] this->_smudge;
                                        this->_smudge = new Smudge(w,h,path);

                                        this->_content = new Content(w,h);
                                        [this->_view addSubview:this->_content->view()];
                                        int frame = 0;
                                        this->_content->set(parser->get(frame,Type::RGB),parser->get(frame,Type::MAP));
                                        this->_content->draw(this->_type);
                                        this->_content->transform();
                                        
                                        
                                        [[NSNotificationCenter defaultCenter] postNotification:[NSNotification notificationWithName:@"onload" object:nil]];

                                    }
                                }
                            }
                        }
                    }
                    else {
                        if(this->_content) {
                            NSString *path = [[NSURL URLFromPasteboard:[sender draggingPasteboard]] path];
                            if([path.pathExtension isEqualToString:@"png"]) {
                                NSData *png = [[NSData alloc] initWithContentsOfFile:path];
                                this->_content->set(png);
                                if(this->_smudge) this->_smudge->setMap(this->_content->map());
                            }
                        }
                    }
                    
                    dispatch_async(dispatch_get_main_queue(),^{
                        this->_isDrop = true;
                    });
                    
                },"@@:@");
                
                this->_view = (NSView *)[[DroppableView alloc] initWithFrame:CGRectMake(0,0,STAGE_WIDTH,STAGE_HEIGHT)];
                [this->_view registerForDraggedTypes:[NSArray arrayWithObjects:NSPasteboardTypeFileURL,nil]];
                this->_view.wantsLayer = YES;
                this->_view.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1].CGColor;
                
                this->_guide = new Guide();
              
                this->_win->addChild(this->_view);
                this->_win->addChild(this->_guide->view());
                this->_win->addChild(this->_footer->view());

                this->_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_queue_create("ENTER_FRAME",0));
                dispatch_source_set_timer(this->_timer,dispatch_time(0,0),(1.0/FPS)*1000000000,0);
                dispatch_source_set_event_handler(this->_timer,^{
                    
                    if(Config::mode!=Mode::PREVIEW) {
                        
                        if(this->_content&&this->_isDrag) {
                            
                            if(Config::mode==Mode::SMUDGE) {
                                
                                if([NSEvent pressedMouseButtons]==1) {
                                                                        
                                    if(this->_smudge) {
                                        float x = 0;
                                        float y = 0;
                                        this->mouse(&x,&y);
                                        this->_content->copy(this->_smudge->map(x,y));
                                        this->_content->draw(this->_type);
                                    }
                                    
                                }
                                else {
                                    this->_isDrag = false;
                                }
                                
                            }
                            else {
                                
                                float left = this->_point.x;
                                float top  = this->_point.y;
                                
                                NSPoint mouseLoc = [NSEvent mouseLocation];
                                float right = left+(mouseLoc.x-this->_mouse.x);
                                float bottom = top+(this->_mouse.y-mouseLoc.y);
                                
                                if(left>right) {
                                    float tmp = left;
                                    left = right;
                                    right = tmp;
                                }
                                
                                if(top>bottom) {
                                    float tmp = top;
                                    top = bottom;
                                    bottom = tmp;
                                }
                                
                                this->clip();
                                
                                left = CLAMP(left,this->_clipped.left,this->_clipped.right);
                                right = CLAMP(right,this->_clipped.left,this->_clipped.right);

                                top = CLAMP(top,this->_clipped.top,this->_clipped.bottom);
                                bottom = CLAMP(bottom,this->_clipped.top,this->_clipped.bottom);

                                if([NSEvent pressedMouseButtons]==0) {
                                   
                                    this->_isDrag = false;
                                    
                                    this->_guide->clear();
                                    this->_guide->draw();
                                    
                                    this->_content->copy();
                                    
                                    float zoom = 1.0/this->_content->scale();
                                    float x = (left-this->LEFT())*zoom;
                                    float y = (top-(STAGE_HEIGHT-this->TOP()))*zoom;
                                                              
                                    this->_selected = {
                                        .left=(int)x,
                                        .top=(int)y,
                                        .right=(int)(x+(right-left)*zoom),
                                        .bottom=(int)(y+(bottom-top)*zoom)
                                    };
                                      
                                    this->_selected.left = CLAMP(this->_selected.left,0,this->_content->width()-1);
                                    this->_selected.right = CLAMP(this->_selected.right,0,this->_content->width()-1);

                                    this->_selected.top = CLAMP(this->_selected.top,0,this->_content->height()-1);
                                    this->_selected.bottom = CLAMP(this->_selected.bottom,0,this->_content->height()-1);

                                    this->_isSelected = true;
                                }
                                else {
                                    
                                    this->_guide->clear();
                                    this->_guide->update(left,top,right,bottom);
                                    this->_guide->draw();
                                    
                                }
                            }
                        }
                    }
                    
                });
                if(this->_timer) dispatch_resume(this->_timer);
            }
        }
        
        ~App() {
            
            if(this->_timer){
                dispatch_source_cancel(this->_timer);
                this->_timer = nullptr;
            }
            
            [this->_view removeFromSuperview];
            
            delete this->_content;
            delete this->_guide;
            delete this->_footer;
            delete this->_win;
            
        }
};

#pragma mark AppDelegate
@interface AppDelegate:NSObject <NSApplicationDelegate> {
    App *app;
    NSMenuItem *resetMapMenuItem;
    NSMenuItem *exportMapMenuItem;
    NSMenuItem *quitMenuItem;
}
@end
@implementation AppDelegate
-(void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    app = new App();
    
    id menu = [[NSMenu alloc] init];
    id rootMenuItem = [[NSMenuItem alloc] init];
    [menu addItem:rootMenuItem];
    id appMenu = [[NSMenu alloc] init];
    
    resetMapMenuItem = [[NSMenuItem alloc] initWithTitle:@"Reset Map" action:nil keyEquivalent:@"r"];
    exportMapMenuItem = [[NSMenuItem alloc] initWithTitle:@"Export Map" action:nil keyEquivalent:@"e"];

    quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:resetMapMenuItem];
    [appMenu addItem:exportMapMenuItem];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItem:quitMenuItem];

    
    [rootMenuItem setSubmenu:appMenu];
    [NSApp setMainMenu:menu];
    
    [[NSNotificationCenter defaultCenter]
        addObserverForName:@"onload"
        object:nil
        queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification *){
            [self->resetMapMenuItem setAction:@selector(resetMap:)];
            [self->exportMapMenuItem setAction:@selector(exportMap:)];
        }
    ];
}
-(void)applicationWillTerminate:(NSNotification *)aNotification {
    delete app;
}

-(void)resetMap:(id)sender {
    if(app) app->resetMap();
}

-(void)exportMap:(id)sender {
    if(app) app->exportMap();
}

@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        srand(CFAbsoluteTimeGetCurrent());
        srandom(CFAbsoluteTimeGetCurrent());
        id app = [NSApplication sharedApplication];
        id delegat = [AppDelegate alloc];
        [app setDelegate:delegat];
        [app run];
    }
}
