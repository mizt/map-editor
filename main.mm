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
    
    
        Win *win;
        NSView *view;
        Guide *guide = nullptr;
        Content *content = nullptr;
        Footer *footer = new Footer();

        Smudge *smudge = nullptr;
    
        dispatch_source_t timer;
    
        bool isDrop = true;
        bool isDrag = false;
        bool isSelected = false;

        NSPoint point = CGPointMake(0.0,0.0);
        NSPoint mouse = CGPointMake(0.0,0.0);
    
        Utils::Bounds selected;
        Utils::Bounds clipped;
        
        unsigned int type = 0;
        float wheel = 0.5;
    
        float LEFT() {
            return this->content->tx()+this->content->width()*(1.0-this->content->scale())*0.5;
        }

        float RIGHT() {
            return this->LEFT()+this->content->width()*this->content->scale();
        }
    
        float BOTTOM() {
            return this->content->ty()+this->content->height()*(1.0-this->content->scale())*0.5;
        }

        float TOP() {
            return this->BOTTOM()+this->content->height()*this->content->scale();
        }

        void clip() {
            this->clipped = {
                .left=(int)this->LEFT(),
                .top=(int)(STAGE_HEIGHT-this->TOP()),
                .right=(int)this->RIGHT(),
                .bottom=(int)(STAGE_HEIGHT-this->BOTTOM())
            };
            int w = STAGE_WIDTH-1;
            if(this->clipped.left<0) this->clipped.left = 0;
            if(this->clipped.right>w) this->clipped.right = w;

            int h = (STAGE_HEIGHT-FOOTER_HEIGHT)-1;
            if(this->clipped.top<0) this->clipped.top = 0;
            if(this->clipped.bottom>h) this->clipped.bottom = h;
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
        
        App() {
            
      
            this->win = new Win();
            
            this->addEventListener(@"RGB",^(NSNotification *){
                this->type = Type::RGB;
                if(this->content) this->content->draw(Type::RGB);
            });
            
            this->addEventListener(@"MAP",^(NSNotification *){
                this->type = Type::MAP;
                if(this->content) this->content->draw(Type::MAP);
            });
            
            if(objc_getClass("DroppableView")==nil) { objc_registerClassPair(objc_allocateClassPair(objc_getClass("NSView"),"DroppableView",0)); }
            Class DroppableView = objc_getClass("DroppableView");
              
            if(DroppableView) {
                
                if(Config::mode!=Mode::PREVIEW) {
                
                    Utils::addMethod(DroppableView,@"mouseDown:",^(id me,NSEvent *theEvent) {
                        
                        
                        this->mouse = [NSEvent mouseLocation];
                        this->point = CGPointMake(theEvent.locationInWindow.x,(STAGE_HEIGHT-1)-theEvent.locationInWindow.y);
                   
                        if(this->content) {
                            this->isDrag = true;
                        }
                        else if(this->smudge) {
                                                    
                            float zoom = 1.0/this->content->scale();
                            float left = this->LEFT();
                            float top = (STAGE_HEIGHT-this->TOP());
                            float px = (theEvent.locationInWindow.x);
                            float py = ((STAGE_HEIGHT-1)-theEvent.locationInWindow.y);
                            this->smudge->reset((px-left)*zoom,(py-top)*zoom);
                            
                        }
                        
                    },"@@:@");
                    
                }
                
                Utils::addMethod(DroppableView,@"otherMouseDragged:",^(id me,NSEvent *theEvent) {
                    if(this->isDrag==false&&this->content) {
                        if(theEvent.buttonNumber==2) {
                            this->content->translate(
                                this->content->tx()+theEvent.deltaX,
                                this->content->ty()-theEvent.deltaY);
                            this->content->transform();
                        }
                    }
                },"@@:@");
                
                if(Config::mode==Mode::COPY_AND_PASTE) {
                    
                    Utils::addMethod(DroppableView,@"rightMouseDown:",^(id me,NSEvent *theEvent) {
                        if(this->isSelected) {
                            float zoom = 1.0/this->content->scale();
                            float left = this->LEFT();
                            float top = (STAGE_HEIGHT-this->TOP());
                            float px = (theEvent.locationInWindow.x);
                            float py = ((STAGE_HEIGHT-1)-theEvent.locationInWindow.y);
                            this->content->copy(&selected,(px-left)*zoom,(py-top)*zoom);
                            this->content->draw(this->type);
                        }
                    },"@@:@");
                
                }
                
                Utils::addMethod(DroppableView,@"scrollWheel:",^(id me,NSEvent *theEvent) {
                    if(this->isDrag==false&&this->content) {
                        float divsion = 4.0;
                        this->wheel+=(theEvent.deltaY/100.0)*8.0;
                        this->wheel = (this->wheel<=0.0)?0.0:(this->wheel>=1.0)?1.0:this->wheel;
                        if(this->wheel>0.5) {
                            this->content->scale(((int)((1.0+(this->wheel-0.5)*2.0)*divsion))*(1.0/divsion));
                        }
                        else {
                            this->content->scale(((int)((0.25+(0.75*2.0)*this->wheel)*divsion))*(1.0/divsion));
                        }
                        this->content->transform();
                        this->footer->scale((int)(this->content->scale()*100.0));
                    }
                },"@@:@");
                
                Utils::addMethod(DroppableView,@"draggingEntered:",^NSDragOperation(id me,id<NSDraggingInfo> sender) {
                    if(!this->isDrop) return NSDragOperationNone;
                    return NSDragOperationLink;
                },"@@:@");

                Utils::addMethod(DroppableView,@"performDragOperation:",^BOOL(id me,id<NSDraggingInfo> sender) {
                    return this->isDrop?YES:NO;
                },"@@:@");
                
                Utils::addMethod(DroppableView,@"concludeDragOperation:",^(id me,id<NSDraggingInfo> sender) {
                    if(this->content==nil) {
                        this->isDrop = false;
                        NSString *path = [[NSURL URLFromPasteboard:[sender draggingPasteboard]] path];
                        if([path.pathExtension isEqualToString:@"mov"]) {
                            MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(path);
                            if(parser->tracks()==2&&parser->type(Type::RGB)=="jpeg"&&parser->type(Type::MAP)=="png ") {
                                if(parser->length(Type::RGB)>=1&&parser->length(Type::MAP)>=1) {
                                    int w = parser->width(Type::RGB);
                                    int h = parser->height(Type::RGB);
                                    if(w>=1&&h>=1&&w==parser->width(Type::MAP)&&h==parser->height(Type::MAP)) {
                                        
                                        NSString *path = FileManager::addPlatform(FileManager::path(@"smudge.metallib",[[NSBundle mainBundle] bundleIdentifier]));
                                                                                                         
                                        if(this->smudge) delete[] this->smudge;
                                        this->smudge = new Smudge(w,h,path);
                                        
                                        this->content = new Content(w,h);
                                        [this->view addSubview:this->content->view()];
                                        int frame = 0;
                                        this->content->set(parser->get(frame,Type::RGB),parser->get(frame,Type::MAP));
                                        this->content->draw(this->type);
                                        this->content->transform();
                                    }
                                }
                            }
                        }
                        else {
                            dispatch_async(dispatch_get_main_queue(),^{
                                this->isDrop = true;
                            });
                        }
                    }
                    
                },"@@:@");
                
                this->view = (NSView *)[[DroppableView alloc] initWithFrame:CGRectMake(0,0,STAGE_WIDTH,STAGE_HEIGHT)];
                [this->view registerForDraggedTypes:[NSArray arrayWithObjects:NSPasteboardTypeFileURL,nil]];
                this->view.wantsLayer = YES;
                this->view.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1].CGColor;
                
                this->guide = new Guide();
              
                this->win->addChild(this->view);
                this->win->addChild(this->guide->view());
                this->win->addChild(this->footer->view());

                this->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_queue_create("ENTER_FRAME",0));
                dispatch_source_set_timer(this->timer,dispatch_time(0,0),(1.0/30.0)*1000000000,0);
                dispatch_source_set_event_handler(this->timer,^{
                    
                    if(Config::mode!=Mode::PREVIEW) {
                        
                        if(this->content&&this->isDrag) {
                            
                            if(Config::mode==Mode::SMUDGE) {
                                
                                if([NSEvent pressedMouseButtons]==1) {
                                                                        
                                    if(this->smudge) {
                                        
                                        NSPoint mouseLoc = [NSEvent mouseLocation];
                                        float px = this->point.x+(mouseLoc.x-this->mouse.x);
                                        float py = this->point.y+(this->mouse.y-mouseLoc.y);
                                                                                
                                        float zoom = 1.0/this->content->scale();
                                        float left = this->LEFT();
                                        float top = (STAGE_HEIGHT-this->TOP());
                                                                 
                                        this->content->copy(this->smudge->map((px-left)*zoom,(py-top)*zoom));
                                        this->content->draw(this->type);
                                    }
                                    
                                }
                                else {
                                    this->isDrag = false;
                                }
                                
                            }
                            else {
                                
                                float left = this->point.x;
                                float top  = this->point.y;
                                
                                NSPoint mouseLoc = [NSEvent mouseLocation];
                                float right = left+(mouseLoc.x-this->mouse.x);
                                float bottom = top+(this->mouse.y-mouseLoc.y);
                                
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
                                
                                left = CLAMP(left,this->clipped.left,this->clipped.right);
                                right = CLAMP(right,this->clipped.left,this->clipped.right);

                                top = CLAMP(top,this->clipped.top,this->clipped.bottom);
                                bottom = CLAMP(bottom,this->clipped.top,this->clipped.bottom);

                                if([NSEvent pressedMouseButtons]==0) {
                                   
                                    this->isDrag = false;
                                    
                                    this->guide->clear();
                                    this->guide->draw();
                                    
                                    this->content->copy();
                                    
                                    float zoom = 1.0/this->content->scale();
                                    float x = (left-this->LEFT())*zoom;
                                    float y = (top-(STAGE_HEIGHT-this->TOP()))*zoom;
                                                              
                                    this->selected = {
                                        .left=(int)x,
                                        .top=(int)y,
                                        .right=(int)(x+(right-left)*zoom),
                                        .bottom=(int)(y+(bottom-top)*zoom)
                                    };
                                      
                                    this->selected.left = CLAMP(this->selected.left,0,this->content->width()-1);
                                    this->selected.right = CLAMP(this->selected.right,0,this->content->width()-1);

                                    this->selected.top = CLAMP(this->selected.top,0,this->content->height()-1);
                                    this->selected.bottom = CLAMP(this->selected.bottom,0,this->content->height()-1);

                                    this->isSelected = true;
                                }
                                else {
                                    
                                    this->guide->clear();
                                    this->guide->update(left,top,right,bottom);
                                    this->guide->draw();
                                    
                                }
                            }
                        }
                    }
                    
                });
                if(this->timer) dispatch_resume(this->timer);
            }
        }
        
        ~App() {
            
            if(this->timer){
                dispatch_source_cancel(this->timer);
                this->timer = nullptr;
            }
            
            [this->view removeFromSuperview];
            
            delete this->content;
            delete this->guide;
            delete this->footer;
            delete this->win;
            
        }
};

#pragma mark AppDelegate
@interface AppDelegate:NSObject <NSApplicationDelegate> {
    App *app;
}
@end
@implementation AppDelegate
-(void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    app = new App();
}
-(void)applicationWillTerminate:(NSNotification *)aNotification {
    delete app;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        srand(CFAbsoluteTimeGetCurrent());
        srandom(CFAbsoluteTimeGetCurrent());
        id app = [NSApplication sharedApplication];
        id delegat = [AppDelegate alloc];
        [app setDelegate:delegat];

        id menu = [[NSMenu alloc] init];
        id rootMenuItem = [[NSMenuItem alloc] init];
        [menu addItem:rootMenuItem];
        id appMenu = [[NSMenu alloc] init];
        id quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [rootMenuItem setSubmenu:appMenu];
        [NSApp setMainMenu:menu];
        [app run];
    }
}
