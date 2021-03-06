#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#import <WebKit/WebKit.h>
#import <objc/runtime.h>

#import "Types.h"
#import "FileManager.h"

#import "turbojpeg.h"
#import "spng.h"

#import "MultiTrackQTMovieParser.h"

#import "Utils.h"
#import "UI.h"

#import "MTLUtils.h"
#import "MTLReadPixels.h"

#import "Plane.h"
#import "Mesh.h"
#import "MetalBaseLayer.h"
#import "MeshLayer.h"

#import "Guide.h"
#import "Content.h"

#import "ComputeShaderBase.h"

#import "MapRG16Unorm.h"
#import "Smudge.h"

#import "Win.h"

#define FPS 60.0

enum Mode {
    PREVIEW = 0,
    COPY_AND_PASTE = 1,
    SMUDGE = 2,
    MESH = 3
};

namespace Info {
    unsigned short width = 0;
    unsigned short height = 0;
}

namespace Config {
    unsigned int mode = Mode::MESH;
};

#import "Droppable.h"

class App : public Droppable {
    
    private:
        
        Win *_win;
        NSView *_view;
        Guide *_guide = nullptr;
        
        Smudge *_smudge = nullptr;
    
        MeshLayer<Mesh> *_meshLayer = nullptr;
    
        dispatch_source_t _timer;
    
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
    
