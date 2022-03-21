#import <vector>
#import "fpng.h"
#import "ContentLayer.h"

class Content {
    
    private:
    
        unsigned int *buffer[2] = {nullptr,nullptr};

        unsigned int *RGB = nullptr;
        unsigned int *MAP = nullptr;
        
        unsigned int *RESET = nullptr;
    
        std::vector<unsigned char> fpng_file_buf;


        int _width = 0;
        int _height = 0;
        
        NSView *_view = nil;
        ContentLayer<Plane> *_layer = nullptr;
        
        float _tx = 0.0;
        float _ty = 0.0;
    
        float _scale = 1.0;
    
        void png(unsigned int *texure, NSData *data, int w, int h) {
            char *bytes = (char *)[data bytes];
            long length = [data length];
            spng_ctx *ctx = spng_ctx_new(0);
            spng_set_crc_action(ctx,SPNG_CRC_USE,SPNG_CRC_USE);
            spng_set_png_buffer(ctx,bytes,length);
            struct spng_ihdr ihdr;
            spng_get_ihdr(ctx,&ihdr);
            if(ihdr.width==w&&ihdr.height==h&&ihdr.bit_depth==8) {
                if(ihdr.color_type==SPNG_COLOR_TYPE_TRUECOLOR_ALPHA) {
                    spng_decode_image(ctx,texure,ihdr.width*ihdr.height*4,SPNG_FMT_RGBA8,0);
                }
            }
            spng_ctx_free(ctx);
        }
        
        void jpeg(unsigned int *texure, NSData *data, int w, int h) {
                   
            tjhandle handle = tjInitDecompress();
            
            unsigned char *bytes = (unsigned char *)[data bytes];
            unsigned long length = [data length];
                    
            int width;
            int height;
            int subsample;
            int colorspace;
            
            if(!tjDecompressHeader3(handle,bytes,length,&width,&height,&subsample,&colorspace)) {
                if(width==w&&height==h) {
                    unsigned char *dst = new unsigned char[width*height*3];

                    if(subsample==0) {
                        tjDecompress2(handle,bytes,length,dst,width,0,height,TJPF_RGB,TJFLAG_FASTDCT);
                    }
                    else if(subsample==2) {
                        unsigned char *yuv420 = new unsigned char[tjBufSizeYUV(width,height,subsample)];
                        tjDecompressToYUV(handle,bytes,length,yuv420,TJFLAG_FASTDCT);
                        tjDecodeYUV(handle,yuv420,1,subsample,dst,width,0,height,TJPF_RGB,0);
                        delete[] yuv420;
                    }
                    
                    if(dst) {
                        
                        for(int i=0; i<height; i++) {
                            for(int j=0; j<width; j++) {
                                
                                int addr = (i*width+j)*3;
                                
                                texure[i*width+j] = 0xFF000000|dst[addr+2]<<16|dst[addr+1]<<8|dst[addr+0];
                            }
                        }
                        
                        delete[] dst;
                    }
                }
            }
            tjDestroy(handle);
        }
    
    
    public:
    
        unsigned int *map() {
            return this->MAP;
        }
    
        int width() {
            return this->_width;
        }
    
        int height() {
            return this->_height;
        }
    
        void translate(float tx, float ty) {
            this->_tx = tx;
            this->_ty = ty;
        }
        float tx() { return this->_tx; }
        float ty() { return this->_ty; }

        void scale(float s) { this->_scale = s; }
        float scale() { return this->_scale; }
        
        void transform() {
            if(this->_layer) {
                this->_layer->layer().anchorPoint = CGPointMake(0.5,0.5);
                this->_layer->layer().position = CGPointMake(this->_width>>1,this->_height>>1);
                CGAffineTransform transform = CGAffineTransformMakeTranslation((int)this->_tx,(int)this->_ty);
                transform = CGAffineTransformScale(transform,this->_scale,this->_scale);
                [this->_layer->layer() setAffineTransform:transform];
                [this->_layer->layer() removeAllAnimations];
            }
        }
        
        NSView *view() {
            return this->_view;
        }
    
        void draw(int type=0) {
            this->_layer->type(type);
            this->_layer->update(^(id<MTLCommandBuffer> commandBuffer){
                this->_layer->cleanup();
            });
        }
    
        void src(unsigned int *p) {
            for(int k=0; k<this->_width*this->_height; k++) {
                this->RGB[k] = p[k];
            }
            this->_layer->RGB(this->RGB);
        }
    
        void copy(unsigned int *map) {
            for(int k=0; k<this->_width*this->_height; k++) {
                this->MAP[k] = map[k];
            }
            this->_layer->MAP(this->MAP);
        }
    
