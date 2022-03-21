class MapRG16UnormBase {

    protected:

        int cnt = -1;

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLFunction> function;
        id<MTLComputePipelineState> pipelineState;
        
        id<MTLLibrary> _library;

        id<MTLTexture> texture[2];
        id<MTLBuffer> _resolution = nil;
    
        std::vector<id<MTLBuffer>> _params;
    
        unsigned int *_map[2];

        int width = 0;
        int height = 0;
    
        unsigned int *exec() {
            
            if(this->cnt==-1) {
                this->cnt = 0;
            }
            else {
                
                id<MTLCommandQueue> queue = [this->device newCommandQueue];
                
                [this->texture[0] replaceRegion:MTLRegionMake2D(0,0,this->width,this->height) mipmapLevel:0 withBytes:this->_map[this->cnt&1] bytesPerRow:(this->width)<<2];

                id<MTLCommandBuffer> commandBuffer = queue.commandBuffer;
                id<MTLComputeCommandEncoder> encoder = commandBuffer.computeCommandEncoder;
                [encoder setComputePipelineState:pipelineState];
                [encoder setTexture:this->texture[0] atIndex:0];
                [encoder setTexture:this->texture[1] atIndex:1];
              
                [encoder setBuffer:this->_resolution offset:0 atIndex:0];
                
                for(int k=0; k<_params.size(); k++) {
                    [encoder setBuffer:this->_params[k] offset:0 atIndex:1+k];
                }
                
                MTLSize threadGroupSize = MTLSizeMake(8,8,1);
                MTLSize threadGroups = MTLSizeMake(ceil((float)(texture[1].width/threadGroupSize.width)),ceil((float)(texture[1].height/threadGroupSize.height)),1);
                    
                [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];
                [encoder endEncoding];
                [commandBuffer commit];
                [commandBuffer waitUntilCompleted];
                
                this->cnt++;
                [texture[1] getBytes:this->_map[this->cnt&1] bytesPerRow:this->width<<2 fromRegion:MTLRegionMake2D(0,0,this->width,this->height) mipmapLevel:0];
                
            }
                            
            return this->_map[(this->cnt)&1];
            
        }
        
    
    public:
        
        MapRG16UnormBase(int w,int h,NSString *path) {
        
            this->width = w;
            this->height = h;
        
            this->_map[0] = new unsigned int[w*h];
            for(int i=0; i<h; i++) {
                for(int j=0; j<w; j++) {
                    this->_map[0][i*w+j] = (0x7FFF+j*4)<<16|(0x7FFF+i*4);
                }
            }
            
            this->_map[1] = new unsigned int[w*h];
            
            MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRG16Unorm width:w height:h mipmapped:NO];
            descriptor.usage = MTLTextureUsageShaderWrite|MTLTextureUsageShaderRead;
            
            this->texture[0] = [this->device newTextureWithDescriptor:descriptor];
            this->texture[1] = [this->device newTextureWithDescriptor:descriptor];
        
            this->_resolution = [this->device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault];
        
             float *resolution = (float *)[this->_resolution contents];
             resolution[0] = this->width;
             resolution[1] = this->height;
            
            NSError *error = nil;
            this->_library = [device newLibraryWithFile:path error:&error];

            this->function = [this->_library newFunctionWithName:@"processimage"];
            this->pipelineState = [this->device newComputePipelineStateWithFunction:this->function error:nil];
        }
    
        ~MapRG16UnormBase() {
            delete[] this->_map[0];
            delete[] this->_map[1];
        }
    
};

class MapRG16Unorm : public MapRG16UnormBase {

    public:
    
        unsigned int *map() {
            return this->exec();
        }
    
        MapRG16Unorm(int w,int h,NSString *path) : MapRG16UnormBase(w, h, path) {
        }
            
        ~MapRG16Unorm() {
        }
};