        void initialize() {
            NSString *path = FileManager::addPlatform(FileManager::path(@"smudge.metallib",[[NSBundle mainBundle] bundleIdentifier]));
            if(this->_smudge) delete[] this->_smudge;
            this->_smudge = new Smudge(Info::width,Info::height,path);
            
            if(Config::mode==Mode::MESH) {
                this->_meshLayer = new MeshLayer<Mesh>();
                if(this->_meshLayer->init(Info::width,Info::height,@"mesh.metallib",[[NSBundle mainBundle] bundleIdentifier],false)) {
                    this->_meshLayer->update(^(id<MTLCommandBuffer> commandBuffer){
                        
                        this->_content->copy(this->_meshLayer->getByte());
                        this->_meshLayer->cleanup();
                      
                        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC/30.0),dispatch_get_main_queue(),^{
                            
                            this->_content->draw(this->_type);
                            this->_content->transform();
                            
                            [[NSNotificationCenter defaultCenter] postNotification:[NSNotification notificationWithName:@"onload" object:nil]];
                        
                        });
                    });
                }
            }
            else {
                this->_content->draw(this->_type);
                this->_content->transform();
                
                [[NSNotificationCenter defaultCenter] postNotification:[NSNotification notificationWithName:@"onload" object:nil]];
            }
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
            
            Class DroppableView = objc_getClass("DroppableView");
              
            if(DroppableView) {
              
                if(Config::mode!=Mode::PREVIEW) {
                    
                    Utils::addMethod(DroppableView,@"mouseDown:",^(id me,NSEvent *theEvent) {
                        
                        if(this->_content) {
                        
                            this->_mouse = [NSEvent mouseLocation];
                            this->_point = CGPointMake(theEvent.locationInWindow.x,(STAGE_HEIGHT-1)-theEvent.locationInWindow.y);
                            this->_isDrag = true;
                            
                            if(Config::mode==Mode::SMUDGE) {
                                float x = 0;
                                float y = 0;
                                this->mouse(&x,&y);
                                this->_smudge->reset(x,y);
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
                    
                }
                
                Utils::addMethod(DroppableView,@"concludeDragOperation:",^(id me,id<NSDraggingInfo> sender) {
                    
                    this->_isDrop = false;
                    
                    if(this->_content==nil) {
                        
                        int frame = 0;
                        
                        NSString *path = [[NSURL URLFromPasteboard:[sender draggingPasteboard]] path];
                        if([path.pathExtension isEqualToString:@"mov"]) {
                            MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(path);
                            if(parser->tracks()==2&&parser->type(Type::RGB)=="jpeg"&&parser->type(Type::MAP)=="png ") {
                                if(parser->length(Type::RGB)>=1&&parser->length(Type::MAP)>=1) {
                                    int w = parser->width(Type::RGB);
                                    int h = parser->height(Type::RGB);
                                    if(w>=1&&h>=1&&w==parser->width(Type::MAP)&&h==parser->height(Type::MAP)) {
                                        Info::width = w;
                                        Info::height = h;
                                        this->_content = new Content(Info::width,Info::height);
                                        [this->_view addSubview:this->_content->view()];
                                        this->_content->png(parser->get(frame,Type::MAP));
                                        this->_content->jpeg(parser->get(frame,Type::RGB));
                                        this->_content->resetMap();
                                        this->initialize();
                                    }
                                }
                            }
                        }
                        else if([path.pathExtension isEqualToString:@"jpg"]) {
                            
                            NSData *jpg = [[NSData alloc] initWithContentsOfFile:path];
                            if(jpg) {
                                
                                unsigned char *bytes = (unsigned char *)[jpg bytes];
                                unsigned long length = [jpg length];
                                
                                int width;
                                int height;
                                int subsample;
                                int colorspace;
                                
                                tjhandle handle = tjInitDecompress();
                                
                                if(!tjDecompressHeader3(handle,bytes,length,&width,&height,&subsample,&colorspace)) {
                                    
                                    Info::width = width;
                                    Info::height = height;
                                    
                                    this->_content = new Content(Info::width,Info::height);
                                    [this->_view addSubview:this->_content->view()];
                                    
                                    this->_content->jpeg(jpg);
                                    this->_content->resetMap();
                                    
                                    this->initialize();
                                
                                }
                                
                                tjDestroy(handle);
                            }
                            
                        }
                        else if([path.pathExtension isEqualToString:@"png"]) {
                            
                            NSData *png = [[NSData alloc] initWithContentsOfFile:path];
                            if(png) {
                                
                                unsigned char *bytes = (unsigned char *)[png bytes];
                                unsigned long length = [png length];
                                
                                spng_ctx *ctx = spng_ctx_new(0);
                                spng_set_crc_action(ctx,SPNG_CRC_USE,SPNG_CRC_USE);
                                spng_set_png_buffer(ctx,bytes,length);
                                struct spng_ihdr ihdr;
                                spng_get_ihdr(ctx,&ihdr);
                                
                                Info::width = ihdr.width;
                                Info::height = ihdr.height;
                               
                                spng_ctx_free(ctx);
                                
                                this->_content = new Content(Info::width,Info::height);
                                [this->_view addSubview:this->_content->view()];
                                
                                this->_content->png(png);
                                this->initialize();
                                                                
                            }
                        }
                    }
                    else {
                        
                        if(this->_content) {
                            NSString *path = [[NSURL URLFromPasteboard:[sender draggingPasteboard]] path];
                            if([path.pathExtension isEqualToString:@"png"]) {
                                NSData *png = [[NSData alloc] initWithContentsOfFile:path];
                                if(png) {
                                    
                                    unsigned char *bytes = (unsigned char *)[png bytes];
                                    unsigned long length = [png length];
                                    
                                    spng_ctx *ctx = spng_ctx_new(0);
                                    spng_set_crc_action(ctx,SPNG_CRC_USE,SPNG_CRC_USE);
                                    spng_set_png_buffer(ctx,bytes,length);
                                    struct spng_ihdr ihdr;
                                    spng_get_ihdr(ctx,&ihdr);
                                    
                                    if(Info::width==ihdr.width&&Info::height==ihdr.height) {
                                        this->_content->png(png);
                                        this->_content->resetMap();
                                        if(this->_smudge) this->_smudge->setMap(this->_content->map());
                                    }
                                    else {
                                        
                                        
                                        if(Config::mode==Mode::MESH) {
                                                                    
                                            Mesh *mesh = this->_meshLayer->data();
                                            int mw = mesh->width();
                                            int mh = mesh->height();
                                            
                                            if(mw==(ihdr.width>>1)&&mh==ihdr.height) {
                                                
                                                unsigned int *MV = new unsigned int[ihdr.width*ihdr.height];
                                                
                                                if(ihdr.color_type==SPNG_COLOR_TYPE_TRUECOLOR_ALPHA) {
                                                    spng_decode_image(ctx,MV,ihdr.width*ihdr.height*4,SPNG_FMT_RGBA8,0);
                                                }
                                                
                                                float *vertices = this->_meshLayer->vertices();
                                                                                               
                                                for(int i=1; i<mh-1; i++) {
                                                    for(int j=1; j<mw-1; j++) {
                                                        
                                                        int src = i*ihdr.width+j*2;
                                                        int dst = (i*mw+j)<<2;
                                                        
                                                        float mx = *((float *)(MV+src+0));
                                                        float my = *((float *)(MV+src+1));

                                                        vertices[dst+0] = ((j/((float)(mw-1)))*2.0-1.0)+(mx/mw)*1.0;
                                                        vertices[dst+1] = ((i/((float)(mh-1)))*2.0-1.0)+(my/mh)*1.0;
                                                        
                                                    }
                                                }
                                                
                                                delete[] MV;
                                                
                                                this->_meshLayer->update(^(id<MTLCommandBuffer> commandBuffer){
                                                    
                                                    this->_content->copy(this->_meshLayer->getByte());
                                                    this->_meshLayer->cleanup();
                                                    
                                                    this->_content->draw(this->_type);
                                                    this->_content->transform();
                                                    
                                                });
                                                
                                            }
                                        }
                                        else {
                                            
                                            // reset
                                            
                                        }
                                    }
                                    
                                    spng_ctx_free(ctx);
                                    
                                }
                            }
                            else if([path.pathExtension isEqualToString:@"jpg"]) {
                                
                                NSData *jpg = [[NSData alloc] initWithContentsOfFile:path];
                                if(jpg) {
                                    
                                    unsigned char *bytes = (unsigned char *)[jpg bytes];
                                    unsigned long length = [jpg length];
                                    
                                    int width;
                                    int height;
                                    int subsample;
                                    int colorspace;
                                    
                                    tjhandle handle = tjInitDecompress();
                                    if(!tjDecompressHeader3(handle,bytes,length,&width,&height,&subsample,&colorspace)) {
                                        if(Info::width==width&&Info::height==height) {
                                            this->_content->jpeg(jpg);
                                            this->_content->resetMap();
                                            
                                        }
                                        else {
                                            // reset
                                        }
                                    }
                                    tjDestroy(handle);
                                }
                                
                                
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
                            else if(Config::mode==Mode::MESH) {
                                if([NSEvent pressedMouseButtons]==1) {
                                    
                                    this->_meshLayer->update(^(id<MTLCommandBuffer> commandBuffer){
                                        
                                        this->_content->copy(this->_meshLayer->getByte());
                                        this->_meshLayer->cleanup();
                                        
                                        this->_content->draw(this->_type);
                                        this->_content->transform();
                                        
                                    });
                                    
                                }
                                else {
                                    this->_isDrag = false;
                                }
                                
                            }
                            else if(Config::mode==Mode::COPY_AND_PASTE) {
                                
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
                            else {
                                
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