        void copy(Utils::Bounds *selected, int ox, int oy) {
            int top = selected->top;
            int left = selected->left;
            int bx = CLAMP(ox,0,this->_width-1);
            int by = CLAMP(oy,0,this->_height-1);
            int ex = ox + (selected->right-left);
            int ey = oy + (selected->bottom-top);
            ex = CLAMP(ex,0,this->_width-1);
            ey = CLAMP(ey,0,this->_height-1);
            for(int i=by; i<ey; i++) {
                for(int j=bx; j<ex; j++) {
                    this->MAP[i*this->_width+j] = this->buffer[1][(top+(i-by))*this->_width+(left+(j-bx))];
                }
            }
            this->_layer->MAP(this->MAP);
        }
    
        void copy() {
            for(int k=0; k<this->_width*this->_height; k++) {
                this->buffer[1][k] = this->MAP[k];
            }
        }
    
        void set(NSData *map) {
            this->png(this->RESET,map,this->_width,this->_height);
            this->resetMap();
        }
    
        void set(NSData *rgb, NSData *map) {
            this->jpeg(this->RGB,rgb,this->_width,this->_height);
            this->png(this->RESET,map,this->_width,this->_height);
            this->resetMap();
        }
        
        void resetMap() {
            for(int k=0; k<this->_width*this->_height; k++) this->MAP[k] = this->RESET[k];
            
            this->_layer->RGB(this->RGB);
            this->_layer->MAP(this->MAP);
            
            this->_layer->update(^(id<MTLCommandBuffer> commandBuffer){
                this->_layer->cleanup();
                dispatch_async(dispatch_get_main_queue(),^{
                    this->transform();
                });
            });
            
        }
    
        void exportMap() {
            if(fpng::fpng_encode_image_to_memory((const char *)this->MAP,this->_width,this->_height,4,this->fpng_file_buf)) {
                
                long unixtime = (CFAbsoluteTimeGetCurrent()+kCFAbsoluteTimeIntervalSince1970)*1000;
                NSString *timeStampString = [NSString stringWithFormat:@"%f",(unixtime/1000.0)];
                NSDate *date = [NSDate dateWithTimeIntervalSince1970:[timeStampString doubleValue]];
                NSDateFormatter *format = [[NSDateFormatter alloc] init];
                [format setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"ja_JP"]];
                [format setDateFormat:@"yyyy_MM_dd_HH_mm_ss_SSS"];

                NSDictionary* environ = [[NSProcessInfo processInfo] environment];
                BOOL inSandbox = (nil != [environ objectForKey:@"APP_SANDBOX_CONTAINER_ID"]);
                
                NSString *path = [NSString stringWithFormat:@"%@/%@.png",[NSSearchPathForDirectoriesInDomains((inSandbox)?NSMoviesDirectory:NSDesktopDirectory,NSUserDomainMask,YES) objectAtIndex:0],[format stringFromDate:date]];
                                
                NSData *png = [NSData dataWithBytes:this->fpng_file_buf.data() length:this->fpng_file_buf.size()];
                [png writeToFile:path options:NSDataWritingAtomic error:nil];
                
            }
        }
    
        Content(int w, int h) {
            
            this->_width = w;
            this->_height = h;
            
            if(this->RGB) delete this->RGB;
            this->RGB = new unsigned int[w*h];
            
            if(this->MAP) delete this->MAP;
            this->MAP = new unsigned int[w*h];
            
            if(this->RESET) delete this->RESET;
            this->RESET = new unsigned int[w*h];
            
            if(this->buffer[0]) delete this->buffer[0];

            this->buffer[0] = new unsigned int[w*h];
            this->buffer[1] = new unsigned int[w*h];
            
            this->_view = (NSView *)[[NSView alloc] initWithFrame:CGRectMake(0,0,this->_width,this->_height)];
            this->_view.wantsLayer = YES;
            this->_view.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.0 green:0.0 blue:0.0 alpha:1].CGColor;
            
            this->_layer = new ContentLayer<Plane>();
            if(this->_layer->init(this->_width,this->_height,@"content.metallib",[[NSBundle mainBundle] bundleIdentifier])) {
                
                this->_layer->resolution(this->_width,this->_height);
                
                this->_view.layer = this->_layer->layer();
             
                NSLog(@"%@",NSStringFromRect([this->_view frame]));
                
                this->_tx+=(STAGE_WIDTH-this->_width)>>1;
                this->_ty+=(STAGE_HEIGHT-this->_height+FOOTER_HEIGHT)>>1;
                
            }
        }
        
        ~Content() {
            delete[] this->buffer[0];
            delete[] this->buffer[1];
            delete[] this->RGB;
            delete[] this->MAP;
            [this->_view removeFromSuperview];
        }
    
};
